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

#include "collectiontree/view.h"

#include "akobackit/akonadi_manager.h"
#include "akobackit/folder_manager.h"

#include <Akonadi/EntityTreeModel>
#include <Akonadi/KMime/SpecialMailCollections>
#include <KAction>
#include <KLocalizedString>
#include <KMenu>
#include <KXMLGUIClient>
#include <KXMLGUIFactory>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>

namespace KNode {
namespace CollectionTree {

View::View( KXMLGUIClient *xmlGuiClient, QWidget *parent )
  : EntityTreeView( xmlGuiClient, parent ),
    mGuiClient( xmlGuiClient )
{
  setObjectName( QLatin1String( "CollectionTree/View" ) );

  header()->setContextMenuPolicy( Qt::CustomContextMenu );
  connect( header(), SIGNAL( customContextMenuRequested( const QPoint & ) ),
           this, SLOT( displayHeaderContextMenu( const QPoint & ) ) );
}

View::~View()
{
}


void View::contextMenuEvent( QContextMenuEvent *event )
{
  const QModelIndex index = indexAt( event->pos() );
  if ( index.isValid() ) {
    const Akonadi::Collection col = index.data( Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
    if ( col.isValid() ) {
      const char *popupName = 0;

      Akobackit::CollectionType collectionType = Akobackit::manager()->type( col );
      switch( collectionType ) {
        case Akobackit::RootFolder:
          popupName = "root_folder_popup";
          break;
        case Akobackit::OutboxFolder:
        case Akobackit::SentmailFolder:
        case Akobackit::DraftFolder:
        case Akobackit::UserFolder:
          popupName = "folder_popup";
          break;
        case Akobackit::NewsGroup:
          popupName = "group_popup";
          break;
        case Akobackit::NntpServer:
          popupName = "account_popup";
          break;
        case Akobackit::InvalidCollection:
          // Nothing to do
          break;
        default:
          kWarning() << "View::contextMenuEvent: Collection type not handled" << collectionType << "for collection" << col;
      }

      if ( popupName ) {
        QMenu *popup = static_cast<QMenu*>( mGuiClient->factory()->container( popupName, mGuiClient ) );
        if ( popup ) {
          popup->popup( event->globalPos() );
        }
        return;
      }
    }
  }

  Akonadi::EntityTreeView::contextMenuEvent(event);
}


void View::displayHeaderContextMenu( const QPoint &pos )
{
  KMenu *menu = new KMenu( this );
  menu->addTitle( i18n( "View columns" ) );

  const int columnCount = header()->count();
  for ( int i = 0 ; i < columnCount ; ++i ) {
    if ( i == View::NameColumn ) { // Never hide the name column
      continue;
    }
    if ( i == View::SizeColumn ) { // Always hidden in CollectionTree::Widget
      continue;
    }
    const QString name = model()->headerData( i, Qt::Horizontal, Qt::DisplayRole ).toString();
    KAction *action = new KAction( name, menu );
    action->setCheckable( true );
    action->setChecked( !isColumnHidden( i ) );
    action->setData( i );
    menu->addAction( action );
  }
  connect( menu, SIGNAL( triggered( QAction * ) ),
           this, SLOT( showHideColumnRequest( QAction * ) ) );

  menu->popup( mapToGlobal( pos ) );
}

void View::showHideColumnRequest( QAction *selectedAction )
{
  kDebug() << selectedAction;
  Q_ASSERT( selectedAction );
  const QVariant d = selectedAction->data();
  Q_ASSERT( d.isValid() );
  bool ok;
  const int columnIndex = d.toInt( &ok );
  Q_ASSERT( ok );

  if ( columnIndex == View::NameColumn ) {
    // Should never happen, but better be safe.
    return;
  }

  setColumnHidden( columnIndex, !isColumnHidden( columnIndex ) );
}


}
}

#include "collectiontree/view.moc"
