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


#ifndef KNODE_GROUPSELECTIONPROXYMODEL_H
#define KNODE_GROUPSELECTIONPROXYMODEL_H

#include <QSortFilterProxyModel>

namespace KNode {

/**
 * This proxy convert checking state to/form SubscriptionStateModel::StateChange.
 *
 * This proxy model is made to work downstream a SubscriptionStateModel.
 */
class GroupSelectionProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

  public:
    GroupSelectionProxyModel( QObject *parent );
    virtual ~GroupSelectionProxyModel();


    /**
     * @reimp
     * Enable checking.
     */
    virtual Qt::ItemFlags flags( const QModelIndex &index ) const;

    /**
     * Reimplemented to convert (un)checking of items into change to
     * an upstream SubscriptionStateModel.
     */
    virtual bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );
    /**
     * Reimplemented to display checkbox.
     */
    virtual QVariant data( const QModelIndex &proxyIndex, int role = Qt::DisplayRole ) const;
};

}

#endif
