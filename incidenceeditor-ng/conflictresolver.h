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

#ifndef CONFLICTRESOLVER_H
#define CONFLICTRESOLVER_H

#include "incidenceeditors-ng_export.h"


#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QTimerEvent>
#include <QtCore/QSet>
#include <QtCore/QBitArray>
#include <QtCore/QVector>

#include <KDateTime>

#include <kcalcore/freebusy.h>
#include <kcalcore/attendee.h>
#include <kcalcore/period.h>


namespace IncidenceEditorsNG
{

class FreeBusyItem;

/**
 * Takes a list of attendees and event info (e.g., min time start, max time end)
 * fetches their freebusy information, then identifies conflicts and periods of non-conflict.
 *
 * It exposes these periods so another class can display them to the user and allow
 * them to choose a correct time.
 * @author Casey Link
 */
class INCIDENCEEDITORS_NG_EXPORT ConflictResolver : public QObject
{
    Q_OBJECT
public:
   /**
    * @param parentWidget is passed to Akonadi when fetching free/busy data.
    */
    ConflictResolver( QWidget *parentWidget, QObject* parent = 0 );

    /**
     *  Add an attendee
     * The attendees free busy info will be fetched
     * and integrated into the resolver.
     */
    void insertAttendee( const KCalCore::Attendee::Ptr &attendee );

    void insertAttendee( FreeBusyItem* freebusy );
    /**
     * Removes an attendee
     * The attendee will no longer be considered when
     * resolving conflicts
     * */
    void removeAttendee( const KCalCore::Attendee::Ptr &attendee );
    /**
     * Clear all attendees
     **/
    void clearAttendees();

    /**
     * Returns whether the resolver contains the attendee
     */
    bool containsAttendee( const KCalCore::Attendee::Ptr &attendee );

    /**
     * Queues a reload of free/busy data.
     * All current attendees will have their free/busy data
     * redownloaded from Akonadi.
     */
    void triggerReload();
    /**
     * cancel reloading
     * */
    void cancelReload();

    /**
     * Constrain the free time slot search to the weekdays
     * identified by their KCalendarSystem integer representation
     * Default is Monday - Friday
     * @param weekdays a 7 bit array indicating the allowed days (bit 0 = Monday, value 1 = allowed).
     * @see KCalendarSystem
     */
    void setAllowedWeekdays( const QBitArray &weekdays );

    /**
     * Constrain the free time slot search to the set participant roles.
     * Mandatory roles are considered the minimum required to attend
     * the meeting, so only those attendees with the mandatory roles will
     * be considered  in the search.
     * Default is all roles are mandatory.
     * @param roles the set of mandatory participant roles
     */
    void setMandatoryRoles( const QSet<KCalCore::Attendee::Role> &roles );

    /**
     * Returns a list of date time ranges that conform to the
     * search constraints.
     * @see setMandatoryRoles
     * @see setAllowedWeekdays
     */
    KCalCore::Period::List availableSlots() const;

    /**
      Finds a free slot in the future which has at least the same size as
      the initial slot.
    */
    bool findFreeSlot( const KCalCore::Period &dateTimeRange );

signals:
    /**
     * Emitted when the user changes the start and end dateTimes
     * for the incidence.
     **/
    void dateTimesChanged( const KDateTime & newStart, const KDateTime & newEnd );

    /**
     * Emitted when there are conflicts
     * @param number the number of conflicts
     */
    void conflictsDetected( int number );

    /**
     * Emitted when the resolver locates new free slots.
     */
    void freeSlotsAvailable( const KCalCore::Period::List & );

public slots:
    /**
     * Set the timeframe constraints
     *
     * These control the timeframe for which conflicts are to be resolved.
     */
    void setEarliestDate( const QDate &newDate );
    void setEarliestTime( const QTime &newTime );
    void setLatestDate( const QDate &newDate );
    void setLatestTime( const QTime &newTime );

    void setEarliestDateTime( const KDateTime &newDateTime );
    void setLatestDateTime( const KDateTime &newDateTime );

    void findAllFreeSlots();

    void setResolution( int seconds );

protected:
    void timerEvent( QTimerEvent * );

private slots:
    void slotInsertFreeBusy( const KCalCore::FreeBusy::Ptr &fb, const QString &email );

    // Force the download of FB information
    void manualReload();
    // Only download FB if the auto-download option is set in config
    void autoReload();

private:
    void updateFreeBusyData( FreeBusyItem * );

    /**
      Checks whether the slot specified by (tryFrom, tryTo) matches the
      search constraints. If yes, return true. The return value is the
      number of conflicts that were detected, and (tryFrom, tryTo) contain the next free slot for
      that participant. In other words, the returned slot does not have to
      be free for everybody else.
    */
    int tryDate( KDateTime &tryFrom, KDateTime &tryTo );

    /**
      Checks whether the slot specified by (tryFrom, tryTo) is available
      for the participant specified with attendee. If yes, return true. If
      not, return false and change (tryFrom, tryTo) to contain the next
      possible slot for this participant (not necessarily a slot that is
      available for all participants).
    */
    bool tryDate( FreeBusyItem *attendee, KDateTime &tryFrom, KDateTime &tryTo );

    /**
     * Checks whether the supplied attendee passes the
     * current mandatory role constraint.
     * @return true if the attendee is of one of the mandatory roles, false if not
     */
    bool matchesRoleConstraint( const KCalCore::Attendee::Ptr &attendee );

    void calculateConflicts();
    /**
     * Reload FB items
     * */
    void reload();

    KCalCore::Period mTimeframeConstraint; //!< the datetime range for outside of which free slots won't be searched.
    KCalCore::Period::List mAvailableSlots;

    QTimer mReloadTimer;
    QTimer mCalculateTimer; /*!< A timer is used control the calculation of
                                   conflicts to prevent the process from being
                                   repeated many times after a series of quick
                                   parameter changes.
                              */
    bool mForceDownload;
    QList<FreeBusyItem*> mFreeBusyItems;
    QWidget *mParentWidget;

    QSet<KCalCore::Attendee::Role> mMandatoryRoles;
    QBitArray mWeekdays; //!< a 7 bit array indicating the allowed days (bit 0 = Monday, value 1 = allowed).
    int mSlotResolutionSeconds;
};

}

#endif // CONFLICTRESOLVER_H
