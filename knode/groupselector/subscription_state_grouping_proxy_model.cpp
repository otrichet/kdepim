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


#include "groupselector/subscription_state_grouping_proxy_model.h"

#include "groupselector/subscription_state_model.h"

#include <KLocalizedString>
#include <QVector>
#include <QFont>


static const int SUBSCRIBE_TITLE_ROW = 0;
static const int UNSUBSCRIBE_TITLE_ROW = 1;
static const int EXISTING_TITLE_ROW = 2;

static const int COLUMN = 0;

static const quint32 INTERNAL_ID_TITLE = 0;
static const quint32 INTERNAL_ID_SUBSCRIBED = 1;
static const quint32 INTERNAL_ID_UNSUBSCRIBED = 2;
static const quint32 INTERNAL_ID_EXISTING = 3;


namespace KNode {

SubscriptionStateGroupingProxyModel::SubscriptionStateGroupingProxyModel( QObject *parent )
  : QAbstractProxyModel( parent )
{
}

SubscriptionStateGroupingProxyModel::~SubscriptionStateGroupingProxyModel()
{
}


QModelIndex SubscriptionStateGroupingProxyModel::mapFromSource( const QModelIndex &sourceIndex ) const
{
  if ( !sourceIndex.isValid() ) {
    return QModelIndex();
  }

  const QPersistentModelIndex index( sourceIndex );

  int idx = mSubscribed.indexOf( index );
  if ( idx != -1 ) {
    return createIndex( idx, COLUMN, INTERNAL_ID_SUBSCRIBED );
  }

  idx = mUnsubscribed.indexOf( index );
  if ( idx != -1 ) {
    return createIndex( idx, COLUMN, INTERNAL_ID_UNSUBSCRIBED );
  }

  idx = mExisting.indexOf( index );
  if ( idx != -1 ) {
    return createIndex( idx, COLUMN, INTERNAL_ID_EXISTING );
  }

  return QModelIndex();
}

QModelIndex SubscriptionStateGroupingProxyModel::mapToSource( const QModelIndex &proxyIndex ) const
{
  if ( !proxyIndex.isValid() ) {
    return QModelIndex();
  }

  switch ( proxyIndex.internalId() ) {
    case INTERNAL_ID_SUBSCRIBED:
      Q_ASSERT( proxyIndex.row() < mSubscribed.size() );
      return mSubscribed.at( proxyIndex.row() );
    case INTERNAL_ID_UNSUBSCRIBED:
      Q_ASSERT( proxyIndex.row() < mUnsubscribed.size() );
      return mUnsubscribed.at( proxyIndex.row() );
    case INTERNAL_ID_EXISTING:
      Q_ASSERT( proxyIndex.row() < mExisting.size() );
      return mExisting.at( proxyIndex.row() );
  }

  return QModelIndex();
}




QVariant SubscriptionStateGroupingProxyModel::data( const QModelIndex &proxyIndex, int role ) const
{
  if ( proxyIndex.internalId() == INTERNAL_ID_TITLE ) {
    switch ( role ) {
      case Qt::DisplayRole:
        if ( proxyIndex.row() == SUBSCRIBE_TITLE_ROW ) {
          return i18nc( "@title %1:number of groups", "Subscribed groups (%1)", mSubscribed.size() );
        }
        if ( proxyIndex.row() == UNSUBSCRIBE_TITLE_ROW ) {
          return i18nc( "@title %1:number of groups", "Unsubscribed groups (%1)", mUnsubscribed.size() );
        }
        if ( proxyIndex.row() == EXISTING_TITLE_ROW ) {
          return i18nc( "@title %1:number of groups", "Current subscription (%1)", mExisting.size() );
        }
        break;

      case Qt::FontRole:
        {
        QFont f = QAbstractProxyModel::data( proxyIndex, role ).value<QFont>();
        f.setBold( true );
        return QVariant::fromValue( f );
        }
        break;

      default:
        return QVariant();
        break;
    }
  }

  return QAbstractProxyModel::data( proxyIndex, role );
}

Qt::ItemFlags SubscriptionStateGroupingProxyModel::flags( const QModelIndex &index ) const
{
  qint64 internalId = index.internalId();
  if ( internalId == INTERNAL_ID_TITLE || internalId == INTERNAL_ID_EXISTING ) {
    return Qt::NoItemFlags; // Inactive, unselectable, etc.
  }
  return QAbstractProxyModel::flags( index );
}




int SubscriptionStateGroupingProxyModel::rowCount( const QModelIndex &parent ) const
{
  if ( !parent.isValid() ) {
    return 3;
  }

  if ( parent.internalId() == INTERNAL_ID_TITLE ) {
    switch ( parent.row() ) {
      case SUBSCRIBE_TITLE_ROW:
        return mSubscribed.size();
      case UNSUBSCRIBE_TITLE_ROW:
        return mUnsubscribed.size();
      case EXISTING_TITLE_ROW:
        return mExisting.size();
    }
  }

  return 0;
}

int SubscriptionStateGroupingProxyModel::columnCount( const QModelIndex &parent ) const
{
  Q_UNUSED( parent );
  return 1;
}

QModelIndex SubscriptionStateGroupingProxyModel::parent( const QModelIndex &child ) const
{
  switch ( child.internalId() ) {
    case INTERNAL_ID_SUBSCRIBED:
      return createIndex( SUBSCRIBE_TITLE_ROW, COLUMN, INTERNAL_ID_TITLE );
    case INTERNAL_ID_UNSUBSCRIBED:
      return createIndex( UNSUBSCRIBE_TITLE_ROW, COLUMN, INTERNAL_ID_TITLE );
    case INTERNAL_ID_EXISTING:
      return createIndex( EXISTING_TITLE_ROW, COLUMN, INTERNAL_ID_TITLE );
  }

  return QModelIndex();
}

QModelIndex SubscriptionStateGroupingProxyModel::index( int row, int column, const QModelIndex &parent ) const
{
  if ( !parent.isValid() ) {
    if ( row == SUBSCRIBE_TITLE_ROW || row == UNSUBSCRIBE_TITLE_ROW || row == EXISTING_TITLE_ROW ) {
      return createIndex( row, column, 0 );
    }
    return QModelIndex();
  }

  if ( parent.internalId() == INTERNAL_ID_TITLE ) {
    if ( parent.row() == SUBSCRIBE_TITLE_ROW && row < mSubscribed.size() ) {
      return createIndex( row, column, INTERNAL_ID_SUBSCRIBED );
    }
    if ( parent.row() == UNSUBSCRIBE_TITLE_ROW && row < mUnsubscribed.size() ) {
      return createIndex( row, column, INTERNAL_ID_UNSUBSCRIBED );
    }
    if ( parent.row() == EXISTING_TITLE_ROW && row < mExisting.size() ) {
      return createIndex( row, column, INTERNAL_ID_EXISTING );
    }
    return QModelIndex();
  }

  return QModelIndex();
}

void SubscriptionStateGroupingProxyModel::setSourceModel( QAbstractItemModel *source )
{
  beginResetModel();

  if ( sourceModel() ) {
    disconnect( sourceModel(), SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ),
                this, SLOT( sourceModelDataChanged( const QModelIndex &, const QModelIndex & ) ) );
    disconnect( sourceModel(), SIGNAL( rowsInserted( const QModelIndex &, int, int ) ),
                this, SLOT( sourceModelRowsInserted( const QModelIndex &, int, int ) ) );
    disconnect( sourceModel(), SIGNAL( rowsAboutToBeRemoved( const QModelIndex &, int, int ) ),
                this, SLOT( sourceModelRowsAboutToBeRemoved( const QModelIndex &, int, int ) ) );
  }

