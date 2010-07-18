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


#include "groupselector/subscription_state_model.h"

#include <KIcon>
#include <QFont>


namespace KNode {

SubscriptionStateModel::SubscriptionStateModel( QObject *parent )
  : QSortFilterProxyModel( parent )
{
}

SubscriptionStateModel::~SubscriptionStateModel()
{
}

void SubscriptionStateModel::setOriginalSelection( const Akonadi::Collection::List &selection )
{
  mOriginalSelection = selection.toSet();
}

QList<Akonadi::Collection> SubscriptionStateModel::subscribed() const
{
  return mSubscriptionChange.keys( true );
}
QList<Akonadi::Collection> SubscriptionStateModel::unsubscribed() const
{
  return mSubscriptionChange.keys( false );
}




Akonadi::Collection SubscriptionStateModel::collectionOfIndex( const QModelIndex &index ) const
{
  return index.data( Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
}


QVariant SubscriptionStateModel::data( const QModelIndex &proxyIndex, int role ) const
{
  switch ( role ) {
    case SubscriptionChangeRole:
      {
        const Akonadi::Collection c = collectionOfIndex( proxyIndex );
        if ( mSubscriptionChange.contains( c ) ) {
          const bool subscribe = mSubscriptionChange.value( c );
          return QVariant::fromValue( ( subscribe ? NewSubscription : NewUnsubscription ) );
        } else if ( mOriginalSelection.contains( c ) ) {
          return QVariant::fromValue( ExistingSubscription );
        } else {
          return QVariant::fromValue( Other );
        }
      }
      break;

    case Qt::DecorationRole:
      {
        const Akonadi::Collection c = collectionOfIndex( proxyIndex );
        if ( mSubscriptionChange.contains( c ) ) {
          return ( mSubscriptionChange.value( c ) ? KIcon( "news-subscribe" ) : KIcon( "news-unsubscribe" ) );
        } else if ( mOriginalSelection.contains( c ) ) {
          return KIcon( "view-pim-news" );
        } else {
          return QVariant(); // No icon if not subscribed, nor state changed.
        }
      }
  }

  return QAbstractProxyModel::data( proxyIndex, role );
}

bool SubscriptionStateModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  if ( role == SubscriptionChangeRole ) {
    bool subscribe;
    const SubscriptionStateModel::StateChange state = value.value<SubscriptionStateModel::StateChange>();
    if ( state == SubscriptionStateModel::NewSubscription ) {
      subscribe = true;
    } else if ( state == SubscriptionStateModel::NewUnsubscription ) {
      subscribe = false;
    } else {
      return false;
    }

    const Akonadi::Collection c = index.data( Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
    if ( mSubscriptionChange.contains( c ) ) {
      if ( mSubscriptionChange.value( c ) != subscribe ) {
        mSubscriptionChange.remove( c ); // change reverted
        emit dataChanged( index, index );
        return true;
      }
    } else {
      mSubscriptionChange.insert( c, subscribe );
      emit dataChanged( index, index );
      return true;
    }
    return false;
  }

  return QAbstractProxyModel::setData( index, value, role );
}


}
