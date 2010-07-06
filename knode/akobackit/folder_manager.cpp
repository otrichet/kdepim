/*
  Copyright 2010 Olivier Trichet <nive@nivalis.org>

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include "akobackit/folder_manager.h"

#include "akobackit/akonadi_manager.h"
#include "akobackit/item_merge_job.h"
#include "akobackit/localfolders_setup_job.h"
#include "knglobals.h"

#include <Akonadi/AgentInstanceCreateJob>
#include <Akonadi/AgentManager>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/CollectionCreateJob>
#include <Akonadi/CollectionDeleteJob>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/KMime/SpecialMailCollections>
#include <Akonadi/ItemDeleteJob>
#include <KConfigGroup>


namespace KNode {
namespace Akobackit {

FolderManager::FolderManager( AkoManager *parent )
  : QObject( parent ),
    mMainManager( parent )
{
}

FolderManager::~FolderManager()
{
}


/**
 * Local helper method to retrieve special folders.
 */
Akonadi::Collection specialFolder( Akonadi::SpecialMailCollections::Type type, const Akonadi::AgentInstance &resource )
{
  Akonadi::SpecialMailCollections *smc = Akonadi::SpecialMailCollections::self();
  return smc->collection( type, resource );
}

Akonadi::Collection FolderManager::rootFolder() const
{
  return specialFolder( Akonadi::SpecialMailCollections::Root, foldersResource() );
}
Akonadi::Collection FolderManager::outboxFolder() const
{
  return specialFolder( Akonadi::SpecialMailCollections::Outbox, foldersResource() );
}
Akonadi::Collection FolderManager::sentmailFolder() const
{
  return specialFolder( Akonadi::SpecialMailCollections::SentMail, foldersResource() );
}
Akonadi::Collection FolderManager::draftsFolder() const
{
  return specialFolder( Akonadi::SpecialMailCollections::Drafts, foldersResource() );
}

bool FolderManager::isFolder( const Akonadi::Collection &col )
{
  if ( !col.isValid() ) {
    return false;
  }
  return col.resource().startsWith( MAILDIR_RESOURCE_AGENTTYPE );
}

bool FolderManager::hasChild( const Akonadi::Collection &parent, const QString &childName )
{
  Akonadi::EntityTreeModel *model = mMainManager->collectionModel();
  const QModelIndex pIdx = Akonadi::EntityTreeModel::modelIndexForCollection( model, parent );
  if ( !pIdx.isValid() ) {
    return false;
  }

  for ( int i = 0 ; i < model->rowCount( pIdx ) ; ++i ) {
    const QModelIndex idx = pIdx.child( i, 0 );
    const Akonadi::Collection c = idx.data( Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
    // Note: doing a toLower() because MySQL unique constraint on parent/name is case insensitive
    if ( c.name().toLower() == childName.toLower() ) {
      return true;
    }
  }
  return false;
}




void FolderManager::createNewFolder( const Akonadi::Collection &parent, const QString &name )
{
  if ( !isFolder( parent ) ) {
    return;
  }
  Akonadi::Collection col;
  col.setParentCollection( parent );
  col.setName( name );
  Akonadi::CollectionCreateJob *job = new Akonadi::CollectionCreateJob( col );
  connect( job, SIGNAL( result( KJob * ) ), this, SLOT( folderCreationResult( KJob * ) ) );
}

void FolderManager::folderCreationResult( KJob *job )
{
  if ( job->error() ) {
    // TODO: display an error message (a new errorMessage(...) signal?)
    kError() << "Unable to create a folder:" << job->error() << job->errorString();
  } else {
    Akonadi::CollectionCreateJob *acj = static_cast<Akonadi::CollectionCreateJob*>( job );
    emit folderCreated( acj->collection() );
  }
}

void FolderManager::removeFolder( const Akonadi::Collection &f )
{
  if ( mMainManager->type( f ) != Akobackit::UserFolder ) {
    return;
  }

  // FIXME: special collection are not recreated
  Akonadi::CollectionDeleteJob *job = new Akonadi::CollectionDeleteJob( f );
  connect( job, SIGNAL( result( KJob * ) ), this, SLOT( folderDeletionResult( KJob * ) ) );
}

void FolderManager::folderDeletionResult ( KJob *job )
{
  if ( job->error() ) {
    // TODO: display an error message (a new errorMessage(...) signal?)
    kError() << "Unable to delete a folder:" << job->error() << job->errorString();
  } else {
    const Akonadi::AgentInstance resource = foldersResource();
    Q_ASSERT( resource.isValid() );
    if ( resource.isValid() ) {
      LocalFoldersSetupJob *job = new LocalFoldersSetupJob( resource, this, true/*ensureSpecialFoldersExistOnly*/ );
      job->start();
    }
  }
}

void FolderManager::emptyFolder( const Akonadi::Collection &folder )
{
  if ( !isFolder( folder ) ) {
    return;
  }

  Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob( folder, this );
  connect( job, SIGNAL( result( KJob * ) ), this, SLOT( folderEmptyingResult( KJob * ) ) );
}

void FolderManager::folderEmptyingResult( KJob *job )
{
  if ( job->error() ) {
    // TODO: display an error message (a new errorMessage(...) signal?)
    kError() << "Unable to empty a folder:" << job->error() << job->errorString();
  }
}

void FolderManager::moveIntoFolder( const LocalArticle::List &articles, const Akonadi::Collection &folder )
{
  if ( !folder.isValid() ) {
    // TODO: display an error message or move message in a default folder.
    return;
  }

  ItemsMergeJob *job = new ItemsMergeJob( articles, folder, this );
  // TODO: connect to a result slot
  job->start();
}




Akonadi::AgentInstance FolderManager::foldersResource( bool setup ) const
{
  Akonadi::AgentManager *mngr = Akonadi::AgentManager::self();

  KConfig *config = KNGlobals::self()->config();
  const KConfigGroup group = config->group( "Akonadi" ).group( "Folders" );
  const QString resourceId = group.readEntry<QString>( "resource", QString() );

  // Create the folder maildir if it does not exist yet/anymore.
  Akonadi::AgentInstance resource;
  if ( !resourceId.isEmpty() ) {
    resource = mngr->instance( resourceId );
  }

  if ( setup ) {
    LocalFoldersSetupJob *job = new LocalFoldersSetupJob( resource );
    connect( job, SIGNAL( result( KJob * ) ),
             this, SLOT( foldersResourceSetupResult( KJob * ) ) );
    connect( job, SIGNAL( resourceCreated( const Akonadi::AgentInstance & ) ),
             this, SLOT( foldersResourceCreated( const Akonadi::AgentInstance & ) ) );
    job->start();
  }

  return resource;
}

void FolderManager::foldersResourceSetupResult( KJob *job )
{
  if ( job->error() ) {
    // TODO: display a warning to the user?
    kError() << "An errors occurs while setting the local folder resource up:" << job->error() << job->errorString();
  }
}

void FolderManager::foldersResourceCreated( const Akonadi::AgentInstance &resource )
{
  Q_ASSERT( resource.isValid() );
  if ( !resource.isValid() ) {
    return;
  }

  KConfig *config = KNGlobals::self()->config();
  KConfigGroup group = config->group( "Akonadi" ).group( "Folders" );
  group.writeEntry( "resource", resource.identifier() );
  group.sync(); // ensure this is written to disk
}


}
}

#include "folder_manager.moc"
