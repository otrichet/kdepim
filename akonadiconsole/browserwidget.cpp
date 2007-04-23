/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "browserwidget.h"

#include <libakonadi/job.h>
#include <libakonadi/collectionview.h>
#include <libakonadi/item.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemserializer.h>
#include <libakonadi/messagemodel.h>
#include <libakonadi/messagecollectionmodel.h>
#include <libakonadi/collectionfilterproxymodel.h>

#include <libkdepim/addresseeview.h>

#include <kdebug.h>
#include <kconfig.h>

#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QStackedWidget>

using namespace Akonadi;

BrowserWidget::BrowserWidget(QWidget * parent) :
    QWidget( parent )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  QSplitter *splitter = new QSplitter( Qt::Horizontal, this );
  layout->addWidget( splitter );

  mCollectionView = new Akonadi::CollectionView();
  connect( mCollectionView, SIGNAL(clicked(QModelIndex)), SLOT(collectionActivated(QModelIndex)) );
  splitter->addWidget( mCollectionView );

  mCollectionModel = new Akonadi::MessageCollectionModel( this );

//   CollectionFilterProxyModel *colProxy = new CollectionFilterProxyModel( this );
//   colProxy->setSourceModel( mCollectionModel );
//   colProxy->addMimeType( "text/calendar" );
  mCollectionView->setModel( mCollectionModel );

  QSplitter *splitter2 = new QSplitter( Qt::Vertical, this );
  splitter->addWidget( splitter2 );

  mItemModel = new Akonadi::ItemModel( this );
//   mItemModel = new Akonadi::MessageModel( this );

  mItemView = new QTreeView( this );
  mItemView->setRootIsDecorated( false );
  mItemView->setModel( mItemModel );
  connect( mItemView, SIGNAL(clicked(QModelIndex)), SLOT(itemActivated(QModelIndex)) );
  splitter2->addWidget( mItemView );

  mStack = new QStackedWidget( this );
  mDataView = new QTextEdit( mStack );
  mDataView->setReadOnly( true );
  mAddresseeView = new KPIM::AddresseeView( mStack );
  mStack->addWidget( mDataView );
  mStack->addWidget( mAddresseeView );
  splitter2->addWidget( mStack );
}

void BrowserWidget::collectionActivated(const QModelIndex & index)
{
  int colId = mCollectionView->model()->data( index, CollectionModel::CollectionIdRole ).toInt();
  if ( colId <= 0 )
    return;
  mItemModel->setCollection( Collection( colId ) );
}

void BrowserWidget::itemActivated(const QModelIndex & index)
{
  DataReference ref = mItemModel->referenceForIndex( index );
  if ( ref.isNull() )
    return;
  ItemFetchJob *job = new ItemFetchJob( ref, this );
  connect( job, SIGNAL(result(KJob*)), SLOT(itemFetchDone(KJob*)) );
  job->start();
}

void BrowserWidget::itemFetchDone(KJob * job)
{
  ItemFetchJob *fetch = static_cast<ItemFetchJob*>( job );
  if ( job->error() ) {
    qWarning() << "Item fetch failed: " << job->errorString();
  } else if ( fetch->items().isEmpty() ) {
    qWarning() << "No item found!";
  } else {
    const Item item = fetch->items().first();
    if ( item.mimeType() == QLatin1String( "text/vcard" ) ) {
      const KABC::Addressee addr = item.payload<KABC::Addressee>();

      mAddresseeView->setAddressee( addr );
      mStack->setCurrentWidget( mAddresseeView );
    } else {
      QByteArray data;
      ItemSerializer::serialize( item, "RFC822", data );
      mDataView->setPlainText( data );
      mStack->setCurrentWidget( mDataView );
    }
  }
}

#include "browserwidget.moc"
