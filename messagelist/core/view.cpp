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

#include "core/view.h"
#include "core/aggregation.h"
#include "core/delegate.h"
#include "core/groupheaderitem.h"
#include "core/item.h"
#include "core/manager.h"
#include "core/messageitem.h"
#include "core/model.h"
#include "core/theme.h"
#include "core/settings.h"
#include "core/storagemodelbase.h"
#include "core/widgetbase.h"

#include "messagecore/stringutil.h"

#include <kmime/kmime_dateformatter.h> // kdepimlibs

#include <QHelpEvent>
#include <QToolTip>
#include <QHeaderView>
#include <QTimer>
#include <QPaintEvent>
#include <QTextDocument>
#include <QApplication>

#include <KMenu>
#include <KLocale>
#include <KDebug>
#include <KGlobalSettings>
#include <KIcon>

using namespace MessageList::Core;


class View::Private
{
public:
  Private( View *owner, Widget *parent )
    : q( owner ), mWidget( parent ), mModel( 0 ), mDelegate( new Delegate( owner ) ),
      mAggregation( 0 ), mTheme( 0 ), mNeedToApplyThemeColumns( false ),
      mLastCurrentItem( 0 ), mFirstShow( true ), mSaveThemeColumnStateOnSectionResize( true ),
      mSaveThemeColumnStateTimer( 0 ), mApplyThemeColumnsTimer( 0 ),
      mIgnoreUpdateGeometries( false ) { }

  void expandFullThread( const QModelIndex &index );

  View * const q;

  Widget *mWidget;
  Model *mModel;
  Delegate *mDelegate;

  const Aggregation *mAggregation;          ///< The Aggregation we're using now, shallow pointer
  Theme *mTheme;                            ///< The Theme we're using now, shallow pointer
  bool mNeedToApplyThemeColumns;            ///< Flag signaling a pending application of theme columns
  Item *mLastCurrentItem;
  QPoint mMousePressPosition;
  bool mFirstShow;
  bool mSaveThemeColumnStateOnSectionResize;      ///< This is used to filter out programmatic column resizes in slotSectionResized().
  QTimer * mSaveThemeColumnStateTimer;            ///< Used to trigger a delayed "save theme state"
  QTimer * mApplyThemeColumnsTimer;               ///< Used to trigger a delayed "apply theme columns"
  bool mIgnoreUpdateGeometries;                   ///< Shall we ignore the "update geometries" calls ?
};

View::View( Widget *pParent )
  : QTreeView( pParent ), d( new Private( this, pParent ) )
{
  d->mSaveThemeColumnStateTimer = new QTimer();
  connect( d->mSaveThemeColumnStateTimer, SIGNAL( timeout() ), this, SLOT( saveThemeColumnState() ) );

  d->mApplyThemeColumnsTimer = new QTimer();
  connect( d->mApplyThemeColumnsTimer, SIGNAL( timeout() ), this, SLOT( applyThemeColumns() ) );

  setItemDelegate( d->mDelegate );
  setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
  setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
  setAlternatingRowColors( true );
  setAllColumnsShowFocus( true );
  setSelectionMode( QAbstractItemView::ExtendedSelection );
  viewport()->setAcceptDrops( true );

  header()->setContextMenuPolicy( Qt::CustomContextMenu );
  connect( header(), SIGNAL( customContextMenuRequested( const QPoint& ) ),
           SLOT( slotHeaderContextMenuRequested( const QPoint& ) ) );
  connect( header(), SIGNAL( sectionResized( int, int, int ) ),
           SLOT( slotHeaderSectionResized( int, int ,int ) ) );

  header()->setClickable( true );
  header()->setResizeMode( QHeaderView::Interactive );
  header()->setMinimumSectionSize( 2 ); // QTreeView overrides our sections sizes if we set them smaller than this value
  header()->setDefaultSectionSize( 2 ); // QTreeView overrides our sections sizes if we set them smaller than this value

  d->mModel = new Model( this );
  setModel( d->mModel );

  connect( d->mModel, SIGNAL( statusMessage( const QString & ) ),
           pParent, SIGNAL( statusMessage( const QString & ) ) );

  //connect( selectionModel(), SIGNAL( currentChanged( const QModelIndex &, const QModelIndex & ) ),
  //         this, SLOT( slotCurrentIndexChanged( const QModelIndex &, const QModelIndex & ) ) );
  connect( selectionModel(), SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
           this, SLOT( slotSelectionChanged( const QItemSelection &, const QItemSelection & ) ),
           Qt::UniqueConnection );

  // as in KDE3, when a root-item of a message thread is expanded, expand all children
  connect( this, SIGNAL( expanded ( const QModelIndex & ) ), this, SLOT( expandFullThread( const QModelIndex & ) ) );
}

View::~View()
{
  if ( d->mSaveThemeColumnStateTimer->isActive() )
    d->mSaveThemeColumnStateTimer->stop();
  delete d->mSaveThemeColumnStateTimer;
  if ( d->mApplyThemeColumnsTimer->isActive() )
    d->mApplyThemeColumnsTimer->stop();
  delete d->mApplyThemeColumnsTimer;

  // Zero out the theme, aggregation and ApplyThemeColumnsTimer so Model will not cause accesses to them in its destruction process
  d->mApplyThemeColumnsTimer = 0;

  d->mTheme = 0;
  d->mAggregation = 0;

  delete d; d = 0;
}

Model *View::model() const
{
  return d->mModel;
}

Delegate *View::delegate() const
{
  return d->mDelegate;
}

void View::ignoreCurrentChanges( bool ignore )
{
  if ( ignore )
  {
    disconnect( selectionModel(), SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
                this, SLOT( slotSelectionChanged( const QItemSelection &, const QItemSelection & ) ) );
    viewport()->setUpdatesEnabled( false );
  } else {
    connect( selectionModel(), SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
             this, SLOT( slotSelectionChanged( const QItemSelection &, const QItemSelection & ) ),
             Qt::UniqueConnection );
    viewport()->setUpdatesEnabled( true );
  }
}

void View::ignoreUpdateGeometries( bool ignore )
{
  d->mIgnoreUpdateGeometries = ignore;
}

void View::updateGeometries()
{
  if( d->mIgnoreUpdateGeometries )
    return;
  QTreeView::updateGeometries();
}

StorageModel * View::storageModel() const
{
  return d->mModel->storageModel();
}

void View::setAggregation( const Aggregation * aggregation )
{
  d->mAggregation = aggregation;
  d->mModel->setAggregation( aggregation );

  // use uniform row heights to speed up, but only if there are no group headers used
  setUniformRowHeights( d->mAggregation->grouping() == Aggregation::NoGrouping );
}

void View::setTheme( Theme * theme )
{
  d->mNeedToApplyThemeColumns = true;
  d->mTheme = theme;
  d->mDelegate->setTheme( theme );
  d->mModel->setTheme( theme );
}

void View::setSortOrder( const SortOrder * sortOrder )
{
  d->mModel->setSortOrder( sortOrder );
}

void View::reload()
{
  setStorageModel( storageModel() );
}

void View::setStorageModel( StorageModel * storageModel, PreSelectionMode preSelectionMode )
{
  // This will cause the model to be reset.
  d->mSaveThemeColumnStateOnSectionResize = false;
  d->mModel->setStorageModel( storageModel, preSelectionMode );
  d->mSaveThemeColumnStateOnSectionResize = true;
}

void View::modelJobBatchStarted()
{
  // This is called by the model when the first job of a batch starts
  d->mWidget->viewJobBatchStarted();
}

void View::modelJobBatchTerminated()
{
  // This is called by the model when all the pending jobs have been processed
  d->mWidget->viewJobBatchTerminated();
}

