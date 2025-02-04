/******************************************************************************
 *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#ifndef __MESSAGELIST_CORE_VIEW_H__
#define __MESSAGELIST_CORE_VIEW_H__

#include <QTreeView>
#include <QList>
#include <QPoint>

#include <messagelist/core/enums.h>
#include <messagelist/messagelist_export.h>

class KMenu;

class QTimer;

namespace Akonadi
{
  class MessageStatus;
}

namespace MessageList
{

namespace Core
{

typedef long int MessageItemSetReference;

class Aggregation;
class Delegate;
class Item;
class MessageItem;
class Model;
class Theme;
class SortOrder;
class StorageModel;
class Widget;

/**
 * The MessageList::View is the real display of the message list. It is
 * based on QTreeView, has a Model that manipulates the underlying message storage
 * and a Delegate that is responsable of painting the items.
 */
class MESSAGELIST_EXPORT View : public QTreeView
{
  friend class Model;
  friend class ModelPrivate;
  Q_OBJECT
public:
  View( Widget *parent );
  ~View();

  /**
   * Returns the Model attacched to this View. You probably never need to manipulate
   * it directly.
   */
  Model * model() const;

  /**
   * Returns the Delegate attacched to this View. You probably never need to manipulate
   * it directly. Model uses it to obtain size hints.
   */
  Delegate * delegate() const;

  /**
   * Sets the StorageModel to be displayed in this view. The StorageModel may be 0 (so no content is displayed).
   * Setting the StorageModel will obviously trigger a view reload.
   * Be sure to set the Aggregation and the Theme BEFORE calling this function.
   *
   * Pre-selection is the action of automatically selecting a message just after the folder
   * has finished loading. See Model::setStorageModel() for more information.
   */
  void setStorageModel( StorageModel * storageModel, PreSelectionMode preSelectionMode = PreSelectLastSelected );

  /**
   * Returns the currently displayed StorageModel. May be 0.
   */
  StorageModel * storageModel() const;

  /**
   * Sets the aggregation for this view.
   * Does not trigger a reload of the view: you *MUST* trigger it manually.
   */
  void setAggregation( const Aggregation * aggregation );

  /**
   * Sets the specified theme for this view.
   * Does not trigger a reload of the view: you *MUST* trigger it manually.
   */
  void setTheme( Theme * theme );

  /**
   * Sets the specified sort order.
   * Does not trigger a reload of the view: you *MUST* trigger it manually.
   */
  void setSortOrder( const SortOrder * sortOrder );

  /**
   * Triggers a reload of the view in order to re-display the current folder.
   * Call this function after changing the Aggregation or the Theme.
   */
  void reload();

  /**
   * Returns the current MessageItem (that is bound to current StorageModel).
   * May return 0 if there is no current message or no current StorageModel.
   * If the current message item isn't currently selected (so is only focused)
   * then it's selected when this function is called, unless selectIfNeeded is false.
   */
  MessageItem * currentMessageItem( bool selectIfNeeded = true ) const;

  /**
   * Sets the current message item.
   */
  void setCurrentMessageItem( MessageItem * it, bool center = false );

  /**
   * Returns true if the specified item is currently displayed in the tree
   * and has all the parents expanded. This means that the user can
   * see the message (by eventually scrolling the view).
   */
  bool isDisplayedWithParentsExpanded( Item * it ) const;

  /**
   * Makes sure that the specified is currently viewable by the user.
   * This means that the user can see the message (by eventually scrolling the view).
   */
  void ensureDisplayedWithParentsExpanded( Item * it );

  /**
   * Returns the currently selected MessageItems (bound to current StorageModel).
   * The list may be empty if there are no selected messages or no StorageModel.
   *
   * If includeCollapsedChildren is true then the children of the selected but
   * collapsed items are also added to the list.
   *
   * The returned list is guaranteed to be valid only until you return control
   * to the main even loop. Don't store it for any longer. If you need to reference
   * this set of messages at a later stage then take a look at createPersistentSet().
   */
  QList< MessageItem * > selectionAsMessageItemList( bool includeCollapsedChildren = true ) const;

  /**
   * Returns the MessageItems bound to the current StorageModel that
   * are part of the current thread. The current thread is the thread
   * that contains currentMessageItem().
   * The list may be empty if there is no currentMessageItem() or no StorageModel.
   *
   * The returned list is guaranteed to be valid only until you return control
   * to the main even loop. Don't store it for any longer. If you need to reference
   * this set of messages at a later stage then take a look at createPersistentSet().
   */
  QList< MessageItem * > currentThreadAsMessageItemList() const;

  /**
   * Fast function that determines if the selection is empty
   */
  bool selectionEmpty() const;

  /**
   * Selects the specified MessageItems. The current selection is NOT removed.
   * Use clearSelection() for that purpose.
   */
  void selectMessageItems( const QList< MessageItem * > &list );

