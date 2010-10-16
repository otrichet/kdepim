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

#include "akobackit/group_manager.h"

#include "akobackit/akonadi_manager.h"
#include "akobackit/nntpaccount_manager.h"
#include "groupselector/group_subscription_dialog.h"
#include "groupselector/subscriptionjob_p.h"
#include "kngrouppropdlg.h"

#include <Akonadi/AgentManager>
#include <Akonadi/Collection>


namespace KNode {
namespace Akobackit {

GroupManager::GroupManager( AkoManager *parent )
  : QObject( parent ),
    mMainManager( parent )
{
}

GroupManager::~GroupManager()
{
}


bool GroupManager::isGroup( const Akonadi::Collection &col )
{
  if ( !col.isValid() ) {
    return false;
  }
  return ( col.resource().startsWith( NNTP_RESOURCE_AGENTTYPE )
           && col.parentCollection() != Akonadi::Collection::root() );
}

Group::List GroupManager::groups( NntpAccount::Ptr account )
{
  kDebug() << "AKONADI PORT: Not implemented";
  return Group::List();
}

Group::Ptr GroupManager::group( const Akonadi::Collection &collection )
{
  return Group::Ptr( new Group( collection ) );
}

NntpAccount::Ptr GroupManager::account( const Group::Ptr &group )
{
  return Akobackit::manager()->accountManager()->account( group->mCollection );
}




void GroupManager::editGroup( Group::Ptr group, QWidget *parentWidget )
{
  KNGroupPropDlg *dialog = new KNGroupPropDlg( group, parentWidget );
  dialog->open();
}

void GroupManager::saveGroup( Group::Ptr group )
{
  group->save();
}



void GroupManager::showSubscriptionDialog( NntpAccount::Ptr account, QWidget *parentWidget )
{
  GroupSubscriptionDialog *subscription = new GroupSubscriptionDialog( parentWidget, account );
  subscription->open();
}

void GroupManager::unsubscribeGroup( const Group::Ptr &group )
{
  Akonadi::SubscriptionJob *job = new Akonadi::SubscriptionJob();
  job->unsubscribe( Akonadi::Collection::List() << group->mCollection);
  job->start();
}




void GroupManager::fetchNewHeaders( Group::Ptr group, bool silent )
{
  Q_UNUSED( silent );
  // TODO: feed back in case of failure.
  Akonadi::AgentManager::self()->synchronizeCollection( group->mCollection );
}

void GroupManager::fetchNewHeaders( NntpAccount::Ptr account, bool silent )
{
  Q_UNUSED( silent );
  // TODO: feed back in case of failure.
  account->agent().synchronize();
}

void GroupManager::fetchNewHeaders( bool silent )
{
  NntpAccount::List accounts = mMainManager->accountManager()->accounts();
  foreach( const NntpAccount::Ptr account, accounts ) {
    // TODO: single feed back in case of failure for all account.
    fetchNewHeaders( account, silent );
  }
}



}
}

#include "akobackit/group_manager.moc"
