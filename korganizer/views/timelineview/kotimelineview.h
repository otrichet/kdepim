/*
  This file is part of KOrganizer.

  Copyright (c) 2007 Till Adam <adam@kde.org>

  Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Copyright (c) 2010 Andras Mantia <andras@kdab.com>
  Copyright (c) 2010 Sérgio Martins <sergio.martins@kdab.com>

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
#ifndef KOTIMELINEVIEW_H
#define KOTIMELINEVIEW_H

#include <koeventview.h>

#include <Akonadi/Collection>
#include <Akonadi/Item>

#include <QMap>

class QStandardItem;
class QTreeWidget;


namespace CalendarSupport {
  class Calendar;
}


/**
  This class provides a view ....
*/
class KOTimelineView : public KOEventView
{
    Q_OBJECT
  public:
    explicit KOTimelineView( QWidget *parent = 0 );
    ~KOTimelineView();

    virtual Akonadi::Item::List selectedIncidences();
    virtual KCalCore::DateList selectedIncidenceDates();
    virtual int currentDateCount() const;
    virtual void showDates( const QDate &, const QDate & );
    virtual void showIncidences( const Akonadi::Item::List &incidenceList, const QDate &date );
    virtual void updateView();
    virtual void changeIncidenceDisplay( const Akonadi::Item &incidence, int mode );
    virtual int maxDatesHint() const { return 0; }
    virtual bool eventDurationHint( QDateTime &startDt, QDateTime &endDt, bool &allDay );
    virtual void setCalendar( CalendarSupport::Calendar *cal );
    virtual void setIncidenceChanger( CalendarSupport::IncidenceChanger *changer );

    // Specific for korg, not in eventviews
    virtual KOrg::CalPrinterBase::PrintType printType() const;

  private:
    class Private;
    Private * const d;
};

#endif
