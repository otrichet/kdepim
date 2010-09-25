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

#include "akobackit/akonadi_manager.h"

#include "akobackit/folder_manager.h"
#include "akobackit/group_manager.h"
#include "akobackit/localfolders_setup_job.h"
#include "akobackit/nntpaccount_manager.h"
#include "knglobals.h"

#include <Akonadi/ChangeRecorder>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Session>
#include <KGlobal>

namespace KNode {
namespace Akobackit {


class AkoManagerPrivate
{
  public:
    AkoManager instance;
};

K_GLOBAL_STATIC( AkoManagerPrivate, akoManagerPrivate )



AkoManager * manager()
{
  Q_ASSERT ( !akoManagerPrivate.isDestroyed() );

  return &akoManagerPrivate->instance;
}




AkoManager::AkoManager( QObject *parent )
  : QObject( parent ),
    mMonitor( 0 ),
    mEntityModel( 0 ),
    mSession( new Akonadi::Session( "KNode", this ) ),
    mAccountManager( new NntpAccountManager( this ) ),
    mFolderManager( new FolderManager( this ) ),
    mGroupManager( new GroupManager( this ) )
{
  setObjectName( "KNode::Akobackit::AkoManager" );
}

AkoManager::~AkoManager()
{
}


Akonadi::ChangeRecorder * AkoManager::monitor()
{
  if ( !mMonitor ) {
    // Monitor
    mMonitor = new Akonadi::ChangeRecorder( this );
    mMonitor->setAllMonitored( true );
    // EntityTreeModel requiers that only one collection is monitored
    mMonitor->setCollectionMonitored( Akonadi::Collection::root() );
//     mMonitor->fetchCollectionStatistics( true );
    mMonitor->setSession( mSession );

    mMonitor->fetchCollection( true );
    mMonitor->itemFetchScope().fetchFullPayload( true );
  }
  return mMonitor;
}

Akonadi::EntityTreeModel* AkoManager::entityModel()
{
  if ( !mEntityModel ) {
    mEntityModel = new Akonadi::EntityTreeModel( monitor(), this );
    mEntityModel->setItemPopulationStrategy( Akonadi::EntityTreeModel::LazyPopulation );
    mEntityModel->setIncludeUnsubscribed( false );
  }
  return mEntityModel;
}

Akonadi::Session * AkoManager::session()
{
  return mSession;
}




CollectionType AkoManager::type( const Akonadi::Collection &col )
{
  if ( !col.isValid() ) {
    return Akobackit::InvalidCollection;
  }

  if ( col.resource().startsWith( MAILDIR_RESOURCE_AGENTTYPE ) ) {
    if ( col == folderManager()->rootFolder() ) {
      return Akobackit::RootFolder;
    } else if ( col == folderManager()->outboxFolder() ) {
      return Akobackit::OutboxFolder;
    } else if ( col == folderManager()->sentmailFolder() ) {
      return Akobackit::SentmailFolder;
    } else if ( col == folderManager()->draftsFolder() ) {
      return Akobackit::DraftFolder;
    } else {
      return Akobackit::UserFolder;
    }
  } else if ( col.resource().startsWith( NNTP_RESOURCE_AGENTTYPE ) ) {
    if ( col.parentCollection() == Akonadi::Collection::root() ) {
      return Akobackit::NntpServer;
    } else {
      return Akobackit::NewsGroup;
    }
  }

  kError() << "Collection not handled: " << col;
  return Akobackit::InvalidCollection;
}


}
}

#include "akobackit/akonadi_manager.moc"
