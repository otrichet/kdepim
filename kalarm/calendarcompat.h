/*
 *  calendarcompat.h  -  compatibility for old calendar file formats
 *  Program:  kalarm
 *  Copyright © 2005-2008,2010 by David Jarvie <djarvie@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef CALENDARCOMPAT_H
#define CALENDARCOMPAT_H

/* @file calendarcompat.h - compatibility for old calendar file formats */

#include "kacalendar.h"
#ifdef USE_AKONADI
#include <akonadi/collection.h>
#include <kcalcore/memorycalendar.h>
#include <kcalcore/filestorage.h>
#else
#include "alarmresource.h"
namespace KCal { class CalendarLocal; }
#endif


class CalendarCompat
{
    public:
#ifdef USE_AKONADI
        /** Whether the fix function should convert old format KAlarm calendars. */
        enum FixFunc { PROMPT, PROMPT_PART, CONVERT, NO_CONVERT };

        static KAlarm::Calendar::Compat fix(const KCalCore::FileStorage::Ptr&, 
                                            const Akonadi::Collection& = Akonadi::Collection(),
                                            FixFunc = PROMPT, bool* wrongType = 0);
#else
        static KAlarm::Calendar::Compat fix(KCal::CalendarLocal&, const QString& localFile,
                                            AlarmResource* = 0, AlarmResource::FixFunc = AlarmResource::PROMPT, bool* wrongType = 0);
#endif
};

#endif // CALENDARCOMPAT_H

// vim: et sw=4:
