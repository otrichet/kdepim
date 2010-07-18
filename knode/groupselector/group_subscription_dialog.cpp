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


#include "groupselector/group_subscription_dialog.h"

#include "akobackit/akonadi_manager.h"
#include "groupselector/group_selection_proxy_model.h"
#include "groupselector/subscription_change_filter_proxy_model.h"
#include "groupselector/subscription_state_grouping_proxy_model.h"
#include "groupselector/subscription_state_model.h"
#include "subscriptionjob_p.h" // FIXME: file copied from kdepimlibs. It should be public interface

#include <Akonadi/ChangeRecorder>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/EntityTreeModel>


namespace KNode {

GroupSubscriptionDialog::GroupSubscriptionDialog( QWidget *parent, NntpAccount::Ptr account )
  : KDialog( parent ),
    mSubscriptionModel( 0 ),
    mSubscribedFetched( false ),
    mRoot( false )
{
  setupUi( this );
  setCaption( i18nc( "@title:window", "Subscribe to Newsgroups" ) );
  setMainWidget( page );
  if ( QApplication::isLeftToRight() ) {
    mRevertFromGroupViewButton->setIcon( KIcon( "arrow-right" ) );
    mRevertFromChangeViewButton->setIcon( KIcon( "arrow-left" ) );
  } else {
    mRevertFromGroupViewButton->setIcon( KIcon( "arrow-left" ) );
    mRevertFromChangeViewButton->setIcon( KIcon( "arrow-right" ) );
  }

  Akonadi::CollectionFetchJob *fetchRoot =
        new Akonadi::CollectionFetchJob( Akonadi::Collection::root(), Akonadi::CollectionFetchJob::FirstLevel, this );
  fetchRoot->fetchScope().setResource( account->agent().identifier() );
  fetchRoot->setProperty( "fetch-type", "root" );
  connect( fetchRoot, SIGNAL( result( KJob * ) ),
           this, SLOT( fetchResult( KJob * ) ) );

  Akonadi::CollectionFetchJob *fetchSubscribed =
        new Akonadi::CollectionFetchJob( Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive, this );
  fetchSubscribed->fetchScope().setResource( account->agent().identifier() );
  fetchSubscribed->fetchScope().setIncludeUnsubscribed( false );
  fetchSubscribed->setProperty( "fetch-type", "subscribed" );
  connect( fetchSubscribed, SIGNAL( result( KJob * ) ),
           this, SLOT( fetchResult( KJob * ) ) );
}

GroupSubscriptionDialog::~GroupSubscriptionDialog()
{
}



void GroupSubscriptionDialog::fetchResult( KJob *job )
{
  if ( job->error() ) {
    // TODO: error to deal with
    kDebug() << "Error" << job->error() << job->errorString();
  } else {
    Akonadi::CollectionFetchJob *fetchJob = static_cast<Akonadi::CollectionFetchJob *>( job );

    if ( fetchJob->property( "fetch-type" ) == "subscribed" ) {
      mSubscribedCollections = fetchJob->collections();
      mSubscribedFetched = true;
    } else if ( fetchJob->property( "fetch-type" ) == "root" ) {
      Q_ASSERT( fetchJob->collections().size() == 1 );
      if ( fetchJob->collections().size() == 0 ) {
        // TODO: error to deal with
        return;
      }
      mRoot = fetchJob->collections().at( 0 );
      mRootFetched = true;
    }
  }

  if ( mRootFetched && mSubscribedFetched ) {
    initModelView();
  }
}


void GroupSubscriptionDialog::initModelView()
{
  Akonadi::ChangeRecorder *monitor = new Akonadi::ChangeRecorder( this );
  monitor->fetchCollection( true );
  monitor->setAllMonitored( true );
  // EntityTreeModel requiers that only one collection is monitored
  monitor->setCollectionMonitored( mRoot );
  monitor->fetchCollectionStatistics( false );
  monitor->collectionFetchScope().setAncestorRetrieval( Akonadi::CollectionFetchScope::None );
  monitor->collectionFetchScope().setIncludeUnsubscribed( true );
  monitor->setSession( Akobackit::manager()->session() );

  // Entity tree model
  Akonadi::EntityTreeModel *model = new Akonadi::EntityTreeModel( monitor, this );
  model->setItemPopulationStrategy( Akonadi::EntityTreeModel::NoItemPopulation );
  // Proxy that keep trace of (un)subscription change
  mSubscriptionModel = new SubscriptionStateModel( this );
  mSubscriptionModel->setOriginalSelection( mSubscribedCollections );
  mSubscriptionModel->setSourceModel( model );

  // Main view and its proxy model
  GroupSelectionProxyModel *groupModel = new GroupSelectionProxyModel( this );
  groupModel->setSourceModel( mSubscriptionModel );
  mGroupsView->setModel( groupModel );

  // View of subscription changes and its model
  SubscriptionChangeFilterProxyModel *subscribedModel = new SubscriptionChangeFilterProxyModel( this );
  subscribedModel->setSourceModel( mSubscriptionModel );
  SubscriptionStateGroupingProxyModel *stateGroupingProxy = new SubscriptionStateGroupingProxyModel( this );
  stateGroupingProxy->setSourceModel( subscribedModel );
  mChangeView->setModel( stateGroupingProxy );
  connect( stateGroupingProxy, SIGNAL( rowsInserted( const QModelIndex &, int, int ) ),
           mChangeView, SLOT( expand( const QModelIndex & ) ) );


  // Filter
  groupModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
  connect( mFilterLineEdit, SIGNAL( textChanged( const QString & ) ),
           groupModel, SLOT( setFilterWildcard( const QString & ) ) );

  // Operation on selection
  connect( mRevertFromGroupViewButton, SIGNAL( clicked( bool ) ),
           this, SLOT( revertStateChangeFromGroupView() ) );
  connect( mRevertFromChangeViewButton, SIGNAL( clicked( bool ) ),
           this, SLOT( revertStateChangeFromChangeView()) );
  connect( mChangeView->selectionModel(), SIGNAL( selectionChanged( QItemSelection, QItemSelection ) ),
           this, SLOT( slotSelectionChange() ) );
  connect( mGroupsView->selectionModel(), SIGNAL( selectionChanged( QItemSelection, QItemSelection ) ),
           this, SLOT( slotSelectionChange() ) );
}



void GroupSubscriptionDialog::slotButtonClicked( int button )
{
  if ( button == KDialog::Ok ) {
    Akonadi::SubscriptionJob *job = new Akonadi::SubscriptionJob();
    job->subscribe( mSubscriptionModel->subscribed() );
    job->unsubscribe( mSubscriptionModel->unsubscribed() );
    job->start();
  }
  KDialog::slotButtonClicked( button );
}


void GroupSubscriptionDialog::revertStateChangeFromGroupView()
{
  revertSelectionStateChange( mGroupsView );
}

void GroupSubscriptionDialog::revertStateChangeFromChangeView()
{
  revertSelectionStateChange( mChangeView );
}

void GroupSubscriptionDialog::revertSelectionStateChange( QAbstractItemView *view )
{
  // Operate on QPersistentModelIndex because call
  // to setData() below will invalidates QModelIndex.

  QList<QPersistentModelIndex> persistentIndices;
  const QModelIndexList &indexes = view->selectionModel()->selectedIndexes();
  foreach( const QModelIndex &index, indexes ) {
    persistentIndices << index;
  }

  QAbstractItemModel *model = view->model();
  foreach( const QPersistentModelIndex &index, persistentIndices) {
    QVariant v = index.data( SubscriptionStateModel::SubscriptionChangeRole );
    if ( !v.isValid() ) {
      continue;
    }
    SubscriptionStateModel::StateChange state = v.value<SubscriptionStateModel::StateChange>();
    if ( view == mChangeView ) {
      if ( state == SubscriptionStateModel::NewSubscription ) {
        state = SubscriptionStateModel::NewUnsubscription;
      } else if ( state == SubscriptionStateModel::NewUnsubscription ) {
        state = SubscriptionStateModel::NewSubscription;
      } else {
        continue;
      }
    } else if ( view == mGroupsView ) {
      if ( state == SubscriptionStateModel::ExistingSubscription ) {
        state = SubscriptionStateModel::NewUnsubscription;
      } else if ( state == SubscriptionStateModel::Other ) {
        state = SubscriptionStateModel::NewSubscription;
      } else {
        continue;
      }
    } else {
      continue;
    }

    model->setData( index,
                    QVariant::fromValue( state ),
                    SubscriptionStateModel::SubscriptionChangeRole );
  }
}

void GroupSubscriptionDialog::slotSelectionChange()
{
  mRevertFromGroupViewButton->setEnabled( mGroupsView->selectionModel()->hasSelection() );
  mRevertFromChangeViewButton->setEnabled( mChangeView->selectionModel()->hasSelection() );
}


}


#include "groupselector/group_subscription_dialog.moc"
