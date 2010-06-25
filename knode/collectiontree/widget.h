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

#ifndef KNODE_COLLECTIONTREE_WIDGET_H
#define KNODE_COLLECTIONTREE_WIDGET_H

#include <QtGui/QSplitter>

namespace Akonadi {
  class Collection;
  class EntityTreeViewStateSaver;
  class SelectionProxyModel;
}
class KXMLGUIClient;

namespace KNode {
namespace CollectionTree {

class View;

/**
 * The collections (accounts, groups and folder) widget.
 */
class Widget : public QSplitter
{
  Q_OBJECT

  public:
    Widget( KXMLGUIClient *guiClient, QWidget *parent );
    virtual ~Widget();

    /**
     * Returns the currently selected collection or an invalid
     * collection if none is selected.
     */
    Akonadi::Collection selectedCollection() const;

    /**
     * Start editing the name of a collection.
     * @note this changes the current collection.
     * @param col The collection to rename.
     */
    void renameCollection( const Akonadi::Collection &col );

  signals:
    /**
     * This signal is emitted when the selected collection changes.
     * @param col The new selected collection.
     */
    void selectedCollectionChanged( const Akonadi::Collection &col );

  private slots:
    /**
     * Initialize this widget.
     */
    void init();

  private:
    View *mTreeView;
    Akonadi::EntityTreeViewStateSaver *mViewSaver;
    Akonadi::SelectionProxyModel *mSelectionModel;
};

}
}

#endif