  QAbstractProxyModel::setSourceModel( source );

  mSubscribed.clear();
  mUnsubscribed.clear();
  mExisting.clear();
  connect( source, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ),
           this, SLOT( sourceModelDataChanged( const QModelIndex &, const QModelIndex & ) ) );
  connect( source, SIGNAL( rowsInserted( const QModelIndex &, int, int ) ),
           this, SLOT( sourceModelRowsInserted( const QModelIndex &, int, int ) ) );
  connect( source, SIGNAL( rowsAboutToBeRemoved( const QModelIndex &, int, int ) ),
           this, SLOT( sourceModelRowsAboutToBeRemoved( const QModelIndex &, int, int ) ) );

  endResetModel();
}



void SubscriptionStateGroupingProxyModel::sourceModelRowsInserted( const QModelIndex &parent, int start, int end )
{
  Q_ASSERT( start <= end );

#define INSERT_SOURCE_INDEX( SourceModelIndexList, TitleRow ) \
  { \
      int i = findInsertionPlace( SourceModelIndexList, index ); \
      const QModelIndex parent = createIndex( TitleRow, COLUMN, INTERNAL_ID_TITLE ); \
      beginInsertRows( parent, i, i ); \
      SourceModelIndexList.insert( i, index ); \
      endInsertRows(); \
      emit dataChanged( parent, parent ); \
  } \
  break;

  for( int row = start ; row <= end ; ++row ) {
    const QModelIndex index = sourceModel()->index( row, 0, parent );

    const QVariant value = index.data( SubscriptionStateModel::SubscriptionChangeRole );
    SubscriptionStateModel::StateChange state = value.value<SubscriptionStateModel::StateChange>();
    switch ( state ) {
      case SubscriptionStateModel::NewSubscription:
        INSERT_SOURCE_INDEX( mSubscribed, SUBSCRIBE_TITLE_ROW )
      case SubscriptionStateModel::NewUnsubscription:
        INSERT_SOURCE_INDEX( mUnsubscribed, UNSUBSCRIBE_TITLE_ROW )
      case SubscriptionStateModel::ExistingSubscription:
        INSERT_SOURCE_INDEX( mExisting, EXISTING_TITLE_ROW )
      case SubscriptionStateModel::Other:
        break;
    }
  }
}

