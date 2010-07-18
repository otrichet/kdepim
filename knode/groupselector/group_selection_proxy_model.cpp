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


#include "groupselector/group_selection_proxy_model.h"

#include "groupselector/subscription_state_model.h"

namespace KNode {

GroupSelectionProxyModel::GroupSelectionProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent )
{
}

GroupSelectionProxyModel::~GroupSelectionProxyModel()
{
}



Qt::ItemFlags GroupSelectionProxyModel::flags( const QModelIndex &index ) const
{
  return QSortFilterProxyModel::flags( index ) | Qt::ItemIsUserCheckable;
}

QVariant GroupSelectionProxyModel::data( const QModelIndex &proxyIndex, int role ) const
{
  if ( role == Qt::CheckStateRole ) {
    QVariant state = proxyIndex.data( SubscriptionStateModel::SubscriptionChangeRole )
                               .value<SubscriptionStateModel::StateChange>();
    if ( state == SubscriptionStateModel::NewSubscription || state == SubscriptionStateModel::ExistingSubscription ) {
      return Qt::Checked;
    } else {
      return Qt::Unchecked;
    }
  }

  return QAbstractProxyModel::data( proxyIndex, role );
}

bool GroupSelectionProxyModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  if ( role == Qt::CheckStateRole ) {
    SubscriptionStateModel::StateChange newState;
    if ( value.toUInt() == Qt::Checked ) {
      newState = SubscriptionStateModel::NewSubscription;
    } else {
      newState = SubscriptionStateModel::NewUnsubscription;
    }

    const QVariant newValue = QVariant::fromValue( newState );
    return setData( index, newValue, SubscriptionStateModel::SubscriptionChangeRole );
  }

  return QSortFilterProxyModel::setData( index, value, role );
}


}