  /**
   * Creates a persistent set for the specified MessageItems and
   * returns its reference. Later you can use this reference
   * to retrieve the list of MessageItems that are still valid.
   * See persistentSetCurrentMessageList() for that.
   *
   * Persistent sets consume resources (both memory and CPU time
   * while manipulating the view) so be sure to call deletePersistentSet()
   * when you no longer need it.
   */
  MessageItemSetReference createPersistentSet( const QList< MessageItem * > &items );

  /**
   * Returns the list of MessageItems that are still existing in the
   * set pointed by the specified reference. This list will contain
   * at most the messages that you have passed to createPersistentSet()
   * but may contain less (even 0) if these MessageItem object were removed
   * from the view for some reason.
   */
  QList< MessageItem * > persistentSetCurrentMessageItemList( MessageItemSetReference ref );

  /**
   * Deletes the persistent set pointed by the specified reference.
   * If the set does not exist anymore, nothing happens.
   */
  void deletePersistentSet( MessageItemSetReference ref );

  /**
   * If bMark is true this function marks the messages as "about to be removed"
   * so they appear dimmer and aren't selectable in the view.
   * If bMark is false then this function clears the "about to be removed" state
   * for the specified MessageItems.
   */
  void markMessageItemsAsAboutToBeRemoved( QList< MessageItem * > &items, bool bMark );

  /**
   * Returns true if the current Aggregation is threaded, false otherwise
   * (or if there is no current Aggregation).
   */
  bool isThreaded() const;

  /**
   * If expand is true then it expands the current thread, otherwise
   * collapses it.
   */
  void setCurrentThreadExpanded( bool expand );

  /**
   * If expand is true then it expands all the threads, otherwise
   * collapses them.
   */
  void setAllThreadsExpanded( bool expand );

  /**
   * If expand is true then it expands all the groups (only the toplevel
   * group item: inner threads are NOT expanded). If expand is false
   * then it collapses all the groups. If no grouping is in effect
   * then this function does nothing.
   */
  void setAllGroupsExpanded( bool expand );

  /**
   * Selects the next message item in the view.
   *
   * messageTypeFilter can be used to limit the selection to
   * a certain category of messages.
   *
   * existingSelectionBehaviour specifies how the existing selection
   * is manipulated. It may be cleared, expanded or grown/shrinked.
   *
   * If centerItem is true then the specified item will be positioned
   * at the center of the view, if possible.
   * If loop is true then the "next" algorithm will restart from the beginning
   * of the list if the end is reached, otherwise it will just stop returning false.
   *
   * \sa MessageList::Core::MessageTypeFilter
   * \sa MessageList::Core::ExistingSelectionBehaviour
   */
  bool selectNextMessageItem(
      MessageTypeFilter messageTypeFilter,
      ExistingSelectionBehaviour existingSelectionBehaviour,
      bool centerItem,
      bool loop
    );

  /**
   * Selects the previous message item in the view.
   *
   * messageTypeFilter can be used to limit the selection to
   * a certain category of messages.
   *
   * existingSelectionBehaviour specifies how the existing selection
   * is manipulated. It may be cleared, expanded or grown/shrinked.
   *
   * If centerItem is true then the specified item will be positioned
   * at the center of the view, if possible.
   * If loop is true then the "previous" algorithm will restart from the end
   * of the list if the beginning is reached, otherwise it will just stop returning false.
   *
   * \sa MessageList::Core::MessageTypeFilter
   * \sa MessageList::Core::ExistingSelectionBehaviour
   */
  bool selectPreviousMessageItem(
      MessageTypeFilter messageTypeFilter,
      ExistingSelectionBehaviour existingSelectionBehaviour,
      bool centerItem,
      bool loop
    );

  /**
   * Focuses the next message item in the view without actually selecting it.
   *
   * messageTypeFilter can be used to limit the selection to
   * a certain category of messages.
   *
   * If centerItem is true then the specified item will be positioned
   * at the center of the view, if possible.
   * If loop is true then the "next" algorithm will restart from the beginning
   * of the list if the end is reached, otherwise it will just stop returning false.
   */
  bool focusNextMessageItem( MessageTypeFilter messageTypeFilter, bool centerItem, bool loop );

  /**
   * Focuses the previous message item in the view without actually selecting it.
   * If unread is true then focuses the previous unread message item.
   * If centerItem is true then the specified item will be positioned
   * at the center of the view, if possible.
   * If loop is true then the "previous" algorithm will restart from the end
   * of the list if the beginning is reached, otherwise it will just stop returning false.
   */
  bool focusPreviousMessageItem( MessageTypeFilter messageTypeFilter, bool centerItem, bool loop );

