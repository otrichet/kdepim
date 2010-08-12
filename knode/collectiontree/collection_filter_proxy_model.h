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


#ifndef KNODE_COLLECTIONTREE_COLLECTIONFILTERPROXYMODEL_H
#define KNODE_COLLECTIONTREE_COLLECTIONFILTERPROXYMODEL_H

#include <krecursivefilterproxymodel.h>

namespace KNode {
namespace CollectionTree {

/**
 * Proxy model to filter:
 * @li Akonadi collections from NNTP resource
 * @li KNode's local folders.
 */
class CollectionFilterProxyModel : public KRecursiveFilterProxyModel
{
  public:
    CollectionFilterProxyModel( QObject *parent );
    virtual ~CollectionFilterProxyModel();

    /**
     * Accepts only NNTP account and our folders.
     */
    virtual bool acceptRow( int sourceRow, const QModelIndex &sourceParent ) const;
    /**
     * Places the folders after any NNTP accounts.
     */
    virtual bool lessThan( const QModelIndex &left, const QModelIndex &right ) const;
};

}

}

#endif
