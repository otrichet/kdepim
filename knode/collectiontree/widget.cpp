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
#include "collectiontree/view.h"
#include "knglobals.h"

#include <Akonadi/AgentManager>
#include <akonadi/recursivecollectionfilterproxymodel.h>
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
#include <QVBoxLayout>
#include <QSplitter>

namespace KNode {
namespace CollectionTree {

Widget::Widget( KXMLGUIClient *guiClient, QWidget *parent )
  : QSplitter( Qt::Vertical, parent ),
    mTreeView( new View( guiClient, this ) ),
    mViewSaver( 0 ),
    mSelectionModel( 0 )
{
  addWidget( mTreeView );
  setContentsMargins( 0, 0, 0, 0 );

  QTimer::singleShot( 0, this, SLOT( init() ) );
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
  statisticsModel->setSourceModel( Akobackit::manager()->collectionModel() );
//   statisticsModel->setToolTipEnabled( true );
  // filter collections containing news articles
  Akonadi::RecursiveCollectionFilterProxyModel *filterModel = new Akonadi::RecursiveCollectionFilterProxyModel( this );
//   filterModel->addContentMimeTypeInclusionFilter( KMime::NewsArticle::mimeType() );
  filterModel->setSourceModel( statisticsModel );
//   filterModel->setDynamicSortFilter( true );
  filterModel->setSortCaseSensitivity( Qt::CaseInsensitive );

  // View
  mTreeView->setModel( filterModel );
  mTreeView->setColumnHidden( View::SizeColumn, true );

  // Selection model
  mSelectionModel = new Akonadi::SelectionProxyModel( mTreeView->selectionModel(), this );
  mSelectionModel->setSourceModel( filterModel );

  // Restore/save view layout
  // FIXME: disabled until the crash it induce is fixed. Seems like the connnection
  //        between currentChanged(Ak::Collection) and selectedCollectionChanged(Ak::Collection)
  //        triggers this crash.
//   connect( mTreeView->model(), SIGNAL( modelAboutToBeReset() ),
//            this, SLOT( saveState() ) );
//   connect( mTreeView->model(), SIGNAL( modelReset() ),
//            this, SLOT( restoreState() ) );
//   connect( qApp, SIGNAL( aboutToQuit() ),
//            this, SLOT( saveState() ) );
//   restoreState();
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


}
}

#include "collectiontree/widget.moc"