  /**
   * Selects the currently focused message item. If the currently focused
   * message is already selected (which is very likely) nothing happens.
   * If centerItem is true then the specified item will be positioned
   * at the center of the view, if possible.
   */
  void selectFocusedMessageItem( bool centerItem );

  /**
   * Selects the first message item in the view that matches messageTypeFilter.
   * If centerItem is true then the specified item will be positioned
   * at the center of the view, if possible.
   */
  bool selectFirstMessageItem( MessageTypeFilter messageTypeFilter, bool centerItem );

  /**
   * Fills menu by adding actions for manipulating the view.
   */
  void fillViewMenu( KMenu * menu );

  /**
   * Sets the focus on the quick search line of the currently active tab.
   */
  void focusQuickSearch();

  /**
   * Returns the Akonadi::MessageStatus in the current quicksearch field.
   */
  Akonadi::MessageStatus currentFilterStatus() const;

  /**
   * Returns the search term in the current quicksearch field.
   */
  QString currentFilterSearchString() const;

  /**
   * Called to hide or show the specified row from the view.
   * @reimp
   */
  virtual void setRowHidden( int row, const QModelIndex & parent, bool hide );

protected:
  /**
   * Reimplemented in order to catch QHelpEvent
   */
  virtual bool event( QEvent *e );

  /**
   * Reimplemented in order to catch palette, font and style changes
   */
  virtual void changeEvent( QEvent *e );

  /**
   * Reimplemented in order to apply theme column widths on the first show
   */
  virtual void showEvent( QShowEvent *e );

  /**
   * Reimplemented in order to handle clicks with sub-item precision.
   */
  virtual void mousePressEvent( QMouseEvent * e );

  /**
   * Reimplemented in order to handle double clicks with sub-item precision.
   */
  virtual void mouseDoubleClickEvent( QMouseEvent * e );

  /**
   * Reimplemented in order to handle DnD
   */
  virtual void mouseMoveEvent( QMouseEvent * e );

  /**
   * Reimplemented in order to handle message DnD
   */
  virtual void dragEnterEvent( QDragEnterEvent * e );

  /**
   * Reimplemented in order to handle message DnD
   */
  virtual void dragMoveEvent( QDragMoveEvent * e );

  /**
   * Reimplemented in order to handle message DnD
   */
  virtual void dropEvent( QDropEvent * e );

  /**
   * Reimplemented in order to resize columns when header is not visible
   */
  virtual void resizeEvent( QResizeEvent * e );

  /**
   * Reimplemented in order to kill the QTreeView column auto-resizing
   */
  virtual int sizeHintForColumn( int logicalColumnIndex ) const;

  /**
   * Reimplemented in order to disable update of the geometries
   * while a job step is running (as it takes a very long time and it's called for every item insertion...)
   */
  virtual void updateGeometries();

  /**
   *  Used to enable/disable the ignoring of updateGeometries() calls.
   */
  void ignoreUpdateGeometries( bool ignore );

  /**
   * This is called by the model from inside setFolder().
   * It's called just after the model has been reset but before
   * any row has been inserted. This allows us to call updateThemeColumns() as needed.
   */
  void modelHasBeenReset();

  void modelAboutToEmitLayoutChanged();
  void modelEmittedLayoutChanged();

  /**
   * Recursive helper for currentThreadAsMessageItemList()
   */
  void appendMessageItemChildren( MessageItem * par, QList< MessageItem * > &list );

  /**
   * This is called by the model to insulate us from certain QTreeView signals
   * This is because they may be spurious (caused by Model item rearrangements).
   */
  void ignoreCurrentChanges( bool ignore );

  /**
   * Expands or collapses the children of the specified item, recursively.
   */
  void setChildrenExpanded( const Item * parent, bool expand );

  /**
   * Finds the next message item with respect to the current item.
   * If there is no current item then the search starts from the beginning.
   * Returns 0 if no next message could be found.
   *
   * messageTypeFilter can be used to limit the selection to
   * a certain category of messages.
   * If loop is true then restarts from the beginning if end is
   * reached, otherwise it just returns 0 in this case.
   */
  Item * nextMessageItem( MessageTypeFilter messageTypeFilter, bool loop );

  /**
   * Finds message item that comes "after" the reference item.
   * If reference item is 0 then the search starts from the beginning.
   * Returns 0 if no next message could be found.
   *
   * messageTypeFilter can be used to limit the selection to
   * a certain category of messages.
   * If loop is true then restarts from the beginning if end is
   * reached, otherwise it just returns 0 in this case.
   */
  Item * messageItemAfter( Item * referenceItem, MessageTypeFilter messageTypeFilter, bool loop );

  /**
   * Finds the first message item in the view.
   *
   * messageTypeFilter can be used to limit the selection to
   * a certain category of messages.
   *
   * Returns 0 if the view is empty.
   */
  Item * firstMessageItem( MessageTypeFilter messageTypeFilter )
    { return messageItemAfter( 0, messageTypeFilter, false ); };