void SubscriptionStateGroupingProxyModel::sourceModelRowsAboutToBeRemoved( const QModelIndex &parent, int start, int end )
{
  Q_ASSERT( start <= end );

#define TRY_REMOVE_INDEX_ROW( SourceModelIndexList, TitleRow ) \
    { \
      int i = SourceModelIndexList.indexOf( index ); \
      if ( i != -1 ) { \
        const QModelIndex parent = createIndex( TitleRow, COLUMN, INTERNAL_ID_TITLE ); \
        beginRemoveRows( parent, i, i ); \
        SourceModelIndexList.remove( i ); \
        endRemoveRows(); \
        emit dataChanged( parent, parent ); \
        continue; \
      } \
    }

  for( int row = start ; row <= end ; ++row ) {
    const QPersistentModelIndex index( sourceModel()->index( row, 0, parent ) );
    TRY_REMOVE_INDEX_ROW( mSubscribed, SUBSCRIBE_TITLE_ROW );
    TRY_REMOVE_INDEX_ROW( mUnsubscribed, UNSUBSCRIBE_TITLE_ROW );
    TRY_REMOVE_INDEX_ROW( mExisting, EXISTING_TITLE_ROW );
  }
}

void SubscriptionStateGroupingProxyModel::sourceModelDataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight )
{
  Q_ASSERT( topLeft.row() <= bottomRight.row() );
  Q_ASSERT( topLeft.parent() == bottomRight.parent() );

  sourceModelRowsAboutToBeRemoved( topLeft.parent(), topLeft.row(), bottomRight.row() );
  sourceModelRowsInserted( topLeft.parent(), topLeft.row(), bottomRight.row() );
}


int SubscriptionStateGroupingProxyModel::findInsertionPlace( QVector<QPersistentModelIndex> &list, const QModelIndex &idx ) const
{
  const QString name = idx.data( Qt::DisplayRole ).toString();
  int i = 0;
  while ( i < list.size() ) {
    const QString listItemName = list.at( i ).data( Qt::DisplayRole ).toString();
    if ( name < listItemName ) {
      break;
    }
    ++i;
  }
  return i;
}


}

#include "groupselector/subscription_state_grouping_proxy_model.moc"
