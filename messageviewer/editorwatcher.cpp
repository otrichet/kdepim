/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <config-messageviewer.h>
#include "editorwatcher.h"
#include "autoqpointer.h"

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kopenwithdialog.h>
#include <kprocess.h>
#include <kmimetypetrader.h>
#include <krun.h>

#include <qsocketnotifier.h>

#include <cassert>

// inotify stuff taken from kdelibs/kio/kio/kdirwatch.cpp
#ifdef HAVE_SYS_INOTIFY_H
#include <unistd.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#endif

namespace MessageViewer {
EditorWatcher::EditorWatcher( const KUrl & url, const QString &mimeType, bool openWith,
                              QObject * parent, QWidget *parentWidget ) :
    QObject( parent ),
    mUrl( url ),
    mMimeType( mimeType ),
    mOpenWith( openWith ),
    mEditor( 0 ),
    mParentWidget( parentWidget ),
    mHaveInotify( false ),
    mFileOpen( false ),
    mEditorRunning( false ),
    mFileModified( true ), // assume the worst unless we know better
    mDone( false )
{
  assert( mUrl.isLocalFile() );
  mTimer.setSingleShot( true );
  connect( &mTimer, SIGNAL(timeout()), SLOT(checkEditDone()) );
}

bool EditorWatcher::start()
{
  // find an editor
  KUrl::List list;
  list.append( mUrl );
  KService::Ptr offer = KMimeTypeTrader::self()->preferredService( mMimeType, "Application" );
  if ( mOpenWith || !offer ) {
#ifndef Q_OS_WINCE
    AutoQPointer<KOpenWithDialog> dlg( new KOpenWithDialog( list, i18n("Edit with:"),
                                                            QString(), mParentWidget ) );
    int dlgrc = dlg->exec();
    if ( dlgrc && dlg ) {
      offer = dlg->service();
    }
    if ( !dlgrc || !offer )
#endif
      return false;
  }

#ifdef HAVE_SYS_INOTIFY_H
  // monitor file
  mInotifyFd = inotify_init();
  if ( mInotifyFd > 0 ) {
    mInotifyWatch = inotify_add_watch( mInotifyFd, mUrl.path().toLatin1(), IN_CLOSE | IN_OPEN | IN_MODIFY );
    if ( mInotifyWatch >= 0 ) {
      QSocketNotifier *sn = new QSocketNotifier( mInotifyFd, QSocketNotifier::Read, this );
      connect( sn, SIGNAL(activated(int)), SLOT(inotifyEvent()) );
      mHaveInotify = true;
      mFileModified = false;
    }
  } else {
    kWarning() << "Failed to activate INOTIFY!";
  }
#endif

  // start the editor
  QStringList params = KRun::processDesktopExec( *offer, list, false );
  mEditor = new KProcess( this );
  mEditor->setProgram( params );
  connect( mEditor, SIGNAL( finished( int, QProcess::ExitStatus ) ),
           SLOT( editorExited() ) );
  mEditor->start();
  if ( !mEditor->waitForStarted() )
    return false;
  mEditorRunning = true;

  mEditTime.start();
  return true;
}

void EditorWatcher::inotifyEvent()
{
  assert( mHaveInotify );
#ifdef HAVE_SYS_INOTIFY_H
  int pending = -1;
  char buffer[4096];
  ioctl( mInotifyFd, FIONREAD, &pending );
  while ( pending > 0 ) {
    int size = read( mInotifyFd, buffer, qMin( pending, (int)sizeof(buffer) ) );
    pending -= size;
    if ( size < 0 )
      break; // error
    int offset = 0;
    while ( size > 0 ) {
      struct inotify_event *event = (struct inotify_event *) &buffer[offset];
      size -= sizeof( struct inotify_event ) + event->len;
      offset += sizeof( struct inotify_event ) + event->len;
      if ( event->mask & IN_OPEN )
        mFileOpen = true;
      if ( event->mask & IN_CLOSE )
        mFileOpen = false;
      if ( event->mask & IN_MODIFY )
        mFileModified = true;
    }
  }
#endif
  mTimer.start( 500 );

}

void EditorWatcher::editorExited()
{
  mEditorRunning = false;
  mTimer.start( 500 );
}

void EditorWatcher::checkEditDone()
{
  if ( mEditorRunning || (mFileOpen && mHaveInotify) || mDone )
    return;
  // protect us against double-deletion by calling this method again while
  // the subeventloop of the message box is running
  mDone = true;
  // nobody can edit that fast, we seem to be unable to detect
  // when the editor will be closed
  if ( mEditTime.elapsed() <= 3000 ) {
    KMessageBox::information( mParentWidget,
                        i18n( "KMail is unable to detect when the chosen editor is closed. "
                              "To avoid data loss, editing the attachment will be aborted." ),
                        i18n( "Unable to edit attachment" ),
                        "UnableToEditAttachment" );
  }

  emit editDone( this );
  deleteLater();
}
}
#include "editorwatcher.moc"