  /**
   * Finds the previous message item with respect to the current item.
   * If there is no current item then the search starts from the end.
   * Returns 0 if no previous message could be found.
   *
   * messageTypeFilter can be used to limit the selection to
   * a certain category of messages.
   * If loop is true then restarts from the end if beginning is
   * reached, otherwise it just return 0 in this case.
   */
  Item * previousMessageItem( MessageTypeFilter messageTypeFilter, bool loop );

  /**
   * Returns the deepest child that is visible (i.e. not in a collapsed tree) of
   * the specified reference item.
   */
  Item * deepestExpandedChild( Item * referenceItem ) const;

  /**
   * Finds message item that comes "before" the reference item.
   * If reference item is 0 then the search starts from the end.
   * Returns 0 if no next message could be found.
   *
   * messageTypeFilter can be used to limit the selection to
   * a certain category of messages.
   * If loop is true then restarts from the beginning if end is
   * reached, otherwise it just returns 0 in this case.
   */
  Item * messageItemBefore( Item * referenceItem, MessageTypeFilter messageTypeFilter, bool loop );

  /**
   * Finds the last message item in the view.
   *
   * messageTypeFilter can be used to limit the selection to
   * a certain category of messages.
   *
   * Returns 0 if the view is empty.
   */
  Item * lastMessageItem( MessageTypeFilter messageTypeFilter )
    { return messageItemBefore( 0, messageTypeFilter, false ); };

  /**
   * This is called by Model to signal a start of a lengthy job batch.
   * Note that this is NOT called for jobs that can be completed in a single step.
   */
  void modelJobBatchStarted();

  /**
   * This is called by Model to signal the end of a lengthy job batch.
   * Note that this is NOT called for jobs that can be completed in a single step.
   */
  void modelJobBatchTerminated();

  /**
   * This is called by Model to signal that the initial loading stage of a newly
   * attached StorageModel is terminated.
   */
  void modelFinishedLoading();

  /**
   * Performs a change in the specified MessageItem status.
   * It first applies the change to the cached state in MessageItem and
   * then requests our parent widget to act on the storage.
   */
  void changeMessageStatus( MessageItem * it, const Akonadi::MessageStatus &set, const Akonadi::MessageStatus &unset );

  /**
   * Starts a short-delay timer connected to saveThemeColumnState().
   * Used to accumulate consecutive changes and break out of the call stack
   * up to the main event loop (since in the call stack the column state might be left undefined).
   */
  void triggerDelayedSaveThemeColumnState();

  /**
   * Starts a short-delay timer connected to applyThemeColumns().
   * Used to accumulate consecutive changes and break out of the call stack
   * up to the main event loop (since multiple resize events tend to be sent by Qt at startup).
   */
  void triggerDelayedApplyThemeColumns();

  /**
   * This is used by the selection functions to grow/shrink the existing selection
   * according to the newly selected item passed as parameter.
   * If movingUp is true then: if the newly selected item is above the current selection top
   * then the selection is expanded, otherwise it's shrunk. If movingUp is false then: if the
   * newly selected item is below the current selection bottom then the selection is expanded
   * otherwise it's shrunk.
   */
  void growOrShrinkExistingSelection( const QModelIndex &newSelectedIndex, bool movingUp );

public slots:
  /**
   * Collapses all the group headers (if present in the current Aggregation)
   */
  void slotCollapseAllGroups();

  /**
   * Expands all the group headers (if present in the current Aggregation)
   */
  void slotExpandAllGroups();

protected slots:

  /**
   * Handles context menu requests for the header.
   */
  void slotHeaderContextMenuRequested( const QPoint &pnt );

  /**
   * Handles header context menu selections. Shows or hides
   * the header columns.
   */
  void slotHeaderContextMenuTriggered( QAction * act );

  /**
   * Handles section resizes in order to save the column widths
   */
  void slotHeaderSectionResized( int logicalIndex, int oldWidth, int newWidth );

  /**
   * Handles selection item management
   */
  void slotSelectionChanged( const QItemSelection &current, const QItemSelection & );

  /**
   * Saves the state of the columns (width and visility) to the currently selected theme object.
   */
  void saveThemeColumnState();

  /**
   * Applies the theme columns to this view.
   * Columns visible by default are shown, the other are hidden.
   * Visible columns are assigned space inside the view by using the size hints and some heuristics.
   */
  void applyThemeColumns();

private:
  /// expand the whole thread (including all child messages)
  Q_PRIVATE_SLOT(d, void expandFullThread( const QModelIndex& ))

  class Private;
  Private * d;
}; // class View

} // namespace Core

} // namespace MessageList


#endif //!__MESSAGELIST_CORE_VIEW_H__

