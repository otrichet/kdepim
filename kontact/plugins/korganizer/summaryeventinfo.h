/*
  This file is part of Kontact.

  Copyright (c) 2003 Tobias Koenig <tokoe@kde.org>
  Copyright (c) 2005-2006,2008-2009 Allen Winter <winter@kde.org>
  Copyright (c) 2008 Thomas McGuire <mcguire@kde.org>

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
#ifndef SUMMARYEVENTINFO_H
#define SUMMARYEVENTINFO_H

#include <QList>
#include <QString>

#include <kcalcore/event.h>
#include <kcalcore/calendar.h>

class QDate;

class SummaryEventInfo
{
  public:

    typedef QList<SummaryEventInfo*> List;

    SummaryEventInfo();

    static List eventsForDate( const QDate &date,
                               const KCalCore::Calendar::Ptr &calendar );
    static void setShowSpecialEvents( bool skipBirthdays, bool skipAnniversaries );

    KCalCore::Event::Ptr ev;
    QString startDate;
    QString dateSpan;
    QString daysToGo;
    QString timeRange;
    QString summaryText;
    QString summaryUrl;
    QString summaryTooltip;
    bool makeBold;

  private:

    static void dateDiff( const QDate &date, int &days );
    static bool skip( const KCalCore::Event::Ptr &event );
    static bool mShowBirthdays, mShowAnniversaries;
};

#endif
