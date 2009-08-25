/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "pane.h"

#include <KDE/KIcon>
#include <KDE/KLocale>
#include <KDE/KMenu>

#include <QtCore/QAbstractItemModel>
#include <QtGui/QAbstractProxyModel>
#include <QtGui/QItemSelectionModel>
#include <QtGui/QTabBar>
#include <QtGui/QToolButton>

#include "storagemodel.h"
#include "widget.h"
#include "core/settings.h"

using namespace MessageList;


Pane::Pane( QAbstractItemModel *model, QItemSelectionModel *selectionModel, QWidget *parent )
  : QTabWidget( parent ), mModel( model ), mSelectionModel( selectionModel )
{
  // Build the proxy stack
  const QAbstractProxyModel *proxyModel = qobject_cast<const QAbstractProxyModel*>( mSelectionModel->model() );

  while (proxyModel) {
    if (static_cast<const QAbstractItemModel*>(proxyModel) == mModel) {
      break;
    }

    mProxyStack << proxyModel;
    const QAbstractProxyModel *nextProxyModel = qobject_cast<const QAbstractProxyModel*>(proxyModel->sourceModel());

    if (!nextProxyModel) {
      // It's the final model in the chain, so it is necessarily the sourceModel.
      Q_ASSERT(qobject_cast<const QAbstractItemModel*>(proxyModel->sourceModel()) == mModel);
      break;
    }
    proxyModel = nextProxyModel;
  } // Proxy stack done

  mNewTabButton = new QToolButton( this );
  mNewTabButton->setIcon( KIcon( "tab-new" ) );
  mNewTabButton->adjustSize();
  mNewTabButton->setToolTip( i18nc("@info:tooltip", "Open a new tab"));
  setCornerWidget( mNewTabButton, Qt::TopLeftCorner );
  connect( mNewTabButton, SIGNAL( clicked() ),
           SLOT( onNewTabClicked() ) );

  mCloseTabButton = new QToolButton( this );
  mCloseTabButton->setIcon( KIcon( "tab-close" ) );
  mCloseTabButton->adjustSize();
  mCloseTabButton->setToolTip( i18nc("@info:tooltip", "Close the current tab"));
  setCornerWidget( mCloseTabButton, Qt::TopRightCorner );
  connect( mCloseTabButton, SIGNAL( clicked() ),
           SLOT( onCloseTabClicked() ) );

  createNewTab();
  setMovable( true );

  connect( mSelectionModel, SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
           this, SLOT(onSelectionChanged(QItemSelection, QItemSelection)) );
  connect( this, SIGNAL(currentChanged(int)),
           this, SLOT(onCurrentTabChanged()) );

  setContextMenuPolicy( Qt::CustomContextMenu );
  connect( this, SIGNAL(customContextMenuRequested(QPoint)),
           this, SLOT(onTabContextMenuRequest(QPoint)) );
}

Pane::~Pane()
{
}

bool Pane::selectNextMessageItem( MessageList::Core::MessageTypeFilter messageTypeFilter,
                                    MessageList::Core::ExistingSelectionBehaviour existingSelectionBehaviour,
                                    bool centerItem,
                                    bool loop )
{
  Widget *w = static_cast<Widget*>( currentWidget() );

  if ( w ) {
    return w->selectNextMessageItem( messageTypeFilter, existingSelectionBehaviour, centerItem, loop );
  } else {
    return false;
  }
}

bool Pane::selectPreviousMessageItem( MessageList::Core::MessageTypeFilter messageTypeFilter,
                                        MessageList::Core::ExistingSelectionBehaviour existingSelectionBehaviour,
                                        bool centerItem,
                                        bool loop )
{
  Widget *w = static_cast<Widget*>( currentWidget() );

  if ( w ) {
    return w->selectPreviousMessageItem( messageTypeFilter, existingSelectionBehaviour, centerItem, loop );
  } else {
    return false;
  }
}

bool Pane::focusNextMessageItem( MessageList::Core::MessageTypeFilter messageTypeFilter, bool centerItem, bool loop )
{
  Widget *w = static_cast<Widget*>( currentWidget() );

  if ( w ) {
    return w->focusNextMessageItem( messageTypeFilter, centerItem, loop );
  } else {
    return false;
  }
}

bool Pane::focusPreviousMessageItem( MessageList::Core::MessageTypeFilter messageTypeFilter, bool centerItem, bool loop )
{
  Widget *w = static_cast<Widget*>( currentWidget() );

  if ( w ) {
    return w->focusNextMessageItem( messageTypeFilter, centerItem, loop );
  } else {
    return false;
  }
}

void Pane::selectFocusedMessageItem( bool centerItem )
{
  Widget *w = static_cast<Widget*>( currentWidget() );

  if ( w ) {
    w->selectFocusedMessageItem( centerItem );
  }
}

bool Pane::selectFirstMessageItem( MessageList::Core::MessageTypeFilter messageTypeFilter, bool centerItem )
{
  Widget *w = static_cast<Widget*>( currentWidget() );

  if ( w ) {
    return w->selectFirstMessageItem( messageTypeFilter, centerItem );
  } else {
    return false;
  }
}

void Pane::selectAll()
{
  Widget *w = static_cast<Widget*>( currentWidget() );

  if ( w ) {
    w->selectAll();
  }
}