void View::modelHasBeenReset()
{
  // This is called by Model when it has been reset.
  if ( d && d->mNeedToApplyThemeColumns )
    applyThemeColumns();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Theme column state machinery
//
// This is yet another beast to beat. The QHeaderView behaviour, at the time of writing,
// is quite unpredictable. This is due to the complex interaction with the model, with the QTreeView
// and due to its attempts to delay the layout jobs. The delayed layouts, especially, may
// cause the widths of the columns to quickly change in an unexpected manner in a place
// where previously they have been always settled to the values you set...
//
// So here we have the tools to:
//
// - Apply the saved state of the theme columns (applyThemeColumns()).
//   This function computes the "best fit" state of the visible columns and tries
//   to apply it to QHeaderView. It also saves the new computed state to the Theme object.
//
// - Explicitly save the column state, used when the user changes the widths or visibility manually.
//   This is called through a delayed timer after a column has been resized or used directly
//   when the visibility state of a column has been changed by toggling a popup menu entry.
//
// - Display the column state context popup menu and handle its actions
//
// - Apply the theme columns when the theme changes, when the model changes or in certain
//   ugly corner cases when the widget is resized or shown.
//
// - Avoid saving a corrupted column state in that QHeaderView can be found *very* frequently.
//

void View::applyThemeColumns()
{
  if ( !d->mApplyThemeColumnsTimer ) {
    return;
  }

  if ( d->mApplyThemeColumnsTimer->isActive() )
    d->mApplyThemeColumnsTimer->stop();

  if ( !d->mTheme )
    return;

  //kDebug() << "Apply theme columns";


  const QList< Theme::Column * > & columns = d->mTheme->columns();

  if ( columns.count() < 1 )
    return; // bad theme

  if ( !viewport()->isVisible() )
    return; // invisible

  if ( viewport()->width() < 1 )
    return; // insane width

  // Now we want to distribute the available width on all the visible columns.
  //
  // The rules:
  // - The visible columns will span the width of the view, if possible.
  // - The columns with a saved width should take that width.
  // - The columns on the left should take more space, if possible.
  // - The columns with no text take just slightly more than their size hint.
  //   while the columns with text take possibly a lot more.
  //

  // Note that the first column is always shown (it can't be hidden at all)

  // The algorithm below is a sort of compromise between:
  // - Saving the user preferences for widths
  // - Using exactly the available view space
  //
  // It "tends to work" in all cases:
  // - When there are no user preferences saved and the column widths must be
  //   automatically computed to make best use of available space
  // - When there are user preferences for only some of the columns
  //   and that should be somewhat preserved while still using all the
  //   available space.
  // - When all the columns have well defined saved widths

  QList< Theme::Column * >::ConstIterator it;
  int idx = 0;

  // Gather total size "hint" for visible sections: if the widths of the columns wers
  // all saved then the total hint is equal to the total saved width.

  int totalVisibleWidthHint = 0;
  QList< int > lColumnSizeHints;

  for ( it = columns.begin(); it != columns.end(); ++it )
  {
    if ( ( *it )->currentlyVisible() || ( idx == 0 ) )
    {
      //kDebug() << "Column " << idx << " will be visible";
      // Column visible
      int savedWidth = ( *it )->currentWidth();
      int hintWidth = d->mDelegate->sizeHintForItemTypeAndColumn( Item::Message, idx ).width();
      totalVisibleWidthHint += savedWidth > 0 ? savedWidth : hintWidth;
      lColumnSizeHints.append( hintWidth );
      //kDebug() << "Column " << idx << " size hint is " << hintWidth;
    } else {
      //kDebug() << "Column " << idx << " will be not visible";
      // The column is not visible
      lColumnSizeHints.append( -1 ); // dummy
    }
    idx++;
  }

  if ( totalVisibleWidthHint < 16 )
    totalVisibleWidthHint = 16; // be reasonable

  // Now compute somewhat "proportional" widths.
  idx = 0;

  QList< int > lColumnWidths;
  int totalVisibleWidth = 0;

  for ( it = columns.begin(); it != columns.end(); ++it )
  {
    int savedWidth = ( *it )->currentWidth();
    int hintWidth = savedWidth > 0 ? savedWidth : lColumnSizeHints[ idx ];
    int realWidth;

    if ( ( *it )->currentlyVisible() || ( idx == 0 ) )
    {
      if ( ( *it )->containsTextItems() )
      {
         // the column contains text items, it should get more space (if possible)
         realWidth = ( ( hintWidth * viewport()->width() ) / totalVisibleWidthHint );
      } else {
         // the column contains no text items, it should get exactly its hint/saved width.
         realWidth = hintWidth;
      }

      if ( realWidth < 2 )
        realWidth = 2; // don't allow very insane values

      totalVisibleWidth += realWidth;
    } else {
      // Column not visible
      realWidth = -1;
    }

    lColumnWidths.append( realWidth );

    idx++;
  }

  // Now the algorithm above may be wrong for several reasons...
  // - We're using fixed widths for certain columns and proportional
  //   for others...
  // - The user might have changed the width of the view from the
  //   time in that the widths have been saved
  // - There are some (not well identified) issues with the QTreeView
  //   scrollbar that make our view appear larger or shorter by 2-3 pixels
  //   sometimes.
  // - ...
  // So we correct the previous estimates by trying to use exactly
  // the available space.

  idx = 0;

  if ( totalVisibleWidth != viewport()->width() )
  {
    // The estimated widths were not using exactly the available space.
    if ( totalVisibleWidth < viewport()->width() )
    {
      // We were using less space than available.

      // Give the additional space to the text columns
      // also give more space to the first ones and less space to the last ones
      int available = viewport()->width() - totalVisibleWidth;

      for ( it = columns.begin(); it != columns.end(); ++it )
      {
        if ( ( ( *it )->currentlyVisible() || ( idx == 0 ) ) && ( *it )->containsTextItems() )
        {
          // give more space to this column
          available >>= 1; // eat half of the available space
          lColumnWidths[ idx ] += available; // and give it to this column
          if ( available < 1 )
            break; // no more space to give away
        }

        idx++;
      }

      // if any space is still available, give it to the first column
      if ( available )
        lColumnWidths[ 0 ] += available;
    } else {
      // We were using more space than available

      // If the columns span just a little bit more than the view then
      // try to squeeze them in order to make them fit
      if ( totalVisibleWidth < ( viewport()->width() + 100 ) )
      {
        int missing = totalVisibleWidth - viewport()->width();
        int count = lColumnWidths.count();

        if ( missing > 0 )
        {
          idx = count - 1;

          while ( idx >= 0 )
          {
            if ( columns.at( idx )->currentlyVisible() || ( idx == 0 ) )
            {
              int chop = lColumnWidths[ idx ] - lColumnSizeHints[ idx ];
              if ( chop > 0 )
              {
                if ( chop > missing )
                  chop = missing;
                lColumnWidths[ idx ] -= chop;
                missing -= chop;
                if ( missing < 1 )
                  break; // no more space to recover
              }
            } // else it's invisible
            idx--;
          }
        }
      }
    }
  }

  // We're ready to assign widths.

  bool oldSave = d->mSaveThemeColumnStateOnSectionResize;
  d->mSaveThemeColumnStateOnSectionResize = false;

  // A huge problem here is that QHeaderView goes quite nuts if we show or hide sections
  // while resizing them. This is because it has several machineries aimed to delay
  // the layout to the last possible moment. So if we show a column, it will tend to
  // screw up the layout of other ones.

  // We first loop showing/hiding columns then.

  idx = 0;

  //kDebug() << "Entering column show/hide loop";

  for ( it = columns.begin(); it != columns.end(); ++it )
  {
    bool visible = ( idx == 0 ) || ( *it )->currentlyVisible();
    //kDebug() << "Column " << idx << " visible " << visible;
    ( *it )->setCurrentlyVisible( visible );
    header()->setSectionHidden( idx, !visible );
    idx++;
  }

  // Then we loop assigning widths. This is still complicated since QHeaderView tries
  // very badly to stretch the last section and thus will resize it in the meantime.
  // But seems to work most of the times...

  idx = 0;
  totalVisibleWidth = 0;

  for ( it = columns.begin(); it != columns.end(); ++it )
  {
    if ( ( *it )->currentlyVisible() )
    {
      //kDebug() << "Resize section " << idx << " to " << lColumnWidths[ idx ];
      ( *it )->setCurrentWidth( lColumnWidths[ idx ] );
      header()->resizeSection( idx, lColumnWidths[ idx ] );
      totalVisibleWidth += lColumnWidths[ idx ];
    } else {
      ( *it )->setCurrentWidth( -1 );
    }
    idx++;
  }

  totalVisibleWidth = 0;
  idx = 0;

  bool bTriggeredQtBug = false;

  for ( QList< Theme::Column * >::ConstIterator it = columns.begin(); it != columns.end(); ++it )
  {
    if ( !header()->isSectionHidden( idx ) )
    {
      if ( !( *it )->currentlyVisible() )
      {
        bTriggeredQtBug = true;
      }
      totalVisibleWidth += header()->sectionSize( idx );
    }
    idx++;
  }

  setHeaderHidden( d->mTheme->viewHeaderPolicy() == Theme::NeverShowHeader );

  d->mSaveThemeColumnStateOnSectionResize = oldSave;
  d->mNeedToApplyThemeColumns = false;

  static bool bAllowRecursion = true;

  if (bTriggeredQtBug && bAllowRecursion)
  {
    bAllowRecursion = false;
    //kDebug() << "I've triggered the QHeaderView bug: trying to fix by calling myself again";
    applyThemeColumns();
    bAllowRecursion = true;
  }
}

void View::triggerDelayedApplyThemeColumns()
{
  if ( d->mApplyThemeColumnsTimer->isActive() )
    d->mApplyThemeColumnsTimer->stop();
  d->mApplyThemeColumnsTimer->setSingleShot( true );
  d->mApplyThemeColumnsTimer->start( 100 );
}

void View::saveThemeColumnState()
{
  if ( d->mSaveThemeColumnStateTimer->isActive() )
    d->mSaveThemeColumnStateTimer->stop();

  if ( !d->mTheme )
    return;

  if ( d->mNeedToApplyThemeColumns )
    return; // don't save the state if it hasn't been applied at all

  //kDebug() << "Save theme column state";

  const QList< Theme::Column * > & columns = d->mTheme->columns();

  if ( columns.count() < 1 )
    return; // bad theme

  int idx = 0;


  for ( QList< Theme::Column * >::ConstIterator it = columns.begin(); it != columns.end(); ++it )
  {
    if ( header()->isSectionHidden( idx ) )
    {
      //kDebug() << "Section " << idx << " is hidden";
      ( *it )->setCurrentlyVisible( false );
      ( *it )->setCurrentWidth( -1 ); // reset (hmmm... we could use the "don't touch" policy here too...)
    } else {
      //kDebug() << "Section " << idx << " is visible and has size " << header()->sectionSize( idx );
      ( *it )->setCurrentlyVisible( true );
      ( *it )->setCurrentWidth( header()->sectionSize( idx ) );
    }
    idx++;
  }
}

void View::triggerDelayedSaveThemeColumnState()
{
  if ( d->mSaveThemeColumnStateTimer->isActive() )
    d->mSaveThemeColumnStateTimer->stop();
  d->mSaveThemeColumnStateTimer->setSingleShot( true );
  d->mSaveThemeColumnStateTimer->start( 200 );
}

void View::resizeEvent( QResizeEvent * e )
{
  kDebug() << "Resize event enter (viewport width is " << viewport()->width() << ")";

  QTreeView::resizeEvent( e );

  if ( !isVisible() )
    return; // don't play with

  if ( (!d->mFirstShow) && d->mNeedToApplyThemeColumns )
    triggerDelayedApplyThemeColumns();

  if ( header()->isVisible() )
    return;

  // header invisible

  bool oldSave = d->mSaveThemeColumnStateOnSectionResize;
  d->mSaveThemeColumnStateOnSectionResize = false;

  if ( ( header()->count() - header()->hiddenSectionCount() ) < 2 )
  {
    // a single column visible: resize it
    int visibleIndex;
    int count = header()->count();
    for ( visibleIndex = 0; visibleIndex < count; visibleIndex++ )
    {
      if ( !header()->isSectionHidden( visibleIndex ) )
        break;
    }
    if ( visibleIndex < count )
      header()->resizeSection( visibleIndex, viewport()->width() - 4 );
  }

  d->mSaveThemeColumnStateOnSectionResize = oldSave;

  triggerDelayedSaveThemeColumnState();
}

void View::modelAboutToEmitLayoutChanged()
{
  // QHeaderView goes totally NUTS with a layoutChanged() call
  d->mSaveThemeColumnStateOnSectionResize = false;
}

void View::modelEmittedLayoutChanged()
{
  // This is after a first chunk of work has been done by the model: do apply column states
  d->mSaveThemeColumnStateOnSectionResize = true;
  applyThemeColumns();
}

void View::slotHeaderSectionResized( int logicalIndex, int oldWidth, int newWidth )
{
  Q_UNUSED( logicalIndex );
  Q_UNUSED( oldWidth );
  Q_UNUSED( newWidth );

  if ( d->mSaveThemeColumnStateOnSectionResize )
    triggerDelayedSaveThemeColumnState();
}

int View::sizeHintForColumn( int logicalColumnIndex ) const
{
  // QTreeView: please don't touch my column widths...
  int w = header()->sectionSize( logicalColumnIndex );
  if ( w > 0 )
    return w;
  if ( !d->mDelegate )
    return 32; // dummy
  w = d->mDelegate->sizeHintForItemTypeAndColumn( Item::Message, logicalColumnIndex ).width();
  return w;
}

void View::showEvent( QShowEvent *e )
{
  QTreeView::showEvent( e );
  if ( d->mFirstShow )
  {
    // If we're shown for the first time and the theme has been already set
    // then we need to reapply the theme column widths since the previous
    // application probably used invalid widths.
    //
    if ( d->mTheme )
      triggerDelayedApplyThemeColumns();
    d->mFirstShow = false;
  }
}

const int gHeaderContextMenuAdjustColumnSizesId = -1;
const int gHeaderContextMenuShowDefaultColumnsId = -2;
const int gHeaderContextMenuDisplayToolTipsId = -3;

void View::slotHeaderContextMenuRequested( const QPoint &pnt )
{
  if ( !d->mTheme )
    return;

  const QList< Theme::Column * > & columns = d->mTheme->columns();

  if ( columns.count() < 1 )
    return; // bad theme

  // the menu for the columns
  KMenu menu;

  int idx = 0;
  QAction * act;

  for ( QList< Theme::Column * >::ConstIterator it = columns.begin(); it != columns.end(); ++it )
  {
    act = menu.addAction( ( *it )->label() );
    act->setCheckable( true );
    act->setChecked( !header()->isSectionHidden( idx ) );
    act->setData( QVariant( idx ) );
    if ( idx == 0)
       act->setEnabled( false );

    idx++;
  }

  menu.addSeparator();
  act = menu.addAction( i18n( "Adjust Column Sizes" ) );
  act->setData( QVariant( static_cast< int >( gHeaderContextMenuAdjustColumnSizesId ) ) );

  act = menu.addAction( i18n( "Show Default Columns" ) );
  act->setData( QVariant( static_cast< int >( gHeaderContextMenuShowDefaultColumnsId ) ) );

  menu.addSeparator();
  act = menu.addAction( i18n( "Display Tooltips" ) );
  act->setCheckable( true );
  act->setChecked( Settings::self()->messageToolTipEnabled() );
  act->setData( QVariant( static_cast< int >( gHeaderContextMenuDisplayToolTipsId ) ) );

  QObject::connect(
      &menu, SIGNAL( triggered( QAction * ) ),
      this, SLOT( slotHeaderContextMenuTriggered( QAction *  ) )
    );

  menu.addSeparator();
  fillViewMenu( &menu );

  menu.exec( header()->mapToGlobal( pnt ) );
}

void View::slotHeaderContextMenuTriggered( QAction * act )
{
  if ( !d->mTheme )
    return; // oops

  if ( !act )
    return;

  bool ok;
  int columnIdx = act->data().toInt( &ok );

  if ( !ok )
    return;

  if ( columnIdx < 0 )
  {
    if ( columnIdx == gHeaderContextMenuAdjustColumnSizesId )
    {
      // "Adjust Column Sizes"
      d->mTheme->resetColumnSizes();
      applyThemeColumns();
    } else if ( columnIdx == gHeaderContextMenuShowDefaultColumnsId )
    {
      // "Show Default Columns"
      d->mTheme->resetColumnState();
      applyThemeColumns();
    } else if ( columnIdx == gHeaderContextMenuDisplayToolTipsId )
    {
      Settings::self()->setMessageToolTipEnabled( act->isChecked() );
    }
    return;
  }

  // Single column show or hide action
  if ( columnIdx == 0 )
    return; // can never be hidden

  if ( columnIdx >= d->mTheme->columns().count() )
    return;

  bool showIt = header()->isSectionHidden( columnIdx );

  Theme::Column * column = d->mTheme->columns().at( columnIdx );
  Q_ASSERT( column );

  // first save column state (as it is, with the column still in previous state)
  saveThemeColumnState();

  // If a section has just been shown, invalidate its width in the skin
  // since QTreeView assigned it a (possibly insane) default width.
  // If a section has been hidden, then invalidate its width anyway...
  // so finally invalidate width always, here.
  column->setCurrentlyVisible( showIt );
  column->setCurrentWidth( -1 );

  // then apply theme columns to re-compute proportional widths (so we hopefully stay in the view)
  applyThemeColumns();
}

MessageItem * View::currentMessageItem( bool selectIfNeeded ) const
{
  QModelIndex idx = currentIndex();
  if ( !idx.isValid() )
    return 0;
  Item * it = static_cast< Item * >( idx.internalPointer() );
  Q_ASSERT( it );
  if ( it->type() != Item::Message )
    return 0;

  if ( selectIfNeeded )
  {
    // Keep things coherent, if the user didn't select it, but acted on it via
    // a shortcut, do select it now.
    if ( !selectionModel()->isSelected( idx ) )
      selectionModel()->select( idx, QItemSelectionModel::Select | QItemSelectionModel::Current | QItemSelectionModel::Rows );
  }

  return static_cast< MessageItem * >( it );
}

void View::setCurrentMessageItem( MessageItem * it, bool center )
{
  if ( it )
  {
    kDebug() << "Setting current message to" << it->subject();

    const QModelIndex index = d->mModel->index( it, 0 );
    selectionModel()->setCurrentIndex( index, QItemSelectionModel::Select |
                                       QItemSelectionModel::Current | QItemSelectionModel::Rows );
    if ( center ) {
      scrollTo( index, QAbstractItemView::PositionAtCenter );
    }
  }
  else
    selectionModel()->setCurrentIndex( QModelIndex(), QItemSelectionModel::Current |
                                       QItemSelectionModel::Clear );
}

bool View::selectionEmpty() const
{
  return selectionModel()->selectedRows().isEmpty();
}

QList< MessageItem * > View::selectionAsMessageItemList( bool includeCollapsedChildren ) const
{
  QList< MessageItem * > selectedMessages;

  QModelIndexList lSelected = selectionModel()->selectedRows();
  if ( lSelected.isEmpty() )
    return selectedMessages;

  for ( QModelIndexList::Iterator it = lSelected.begin(); it != lSelected.end(); ++it )
  {
    // The asserts below are theoretically valid but at the time
    // of writing they fail because of a bug in QItemSelectionModel::selectedRows()
    // which returns also non-selectable items.

    //Q_ASSERT( selectedItem->type() == Item::Message );
    //Q_ASSERT( ( *it ).isValid() );

    if ( !( *it ).isValid() )
      continue;

    Item * selectedItem = static_cast< Item * >( ( *it ).internalPointer() );
    Q_ASSERT( selectedItem );

    if ( selectedItem->type() != Item::Message )
      continue;

    if ( !static_cast< MessageItem * >( selectedItem )->isValid() )
      continue;

    Q_ASSERT( !selectedMessages.contains( static_cast< MessageItem * >( selectedItem ) ) );

    if ( includeCollapsedChildren && ( selectedItem->childItemCount() > 0 ) && ( !isExpanded( *it ) ) )
    {
      static_cast< MessageItem * >( selectedItem )->subTreeToList( selectedMessages );
    } else {
      selectedMessages.append( static_cast< MessageItem * >( selectedItem ) );
    }
  }

  return selectedMessages;
}

QList< MessageItem * > View::currentThreadAsMessageItemList() const
{
  QList< MessageItem * > currentThread;

  MessageItem * msg = currentMessageItem();
  if ( !msg )
    return currentThread;

  while ( msg->parent() )
  {
    if ( msg->parent()->type() != Item::Message )
      break;
    msg = static_cast< MessageItem * >( msg->parent() );
  }

  msg->subTreeToList( currentThread );

  return currentThread;
}

void View::setChildrenExpanded( const Item * root, bool expand )
{
  Q_ASSERT( root );
  QList< Item * > * childList = root->childItems();
  if ( !childList )
    return;
  for ( QList< Item * >::Iterator it = childList->begin(); it != childList->end(); ++it )
  {
    QModelIndex idx = d->mModel->index( *it, 0 );
    Q_ASSERT( idx.isValid() );
    Q_ASSERT( static_cast< Item * >( idx.internalPointer() ) == ( *it ) );

    if ( expand )
    {
      setExpanded( idx, true );

      if ( ( *it )->childItemCount() > 0 )
        setChildrenExpanded( *it, true );
    } else {
      if ( ( *it )->childItemCount() > 0 )
        setChildrenExpanded( *it, false );

      setExpanded( idx, false );
    }
  }
}

void View::Private::expandFullThread( const QModelIndex & index )
{
  if ( ! index.isValid() )
    return;

  Item * item = static_cast< Item * >( index.internalPointer() );
  if ( item->type() != Item::Message )
    return;

  if ( ! static_cast< MessageItem * >( item )->parent() ||
       ( static_cast< MessageItem * >( item )->parent()->type() != Item::Message ) )
    q->setChildrenExpanded( item, true );
}

void View::setCurrentThreadExpanded( bool expand )
{
  MessageItem * message = currentMessageItem();
  if ( !message )
    return;

  while ( message->parent() )
  {
    if ( message->parent()->type() != Item::Message )
      break;
    message = static_cast< MessageItem * >( message->parent() );
  }

  if ( expand )
  {
    setExpanded( d->mModel->index( message, 0 ), true );
    setChildrenExpanded( message, true );
  } else {
    setChildrenExpanded( message, false );
    setExpanded( d->mModel->index( message, 0 ), false );
  }
}

void View::setAllThreadsExpanded( bool expand )
{
  if ( d->mAggregation->grouping() == Aggregation::NoGrouping )
  {
    // we have no groups so threads start under the root item: just expand/unexpand all
    setChildrenExpanded( d->mModel->rootItem(), expand );
    return;
  }

  // grouping is in effect: must expand/unexpand one level lower

  QList< Item * > * childList = d->mModel->rootItem()->childItems();
  if ( !childList )
    return;

  foreach ( Item * item, *childList )
    setChildrenExpanded( item, expand );
}

void View::setAllGroupsExpanded( bool expand )
{
  if ( d->mAggregation->grouping() == Aggregation::NoGrouping )
    return; // no grouping in effect

  Item * item = d->mModel->rootItem();

  QList< Item * > * childList = item->childItems();
  if ( !childList )
    return;

  foreach ( Item * item, *childList )
  {
    Q_ASSERT( item->type() == Item::GroupHeader );
    QModelIndex idx = d->mModel->index( item, 0 );
    Q_ASSERT( idx.isValid() );
    Q_ASSERT( static_cast< Item * >( idx.internalPointer() ) == item );
    if ( expand )
    {
      if ( !isExpanded( idx ) )
        setExpanded( idx, true );
    } else {
      if ( isExpanded( idx ) )
        setExpanded( idx, false );
    }
  }
}

void View::selectMessageItems( const QList< MessageItem * > &list )
{
  QItemSelection selection;
  for ( QList< MessageItem * >::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it )
  {
    Q_ASSERT( *it );
    QModelIndex idx = d->mModel->index( *it, 0 );
    Q_ASSERT( idx.isValid() );
    Q_ASSERT( static_cast< MessageItem * >( idx.internalPointer() ) == ( *it ) );
    if ( !selectionModel()->isSelected( idx ) )
      selection.append( QItemSelectionRange( idx ) );
    ensureDisplayedWithParentsExpanded( *it );
  }
  if ( !selection.isEmpty() )
    selectionModel()->select( selection, QItemSelectionModel::Select | QItemSelectionModel::Rows );
}

static inline bool message_type_matches( Item * item, MessageTypeFilter messageTypeFilter )
{
  switch( messageTypeFilter )
  {
    case MessageTypeAny:
      return true;
    break;
    case MessageTypeUnreadOnly:
      return !item->status().isRead();
    break;
    default:
      // nuthin here
    break;
  }

  // never reached
  Q_ASSERT( false );
  return false;
}

Item * View::messageItemAfter( Item * referenceItem, MessageTypeFilter messageTypeFilter, bool loop )
{
  if ( !storageModel() )
    return 0; // no folder

  // find the item to start with
  Item * below;

  if ( referenceItem )
  {
    // there was a current item: we start just below it
    if (
         ( referenceItem->childItemCount() > 0 )
         &&
         (
           ( messageTypeFilter != MessageTypeAny )
           ||
           isExpanded( d->mModel->index( referenceItem, 0 ) )
         )
       )
    {
      // the current item had children: either expanded or we want unread/new messages (and so we'll expand it if it isn't)
      below = referenceItem->itemBelow();
    } else {
      // the current item had no children: ask the parent to find the item below
      Q_ASSERT( referenceItem->parent() );
      below = referenceItem->parent()->itemBelowChild( referenceItem );
    }

    if ( !below )
    {
      // reached the end
      if ( loop )
      {
        // try re-starting from top
        below = d->mModel->rootItem()->itemBelow();
        Q_ASSERT( below ); // must exist (we had a current item)

        if ( below == referenceItem )
          return 0; // only one item in folder: loop complete
      } else {
        // looping not requested
        return 0;
      }
    }

  } else {
    // there was no current item, start from beginning
    below = d->mModel->rootItem()->itemBelow();

    if ( !below )
      return 0; // folder empty
  }

  // ok.. now below points to the next message.
  // While it doesn't satisfy our requirements, go further down

  QModelIndex parentIndex = d->mModel->index( below->parent(), 0 );
  QModelIndex belowIndex = d->mModel->index( below, 0 );
  int belowRowIdx = below->parent()->indexOfChildItem( below );

  Q_ASSERT( belowIndex.isValid() );
  Q_ASSERT( belowRowIdx >= 0 );

  while (
          // is not a message (we want messages, don't we ?)
          ( below->type() != Item::Message ) ||
          // message filter doesn't match
          ( !message_type_matches( below, messageTypeFilter ) ) ||
          // is hidden (and we don't want hidden items as they arent "officially" in the view)
          isRowHidden( belowRowIdx, parentIndex ) ||
          // is not enabled or not selectable
          ( ( d->mModel->flags( belowIndex ) & ( Qt::ItemIsSelectable | Qt::ItemIsEnabled ) ) != ( Qt::ItemIsSelectable | Qt::ItemIsEnabled ) )
    )
  {
    // find the next one
    if ( ( below->childItemCount() > 0 ) && ( ( messageTypeFilter != MessageTypeAny ) || isExpanded( belowIndex ) ) )
    {
      // the current item had children: either expanded or we want unread messages (and so we'll expand it if it isn't)
      below = below->itemBelow();
    } else {
      // the current item had no children: ask the parent to find the item below
      Q_ASSERT( below->parent() );
      below = below->parent()->itemBelowChild( below );
    }

    if ( !below )
    {
      // we reached the end of the folder
      if ( loop )
      {
        // looping requested
        if ( referenceItem ) // <-- this means "we have started from something that is not the top: looping makes sense"
          below = d->mModel->rootItem()->itemBelow();
        // else mi == 0 and below == 0: we have started from the beginning and reached the end (it will fail the test below and exit)
      } else {
        // looping not requested: nothing more to do
        return 0;
      }
    }

    if( below == referenceItem )
    {
      Q_ASSERT( loop );
      return 0; // looped and returned back to the first message
    }

    parentIndex = d->mModel->index( below->parent(), 0 );
    belowIndex = d->mModel->index( below, 0 );
    belowRowIdx = below->parent()->indexOfChildItem( below );

    Q_ASSERT( belowIndex.isValid() );
    Q_ASSERT( belowRowIdx >= 0 );
  }

  return below;
}

Item * View::nextMessageItem( MessageTypeFilter messageTypeFilter, bool loop )
{
  return messageItemAfter( currentMessageItem( false ), messageTypeFilter, loop );
}

Item * View::deepestExpandedChild( Item * referenceItem ) const
{
  int children = referenceItem->childItemCount();
  if ( children > 0 &&
       isExpanded( d->mModel->index( referenceItem, 0 ) ) ) {
    return deepestExpandedChild( referenceItem->childItem( children -1 ) );
  }
  else
    return referenceItem;
}

Item * View::messageItemBefore( Item * referenceItem, MessageTypeFilter messageTypeFilter, bool loop )
{
  if ( !storageModel() )
    return 0; // no folder

  // find the item to start with
  Item * above;

  if ( referenceItem )
  {
    Item *parent = referenceItem->parent();
    Item *siblingAbove = parent ?
                         parent->itemAboveChild( referenceItem ) : 0;
    // there was a current item: we start just above it
    if ( ( siblingAbove && siblingAbove != referenceItem && siblingAbove != parent ) &&
         ( siblingAbove->childItemCount() > 0 ) &&
         (
           ( messageTypeFilter != MessageTypeAny ) ||
           ( isExpanded( d->mModel->index( siblingAbove, 0 ) ) )
         )
       )
    {
      // the current item had children: either expanded or we want unread/new messages (and so we'll expand it if it isn't)
      above = deepestExpandedChild( siblingAbove );
    } else {
      // the current item had no children: ask the parent to find the item above
      Q_ASSERT( referenceItem->parent() );
      above = referenceItem->parent()->itemAboveChild( referenceItem );
    }

    if ( ( !above ) || ( above == d->mModel->rootItem() ) )
    {
      // reached the beginning
      if ( loop )
      {
        // try re-starting from bottom
        above = d->mModel->rootItem()->deepestItem();
        Q_ASSERT( above ); // must exist (we had a current item)
        Q_ASSERT( above != d->mModel->rootItem() );

        if ( above == referenceItem )
          return 0; // only one item in folder: loop complete
      } else {
        // looping not requested
        return 0;
      }

    }
  } else {
    // there was no current item, start from end
    above = d->mModel->rootItem()->deepestItem();

    if ( !above || ( above == d->mModel->rootItem() ) )
      return 0; // folder empty
  }

  // ok.. now below points to the previous message.
  // While it doesn't satisfy our requirements, go further up

  QModelIndex parentIndex = d->mModel->index( above->parent(), 0 );
  QModelIndex aboveIndex = d->mModel->index( above, 0 );
  int aboveRowIdx = above->parent()->indexOfChildItem( above );

  Q_ASSERT( aboveIndex.isValid() );
  Q_ASSERT( aboveRowIdx >= 0 );

  while (
          // is not a message (we want messages, don't we ?)
          ( above->type() != Item::Message ) ||
          // message filter doesn't match
          ( !message_type_matches( above, messageTypeFilter ) ) ||
          // we don't expand items but the item has parents unexpanded (so should be skipped)
          (
            // !expand items
            ( messageTypeFilter == MessageTypeAny ) &&
            // has unexpanded parents or is itself hidden
            ( ! isDisplayedWithParentsExpanded( above ) )
          ) ||
          // is hidden
          isRowHidden( aboveRowIdx, parentIndex ) ||
          // is not enabled or not selectable
          ( ( d->mModel->flags( aboveIndex ) & ( Qt::ItemIsSelectable | Qt::ItemIsEnabled ) ) != ( Qt::ItemIsSelectable | Qt::ItemIsEnabled ) )
    )
  {

    above = above->itemAbove();

    if ( ( !above ) || ( above == d->mModel->rootItem() ) )
    {
      // reached the beginning
      if ( loop )
      {
        // looping requested
        if ( referenceItem ) // <-- this means "we have started from something that is not the beginning: looping makes sense"
          above = d->mModel->rootItem()->deepestItem();
        // else mi == 0 and above == 0: we have started from the end and reached the beginning (it will fail the test below and exit)
      } else {
        // looping not requested: nothing more to do
        return 0;
      }
    }

    if( above == referenceItem )
    {
      Q_ASSERT( loop );
      return 0; // looped and returned back to the first message
    }

    parentIndex = d->mModel->index( above->parent(), 0 );
    aboveIndex = d->mModel->index( above, 0 );
    aboveRowIdx = above->parent()->indexOfChildItem( above );

    Q_ASSERT( aboveIndex.isValid() );
    Q_ASSERT( aboveRowIdx >= 0 );
  }

  return above;
}

Item * View::previousMessageItem( MessageTypeFilter messageTypeFilter, bool loop )
{
  return messageItemBefore( currentMessageItem( false ), messageTypeFilter, loop );
}

void View::growOrShrinkExistingSelection( const QModelIndex &newSelectedIndex, bool movingUp )
{
  // Qt: why visualIndex() is private? ...I'd really need it here...

  int selectedVisualCoordinate = visualRect( newSelectedIndex ).top();

  int topVisualCoordinate = 0xfffffff; // huuuuuge number
  int bottomVisualCoordinate = -(0xfffffff);

  int candidate;

  QModelIndex bottomIndex;
  QModelIndex topIndex;

  // find out the actual selection range
  const QItemSelection selection = selectionModel()->selection();

  foreach ( const QItemSelectionRange &range, selection )
  {
    // We're asking the model for the index as range.topLeft() and range.bottomRight()
    // can return indexes in invisible columns which have a null visualRect().
    // Column 0, instead, is always visible.

    QModelIndex top = d->mModel->index( range.top(), 0, range.parent() );
    QModelIndex bottom = d->mModel->index( range.bottom(), 0, range.parent() );

    if ( top.isValid() )
    {
      if ( !bottom.isValid() )
        bottom = top;
    } else {
      if ( !top.isValid() )
        top = bottom;
    }
    candidate = visualRect( bottom ).bottom();
    if ( candidate > bottomVisualCoordinate )
    {
      bottomVisualCoordinate = candidate;
      bottomIndex = range.bottomRight();
    }

    candidate = visualRect( top ).top();
    if ( candidate < topVisualCoordinate )
    {
      topVisualCoordinate = candidate;
      topIndex = range.topLeft();
    }
  }


  if ( topIndex.isValid() && bottomIndex.isValid() )
  {
    if ( movingUp )
    {
      if ( selectedVisualCoordinate < topVisualCoordinate )
      {
        // selecting something above the top: grow selection
        selectionModel()->select( newSelectedIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select );
      } else {
        // selecting something below the top: shrink selection
        QModelIndexList selectedIndexes = selection.indexes();
        foreach ( const QModelIndex &idx, selectedIndexes )
        {
          if ( ( idx.column() == 0 ) && ( visualRect( idx ).top() > selectedVisualCoordinate ) )
            selectionModel()->select( idx, QItemSelectionModel::Rows | QItemSelectionModel::Deselect );
        }
      }
    } else {
      if ( selectedVisualCoordinate > bottomVisualCoordinate )
      {
        // selecting something below bottom: grow selection
        selectionModel()->select( newSelectedIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select );
      } else {
        // selecting something above bottom: shrink selection
        QModelIndexList selectedIndexes = selection.indexes();
        foreach ( const QModelIndex &idx, selectedIndexes )
        {
          if ( ( idx.column() == 0 ) && ( visualRect( idx ).top() < selectedVisualCoordinate ) )
            selectionModel()->select( idx, QItemSelectionModel::Rows | QItemSelectionModel::Deselect );
        }
      }
    }
  } else {
    // no existing selection, just grow
    selectionModel()->select( newSelectedIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select );
  }
}

bool View::selectNextMessageItem(
    MessageTypeFilter messageTypeFilter,
    ExistingSelectionBehaviour existingSelectionBehaviour,
    bool centerItem,
    bool loop
  )
{
  Item * it = nextMessageItem( messageTypeFilter, loop );
  if ( !it )
    return false;

  setFocus();

  if ( it->parent() != d->mModel->rootItem() )
    ensureDisplayedWithParentsExpanded( it );

  QModelIndex idx = d->mModel->index( it, 0 );

  Q_ASSERT( idx.isValid() );

  switch ( existingSelectionBehaviour )
  {
    case ExpandExistingSelection:
      selectionModel()->setCurrentIndex( idx, QItemSelectionModel::NoUpdate );
      selectionModel()->select( idx, QItemSelectionModel::Rows | QItemSelectionModel::Select );
    break;
    case GrowOrShrinkExistingSelection:
      selectionModel()->setCurrentIndex( idx, QItemSelectionModel::NoUpdate );
      growOrShrinkExistingSelection( idx, false );
    break;
    default:
    //case ClearExistingSelection:
      setCurrentIndex( idx );
    break;
  }

  if ( centerItem )
    scrollTo( idx, QAbstractItemView::PositionAtCenter );

  return true;
}

bool View::selectPreviousMessageItem(
    MessageTypeFilter messageTypeFilter,
    ExistingSelectionBehaviour existingSelectionBehaviour,
    bool centerItem,
    bool loop
  )
{
  Item * it = previousMessageItem( messageTypeFilter, loop );
  if ( !it )
    return false;

  setFocus();

  if ( it->parent() != d->mModel->rootItem() )
    ensureDisplayedWithParentsExpanded( it );

  QModelIndex idx = d->mModel->index( it, 0 );

  Q_ASSERT( idx.isValid() );

  switch ( existingSelectionBehaviour )
  {
    case ExpandExistingSelection:
      selectionModel()->setCurrentIndex( idx, QItemSelectionModel::NoUpdate );
      selectionModel()->select( idx, QItemSelectionModel::Rows | QItemSelectionModel::Select );
    break;
    case GrowOrShrinkExistingSelection:
      selectionModel()->setCurrentIndex( idx, QItemSelectionModel::NoUpdate );
      growOrShrinkExistingSelection( idx, true );
    break;
    default:
    //case ClearExistingSelection:
      setCurrentIndex( idx );
    break;
  }

  if ( centerItem )
    scrollTo( idx, QAbstractItemView::PositionAtCenter );

  return true;
}

bool View::focusNextMessageItem( MessageTypeFilter messageTypeFilter, bool centerItem, bool loop )
{
  Item * it = nextMessageItem( messageTypeFilter, loop );
  if ( !it )
    return false;

  setFocus();

  if ( it->parent() != d->mModel->rootItem() )
    ensureDisplayedWithParentsExpanded( it );

  QModelIndex idx = d->mModel->index( it, 0 );

  Q_ASSERT( idx.isValid() );

  selectionModel()->setCurrentIndex( idx, QItemSelectionModel::NoUpdate );

  if ( centerItem )
    scrollTo( idx, QAbstractItemView::PositionAtCenter );

  return true;
}

bool View::focusPreviousMessageItem( MessageTypeFilter messageTypeFilter, bool centerItem, bool loop )
{
  Item * it = previousMessageItem( messageTypeFilter, loop );
  if ( !it )
    return false;

  setFocus();

  if ( it->parent() != d->mModel->rootItem() )
    ensureDisplayedWithParentsExpanded( it );

  QModelIndex idx = d->mModel->index( it, 0 );

  Q_ASSERT( idx.isValid() );

  selectionModel()->setCurrentIndex( idx, QItemSelectionModel::NoUpdate );

  if ( centerItem )
    scrollTo( idx, QAbstractItemView::PositionAtCenter );

  return true;
}

void View::selectFocusedMessageItem( bool centerItem )
{
  QModelIndex idx = currentIndex();
  if ( !idx.isValid() )
    return;

  setFocus();

  if ( selectionModel()->isSelected( idx ) )
    return;

  selectionModel()->select( idx, QItemSelectionModel::Select | QItemSelectionModel::Current | QItemSelectionModel::Rows );

  if ( centerItem )
    scrollTo( idx, QAbstractItemView::PositionAtCenter );
}

void View::fillViewMenu( KMenu * menu )
{
  KMenu* sortingMenu = new KMenu( i18n( "Sorting" ), menu );
  sortingMenu->setIcon( KIcon( QLatin1String( "view-sort-ascending" ) ) );
  menu->addMenu( sortingMenu );
  connect( sortingMenu, SIGNAL( aboutToShow() ),
           d->mWidget, SLOT( sortOrderMenuAboutToShow() ) );

  KMenu* aggregationMenu = new KMenu( i18n( "Aggregation" ), menu );
  aggregationMenu->setIcon( KIcon( QLatin1String( "view-process-tree" ) ) );
  menu->addMenu( aggregationMenu );
  connect( aggregationMenu, SIGNAL( aboutToShow() ),
           d->mWidget, SLOT( aggregationMenuAboutToShow() ) );

  KMenu* themeMenu = new KMenu( i18n( "Theme" ), menu );
  themeMenu->setIcon( KIcon( QLatin1String( "preferences-desktop-theme" ) ) );
  menu->addMenu( themeMenu );
  connect( themeMenu, SIGNAL( aboutToShow() ),
           d->mWidget, SLOT( themeMenuAboutToShow() ) );
}

bool View::selectFirstMessageItem( MessageTypeFilter messageTypeFilter, bool centerItem )
{
  if ( !storageModel() )
    return false; // nothing to do

  Item * it = firstMessageItem( messageTypeFilter );
  if ( !it )
    return false;

  Q_ASSERT( it != d->mModel->rootItem() ); // must never happen (obviously)

  setFocus();
  ensureDisplayedWithParentsExpanded( it );

  QModelIndex idx = d->mModel->index( it, 0 );

  Q_ASSERT( idx.isValid() );

  setCurrentIndex( idx );

  if ( centerItem )
    scrollTo( idx, QAbstractItemView::PositionAtCenter );

  return true;
}

void View::modelFinishedLoading()
{
  Q_ASSERT( storageModel() );
  Q_ASSERT( !d->mModel->isLoading() );

  // nothing here for now :)
}

MessageItemSetReference View::createPersistentSet( const QList< MessageItem * > &items )
{
  return d->mModel->createPersistentSet( items );
}

QList< MessageItem * > View::persistentSetCurrentMessageItemList( MessageItemSetReference ref )
{
  return d->mModel->persistentSetCurrentMessageItemList( ref );
}

void View::deletePersistentSet( MessageItemSetReference ref )
{
  d->mModel->deletePersistentSet( ref );
}

void View::markMessageItemsAsAboutToBeRemoved( QList< MessageItem * > &items, bool bMark )
{
  if ( !bMark )
  {
    for ( QList< MessageItem * >::Iterator it = items.begin(); it != items.end(); ++it )
    {
      if ( ( *it )->isValid() ) // hasn't been removed in the meantime
        ( *it )->setAboutToBeRemoved( false );
    }

    viewport()->update();

    return;
  }

  // ok.. we're going to mark the messages as "about to be deleted".
  // This means that we're going to make them non selectable.

  // What happens to the selection is generally an untrackable big mess.
  // Several components and entities are involved.

  // Qutie tries to apply some kind of internal logic in order to keep
  // "something" selected and "something" (else) to be current.
  // The results sometimes appear to depend on the current moon phase.

  // The Model will do crazy things in order to preserve the current
  // selection (and possibly the current item). If it's impossible then
  // it will make its own guesses about what should be selected next.
  // A problem is that the Model will do it one message at a time.
  // When item reparenting/reordering is involved then the guesses
  // can produce non-intuitive results.

  // Add the fact that selection and current item are distinct concepts,
  // their relative interaction depends on the settings and is often quite
  // unclear.

  // Add the fact that (at the time of writing) several styles don't show
  // the current item (only Yoda knows why) and this causes some confusion to the user.

  // Add the fact that the operations are asynchronous: deletion will start
  // a job, do some event loop processing and then complete the work at a later time.
  // The Qutie views also tend to accumulate the changes and perform them
  // all at once at the latest possible stage.

  // A radical approach is needed: we FIRST deal with the selection
  // by tring to move it away from the messages about to be deleted
  // and THEN mark the (hopefully no longer selected) messages as "about to be deleted".

  // First of all, find out if we're going to clear the entire selection (very likely).

  bool clearingEntireSelection = true;

  QModelIndexList selectedIndexes = selectionModel()->selectedRows( 0 );

  if ( selectedIndexes.count() > items.count() )
  {
    // the selection is bigger: we can't clear it completely
    clearingEntireSelection = false;
  } else {
    // the selection has same size or is smaller: we can clear it completely with our removal
    foreach ( const QModelIndex &selectedIndex , selectedIndexes )
    {
      Q_ASSERT( selectedIndex.isValid() );
      Q_ASSERT( selectedIndex.column() == 0 );

      Item * selectedItem = static_cast< Item * >( selectedIndex.internalPointer() );
      Q_ASSERT( selectedItem );

      if ( selectedItem->type() != Item::Message )
        continue;

      if ( !items.contains( static_cast< MessageItem * >( selectedItem ) ) )
      {
        // the selection contains something that we aren't going to remove:
        // we will not clear the selection completely
        clearingEntireSelection = false;
        break;
      }
    }
  }

  if ( clearingEntireSelection )
  {
    // Try to clear the current selection and select something sensible instead,
    // so after the deletion we will not end up with a random selection.
    // Pick up a message in the set (which is very likely to be contiguous), walk the tree
    // and select the next message that is NOT in the set.

    MessageItem * aMessage = items.last();
    Q_ASSERT( aMessage );

    // Avoid infinite loops by carrying only a limited number of attempts.
    // If there is any message that is not in the set then items.count() attemps should find it.
    int maxAttempts = items.count();

    while ( items.contains( aMessage ) && ( maxAttempts > 0 ) )
    {
      Item * next = messageItemAfter( aMessage, MessageTypeAny, false );
      if ( !next )
      {
        // no way
        aMessage = 0;
        break;
      }
      Q_ASSERT( next->type() == Item::Message );
      aMessage = static_cast< MessageItem * >( next );
      maxAttempts--;
    }

    if ( !aMessage )
    {
      // try backwards
      aMessage = items.first();
      Q_ASSERT( aMessage );
      maxAttempts = items.count();

      while ( items.contains( aMessage ) && ( maxAttempts > 0 ) )
      {
        Item * prev = messageItemBefore( aMessage, MessageTypeAny, false );
        if ( !prev )
        {
          // no way
          aMessage = 0;
          break;
        }
        Q_ASSERT( prev->type() == Item::Message );
        aMessage = static_cast< MessageItem * >( prev );
        maxAttempts--;
      }
    }

    if ( aMessage )
    {
      QModelIndex aMessageIndex = d->mModel->index( aMessage, 0 );
      Q_ASSERT( aMessageIndex.isValid() );
      Q_ASSERT( static_cast< MessageItem * >( aMessageIndex.internalPointer() ) == aMessage );
      Q_ASSERT( !selectionModel()->isSelected( aMessageIndex ) );
      setCurrentIndex( aMessageIndex );
      selectionModel()->select( aMessageIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
    }
  } // else we aren't clearing the entire selection so something should just stay selected.

  // Now mark messages as about to be removed.

  for ( QList< MessageItem * >::Iterator it = items.begin(); it != items.end(); ++it )
  {
    ( *it )->setAboutToBeRemoved( true );
    QModelIndex idx = d->mModel->index( *it, 0 );
    Q_ASSERT( idx.isValid() );
    Q_ASSERT( static_cast< MessageItem * >( idx.internalPointer() ) == *it );
    if ( selectionModel()->isSelected( idx ) )
      selectionModel()->select( idx, QItemSelectionModel::Deselect | QItemSelectionModel::Rows );
  }

  viewport()->update();
}

void View::ensureDisplayedWithParentsExpanded( Item * it )
{
  Q_ASSERT( it );
  Q_ASSERT( it->parent() );
  Q_ASSERT( it->isViewable() ); // must be attached to the viewable root

  if ( isRowHidden( it->parent()->indexOfChildItem( it ), d->mModel->index( it->parent(), 0 ) ) )
    setRowHidden( it->parent()->indexOfChildItem( it ), d->mModel->index( it->parent(), 0 ), false );

  it = it->parent();

  while ( it->parent() )
  {
    if ( isRowHidden( it->parent()->indexOfChildItem( it ), d->mModel->index( it->parent(), 0 ) ) )
      setRowHidden( it->parent()->indexOfChildItem( it ), d->mModel->index( it->parent(), 0 ), false );

    QModelIndex idx = d->mModel->index( it, 0 );

    Q_ASSERT( idx.isValid() );
    Q_ASSERT( static_cast< Item * >( idx.internalPointer() ) == it );

    if ( !isExpanded( idx ) )
      setExpanded( idx, true );

    it = it->parent();
  }
}

bool View::isDisplayedWithParentsExpanded( Item * it ) const
{
  // An item is currently viewable iff
  //  - it is marked as viewable in the item structure (that is, qt knows about its existence)
  //      (and this means that all of its parents are marked as viewable)
  //  - it is not explicitly hidden
  //  - all of its parents are expanded

  if ( !it )
    return false; // be nice and allow the caller not to care

  if ( !it->isViewable() )
    return false; // item not viewable (not attacched to the viewable root or qt not yet aware of it)

  // the item and all the parents are marked as viewable.

  if ( isRowHidden( it->parent()->indexOfChildItem( it ), d->mModel->index( it->parent(), 0 ) ) )
    return false; // item qt rappresentation explicitly hidden

  // the item (and theoretically all the parents) are not explicitly hidden

  // check the parent chain

  it = it->parent();

  while ( it )
  {
    if ( it == d->mModel->rootItem() )
      return true; // parent is root item: ok

    // parent is not root item

    if ( !isExpanded( d->mModel->index( it, 0 ) ) )
      return false; // parent is not expanded (so child not actually visible)

    it = it->parent(); // climb up
  }

  // parent hierarchy interrupted somewhere
  return false;
}

bool View::isThreaded() const
{
  if ( !d->mAggregation )
    return false;
  return d->mAggregation->threading() != Aggregation::NoThreading;
}

void View::slotSelectionChanged( const QItemSelection &, const QItemSelection & )
{
  // We assume that when selection changes, current item also changes.
  QModelIndex current = currentIndex();

  // Abort any pending message pre-selection as the user is probably
  // already navigating the view (so pre-selection would make his view jump
  // to an unexpected place).
  d->mModel->abortMessagePreSelection();

  if ( !current.isValid() )
  {
    if ( d->mLastCurrentItem )
    {
      d->mWidget->viewMessageSelected( 0 );
      d->mLastCurrentItem = 0;
    }
    d->mWidget->viewMessageSelected( 0 );
    d->mWidget->viewSelectionChanged();
    return;
  }

  if ( !selectionModel()->isSelected( current ) )
  {
    if ( selectedIndexes().count() < 1 )
    {
      // It may happen after row removals: Model calls this slot on currentIndex()
      // that actually might have changed "silently", without being selected.
      QItemSelection selection;
      selection.append( QItemSelectionRange( current ) );
      selectionModel()->select( selection, QItemSelectionModel::Select | QItemSelectionModel::Rows );
    } else {
      // something is still selected anyway
      // This is probably a result of CTRL+Click which unselected current: leave it as it is.
      return;
    }
  }

  Item * it = static_cast< Item * >( current.internalPointer() );
  Q_ASSERT( it );

  switch ( it->type() )
  {
    case Item::Message:
    {
      if ( d->mLastCurrentItem != it )
      {
        kDebug() << "View message selected [" << static_cast< MessageItem * >( it )->subject() << "]";
        d->mWidget->viewMessageSelected( static_cast< MessageItem * >( it ) );
        d->mLastCurrentItem = it;
      }
    }
    break;
    case Item::GroupHeader:
      if ( d->mLastCurrentItem )
      {
        d->mWidget->viewMessageSelected( 0 );
        d->mLastCurrentItem = 0;
      }
    break;
    default:
      // should never happen
      Q_ASSERT( false );
    break;
  }

  d->mWidget->viewSelectionChanged();
}

void View::mouseDoubleClickEvent( QMouseEvent * e )
{
  // Perform a hit test
  if ( !d->mDelegate->hitTest( e->pos(), true ) )
    return;

  // Something was hit :)

  Item * it = static_cast< Item * >( d->mDelegate->hitItem() );
  if ( !it )
    return; // should never happen

  switch ( it->type() )
  {
    case Item::Message:
    {
      // Let QTreeView handle the expansion
      QTreeView::mousePressEvent( e );

      switch ( e->button() )
      {
        case Qt::LeftButton:

          if ( d->mDelegate->hitContentItem() )
          {
            // Double clikcking on clickable icons does NOT activate the message
            if ( d->mDelegate->hitContentItem()->isIcon() && d->mDelegate->hitContentItem()->isClickable() )
              return;
          }

          d->mWidget->viewMessageActivated( static_cast< MessageItem * >( it ) );
        break;
        default:
          // make gcc happy
        break;
      }
    }
    break;
    case Item::GroupHeader:
    {
      // Don't let QTreeView handle the selection (as it deselects the curent messages)
      switch ( e->button() )
      {
        case Qt::LeftButton:
          if ( it->childItemCount() > 0 )
          {
            // toggle expanded state
            setExpanded( d->mDelegate->hitIndex(), !isExpanded( d->mDelegate->hitIndex() ) );
          }
        break;
        default:
          // make gcc happy
        break;
      }
    }
    break;
    default:
      // should never happen
      Q_ASSERT( false );
    break;
  }
}

void View::changeMessageStatus( MessageItem * it, const Akonadi::MessageStatus &set, const Akonadi::MessageStatus &unset )
{
  // We first change the status of MessageItem itself. This will make the change
  // visible to the user even if the Model is actually in the middle of a long job (maybe it's loading)
  // and can't process the status change request immediately.
  // Here we actually desynchronize the cache and trust that the later call to
  // d->mWidget->viewMessageStatusChangeRequest() will really perform the status change on the storage.
  // Well... in KMail it will unless something is really screwed. Anyway, if it will not, at the next
  // load the status will be just unchanged: no animals will be harmed.

  qint32 stat = it->status().toQInt32();
  stat |= set.toQInt32();
  stat &= ~( unset.toQInt32() );
  Akonadi::MessageStatus status;
  status.fromQInt32( stat );
  it->setStatus( status );

  // Trigger an update so the immediate change will be shown to the user

  viewport()->update();

  // This will actually request the widget to perform a status change on the storage.
  // The request will be then processed by the Model and the message will be updated again.

  d->mWidget->viewMessageStatusChangeRequest( it, set, unset );
}

void View::mousePressEvent( QMouseEvent * e )
{
  d->mMousePressPosition = QPoint();

  // Perform a hit test
  if ( !d->mDelegate->hitTest( e->pos(), true ) )
    return;

  // Something was hit :)

  Item * it = static_cast< Item * >( d->mDelegate->hitItem() );
  if ( !it )
    return; // should never happen

  switch ( it->type() )
  {
    case Item::Message:
    {
      d->mMousePressPosition = e->pos();

      switch ( e->button() )
      {
        case Qt::LeftButton:
          // if we have multi selection then the meaning of hitting
          // the content item is quite unclear.
          if ( d->mDelegate->hitContentItem() && ( selectedIndexes().count() > 1 ) )
          {
            kDebug() << "Left hit with selectedIndexes().count() == " << selectedIndexes().count();

            switch ( d->mDelegate->hitContentItem()->type() )
            {
              case Theme::ContentItem::AnnotationIcon:
                static_cast< MessageItem * >( it )->editAnnotation();
                return; // don't select the item
              break;
              case Theme::ContentItem::ActionItemStateIcon:
                changeMessageStatus(
                    static_cast< MessageItem * >( it ),
                    it->status().isToAct() ? Akonadi::MessageStatus() : Akonadi::MessageStatus::statusToAct(),
                    it->status().isToAct() ? Akonadi::MessageStatus::statusToAct() : Akonadi::MessageStatus()
                  );
                return; // don't select the item
              break;
              case Theme::ContentItem::ImportantStateIcon:
                changeMessageStatus(
                    static_cast< MessageItem * >( it ),
                    it->status().isImportant() ? Akonadi::MessageStatus() : Akonadi::MessageStatus::statusImportant(),
                    it->status().isImportant() ? Akonadi::MessageStatus::statusImportant() : Akonadi::MessageStatus()
                  );
                return; // don't select the item
              break;
              case Theme::ContentItem::SpamHamStateIcon:
                changeMessageStatus(
                    static_cast< MessageItem * >( it ),
                    it->status().isSpam() ? Akonadi::MessageStatus() : ( it->status().isHam() ? Akonadi::MessageStatus::statusSpam() : Akonadi::MessageStatus::statusHam() ),
                    it->status().isSpam() ? Akonadi::MessageStatus::statusSpam() : ( it->status().isHam() ? Akonadi::MessageStatus::statusHam() : Akonadi::MessageStatus() )
                  );
                return; // don't select the item
              break;
              case Theme::ContentItem::WatchedIgnoredStateIcon:
                changeMessageStatus(
                    static_cast< MessageItem * >( it ),
                    it->status().isIgnored() ? Akonadi::MessageStatus() : ( it->status().isWatched() ? Akonadi::MessageStatus::statusIgnored() : Akonadi::MessageStatus::statusWatched() ),
                    it->status().isIgnored() ? Akonadi::MessageStatus::statusIgnored() : ( it->status().isWatched() ? Akonadi::MessageStatus::statusWatched() : Akonadi::MessageStatus() )
                  );
                return; // don't select the item
              break;
              default:
                // make gcc happy
              break;
            }
          }

          // Let QTreeView handle the selection and emit the appropriate signals (slotSelectionChanged() may be called)
          QTreeView::mousePressEvent( e );

        break;
        case Qt::RightButton:
          // Let QTreeView handle the selection and emit the appropriate signals (slotSelectionChanged() may be called)
          QTreeView::mousePressEvent( e );

          d->mWidget->viewMessageListContextPopupRequest( selectionAsMessageItemList(), viewport()->mapToGlobal( e->pos() ) );
        break;
        default:
          // make gcc happy
        break;
      }
    }
    break;
    case Item::GroupHeader:
    {
      // Don't let QTreeView handle the selection (as it deselects the curent messages)
      GroupHeaderItem *groupHeaderItem = static_cast< GroupHeaderItem * >( it );

      switch ( e->button() )
      {
        case Qt::LeftButton:
          if ( !d->mDelegate->hitContentItem() )
            return;

          if ( d->mDelegate->hitContentItem()->type() == Theme::ContentItem::ExpandedStateIcon )
          {
            if ( groupHeaderItem->childItemCount() > 0 )
            {
              // toggle expanded state
              setExpanded( d->mDelegate->hitIndex(), !isExpanded( d->mDelegate->hitIndex() ) );
            }
          }
        break;
        case Qt::RightButton:
          clearSelection(); // make sure it's true, so it's clear that the eventual popup belongs to the group header
          d->mWidget->viewGroupHeaderContextPopupRequest( groupHeaderItem, viewport()->mapToGlobal( e->pos() ) );
        break;
        default:
          // make gcc happy
        break;
      }
    }
    break;
    default:
      // should never happen
      Q_ASSERT( false );
    break;
  }
}

void View::mouseMoveEvent( QMouseEvent * e )
{
  if ( !e->buttons() & Qt::LeftButton )
  {
    QTreeView::mouseMoveEvent( e );
    return;
  }

  if ( d->mMousePressPosition.isNull() )
    return;

  if ( ( e->pos() - d->mMousePressPosition ).manhattanLength() <= KGlobalSettings::dndEventDelay() )
    return;

  d->mWidget->viewStartDragRequest();
}

void View::dragEnterEvent( QDragEnterEvent * e )
{
  d->mWidget->viewDragEnterEvent( e );
}

void View::dragMoveEvent( QDragMoveEvent * e )
{
  d->mWidget->viewDragMoveEvent( e );
}

void View::dropEvent( QDropEvent * e )
{
  d->mWidget->viewDropEvent( e );
}

void View::changeEvent( QEvent *e )
{
  switch ( e->type() )
  {
    case QEvent::PaletteChange:
    case QEvent::FontChange:
    case QEvent::StyleChange:
    case QEvent::LayoutDirectionChange:
    case QEvent::LocaleChange:
    case QEvent::LanguageChange:
      // All of these affect the theme's internal cache.
      setTheme( d->mTheme );
      // A layoutChanged() event will screw up the view state a bit.
      // Since this is a rare event we just reload the view.
      reload();
    break;
    default:
      // make gcc happy by default
    break;
  }

  QTreeView::changeEvent( e );
}

bool View::event( QEvent *e )
{
  // We catch ToolTip events and pass everything else

  if( e->type() != QEvent::ToolTip )
    return QTreeView::event( e );

  QHelpEvent * he = dynamic_cast< QHelpEvent * >( e );
  if ( !he )
    return true; // eh ?

  if ( !Settings::self()->messageToolTipEnabled() )
    return true; // don't display tooltips

  QPoint pnt = viewport()->mapFromGlobal( mapToGlobal( he->pos() ) );

  if ( pnt.y() < 0 )
    return true; // don't display the tooltip for items hidden under the header

  QModelIndex idx = indexAt( pnt );
  if ( !idx.isValid() )
    return true; // may be

  Item * it = static_cast< Item * >( idx.internalPointer() );
  if ( !it )
    return true; // hum

  Q_ASSERT( storageModel() );

  QColor bckColor = palette().color( QPalette::ToolTipBase );
  QColor txtColor = palette().color( QPalette::ToolTipText );
  QColor darkerColor(
      ( ( bckColor.red() * 8 ) + ( txtColor.red() * 2 ) ) / 10,
      ( ( bckColor.green() * 8 ) + ( txtColor.green() * 2 ) ) / 10,
      ( ( bckColor.blue() * 8 ) + ( txtColor.blue() * 2 ) ) / 10
    );

  QString bckColorName = bckColor.name();
  QString txtColorName = txtColor.name();
  QString darkerColorName = darkerColor.name();
  const bool textIsLeftToRight = ( QApplication::layoutDirection() == Qt::LeftToRight );
  const QString textDirection =  textIsLeftToRight ? QLatin1String( "left" ) : QLatin1String( "right" );
  const QString firstColumnWidth =  textIsLeftToRight ? QLatin1String( "45" ) : QLatin1String( "55" );

  QString tip = QString::fromLatin1(
      "<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">"
    );

  switch ( it->type() )
  {
    case Item::Message:
    {
      MessageItem *mi = static_cast< MessageItem * >( it );

      tip += QString::fromLatin1(
           "<tr>" \
              "<td bgcolor=\"%1\" align=\"%4\" valign=\"middle\">" \
                "<div style=\"color: %2; font-weight: bold;\">" \
                 "%3" \
                "</div>" \
              "</td>" \
            "</tr>"
        ).arg( txtColorName ).arg( bckColorName ).arg( Qt::escape( mi->subject() ) ).arg( textDirection );

      tip += QString::fromLatin1(
           "<tr>" \
              "<td align=\"center\" valign=\"middle\">" \
                "<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">"
        );

      const QString htmlCodeForStandardRow = QString::fromLatin1(
           "<tr>" \
              "<td align=\"right\" valign=\"top\" width=\"45\">" \
                "<div style=\"font-weight: bold;\"><nobr>" \
                 "%1:" \
                "</nobr></div>" \
              "</td>" \
              "<td align=\"left\" valign=\"top\">" \
                 "%2" \
              "</td>" \
            "</tr>" );


      if ( textIsLeftToRight ) {
        tip += htmlCodeForStandardRow.arg( i18n( "From" ) ).arg( MessageCore::StringUtil::stripEmailAddr( mi->sender() ) );
        tip += htmlCodeForStandardRow.arg( i18nc( "Receiver of the emial", "To" ) ).arg( MessageCore::StringUtil::stripEmailAddr( mi->receiver() ) );
        tip += htmlCodeForStandardRow.arg( i18n( "Date" ) ).arg( mi->formattedDate() );
      } else {
        tip += htmlCodeForStandardRow.arg(  MessageCore::StringUtil::stripEmailAddr( mi->sender() ) ).arg( i18n( "From" ) );
        tip += htmlCodeForStandardRow.arg(  MessageCore::StringUtil::stripEmailAddr( mi->receiver() ) ).arg( i18nc( "Receiver of the emial", "To" ) );
        tip += htmlCodeForStandardRow.arg(  mi->formattedDate() ).arg( i18n( "Date" ) );
      }

      QString status = mi->statusDescription();
      QString tags = mi->tagListDescription();
      if ( !tags.isEmpty () )
      {
        if ( !status.isEmpty() )
          status += QLatin1String( ", " );
        status += tags;
      }

      if ( textIsLeftToRight ) {
        tip += htmlCodeForStandardRow.arg( i18n( "Status" ) ).arg( status );
        tip += htmlCodeForStandardRow.arg( i18n( "Size" ) ).arg( mi->formattedSize() );
      } else {
        tip += htmlCodeForStandardRow.arg( status ).arg( i18n( "Status" ) );
        tip += htmlCodeForStandardRow.arg( mi->formattedSize() ).arg( i18n( "Size" ) );
      }

      if ( mi->hasAnnotation() ) {
        if ( textIsLeftToRight ) {
          tip += htmlCodeForStandardRow.arg( i18n( "Note" ) ).arg( mi->annotation().replace( QLatin1Char( '\n' ), QLatin1String( "<br>" ) ) );
        } else {
          tip += htmlCodeForStandardRow.arg( mi->annotation().replace( QLatin1Char( '\n' ), QLatin1String( "<br>" ) ) ).arg( i18n( "Note" ) );
        }
      }

      QString content = mi->contentSummary();
      if ( !content.isEmpty() ) {
        if ( textIsLeftToRight ) {
          tip += htmlCodeForStandardRow.arg( i18n( "Preview" ) ).arg( content.replace( QLatin1Char( '\n' ), QLatin1String( "<br>" ) ) );
        } else {
          tip += htmlCodeForStandardRow.arg( content.replace( QLatin1Char( '\n' ), QLatin1String( "<br>" ) ) ).arg( i18n( "Preview" ) );
        }
      }

      tip += QString::fromLatin1(
                "</table" \
              "</td>" \
            "</tr>"
        );

      // FIXME: Find a way to show also CC and other header fields ?

      if ( mi->hasChildren() )
      {
        Item::ChildItemStats stats;
        mi->childItemStats( stats );

        QString statsText;

        statsText = i18np( "<b>%1</b> reply", "<b>%1</b> replies", mi->childItemCount() );
        statsText += QLatin1String( ", " );

        statsText += i18np(
            "<b>%1</b> message in subtree (<b>%2</b> unread)",
            "<b>%1</b> messages in subtree (<b>%2</b> unread)",
            stats.mTotalChildCount,
            stats.mUnreadChildCount
          );


        tip += QString::fromLatin1(
             "<tr>" \
                "<td bgcolor=\"%1\" align=\"%3\" valign=\"middle\">" \
                   "<nobr>%2</nobr>" \
                "</td>" \
              "</tr>"
          ).arg( darkerColorName ).arg( statsText ).arg( textDirection );
      }

    }
    break;
    case Item::GroupHeader:
    {
      GroupHeaderItem *ghi = static_cast< GroupHeaderItem * >( it );

      tip += QString::fromLatin1(
           "<tr>" \
              "<td bgcolor=\"%1\" align=\"%4\" valign=\"middle\">" \
                "<div style=\"color: %2; font-weight: bold;\">" \
                 "%3" \
                "</div>" \
              "</td>" \
            "</tr>"
        ).arg( txtColorName ).arg( bckColorName ).arg( ghi->label() ).arg( textDirection );

      QString description;

      switch( d->mAggregation->grouping() )
      {
        case Aggregation::GroupByDate:
          if ( d->mAggregation->threading() != Aggregation::NoThreading )
          {
            switch ( d->mAggregation->threadLeader() )
            {
              case Aggregation::TopmostMessage:
                if ( ghi->label().contains( QRegExp( QLatin1String( "[0-9]" ) ) ) )
                  description = i18nc(
                      "@info:tooltip Formats to something like 'Threads started on 2008-12-21'",
                      "Threads started on %1",
                      ghi->label()
                    );
                else
                  description = i18nc(
                      "@info:tooltip Formats to something like 'Threads started Yesterday'",
                      "Threads started %1",
                      ghi->label()
                    );
              break;
              case Aggregation::MostRecentMessage:
                description = i18n( "Threads with messages dated %1", ghi->label() );
              break;
              default:
                // nuthin, make gcc happy
              break;
            }
          } else {
            if ( ghi->label().contains( QRegExp( QLatin1String( "[0-9]" ) ) ) )
            {
              if ( storageModel()->containsOutboundMessages() )
                description = i18nc(
                    "@info:tooltip Formats to something like 'Messages sent on 2008-12-21'",
                    "Messages sent on %1",
                    ghi->label()
                  );
              else
                description = i18nc(
                    "@info:tooltip Formats to something like 'Messages received on 2008-12-21'",
                    "Messages received on %1",
                    ghi->label()
                  );
            } else {
              if ( storageModel()->containsOutboundMessages() )
                description = i18nc(
                    "@info:tooltip Formats to something like 'Messages sent Yesterday'",
                    "Messages sent %1",
                    ghi->label()
                  );
              else
                description = i18nc(
                    "@info:tooltip Formats to something like 'Messages received Yesterday'",
                    "Messages received %1",
                    ghi->label()
                  );
            }
          }
        break;
        case Aggregation::GroupByDateRange:
          if ( d->mAggregation->threading() != Aggregation::NoThreading )
          {
            switch ( d->mAggregation->threadLeader() )
            {
              case Aggregation::TopmostMessage:
                description = i18n( "Threads started within %1", ghi->label() );
              break;
              case Aggregation::MostRecentMessage:
                description = i18n( "Threads containing messages with dates within %1", ghi->label() );
              break;
              default:
                // nuthin, make gcc happy
              break;
            }
          } else {
            if ( storageModel()->containsOutboundMessages() )
              description = i18n( "Messages sent within %1", ghi->label() );
            else
              description = i18n( "Messages received within %1", ghi->label() );
          }
        break;
        case Aggregation::GroupBySenderOrReceiver:
        case Aggregation::GroupBySender:
          if ( d->mAggregation->threading() != Aggregation::NoThreading )
          {
            switch ( d->mAggregation->threadLeader() )
            {
              case Aggregation::TopmostMessage:
                description = i18n( "Threads started by %1", ghi->label() );
              break;
              case Aggregation::MostRecentMessage:
                description = i18n( "Threads with most recent message by %1", ghi->label() );
              break;
              default:
                // nuthin, make gcc happy
              break;
            }
          } else {
            if ( storageModel()->containsOutboundMessages() )
            {
              if ( d->mAggregation->grouping() == Aggregation::GroupBySenderOrReceiver )
                description = i18n( "Messages sent to %1", ghi->label() );
              else
                description = i18n( "Messages sent by %1", ghi->label() );
            } else {
              description = i18n( "Messages received from %1", ghi->label() );
            }
          }
        break;
        case Aggregation::GroupByReceiver:
          if ( d->mAggregation->threading() != Aggregation::NoThreading )
          {
            switch ( d->mAggregation->threadLeader() )
            {
              case Aggregation::TopmostMessage:
                description = i18n( "Threads directed to %1", ghi->label() );
              break;
              case Aggregation::MostRecentMessage:
                description = i18n( "Threads with most recent message directed to %1", ghi->label() );
              break;
              default:
                // nuthin, make gcc happy
              break;
            }
          } else {
            if ( storageModel()->containsOutboundMessages() )
            {
              description = i18n( "Messages sent to %1", ghi->label() );
            } else {
              description = i18n( "Messages received by %1", ghi->label() );
            }
          }
        break;
        default:
          // nuthin, make gcc happy
        break;
      }

      if ( !description.isEmpty() )
      {
        tip += QString::fromLatin1(
             "<tr>" \
                "<td align=\"%2\" valign=\"middle\">" \
                   "%1" \
                "</td>" \
              "</tr>"
          ).arg( description ).arg( textDirection );
      }

      if ( ghi->hasChildren() )
      {
        Item::ChildItemStats stats;
        ghi->childItemStats( stats );

        QString statsText;

        if ( d->mAggregation->threading() != Aggregation::NoThreading )
        {
          statsText = i18np( "<b>%1</b> thread", "<b>%1</b> threads", ghi->childItemCount() );
          statsText += QLatin1String( ", " );
        }

        statsText += i18np(
            "<b>%1</b> message (<b>%2</b> unread)",
            "<b>%1</b> messages (<b>%2</b> unread)",
            stats.mTotalChildCount,
            stats.mUnreadChildCount
          );

        tip += QString::fromLatin1(
             "<tr>" \
                "<td bgcolor=\"%1\" align=\"%3\" valign=\"middle\">" \
                   "<nobr>%2</nobr>" \
                "</td>" \
              "</tr>"
          ).arg( darkerColorName ).arg( statsText ).arg( textDirection );
      }

    }
    break;
    default:
      // nuthin (just make gcc happy for now)
    break;
  }


  tip += QString::fromLatin1(
      "</table>"
    );

  QToolTip::showText( he->globalPos(), tip, viewport(), visualRect( idx ) );

  return true;
}

void View::slotCollapseAllGroups()
{
  setAllGroupsExpanded( false );
}

void View::slotExpandAllGroups()
{
  setAllGroupsExpanded( true );
}

void View::focusQuickSearch()
{
  d->mWidget->focusQuickSearch();
}

Akonadi::MessageStatus View::currentFilterStatus() const
{
  return d->mWidget->currentFilterStatus();
}


QString View::currentFilterSearchString() const
{
  return d->mWidget->currentFilterSearchString();
}

void View::setRowHidden( int row, const QModelIndex & parent, bool hide )
{
  const QModelIndex rowModelIndex = model()->index( row, 0, parent );
  const Item*  const rowItem = static_cast< Item * >( rowModelIndex.internalPointer() );

  if ( rowItem ) {
    const bool currentlyHidden = isRowHidden( row, parent );

    if ( currentlyHidden != hide ) {
      if ( currentMessageItem() == rowItem ) {
        selectionModel()->clear();
        selectionModel()->clearSelection();
      }
    }
  }

  QTreeView::setRowHidden( row, parent, hide );
}

#include "view.moc"
