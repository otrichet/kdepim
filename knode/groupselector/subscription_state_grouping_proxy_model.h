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


#ifndef KNODE_SUBSCRIPTIONSTATEGROUPINGPROXYMODEL_H
#define KNODE_SUBSCRIPTIONSTATEGROUPINGPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QVector>

namespace KNode {

/**
 * This model place the subscribed (respectively unsubscribed) groups
 * together under a common ancestor.
 *
 * It is assumed that an upstream proxy is a SubscriptionChangeFilterProxyModel.
 */
class SubscriptionStateGroupingProxyModel : public QAbstractProxyModel
{
  Q_OBJECT

  public:
    SubscriptionStateGroupingProxyModel( QObject *parent );
    virtual ~SubscriptionStateGroupingProxyModel();

  public:
    /**
     * Reimplemented to provide data for titles.
     */
    virtual QVariant data( const QModelIndex &proxyIndex, int role = Qt::DisplayRole ) const;
    /** @reimp */
    virtual Qt::ItemFlags flags( const QModelIndex &index ) const;
    /** @reimp */
    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;
    /** @reimp */
    virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;
    /** @reimp */
    virtual QModelIndex parent( const QModelIndex &child ) const;
    /** @reimp */
    virtual QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;

    /** @reimp */
    virtual QModelIndex mapFromSource( const QModelIndex &sourceIndex ) const;
    /** @reimp */
    virtual QModelIndex mapToSource( const QModelIndex &proxyIndex ) const;

    /** @reimp */
    virtual void setSourceModel( QAbstractItemModel *sourceModel );

  private slots:
    /**
     * Connected to the source model to update mSubscribed and mUnsubscribed.
     */
    void sourceModelRowsInserted( const QModelIndex &parent, int start, int end );
    /**
     * Connected to the source model to update mSubscribed and mUnsubscribed.
     */
    void sourceModelRowsAboutToBeRemoved( const QModelIndex &parent, int start, int end );
    /**
     * Connected to the source model to update mSubscribed and mUnsubscribed.
     */
    void sourceModelDataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight );


  private:
    QVector<QPersistentModelIndex> mSubscribed;
    QVector<QPersistentModelIndex> mUnsubscribed;
    QVector<QPersistentModelIndex> mExisting;

    /**
     * Returns the index of @p list to insert @p idx such as the QModelIndex in the list
     * are sorted by their display role.
     */
    int findInsertionPlace( QVector<QPersistentModelIndex> &list, const QModelIndex &idx ) const;
};

}

#endif
