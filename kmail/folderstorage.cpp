/*
    Virtual base class for mail storage.

    This file is part of KMail.

    Copyright (c) 2004 Bo Thorsen <bo@sonofthor.dk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include <config.h>

#include "folderstorage.h"
#include "kmfolder.h"
#include "kmkernel.h"

#include "kmfolderimap.h" //for the nasty imap hacks, FIXME
#include "undostack.h"
#include "kmmsgdict.h"
#include "kmfoldermgr.h"
#include "kmcommands.h"
#include "listjob.h"
using KMail::ListJob;
#include "kmsearchpattern.h"
#include "globalsettings.h"

#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>

//Added by qt3to4:
#include <QFile>
#include <QList>
#include <QRegExp>

#include <mimelib/mimepp.h>
#include <errno.h>

//-----------------------------------------------------------------------------

FolderStorage::FolderStorage( KMFolder* folder, const char* aName )
  : QObject( folder ), mFolder( folder ), mEmitChangedTimer( 0L )
{
  setObjectName( aName );
  mOpenCount = 0;
  mQuiet = 0;
  mChanged = false;
  mAutoCreateIndex = true;
  mExportsSernums = false;
  mDirty = false;
  mUnreadMsgs = -1;
  mGuessedUnreadMsgs = -1;
  mTotalMsgs = -1;
  needsCompact    = false;
  mConvertToUtf8  = false;
  mCompactable     = true;
  mNoContent      = false;
  mNoChildren     = false;
  mRDict = 0;
  mDirtyTimer = new QTimer(this);
  connect(mDirtyTimer, SIGNAL(timeout()),
	  this, SLOT(updateIndex()));

  mHasChildren = HasNoChildren;
  mContentsType = KMail::ContentsTypeMail;
}

//-----------------------------------------------------------------------------
FolderStorage::~FolderStorage()
{
  QObject::disconnect( SIGNAL(destroyed(QObject*)), this, 0 );
  qDeleteAll( mJobList );
  mJobList.clear();
  KMMsgDict::deleteRentry(mRDict);
}


//-----------------------------------------------------------------------------
QString FolderStorage::dotEscape(const QString& aStr)
{
  if (aStr.isEmpty() || aStr[0] != '.') return aStr;
  return aStr.left(aStr.indexOf(QRegExp("[^\\.]"))) + aStr;
}

void FolderStorage::addJob( FolderJob* job ) const
{
  QObject::connect( job, SIGNAL(destroyed(QObject*)),
                    SLOT(removeJob(QObject*)) );
  mJobList.append( job );
}

void FolderStorage::removeJob( QObject* job )
{
  mJobList.removeAll( static_cast<FolderJob*>( job ) );
}


//-----------------------------------------------------------------------------
QString FolderStorage::location() const
{
  QString sLocation(const_cast<FolderStorage*>(this)->folder()->path());

  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += dotEscape(fileName());

  return sLocation;
}

QString FolderStorage::fileName() const
{
  return mFolder->name();
}



//-----------------------------------------------------------------------------
void FolderStorage::setAutoCreateIndex(bool autoIndex)
{
  mAutoCreateIndex = autoIndex;
}

//-----------------------------------------------------------------------------
void FolderStorage::setDirty(bool f)
{
  mDirty = f;
  if (mDirty  && mAutoCreateIndex)
    mDirtyTimer->start( mDirtyTimerInterval );
  else
    mDirtyTimer->stop();
}

//-----------------------------------------------------------------------------
void FolderStorage::markNewAsUnread()
{
  KMMsgBase* msgBase;
  int i;

  for (i=0; i< count(); ++i)
  {
    if (!(msgBase = getMsgBase(i))) continue;
    if (msgBase->status().isNew())
    {
      msgBase->setStatus( MessageStatus::statusUnread() );
      msgBase->setDirty(true);
    }
  }
}

void FolderStorage::markUnreadAsRead()
{
  KMMsgBase* msgBase;
  SerNumList serNums;

  for (int i=count()-1; i>=0; --i)
  {
    msgBase = getMsgBase(i);
    assert(msgBase);
    if (msgBase->status().isNew() || msgBase->status().isUnread())
    {
      serNums.append( msgBase->getMsgSerNum() );
    }
  }
  if (serNums.empty())
    return;

  KMCommand *command = new KMSetStatusCommand( MessageStatus::statusRead(), serNums );
  command->start();
}

//-----------------------------------------------------------------------------
void FolderStorage::quiet(bool beQuiet)
{

  if (beQuiet)
  {
    /* Allocate the timer here to don't have one timer for each folder. BTW,
     * a timer is created when a folder is checked
     */
    if ( !mEmitChangedTimer) {
      mEmitChangedTimer= new QTimer( this );
      connect( mEmitChangedTimer, SIGNAL( timeout() ),
      this, SLOT( slotEmitChangedTimer() ) );
    }
    mQuiet++;
  } else {
    mQuiet--;
    if (mQuiet <= 0)
    {
      delete mEmitChangedTimer;
      mEmitChangedTimer=0L;

      mQuiet = 0;
      if (mChanged) {
       emit changed();
       // Don't hurt emit this if the mUnreadMsg really don't change
       // We emit it here, because this signal is delayed if mQuiet >0
       emit numUnreadMsgsChanged( folder() );
      }
      mChanged = false;
    }
  }
}

