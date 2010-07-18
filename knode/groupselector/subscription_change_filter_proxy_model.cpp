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


#include "groupselector/subscription_change_filter_proxy_model.h"

#include "groupselector/subscription_state_model.h"

#include <Akonadi/Collection>
#include <Akonadi/EntityTreeModel>

namespace KNode {

SubscriptionChangeFilterProxyModel::SubscriptionChangeFilterProxyModel( QObject* parent )
  : KRecursiveFilterProxyModel( parent )
{
}

SubscriptionChangeFilterProxyModel::~SubscriptionChangeFilterProxyModel()
{
}


bool SubscriptionChangeFilterProxyModel::acceptRow( int sourceRow, const QModelIndex &sourceParent ) const
{
  const QModelIndex index = sourceModel()->index( sourceRow, 0, sourceParent );
  const QVariant value = index.data( SubscriptionStateModel::SubscriptionChangeRole );
  if ( !value.isValid() ) {
    return false;
  }
  SubscriptionStateModel::StateChange override = value.value<SubscriptionStateModel::StateChange>();
  return ( override == SubscriptionStateModel::NewSubscription
            || override == SubscriptionStateModel::NewUnsubscription
            || override == SubscriptionStateModel::ExistingSubscription );
}


}