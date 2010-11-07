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

#include "collectiontree/widget.h"

#include "akobackit/akonadi_manager.h"
#include "akobackit/group_manager.h"
#include "collectiontree/collection_filter_proxy_model.h"
#include "collectiontree/view.h"
#include "knglobals.h"

#include <Akonadi/AgentManager>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/Control>
#include <Akonadi/EntityTreeView>
#include <Akonadi/EntityTreeModel>
#include <akonadi/etmviewstatesaver.h>
#include <akonadi/selectionproxymodel.h>
#include <Akonadi/Session>
#include <akonadi/statisticsproxymodel.h>
#include <KConfigGroup>
#include <KXMLGUIClient>
#include <QtGui/QApplication>
#include <QTimer>
#include <QSplitter>

namespace KNode {
namespace CollectionTree {

Widget::Widget( KXMLGUIClient *guiClient, QWidget *parent )
  : QSplitter( Qt::Vertical, parent ),
    mTreeView( new View( guiClient, this ) ),
    mViewSaver( 0 ),
    mSelectionModel( 0 )
{
  setObjectName( QLatin1String( "CollectionTree/Widget" ) );
  addWidget( mTreeView );
  setContentsMargins( 0, 0, 0, 0 );

//   QTimer::singleShot( 0, this, SLOT( init() ) );
  init();
}

Widget::~Widget()
{
}

void Widget::init()
{
  // Connections
  connect( mTreeView, SIGNAL( currentChanged( const Akonadi::Collection & ) ),
           this, SIGNAL( selectedCollectionChanged( const Akonadi::Collection & ) ) );

  // Show unread/total counts
  Akonadi::StatisticsProxyModel *statisticsModel = new Akonadi::StatisticsProxyModel( this );
  statisticsModel->setSourceModel( Akobackit::manager()->entityModel() );
  statisticsModel->setDynamicSortFilter( true );
  statisticsModel->setToolTipEnabled( true );
  // filter collections containing news articles
  CollectionFilterProxyModel *filterModel = new CollectionFilterProxyModel( this );
  filterModel->setSourceModel( statisticsModel );
  filterModel->setDynamicSortFilter( true );

  // View
  mTreeView->setModel( filterModel );
  mTreeView->setColumnHidden( View::SizeColumn, true );

  // Selection model
  mSelectionModel = new Akonadi::SelectionProxyModel( mTreeView->selectionModel(), this );
  mSelectionModel->setSourceModel( Akobackit::manager()->entityModel() );

  // Restore/save view layout
  connect( mTreeView->model(), SIGNAL( modelAboutToBeReset() ),
           this, SLOT( saveState() ) );
  connect( mTreeView->model(), SIGNAL( modelReset() ),
           this, SLOT( restoreState() ) );
  connect( qApp, SIGNAL( aboutToQuit() ),
           this, SLOT( saveState() ) );
  restoreState();
}

void Widget::restoreState()
{
  Akonadi::ETMViewStateSaver *saver = new Akonadi::ETMViewStateSaver();
  saver->setView( mTreeView );
  KConfig *conf = KNGlobals::self()->config();
  KConfigGroup group( conf, "GroupView" );
  saver->restoreState( group );
}

void Widget::saveState()
{
  Akonadi::ETMViewStateSaver saver;
  saver.setView( mTreeView );
  KConfig *conf = KNGlobals::self()->config();
  KConfigGroup group( conf, "GroupView" );
  saver.saveState( group );
  group.sync();
}



Akonadi::Collection Widget::selectedCollection() const
{
  if ( !mSelectionModel ) {
    return Akonadi::Collection();
  }

  const QModelIndexList indexes = mSelectionModel->selectionModel()->selectedIndexes();
  if ( indexes.isEmpty() ) {
    return Akonadi::Collection();
  }
  return indexes[ 0 ].data( Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
}


void Widget::renameCollection( const Akonadi::Collection &col )
{
  if ( !col.isValid() ) {
    return;
  }

  const QModelIndex idx = Akonadi::EntityTreeModel::modelIndexForCollection( mTreeView->model(), col );
  if ( !idx.isValid() ) {
    kDebug() << "Invalid index for collection " << col.id() << col.name();
    return;
  }
  mTreeView->setCurrentIndex( idx );
  mTreeView->scrollTo( idx );
  mTreeView->edit( idx );
}


QItemSelectionModel * Widget::selectionModel() const
{
  return mTreeView->selectionModel();
}


void Widget::nextGroup()
{
  const QModelIndex index = mTreeView->currentIndex();
  findNextGroup( ( index.isValid() ? index : mTreeView->rootIndex() ), 0 );
}

void Widget::previousGroup()
{
  // TODO
}


bool Widget::findNextGroup( const QModelIndex &parent, int testRow )
{
  const QModelIndex child = mTreeView->model()->index( testRow, 0, parent );
  if ( child.isValid() ) {
    const Akonadi::Collection c = child.data( Akonadi::EntityTreeModel::CollectionRole )
                                       .value<Akonadi::Collection>();
    if ( Akobackit::manager()->groupManager()->isGroup( c ) ) {
      mTreeView->setCurrentIndex( child );
      return true;
    }

    // Search among children
    if ( findNextGroup( child, 0 ) ) {
      return true;
    }

    // Search under next sibling
    if ( findNextGroup( parent, testRow + 1 ) ) {
      return true;
    }
  }

  // Search under parent's siblings
  if ( parent.isValid() ) {
    return findNextGroup( parent.parent(), parent.row() + 1 );
  }

  return false;
}

}
}

#include "collectiontree/widget.moc"
