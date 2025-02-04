/*
 * Copyright (c) 2004 Carsten Burghardt <burghardt@kde.org>
 * Copyright (c) 2009 Montel Laurent <montel@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */


#include "folderrequester.h"

#include "folderselectiondialog.h"
#include "mailutil.h"
#include "mailkernel.h"

#include "messageviewer/autoqpointer.h"

#include <kdebug.h>
#include <klineedit.h>
#include <kiconloader.h>
#include <KLocale>
#include <kdialog.h>
#include <QToolButton>
#include <QHBoxLayout>
#include <QKeyEvent>

#include <Akonadi/CollectionFetchJob>

namespace MailCommon {

FolderRequester::FolderRequester( QWidget* parent )
  : QWidget( parent ),
    mMustBeReadWrite( true ), mShowOutbox( true ), mShowImapFolders( true ), mNotCreateNewFolder( false )
{
  QHBoxLayout * hlay = new QHBoxLayout( this );
  hlay->setSpacing( KDialog::spacingHint() );
  hlay->setContentsMargins( 0, 0, 0, 0 );

  edit = new KLineEdit( this );
  edit->setReadOnly( true );
  hlay->addWidget( edit );

  QToolButton* button = new QToolButton( this );
  button->setIcon( KIcon( "folder" ) );
  button->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  hlay->addWidget( button );
  connect( button, SIGNAL(clicked()), this, SLOT(slotOpenDialog()) );

  setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding,
        QSizePolicy::Fixed ) );
  setFocusPolicy( Qt::StrongFocus );
}

//-----------------------------------------------------------------------------
void FolderRequester::slotOpenDialog()
{
  FolderSelectionDialog::SelectionFolderOptions options = FolderSelectionDialog::EnableCheck ;
  options |= FolderSelectionDialog::HideVirtualFolder;
  options |= FolderSelectionDialog::NotUseGlobalSettings;
  if ( mNotCreateNewFolder )
    options |= FolderSelectionDialog::NotAllowToCreateNewFolder;
  if ( !mShowImapFolders )
    options |= FolderSelectionDialog::HideImapFolder;
  if ( !mShowOutbox )
    options |= FolderSelectionDialog::HideOutboxFolder;

  MessageViewer::AutoQPointer<FolderSelectionDialog> dlg(
      new FolderSelectionDialog( this, options ) );
  dlg->setCaption( i18n("Select Folder") );
  dlg->setModal( false );
  dlg->setSelectedCollection( mCollection );

  if ( dlg->exec() && dlg ) {
    setFolder( dlg->selectedCollection() );
  }
}

//-----------------------------------------------------------------------------
FolderRequester::~FolderRequester()
{
}

Akonadi::Collection FolderRequester::folderCollection() const
{
  return mCollection;
}

void FolderRequester::setCollectionFullPath( const Akonadi::Collection&col )
{
  edit->setText( Util::fullCollectionPath( col ) );
}

//-----------------------------------------------------------------------------
void FolderRequester::setFolder( const Akonadi::Collection&col )
{
  mCollection = col;
  if ( mCollection.isValid() ) {
    setCollectionFullPath( mCollection );
    mFolderId = QString::number( mCollection.id() );
    Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob( mCollection, Akonadi::CollectionFetchJob::Base, this );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( slotCollectionsReceived( KJob* ) ) );
  }
  else if ( !mMustBeReadWrite ) // the Local Folders root node was selected
    edit->setText( i18n("Local Folders") );
  emit folderChanged( mCollection );
}

void FolderRequester::slotCollectionsReceived( KJob *job )
{
  if ( job->error() ) {
    mCollection = Akonadi::Collection();
    edit->setText( i18n( "Please select a folder" ) );
    return;
  }

  const Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob*>( job );
  const Akonadi::Collection::List collections = fetchJob->collections();

  if ( !collections.isEmpty() ) {
    const Akonadi::Collection collection = collections.first();
    // in case this is still the collection we are interested in, update
    if ( collection.id() == mCollection.id() ) {
      mCollection = collection;
      setCollectionFullPath( collection );
    }
  } else {
    // the requested collection doesn't exists anymore
    mCollection = Akonadi::Collection();
    edit->setText( i18n( "Please select a folder" ) );
  }
}

//-----------------------------------------------------------------------------
void FolderRequester::setFolder( const QString &idString )
{
  Akonadi::Collection folder = Kernel::self()->collectionFromId( idString );
  if ( folder.isValid() ) {
    setFolder( folder );
  } else {
    if ( !idString.isEmpty() ) {
      edit->setText( i18n( "Unknown folder '%1'", idString ) );
    } else {
      edit->setText( i18n( "Please select a folder" ) );
    }
  }
  mFolderId = idString;
}

//-----------------------------------------------------------------------------
void FolderRequester::keyPressEvent( QKeyEvent * e )
{
  if ( e->key() == Qt::Key_Space )
    slotOpenDialog();
  else
    e->ignore();
}
} // namespace KMail

#include "folderrequester.moc"
