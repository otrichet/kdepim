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


#ifndef KNODE_SUBSCRIPTIONSTATEMODEL_H
#define KNODE_SUBSCRIPTIONSTATEMODEL_H

#include <Akonadi/Collection>
#include <Akonadi/EntityTreeModel>
#include <QSortFilterProxyModel>


namespace KNode {


class SubscriptionStateModel : public QSortFilterProxyModel
{
  public:

    enum UserRole {
      /**
       * Role available throught data() for downstream proxies. A value in the enum
       * StateChange is associated.
       */
      SubscriptionChangeRole = Akonadi::EntityTreeModel::UserRole + 1
    };

    /**
     * Used in the model to indicate subscription overriding
     * to downstream proxy model.
     */
    enum StateChange {
      Other,                   ///< None of the belows.
      ExistingSubscription,    ///< Collection was mark as subscribed before recording of state change.
      NewSubscription,         ///< Collection has been subscribed.
      NewUnsubscription        ///< Collection has been unsubscribed.
    };

    SubscriptionStateModel( QObject *parent );
    virtual ~SubscriptionStateModel();


    void setOriginalSelection( const Akonadi::Collection::List &selection );

    /**
     * Returns the list of collection that were subscribed.
     */
    QList<Akonadi::Collection> subscribed() const;
    /**
     * Returns the list of collection that were unsubscribed.
     */
    QList<Akonadi::Collection> unsubscribed() const;


    /**
     * Reimplemented to monitor subscription changes.
     */
    virtual bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );
    /**
     * Reimplemented to provide data under the role SubscriptionOverrideRole.
     */
    virtual QVariant data( const QModelIndex &proxyIndex, int role = Qt::DisplayRole ) const;

  private:
    /**
     * Returns the Akonadi Collection store in @p index.
     */
    Akonadi::Collection collectionOfIndex( const QModelIndex &index ) const;

    QSet<Akonadi::Collection> mOriginalSelection;
    QHash<Akonadi::Collection, bool> mSubscriptionChange;
};

}

Q_DECLARE_METATYPE( KNode::SubscriptionStateModel::StateChange );

#endif