//-----------------------------------------------------------------------------

/** Compare message's date. This is useful for message sorting */
int operator<( KMMsgBase & m1, KMMsgBase & m2 )
{
  return (m1.date() < m2.date());
}

/** Compare message's date. This is useful for message sorting */
int operator==( KMMsgBase & m1, KMMsgBase & m2 )
{
  return (m1.date() == m2.date());
}


//-----------------------------------------------------------------------------
int FolderStorage::expungeOldMsg(int days)
{
  int i, msgnb=0;
  time_t msgTime, maxTime;
  const KMMsgBase* mb;
  QList<int> rmvMsgList;

  maxTime = time(0) - days * 3600 * 24;

  for (i=count()-1; i>=0; i--) {
    mb = getMsgBase(i);
    assert(mb);
    msgTime = mb->date();

    if (msgTime < maxTime) {
      //kDebug(5006) << "deleting msg " << i << " : " << mb->subject() << " - " << mb->dateStr(); // << endl;
      removeMsg( i );
      msgnb++;
    }
  }
  return msgnb;
}

//------------------------------------------
void FolderStorage::slotEmitChangedTimer()
{
  emit changed();
  mChanged=false;
}
//-----------------------------------------------------------------------------
void FolderStorage::emitMsgAddedSignals(int idx)
{
  quint32 serNum = KMMsgDict::instance()->getMsgSerNum( folder() , idx );
  if (!mQuiet) {
    emit msgAdded(idx);
  } else {
    /** Restart always the timer or not. BTW we get a kmheaders refresh
     * each 3 seg.?*/
    if ( !mEmitChangedTimer->isActive() ) {
      mEmitChangedTimer->start( 3000 );
    }
    mChanged=true;
  }
  emit msgAdded( folder(), serNum );
}

//-----------------------------------------------------------------------------
bool FolderStorage::canAddMsgNow(KMMessage* aMsg, int* aIndex_ret)
{
  if (aIndex_ret) *aIndex_ret = -1;
  KMFolder *msgParent = aMsg->parent();
  // If the message has a parent and is in transfer, bail out. If it does not
  // have a parent we want to be able to add it even if it is in transfer.
  if (aMsg->transferInProgress() && msgParent)
      return false;
  if (!aMsg->isComplete() && msgParent && msgParent->folderType() == KMFolderTypeImap)
  {
    FolderJob *job = msgParent->createJob(aMsg);
    connect(job, SIGNAL(messageRetrieved(KMMessage*)),
            SLOT(reallyAddMsg(KMMessage*)));
    job->start();
    aMsg->setTransferInProgress( true );
    return false;
  }
  return true;
}


//-----------------------------------------------------------------------------
void FolderStorage::reallyAddMsg(KMMessage* aMsg)
{
  if (!aMsg) // the signal that is connected can call with aMsg=0
    return;
  aMsg->setTransferInProgress( false );
  aMsg->setComplete( true );
  KMFolder *aFolder = aMsg->parent();
  int index;
  ulong serNum = aMsg->getMsgSerNum();
  bool undo = aMsg->enableUndo();
  addMsg(aMsg, &index);
  if (index < 0) return;
  unGetMsg(index);
  if (undo)
  {
    kmkernel->undoStack()->pushSingleAction( serNum, aFolder, folder() );
  }
}


