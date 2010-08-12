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


#include "collection_filter_proxy_model.h"

#include "akobackit/akonadi_manager.h"
#include "akobackit/constant.h"
#include "akobackit/folder_manager.h"

#include <Akonadi/EntityTreeModel>
#include <Akonadi/AgentInstance>


namespace KNode {
namespace CollectionTree {

CollectionFilterProxyModel::CollectionFilterProxyModel( QObject *parent )
  : KRecursiveFilterProxyModel( parent )
{
  setSortCaseSensitivity( Qt::CaseInsensitive );
}

CollectionFilterProxyModel::~CollectionFilterProxyModel()
{
}


bool CollectionFilterProxyModel::acceptRow(int sourceRow, const QModelIndex& sourceParent) const
{
  const QModelIndex index = sourceModel()->index( sourceRow, 0, sourceParent );
  if( !index.isValid() ) {
    return false;
  }

  const Akonadi::Collection col = index.data( Akonadi::EntityTreeModel::CollectionRole )
                                       .value<Akonadi::Collection>();
  if ( !col.isValid() ) {
    return false;
  }

  // Note: we inherits KRecursiveFilterProxyModel because this test
  // does not work with the top-level collection in a resource.
  if( col.resource().startsWith( Akobackit::NNTP_RESOURCE_AGENTTYPE ) ) {
    return true;
  }

  const Akonadi::AgentInstance folderResource = Akobackit::manager()->folderManager()->foldersResource();
  if ( col.resource() == folderResource.identifier() ) {
    return true;
  }

  return false;
}

bool CollectionFilterProxyModel::lessThan( const QModelIndex &left, const QModelIndex &right ) const
{
  // Special comparaison is necessary for root collection only.
  if ( !left.parent().isValid() ) {
    Akonadi::Collection col = left.data( Akonadi::EntityTreeModel::CollectionRole )
                                  .value<Akonadi::Collection>();
    if ( Akobackit::manager()->type( col ) == Akobackit::RootFolder ) {
      return false;
    }
    col = right.data( Akonadi::EntityTreeModel::CollectionRole )
               .value<Akonadi::Collection>();
    if ( Akobackit::manager()->type( col ) == Akobackit::RootFolder ) {
      return true;
    }
  }

  // Default sorting by name
  return QSortFilterProxyModel::lessThan( left, right );
}


}
}

