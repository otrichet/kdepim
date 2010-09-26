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

#include "widget.h"

#include "akobackit/akonadi_manager.h"
#include "akobackit/folder_manager.h"
#include "akobackit/group_manager.h"

#include <KLocalizedString>
#include <KXMLGUIClient>
#include <KXMLGUIFactory>
#include <messagelist/core/aggregation.h>
#include <messagelist/core/messageitem.h>
#include <messagelist/widget.h>
#include <QMenu>

namespace KNode {
namespace MessageListView {

Widget::Widget( QWidget *parent, KXMLGUIClient *xmlGuiClient )
  : MessageList::Widget( parent ),
    mGuiClient( xmlGuiClient )
{
  setXmlGuiClient( xmlGuiClient );
  QTimer::singleShot( 100, this, SLOT( initAggregation() ) );
}

Widget::~Widget()
{
}



void Widget::viewMessageListContextPopupRequest( const QList<MessageList::Core::MessageItem*> &selectedItems, const QPoint &globalPos )
{
  QString name;

  if ( Akobackit::manager()->groupManager()->isGroup( currentCollection() ) ) {
    name = QLatin1String( "remote_popup" );
  } else if ( Akobackit::manager()->folderManager()->isFolder( currentCollection() ) ) {
    name = QLatin1String( "local_popup" );
  }

  if ( !name.isEmpty() ) {
    QMenu *popup = static_cast<QMenu*>( mGuiClient->factory()->container( name, mGuiClient ) );
    popup->popup( globalPos );
    return;
  }

  MessageList::Widget::viewMessageListContextPopupRequest( selectedItems, globalPos );
}



void Widget::initAggregation()
{
  view()->setAggregation(
                new MessageList::Core::Aggregation(
                      i18nc( "Messages aggregation name (in the header view)", "News group" ),
                      i18n( "This aggregation of message is the best suited to read news group." ),
                      MessageList::Core::Aggregation::NoGrouping,
                      MessageList::Core::Aggregation::NeverExpandGroups,
                      MessageList::Core::Aggregation::PerfectReferencesAndSubject,
                      MessageList::Core::Aggregation::MostRecentMessage,
                      MessageList::Core::Aggregation::ExpandThreadsWithUnreadMessages,
                      MessageList::Core::Aggregation::FavorInteractivity ) );
  view()->reload();
}



void Widget::markAll( const Akonadi::MessageStatus &newStatus )
{
  kDebug() << "AKONADI PORT: Not implemented code in" << Q_FUNC_INFO;
//   const Akonadi::Item::List items = ...;
//   Akobackit::manager()->changeStatus( items, newStatus );
}

void Widget::markSelection( const Akonadi::MessageStatus &newStatus )
{
  const Akonadi::Item::List items = selectionAsMessageItemList( true/*include collapsed items*/ );
  Akobackit::manager()->changeStatus( items, newStatus );
}

void Widget::markThread( const Akonadi::MessageStatus &newStatus )
{
  const Akonadi::Item::List items = currentThreadAsMessageList();
  Akobackit::manager()->changeStatus( items, newStatus );
}

bool Widget::toggleThread( const Akonadi::MessageStatus& newStatus )
{
  const Akonadi::Item::List items = currentThreadAsMessageList();
  return Akobackit::manager()->toggleStatus( items, newStatus );
}



}
}