void Pane::onSelectionChanged( const QItemSelection &selected, const QItemSelection &deselected )
{
  Widget *w = static_cast<Widget*>( currentWidget() );
  QItemSelectionModel *s = mWidgetSelectionHash[w];

  s->select( mapSelectionToSource( selected ), QItemSelectionModel::Select );
  s->select( mapSelectionToSource( deselected ), QItemSelectionModel::Deselect );

  QString label;
  QIcon icon = KIcon( "folder" );
  foreach ( const QModelIndex &index, s->selectedRows() ) {
    label+= index.data( Qt::DisplayRole ).toString()+", ";
  }
  label.chop( 2 );

  if ( label.isEmpty() ) {
    label = i18nc( "@title:tab Empty messagelist", "Empty" );
    icon = QIcon();
  } else if ( s->selectedRows().size()==1 ) {
    icon = s->selectedRows().first().data( Qt::DecorationRole ).value<QIcon>();
  }

  int index = indexOf( w );
  setTabText( index, label );
  setTabIcon( index, icon );
}

void Pane::onNewTabClicked()
{
  createNewTab();
  updateTabControls();
}

void Pane::onCloseTabClicked()
{
  Widget *w = static_cast<Widget*>( currentWidget() );
  if ( !w || (count() < 2) ) {
    return;
  }

  delete w;
  updateTabControls();
}

void Pane::onCurrentTabChanged()
{
  Widget *w = static_cast<Widget*>( currentWidget() );
  QItemSelectionModel *s = mWidgetSelectionHash[w];

  disconnect( mSelectionModel, SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
              this, SLOT(onSelectionChanged(QItemSelection, QItemSelection)) );

  mSelectionModel->select( mapSelectionFromSource( s->selection() ),
                           QItemSelectionModel::ClearAndSelect );

  connect( mSelectionModel, SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
           this, SLOT(onSelectionChanged(QItemSelection, QItemSelection)) );
}

void Pane::onTabContextMenuRequest( const QPoint &pos )
{
  QTabBar *bar = tabBar();
  int index = bar->tabAt( bar->mapFrom( this, pos ) );
  if ( index == -1 ) return;

  Widget *w = qobject_cast<Widget *>( widget( index ) );
  if ( !w ) return;

  KMenu menu( this );
  QAction *action;

  action = menu.addAction( i18nc( "@action:inmenu", "Close Tab" ) );
  action->setEnabled( count() > 1 );
  action->setIcon( KIcon( "tab-close" ) );
  connect( action, SIGNAL(triggered(bool)),
           this, SLOT(onCloseTabClicked()) ); // Reuse the logic...

  QAction *allOther = menu.addAction( i18nc("@action:inmenu", "Close All Other Tabs" ) );
  action->setEnabled( count() > 1 );
  action->setIcon( KIcon( "tab-close-other" ) );

  action = menu.exec( mapToGlobal( pos ) );

  if ( action == allOther ) { // Close all other tabs
    QList<Widget *> widgets;
    int index = indexOf( w );

    for ( int i=0; i<count(); i++ ) {
      if ( i==index) continue; // Skip the current one

      Widget *other = qobject_cast<Widget *>( widget( i ) );
      widgets << other;
    }

    foreach ( Widget *other, widgets ) {
      delete other;
    }

    updateTabControls();
  }
}

void Pane::createNewTab()
{
  Widget * w = new Widget( this );
  addTab( w, i18nc( "@title:tab Empty messagelist", "Empty" ) );

  QItemSelectionModel *s = new QItemSelectionModel( mModel, w );
  MessageList::StorageModel *m = new MessageList::StorageModel( mModel, s, w );
  w->setStorageModel( m );

  mWidgetSelectionHash[w] = s;

  connect( w, SIGNAL(messageSelected(Akonadi::Item)),
           this, SIGNAL(messageSelected(Akonadi::Item)) );
  connect( w, SIGNAL(messageActivated(Akonadi::Item)),
           this, SIGNAL(messageActivated(Akonadi::Item)) );
  connect( w, SIGNAL(selectionChanged()),
           this, SIGNAL(selectionChanged()) );
  connect( w, SIGNAL(messageStatusChangeRequest(Akonadi::Item, KPIM::MessageStatus, KPIM::MessageStatus)),
           this, SIGNAL(messageStatusChangeRequest(Akonadi::Item, KPIM::MessageStatus, KPIM::MessageStatus)) );

  updateTabControls();
  setCurrentWidget( w );
}

QItemSelection Pane::mapSelectionToSource( const QItemSelection &selection ) const
{
  QItemSelection result = selection;

  foreach ( const QAbstractProxyModel *proxy, mProxyStack ) {
    result = proxy->mapSelectionToSource( result );
  }

  return result;
}

QItemSelection Pane::mapSelectionFromSource( const QItemSelection &selection ) const
{
  QItemSelection result = selection;

  typedef QList<const QAbstractProxyModel*>::ConstIterator Iterator;

  for ( Iterator it = mProxyStack.end()-1; it!=mProxyStack.begin(); --it ) {
    result = (*it)->mapSelectionFromSource( result );
  }
  result = mProxyStack.first()->mapSelectionFromSource( result );

  return result;
}

void Pane::updateTabControls()
{
  mCloseTabButton->setEnabled( count()>1 );

  if ( Core::Settings::self()->autoHideTabBarWithSingleTab() ) {
    tabBar()->setVisible( count()>1 );
  } else {
    tabBar()->setVisible( true );
  }
}
