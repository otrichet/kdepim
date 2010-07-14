/*
    Copyright (c) 2000,2001,2004 Cornelius Schumacher <schumacher@kde.org>
    Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Copyright (c) 2010 Andras Mantia <andras@kdab.com>
    Copyright (C) 2010 Casey Link <casey@kdab.com>

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

#ifndef FREEBUSYITEM_H
#define FREEBUSYITEM_H

#include "attendeedata.h"

namespace KCal {
  class FreeBusy;
}

namespace IncidenceEditorsNG {
/**
 * The FreeBusyItem is the whole line for a given attendee..
 */
class FreeBusyItem
{
  public:
    FreeBusyItem( AttendeeData::Ptr attendee, QWidget *parentWidget );
    ~FreeBusyItem() {}

    AttendeeData::Ptr attendee() const;
    void setFreeBusy( KCal::FreeBusy *fb );
    KCal::FreeBusy *freeBusy() const;

    QString email() const;
    void setUpdateTimerID( int id );
    int updateTimerID() const;

    void startDownload( bool forceDownload );
    void setIsDownloading( bool d );
    bool isDownloading() const;

  private:
    AttendeeData::Ptr mAttendee;
    KCal::FreeBusy *mFreeBusy;

    // This is used for the update timer
    int mTimerID;

    // Only run one download job at a time
    bool mIsDownloading;

    QWidget *mParentWidget;
};

}
#endif //FREEBUSYITEM_H
