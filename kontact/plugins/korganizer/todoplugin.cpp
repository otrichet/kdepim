/*
  This file is part of Kontact.

  Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "todoplugin.h"
#include "calendarinterface.h"
#include "korg_uniqueapp.h"
#include "todosummarywidget.h"

#include <libkdepim/maillistdrag.h>

#include <KABC/VCardDrag>

#include <kcalcore/memorycalendar.h>
#include <kcalutils/icaldrag.h>

#include <KontactInterface/Core>

#include <KAction>
#include <KActionCollection>
#include <KDebug>
#include <KIcon>
#include <KIconLoader>
#include <KLocale>
#include <KMessageBox>
#include <KSystemTimeZone>
#include <KTemporaryFile>

#include <QDropEvent>

EXPORT_KONTACT_PLUGIN( TodoPlugin, todo )

TodoPlugin::TodoPlugin( KontactInterface::Core *core, const QVariantList & )
  : KontactInterface::Plugin( core, core, "korganizer", "todo" ), mIface( 0 )
{
  setComponentData( KontactPluginFactory::componentData() );
  KIconLoader::global()->addAppDir( "korganizer" );
  KIconLoader::global()->addAppDir( "kdepim" );

  KAction *action =
    new KAction( KIcon( "task-new" ),
                 i18nc( "@action:inmenu", "New To-do..." ), this );
  actionCollection()->addAction( "new_todo", action );
  action->setShortcut( QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_T ) );
  action->setHelpText(
    i18nc( "@info:status", "Create a new to-do" ) );
  action->setWhatsThis(
    i18nc( "@info:whatsthis",
           "You will be presented with a dialog where you can create a new to-do item." ) );
  connect( action, SIGNAL(triggered(bool)), SLOT(slotNewTodo()) );
  insertNewAction( action );

  KAction *syncAction =
    new KAction( KIcon( "view-refresh" ),
                 i18nc( "@action:inmenu", "Sync To-do List" ), this );
  syncAction->setHelpText(
    i18nc( "@info:status", "Synchronize groupware to-do list" ) );
  syncAction->setWhatsThis(
    i18nc( "@info:whatsthis",
           "Choose this option to synchronize your groupware to-do list." ) );
  connect( syncAction, SIGNAL(triggered(bool)), SLOT(slotSyncTodos()) );
  insertSyncAction( syncAction );

  mUniqueAppWatcher = new KontactInterface::UniqueAppWatcher(
    new KontactInterface::UniqueAppHandlerFactory<KOrganizerUniqueAppHandler>(), this );
}

TodoPlugin::~TodoPlugin()
{
}

KontactInterface::Summary *TodoPlugin::createSummaryWidget( QWidget *parent )
{
  return new TodoSummaryWidget( this, parent );
}

KParts::ReadOnlyPart *TodoPlugin::createPart()
{
  KParts::ReadOnlyPart *part = loadPart();

  if ( !part ) {
    return 0;
  }

  mIface = new OrgKdeKorganizerCalendarInterface(
    "org.kde.korganizer", "/Calendar", QDBusConnection::sessionBus(), this );

  return part;
}

void TodoPlugin::select()
{
  interface()->showTodoView();
}

QStringList TodoPlugin::invisibleToolbarActions() const
{
  QStringList invisible;
  invisible += "new_event";
  invisible += "new_todo";
  invisible += "new_journal";

  invisible += "view_whatsnext";
  invisible += "view_day";
  invisible += "view_nextx";
  invisible += "view_month";
  invisible += "view_workweek";
  invisible += "view_week";
  invisible += "view_list";
  invisible += "view_todo";
  invisible += "view_journal";
  invisible += "view_timeline";
  invisible += "view_timespent";

  return invisible;
}

OrgKdeKorganizerCalendarInterface *TodoPlugin::interface()
{
  if ( !mIface ) {
    part();
  }
  Q_ASSERT( mIface );
  return mIface;
}

void TodoPlugin::slotNewTodo()
{
  interface()->openTodoEditor( "" );
}

void TodoPlugin::slotSyncTodos()
{
#if 0
  QDBusMessage message =
      QDBusMessage::createMethodCall( "org.kde.kmail", "/Groupware",
                                      "org.kde.kmail.groupware",
                                      "triggerSync" );
  message << QString( "Todo" );
  QDBusConnection::sessionBus().send( message );
#else
  kWarning()<<" TodoPlugin::slotSyncTodos : need to port to Akonadi";
#endif
}

bool TodoPlugin::createDBUSInterface( const QString &serviceType )
{
  if ( serviceType == "DBUS/Organizer" || serviceType == "DBUS/Calendar" ) {
    if ( part() ) {
      return true;
    }
  }
  return false;
}

bool TodoPlugin::canDecodeMimeData( const QMimeData *mimeData ) const
{
  return
    mimeData->hasText() ||
    KPIM::MailList::canDecode( mimeData ) ||
    KABC::VCardDrag::canDecode( mimeData ) ||
    KCalUtils::ICalDrag::canDecode( mimeData );
}

bool TodoPlugin::isRunningStandalone() const
{
  return mUniqueAppWatcher->isRunningStandalone();
}

void TodoPlugin::processDropEvent( QDropEvent *event )
{
  const QMimeData *md = event->mimeData();

  if ( KABC::VCardDrag::canDecode( md ) ) {
    KABC::Addressee::List contacts;

    KABC::VCardDrag::fromMimeData( md, contacts );

    KABC::Addressee::List::Iterator it;

    QStringList attendees;
    for ( it = contacts.begin(); it != contacts.end(); ++it ) {
      QString email = (*it).fullEmail();
      if ( email.isEmpty() ) {
        attendees.append( (*it).realName() + "<>" );
      } else {
        attendees.append( email );
      }
    }

    interface()->openTodoEditor( i18nc( "@item", "Meeting" ),
                                 QString(), QStringList(), attendees );
    return;
  }

  if ( KCalUtils::ICalDrag::canDecode( event->mimeData() ) ) {
    KCalCore::MemoryCalendar::Ptr cal( new KCalCore::MemoryCalendar( KSystemTimeZones::local() ) );
    if ( KCalUtils::ICalDrag::fromMimeData( event->mimeData(), cal ) ) {
      KCalCore::Incidence::List incidences = cal->incidences();
      Q_ASSERT( incidences.count() );
      if ( !incidences.isEmpty() ) {
        event->accept();
        KCalCore::Incidence::Ptr i = incidences.first();
        QString summary;
        if ( i->type() == KCalCore::Incidence::TypeJournal ) {
          summary = i18nc( "@item", "Note: %1", i->summary() );
        } else {
          summary = i->summary();
        }
        interface()->openTodoEditor( summary, i->description(), QStringList() );
        return;
      }
      // else fall through to text decoding
    }
  }

  if ( md->hasText() ) {
    QString text = md->text();
    interface()->openTodoEditor( text );
    return;
  }

  if ( KPIM::MailList::canDecode( md ) ) {
    KPIM::MailList mails = KPIM::MailList::fromMimeData( md );
    event->accept();
    if ( mails.count() != 1 ) {
      KMessageBox::sorry(
        core(),
        i18nc( "@info", "Dropping multiple mails is not supported." ) );
    } else {
      KPIM::MailSummary mail = mails.first();
      QString txt = i18nc( "@item", "From: %1\nTo: %2\nSubject: %3",
                           mail.from(), mail.to(), mail.subject() );
      QString uri = QLatin1String( "kmail:" ) +
                    QString::number( mail.serialNumber() ) + '/' +
                    mail.messageId();
      KTemporaryFile tf;
      tf.setAutoRemove( true );
      tf.write( event->encodedData( "message/rfc822" ) );
      interface()->openTodoEditor(
        i18nc( "@item", "Mail: %1", mail.subject() ),
        txt, uri, tf.fileName(), QStringList(), "message/rfc822" );
      tf.close();
    }
    return;
  }

  kWarning() << QString( "Cannot handle drop events of type '%1'." ).arg( event->format() );
}

#include "todoplugin.moc"
