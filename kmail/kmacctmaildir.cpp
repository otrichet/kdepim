// kmacctmaildir.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qfileinfo.h>
#include "kmacctmaildir.h"
#include "kmfoldermaildir.h"
#include "kmacctfolder.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;

#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kconfig.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#ifdef HAVE_PATHS_H
#include <paths.h>	/* defines _PATH_MAILDIR */
#endif

#undef None

//-----------------------------------------------------------------------------
KMAcctMaildir::KMAcctMaildir(KMAcctMgr* aOwner, const QString& aAccountName, uint id):
  KMAccount(aOwner, aAccountName, id)
{
}


//-----------------------------------------------------------------------------
KMAcctMaildir::~KMAcctMaildir()
{
  mLocation = "";
}


//-----------------------------------------------------------------------------
QString KMAcctMaildir::type(void) const
{
  return "maildir";
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::init() {
  KMAccount::init();

  mLocation = getenv("MAIL");
  if (mLocation.isNull()) {
    mLocation = getenv("HOME");
    mLocation += "/Maildir/";
  }
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::pseudoAssign( const KMAccount * a )
{
  KMAccount::pseudoAssign( a );

  const KMAcctMaildir * m = dynamic_cast<const KMAcctMaildir*>( a );
  if ( !m ) return;

  setLocation( m->location() );
}

//-----------------------------------------------------------------------------
void KMAcctMaildir::processNewMail(bool)
{
  QTime t;
  hasNewMail = false;

  if ( precommand().isEmpty() ) {
    QFileInfo fi( location() );
    if ( !fi.exists() ) {
      checkDone( hasNewMail, CheckOK );
      BroadcastStatus::instance()->setStatusMsgTransmissionCompleted( 0 );
      return;
    }
  }

  KMFolder mailFolder(0, location(), KMFolderTypeMaildir);

  long num = 0;
  long i;
  int rc;
  KMMessage* msg;
  bool addedOk;

  if (!mFolder) {
    checkDone( hasNewMail, CheckError );
    BroadcastStatus::instance()->setStatusMsg( i18n( "Transmission failed." ));
    return;
  }

  //BroadcastStatus::instance()->reset();
  BroadcastStatus::instance()->setStatusMsg(
	i18n("Preparing transmission from \"%1\"...").arg(mName));

  // run the precommand
  if (!runPrecommand(precommand()))
  {
    kdDebug(5006) << "cannot run precommand " << precommand() << endl;
    checkDone( hasNewMail, CheckError );
  }

  mailFolder.setAutoCreateIndex(FALSE);

  rc = mailFolder.open();
  if (rc)
  {
    QString aStr = i18n("<qt>Cannot open folder <b>%1</b>.</qt>").arg( mailFolder.location() );
    KMessageBox::sorry(0, aStr);
    kdDebug(5006) << "cannot open folder " << mailFolder.location() << endl;
    checkDone( hasNewMail, CheckError );
    BroadcastStatus::instance()->setStatusMsg( i18n( "Transmission failed." ));
    return;
  }

  mFolder->open();


  num = mailFolder.count();

  addedOk = true;
  t.start();

  // prepare the static parts of the status message:
  QString statusMsgStub = i18n("Moving message %3 of %2 from %1.")
    .arg(mailFolder.location()).arg(num);

  //BroadcastStatus::instance()->setStatusProgressEnable( "M" + mName, true );
  for (i=0; i<num; i++)
  {

    if( kmkernel->mailCheckAborted() ) {
      BroadcastStatus::instance()->setStatusMsg( i18n("Transmission aborted.") );
      num = i;
      addedOk = false;
    }
    if (!addedOk) break;

    QString statusMsg = statusMsgStub.arg(i);
    BroadcastStatus::instance()->setStatusMsg( statusMsg );
    //BroadcastStatus::instance()->setStatusProgressPercent( "M" + mName, (i*100) / num );

    msg = mailFolder.take(0);
    if (msg)
    {
      msg->setStatus(msg->headerField("Status").latin1(),
        msg->headerField("X-Status").latin1());
      msg->setEncryptionStateChar( msg->headerField( "X-KMail-EncryptionState" ).at(0));
      msg->setSignatureStateChar( msg->headerField( "X-KMail-SignatureState" ).at(0));

      addedOk = processNewMsg(msg);
      if (addedOk)
        hasNewMail = true;
    }

    if (t.elapsed() >= 200) { //hardwired constant
      kapp->processEvents();
      t.start();
    }

  }
  //BroadcastStatus::instance()->setStatusProgressEnable( "M" + mName, false );
  //BroadcastStatus::instance()->reset();

  if (addedOk)
  {
    BroadcastStatus::instance()->setStatusMsgTransmissionCompleted( num );
  }
  // else warning is written already

  mailFolder.close();
  mFolder->close();

  checkDone( hasNewMail, CheckOK );

  return;
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::readConfig(KConfig& config)
{
  KMAccount::readConfig(config);
  mLocation = config.readPathEntry("Location", mLocation);
}


//-----------------------------------------------------------------------------
void KMAcctMaildir::writeConfig(KConfig& config)
{
  KMAccount::writeConfig(config);
  config.writePathEntry("Location", mLocation);
}

//-----------------------------------------------------------------------------
void KMAcctMaildir::setLocation(const QString& aLocation)
{
  mLocation = aLocation;
}
