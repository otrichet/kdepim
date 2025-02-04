/*
  This file is part of KOrganizer.

  Copyright (c) 2000,2001 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2004 David Faure <faure@kde.org>
  Copyright (C) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

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
#ifndef EVENTARCHIVER_H
#define EVENTARCHIVER_H

#include "calendarsupport_export.h"

#include "incidencechanger.h"

#include <kcalcore/event.h>
#include <kcalcore/todo.h>

#include <Akonadi/Item>

#include <QObject>

class QDate;

namespace CalendarSupport {
  class Calendar;

/**
 * This class handles expiring and archiving of events.
 * It is used directly by the archivedialog, and it is also
 * triggered by actionmanager's timer for auto-archiving.
 *
 * The settings are not held in this class, but directly in KOPrefs (from korganizer.kcfg)
 * Be sure to set mArchiveAction and mArchiveFile before a manual archiving
 * mAutoArchive is used for auto archiving.
 */
class CALENDARSUPPORT_EXPORT EventArchiver : public QObject
{
  Q_OBJECT
  public:
    explicit EventArchiver( QObject *parent = 0 );
    virtual ~EventArchiver();

    /**
     * Delete or archive events once
     * @param calendar the calendar to archive
     * @param limitDate all events *before* the limitDate (not included) will be deleted/archived.
     * @param widget parent widget for message boxes
     * Confirmation and "no events to process" dialogs will be shown
     */
    void runOnce( CalendarSupport::Calendar *calendar, CalendarSupport::IncidenceChanger* changer, const QDate &limitDate, QWidget *widget );

    /**
     * Delete or archive events. This is called regularly, when auto-archiving
     * is enabled
     * @param calendar the calendar to archive
     * @param widget parent widget for message boxes
     * @param withGUI whether this is called from the dialog, so message boxes should be shown.
     * Note that error dialogs like "cannot save" are shown even if from this method, so widget
     * should be set in all cases.
     */
    void runAuto( CalendarSupport::Calendar *calendar, CalendarSupport::IncidenceChanger* changer, QWidget *widget, bool withGUI );

  signals:
    void eventsDeleted();

  private:
    void run( CalendarSupport::Calendar *calendar, CalendarSupport::IncidenceChanger* changer, const QDate &limitDate, QWidget *widget,
              bool withGUI, bool errorIfNone );

    void deleteIncidences( CalendarSupport::IncidenceChanger* changer, const QDate &limitDate, QWidget *widget,
                           const Akonadi::Item::List &incidences, bool withGUI );
    void archiveIncidences( CalendarSupport::Calendar *calendar, CalendarSupport::IncidenceChanger* changer, const QDate &limitDate, QWidget *widget,
                            const Akonadi::Item::List &incidences, bool withGUI );

    /**
     * Checks if all to-dos under @p todo and including @p todo were completed before @p limitDate.
     * If not, we can't archive this to-do.
     * @param todo root of the sub-tree we are checking
     * @param limitDate
     * @param checkedUids used internaly to prevent infinit recursion due to invalid calendar files
     */
    bool isSubTreeComplete( CalendarSupport::Calendar *calendar,
                            const KCalCore::Todo::Ptr &todo,
                            const QDate &limitDate, QStringList checkedUids = QStringList() ) const;
};

}

#endif /* EVENTARCHIVER_H */