//-----------------------------------------------------------------------------
void FolderStorage::reallyAddCopyOfMsg(KMMessage* aMsg)
{
  if ( !aMsg ) return; // messageRetrieved(0) is always possible
  aMsg->setParent( 0 );
  aMsg->setTransferInProgress( false );
  addMsg( aMsg );
  unGetMsg( count() - 1 );
}

int FolderStorage::find( const KMMessage * msg ) const {
  return find( &msg->toMsgBase() );
}

//-----------------------------------------------------------------------------
void FolderStorage::removeMsg(const QList<KMMsgBase*>& msgList, bool imapQuiet)
{
  for( QList<KMMsgBase*>::const_iterator it = msgList.begin();
      it != msgList.end(); ++it )
  {
    int idx = find( *it );
    assert( idx != -1 );
    removeMsg( idx, imapQuiet );
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::removeMsg(const QList<KMMessage*>& msgList, bool imapQuiet)
{
  for( QList<KMMessage*>::const_iterator it = msgList.begin();
      it != msgList.end(); ++it )
  {
    int idx = find( *it );
    assert( idx != -1 );
    removeMsg( idx, imapQuiet );
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::removeMsg(int idx, bool)
{
  //assert(idx>=0);
  if(idx < 0)
  {
    kDebug(5006) << "FolderStorage::removeMsg() : idx < 0\n" << endl;
    return;
  }

  KMMsgBase* mb = getMsgBase(idx);

  quint32 serNum = KMMsgDict::instance()->getMsgSerNum( folder(), idx );
  if (serNum != 0)
    emit msgRemoved( folder(), serNum );
  mb = takeIndexEntry( idx );

  setDirty( true );
  needsCompact=true; // message is taken from here - needs to be compacted

  if (mb->status().isUnread() || mb->status().isNew() ||
      (folder() == kmkernel->outboxFolder())) {
    --mUnreadMsgs;
    if ( !mQuiet ) {
//      kDebug( 5006 ) << "FolderStorage::msgStatusChanged" << endl;
      emit numUnreadMsgsChanged( folder() );
    }else{
      if ( !mEmitChangedTimer->isActive() ) {
//        kDebug( 5006 )<< "EmitChangedTimer started" << endl;
        mEmitChangedTimer->start( 3000 );
      }
      mChanged = true;
    }
  }
  --mTotalMsgs;

  QString msgIdMD5 = mb->msgIdMD5();
  emit msgRemoved( idx, msgIdMD5 );
  emit msgRemoved( folder() );
}


//-----------------------------------------------------------------------------
KMMessage* FolderStorage::take(int idx)
{
  KMMsgBase* mb;
  KMMessage* msg;

  assert(idx>=0 && idx<=count());

  mb = getMsgBase(idx);
  if (!mb) return 0;
  if (!mb->isMessage()) readMsg(idx);
  quint32 serNum = KMMsgDict::instance()->getMsgSerNum( folder(), idx );
  emit msgRemoved( folder(), serNum );

  msg = (KMMessage*)takeIndexEntry(idx);

  if (msg->status().isUnread() || msg->status().isNew() ||
      ( folder() == kmkernel->outboxFolder() )) {
    --mUnreadMsgs;
    if ( !mQuiet ) {
      emit numUnreadMsgsChanged( folder() );
    }else{
      if ( !mEmitChangedTimer->isActive() ) {
        mEmitChangedTimer->start( 3000 );
      }
      mChanged = true;
    }
  }
  --mTotalMsgs;
  msg->setParent(0);
  setDirty( true );
  needsCompact=true; // message is taken from here - needs to be compacted
  QString msgIdMD5 = msg->msgIdMD5();
  emit msgRemoved( idx, msgIdMD5 );
  emit msgRemoved( folder() );

  return msg;
}

void FolderStorage::take(QList<KMMessage*> msgList)
{
  for( QList<KMMessage*>::const_iterator it = msgList.begin();
      it != msgList.end(); ++it )
  {
    KMMessage *msg = (*it);
    if (msg->parent())
    {
      int idx = msg->parent()->find(msg);
      take(idx);
    }
  }
}


//-----------------------------------------------------------------------------
KMMessage* FolderStorage::getMsg(int idx)
{
  if ( idx < 0 || idx >= count() )
    return 0;

  KMMsgBase* mb = getMsgBase(idx);
  if (!mb) return 0;

  KMMessage *msg = 0;
  bool undo = mb->enableUndo();
  if (mb->isMessage()) {
      msg = ((KMMessage*)mb);
  } else {
      QString mbSubject = mb->subject();
      msg = readMsg(idx);
      // sanity check
      if (mCompactable && (msg->subject().isEmpty() != mbSubject.isEmpty())) {
        kDebug(5006) << "Error: " << location() <<
          " Index file is inconsistent with folder file. This should never happen." << endl;
        mCompactable = false; // Don't compact
        writeConfig();
      }

  }
  // Either isMessage and we had a sernum, or readMsg gives us one
  // (via insertion into mMsgList). sernum == 0 may still occur due to
  // an outdated or corrupt IMAP cache.
  if ( msg->getMsgSerNum() == 0 )
    return 0;
  msg->setEnableUndo(undo);
  msg->setComplete( true );
  return msg;
}

//-----------------------------------------------------------------------------
KMMessage* FolderStorage::readTemporaryMsg(int idx)
{
  if(!(idx >= 0 && idx <= count()))
    return 0;

  KMMsgBase* mb = getMsgBase(idx);
  if (!mb) return 0;

  unsigned long sernum = mb->getMsgSerNum();

  KMMessage *msg = 0;
  bool undo = mb->enableUndo();
  if (mb->isMessage()) {
    // the caller will delete it, so we must make a copy it
    msg = new KMMessage(*(KMMessage*)mb);
    msg->setMsgSerNum(sernum);
    msg->setComplete( true );
  } else {
    // ## Those two lines need to be moved to a virtual method for KMFolderSearch, like readMsg
    msg = new KMMessage(*(KMMsgInfo*)mb);
    msg->setMsgSerNum(sernum); // before fromDwString so that readyToShow uses the right sernum
    msg->setComplete( true );
    msg->fromDwString(getDwString(idx));
  }
  msg->setEnableUndo(undo);
  return msg;
}


//-----------------------------------------------------------------------------
KMMsgInfo* FolderStorage::unGetMsg(int idx)
{
  KMMsgBase* mb;

  if(!(idx >= 0 && idx <= count()))
    return 0;

  mb = getMsgBase(idx);
  if (!mb) return 0;


  if (mb->isMessage()) {
    // Remove this message from all jobs' list it might still be on.
    // setIndexEntry deletes the message.
    KMMessage *msg = static_cast<KMMessage*>(mb);
    if ( msg->transferInProgress() ) return 0;
    ignoreJobsForMessage( msg );
    return setIndexEntry( idx, msg );
  }

  return 0;
}


//-----------------------------------------------------------------------------
bool FolderStorage::isMessage(int idx)
{
  KMMsgBase* mb;
  if (!(idx >= 0 && idx <= count())) return false;
  mb = getMsgBase(idx);
  return (mb && mb->isMessage());
}

//-----------------------------------------------------------------------------
FolderJob* FolderStorage::createJob( KMMessage *msg, FolderJob::JobType jt,
                                KMFolder *folder, QString partSpecifier,
                                const AttachmentStrategy *as ) const
{
  FolderJob * job = doCreateJob( msg, jt, folder, partSpecifier, as );
  if ( job )
    addJob( job );
  return job;
}

//-----------------------------------------------------------------------------
FolderJob* FolderStorage::createJob( QList<KMMessage*>& msgList, const QString& sets,
                                FolderJob::JobType jt, KMFolder *folder ) const
{
  FolderJob * job = doCreateJob( msgList, sets, jt, folder );
  if ( job )
    addJob( job );
  return job;
}

//-----------------------------------------------------------------------------
int FolderStorage::moveMsg(KMMessage* aMsg, int* aIndex_ret)
{
  assert(aMsg != 0);
  KMFolder* msgParent = aMsg->parent();

  if (msgParent)
    msgParent->open();

  open();
  int rc = addMsg(aMsg, aIndex_ret);
  close();

  if (msgParent)
    msgParent->close();

  return rc;
}

//-----------------------------------------------------------------------------
int FolderStorage::moveMsg(QList<KMMessage*> msglist, int* aIndex_ret)
{
  KMMessage* aMsg = msglist.first();
  assert(aMsg != 0);
  KMFolder* msgParent = aMsg->parent();

  if (msgParent)
    msgParent->open();

  QList<int> index;
  open();
  int rc = addMsg(msglist, index);
  close();
  // FIXME: we want to have a QValueList to pass it back, so change this method
  if ( !index.isEmpty() )
    aIndex_ret = &index.first();

  if (msgParent)
    msgParent->close();

  return rc;
}


//-----------------------------------------------------------------------------
int FolderStorage::rename(const QString& newName, KMFolderDir *newParent)
{
  QString oldLoc, oldIndexLoc, oldIdsLoc, newLoc, newIndexLoc, newIdsLoc;
  QString oldSubDirLoc, newSubDirLoc;
  QString oldName;
  int rc=0, openCount=mOpenCount;
  KMFolderDir *oldParent;

  assert(!newName.isEmpty());

  oldLoc = location();
  oldIndexLoc = indexLocation();
  oldSubDirLoc = folder()->subdirLocation();
  oldIdsLoc =  KMMsgDict::instance()->getFolderIdsLocation( *this );
  QString oldConfigString = "Folder-" + folder()->idString();

  close(true);

  oldName = folder()->fileName();
  oldParent = folder()->parent();
  if (newParent)
    folder()->setParent( newParent );

  folder()->setName(newName);
  newLoc = location();
  newIndexLoc = indexLocation();
  newSubDirLoc = folder()->subdirLocation();
  newIdsLoc = KMMsgDict::instance()->getFolderIdsLocation( *this );

  if (::rename(QFile::encodeName(oldLoc), QFile::encodeName(newLoc))) {
    folder()->setName(oldName);
    folder()->setParent(oldParent);
    rc = errno;
  }
  else {
    // rename/move index file and index.sorted file
    if (!oldIndexLoc.isEmpty()) {
      ::rename(QFile::encodeName(oldIndexLoc), QFile::encodeName(newIndexLoc));
      ::rename(QFile::encodeName(oldIndexLoc) + ".sorted",
               QFile::encodeName(newIndexLoc) + ".sorted");
    }

    // rename/move serial number file
    if (!oldIdsLoc.isEmpty())
      ::rename(QFile::encodeName(oldIdsLoc), QFile::encodeName(newIdsLoc));

    // rename/move the subfolder directory
    KMFolderDir* child = 0;
    if( folder() )
      child = folder()->child();

    if (!::rename(QFile::encodeName(oldSubDirLoc), QFile::encodeName(newSubDirLoc) )) {
      // now that the subfolder directory has been renamed and/or moved also
      // change the name that is stored in the corresponding KMFolderNode
      // (provide that the name actually changed)
      if( child && ( oldName != newName ) ) {
        child->setName( "." + QFile::encodeName(newName) + ".directory" );
      }
    }

    // if the folder is being moved then move its node and, if necessary, also
    // the associated subfolder directory node to the new parent
    if (newParent) {
      int idx = oldParent->indexOf( folder() );
      if ( idx != -1)
        oldParent->takeAt( idx );
      newParent->prepend( folder() );
      qSort( newParent->begin(), newParent->end() );
      if ( child ) {
        int idx = child->parent()->indexOf( child );
        if ( idx != -1 )
          child->parent()->takeAt( idx );
        newParent->prepend( child );
        qSort( newParent->begin(), newParent->end() );
        child->setParent( newParent );
      }
    }
  }

  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }
  writeConfig();

  // delete the old entry as we get two entries with the same ID otherwise
  KMKernel::config()->deleteGroup( oldConfigString );

  emit locationChanged( oldLoc, newLoc );
  emit nameChanged();
  kmkernel->folderMgr()->contentsChanged();
  return rc;
}


//-----------------------------------------------------------------------------
void FolderStorage::remove()
{
  assert(!folder()->name().isEmpty());

  clearIndex( true, mExportsSernums ); // delete and remove from dict if necessary
  close(true);

  if ( mExportsSernums )
    KMMsgDict::mutableInstance()->removeFolderIds( *this );
  unlink(QFile::encodeName(indexLocation()) + ".sorted");
  unlink(QFile::encodeName(indexLocation()));

  int rc = removeContents();

  needsCompact = false; //we are dead - no need to compact us

  // Erase settings, otherwise they might interfere when recreating the folder
  KConfig* config = KMKernel::config();
  config->deleteGroup( "Folder-" + folder()->idString() );

  emit removed(folder(), (rc ? false : true));
}


//-----------------------------------------------------------------------------
int FolderStorage::expunge()
{
  int openCount = mOpenCount;

  assert(!folder()->name().isEmpty());

  clearIndex( true, mExportsSernums );   // delete and remove from dict, if needed
  close( true );

  if ( mExportsSernums )
    KMMsgDict::mutableInstance()->removeFolderIds( *this );
  if ( mAutoCreateIndex )
    truncateIndex();
  else unlink(QFile::encodeName(indexLocation()));

  int rc = expungeContents();
  if (rc) return rc;

  mDirty = false;
  needsCompact = false; //we're cleared and truncated no need to compact

  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }

  mUnreadMsgs = 0;
  mTotalMsgs = 0;
  emit numUnreadMsgsChanged( folder() );
  if ( mAutoCreateIndex ) // FIXME Heh? - Till
    writeConfig();
  emit changed();
  emit expunged( folder() );

  return 0;
}

//-----------------------------------------------------------------------------
QString FolderStorage::label() const
{
  return folder()->label();
}

int FolderStorage::count(bool cache) const
{
  if (cache && mTotalMsgs != -1)
    return mTotalMsgs;
  else
    return -1;
}

//-----------------------------------------------------------------------------
int FolderStorage::countUnread()
{
  if (mGuessedUnreadMsgs > -1)
    return mGuessedUnreadMsgs;
  if (mUnreadMsgs > -1)
    return mUnreadMsgs;

  readConfig();

  if (mUnreadMsgs > -1)
    return mUnreadMsgs;

  open(); // will update unreadMsgs
  int unread = mUnreadMsgs;
  close();
  return (unread > 0) ? unread : 0;
}

//-----------------------------------------------------------------------------
void FolderStorage::msgStatusChanged( const MessageStatus& oldStatus,
                                      const MessageStatus& newStatus, int idx)
{
  int oldUnread = 0;
  int newUnread = 0;

  if ((oldStatus.isUnread() || oldStatus.isNew()) &&
      (folder() == kmkernel->outboxFolder()))
    oldUnread = 1;
  if ((newStatus.isUnread() || newStatus.isNew()) &&
      (folder() == kmkernel->outboxFolder()))
    newUnread = 1;
  int deltaUnread = newUnread - oldUnread;

  mDirtyTimer->start(mDirtyTimerInterval);
  if (deltaUnread != 0) {
    if (mUnreadMsgs < 0) mUnreadMsgs = 0;
    mUnreadMsgs += deltaUnread;
    if ( !mQuiet ) {
      emit numUnreadMsgsChanged( folder() );
    }else{
      if ( !mEmitChangedTimer->isActive() ) {
        mEmitChangedTimer->start( 3000 );
      }
      mChanged = true;
    }
    quint32 serNum = KMMsgDict::instance()->getMsgSerNum(folder(), idx);
    emit msgChanged( folder(), serNum, deltaUnread );
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::headerOfMsgChanged(const KMMsgBase* aMsg, int idx)
{
  if (idx < 0)
    idx = aMsg->parent()->find( aMsg );

  if (idx >= 0 )
  {
    if ( !mQuiet )
      emit msgHeaderChanged(folder(), idx);
    else{
      if ( !mEmitChangedTimer->isActive() ) {
        mEmitChangedTimer->start( 3000 );
      }
      mChanged = true;
    }
  } else
    mChanged = true;
}

//-----------------------------------------------------------------------------
void FolderStorage::readConfig()
{
  //kDebug(5006)<<"#### READING CONFIG  = "<< name() <<endl;
  KConfig* config = KMKernel::config();
  KConfigGroup group(config, "Folder-" + folder()->idString());
  if (mUnreadMsgs == -1)
    mUnreadMsgs = group.readEntry("UnreadMsgs", -1 );
  if (mTotalMsgs == -1)
    mTotalMsgs = group.readEntry("TotalMsgs", -1 );
  mCompactable = group.readEntry("Compactable", true );

  int type = group.readEntry( "ContentsType", 0 );
  if ( type < 0 || type > KMail::ContentsTypeLast ) type = 0;
  setContentsType( static_cast<KMail::FolderContentsType>( type ) );

  if( folder() ) folder()->readConfig( config );
}

//-----------------------------------------------------------------------------
void FolderStorage::writeConfig()
{
  KConfig* config = KMKernel::config();
  KConfigGroup group(config, "Folder-" + folder()->idString());
  group.writeEntry("UnreadMsgs",
      mGuessedUnreadMsgs == -1 ? mUnreadMsgs : mGuessedUnreadMsgs);
  group.writeEntry("TotalMsgs", mTotalMsgs);
  group.writeEntry("Compactable", mCompactable);
  group.writeEntry("ContentsType", (int)mContentsType);

  // Write the KMFolder parts
  if( folder() ) folder()->writeConfig( config );

  GlobalSettings::self()->requestSync();
}

//-----------------------------------------------------------------------------
void FolderStorage::correctUnreadMsgsCount()
{
  open();
  close();
  emit numUnreadMsgsChanged( folder() );
}

void FolderStorage::registerWithMessageDict()
{
  mExportsSernums = true;
  readFolderIdsFile();
}

void FolderStorage::deregisterFromMessageDict()
{
  writeFolderIdsFile();
  mExportsSernums = false;
}

void FolderStorage::readFolderIdsFile()
{
  if ( !mExportsSernums ) return;
  if ( KMMsgDict::mutableInstance()->readFolderIds( *this ) == -1 ) {
    invalidateFolder();
  }
  if ( !KMMsgDict::mutableInstance()->hasFolderIds( *this ) ) {
    invalidateFolder();
  }
}

void FolderStorage::invalidateFolder()
{
  if ( !mExportsSernums ) return;
  unlink(QFile::encodeName( indexLocation()) + ".sorted");
  unlink(QFile::encodeName( indexLocation()) + ".ids");
  fillMessageDict();
  KMMsgDict::mutableInstance()->writeFolderIds( *this );
  emit invalidated( folder() );
}


//-----------------------------------------------------------------------------
int FolderStorage::writeFolderIdsFile() const
{
  if ( !mExportsSernums ) return -1;
  return KMMsgDict::mutableInstance()->writeFolderIds( *this );
}

//-----------------------------------------------------------------------------
int FolderStorage::touchFolderIdsFile()
{
  if ( !mExportsSernums ) return -1;
  return KMMsgDict::mutableInstance()->touchFolderIds( *this );
}

//-----------------------------------------------------------------------------
int FolderStorage::appendToFolderIdsFile( int idx )
{
  if ( !mExportsSernums ) return -1;
  int ret = 0;
  if ( count() == 1 ) {
    ret = KMMsgDict::mutableInstance()->writeFolderIds( *this );
  } else {
    ret = KMMsgDict::mutableInstance()->appendToFolderIds( *this, idx );
  }
  return ret;
}

void FolderStorage::replaceMsgSerNum( unsigned long sernum, KMMsgBase* msg, int idx )
{
  if ( !mExportsSernums ) return;
  KMMsgDict::mutableInstance()->replace( sernum, msg, idx );
}

void FolderStorage::setRDict( KMMsgDictREntry *rentry ) const
{
  if ( ! mExportsSernums )
    kDebug(5006) << "WTF, this FolderStorage should be invisible to the msgdict, who is calling us?" << kBacktrace() << endl;
  assert( mExportsSernums ); // otherwise things are very wrong
  if ( rentry == mRDict )
    return;
  KMMsgDict::deleteRentry( mRDict );
  mRDict = rentry;
}

//-----------------------------------------------------------------------------
void FolderStorage::setStatus(int idx, const MessageStatus& status, bool toggle)
{
  KMMsgBase *msg = getMsgBase(idx);
  if ( msg ) {
    if (toggle)
      msg->toggleStatus(status, idx);
    else
      msg->setStatus(status, idx);
  }
}


//-----------------------------------------------------------------------------
void FolderStorage::setStatus(QList<int>& ids, const MessageStatus& status, bool toggle)
{
  for ( QList<int>::Iterator it = ids.begin(); it != ids.end(); ++it )
  {
    FolderStorage::setStatus(*it, status, toggle);
  }
}

void FolderStorage::ignoreJobsForMessage( KMMessage *msg )
{
  if ( !msg || msg->transferInProgress() )
    return;

  QList<FolderJob*>::iterator it;
  for ( it = mJobList.begin(); it != mJobList.end(); ++it )
  {
    //FIXME: the questions is : should we iterate through all
    //messages in jobs? I don't think so, because it would
    //mean canceling the jobs that work with other messages
    if ( (*it)->msgList().first() == msg )
    {
      FolderJob* job = (*it);
      it = mJobList.erase( it );
      delete job;
    } else
      ++it;
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::removeJobs()
{
  qDeleteAll( mJobList );
  mJobList.clear();
}



//-----------------------------------------------------------------------------
void FolderStorage::updateChildrenState()
{
  if ( folder() && folder()->child() )
  {
    if ( kmkernel->folderMgr()->folderCount( folder()->child() ) > 0 )
      setHasChildren( HasChildren );
    else
      setHasChildren( HasNoChildren );
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::setNoChildren( bool aNoChildren )
{
  mNoChildren = aNoChildren;
  if ( aNoChildren )
    setHasChildren( HasNoChildren );
}

//-----------------------------------------------------------------------------
void FolderStorage::setContentsType( KMail::FolderContentsType type )
{
  if ( type != mContentsType ) {
    mContentsType = type;
    emit contentsTypeChanged( type );
  }
}

//-----------------------------------------------------------------------------
void FolderStorage::search( const KMSearchPattern* pattern )
{
  mSearchPattern = pattern;
  mCurrentSearchedMsg = 0;
  if ( pattern )
    slotProcessNextSearchBatch();
}

void FolderStorage::slotProcessNextSearchBatch()
{
  if ( !mSearchPattern ) return;
  QList<quint32> matchingSerNums;
  int end = ( count() - mCurrentSearchedMsg > 100 ) ? 100+mCurrentSearchedMsg : count();
  for ( int i = mCurrentSearchedMsg; i < end; ++i )
  {
    quint32 serNum = KMMsgDict::instance()->getMsgSerNum( folder(), i );
    if ( mSearchPattern->matches( serNum ) )
      matchingSerNums.append( serNum );
  }
  mCurrentSearchedMsg = end;
  bool complete = ( end == count() ) ? true : false;
  emit searchResult( folder(), matchingSerNums, mSearchPattern, complete );
  if ( !complete )
    QTimer::singleShot( 0, this, SLOT(slotProcessNextSearchBatch()) );
}

//-----------------------------------------------------------------------------
void FolderStorage::search( const KMSearchPattern* pattern, quint32 serNum )
{
  bool matches = pattern && pattern->matches( serNum );

  emit searchDone( folder(), serNum, pattern, matches );
}

//-----------------------------------------------------------------------------
int FolderStorage::addMsg( QList<KMMessage*>& msgList, QList<int>& index_ret )
{
  int ret = 0;
  int index;
  for ( QList<KMMessage*>::const_iterator it = msgList.begin();
        it != msgList.end(); ++it )
  {
    int aret = addMsg( *it, &index );
    index_ret << index;
    if ( aret != 0 ) // error condition
      ret = aret;
  }
  return ret;
}

//-----------------------------------------------------------------------------
bool FolderStorage::isMoveable() const
{
  return ( folder()->isSystemFolder() ) ? false : true;
}

#include "folderstorage.moc"
