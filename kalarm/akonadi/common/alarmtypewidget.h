/*
 *  alarmtypewidget.h  -  KAlarm Akonadi configuration alarm type selection widget
 *  Program:  kalarm
 *  Copyright © 2011 by David Jarvie <djarvie@kde.org>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#ifndef ALARMTYPEWIDGET_H
#define ALARMTYPEWIDGET_H

#include "ui_alarmtypewidget.h"
#include "kacalendar.h"


class AlarmTypeWidget : public QWidget
{
        Q_OBJECT
    public:
        AlarmTypeWidget(QWidget* parent, QLayout* layout);
        void setAlarmTypes(KAlarm::CalEvent::Types);
        KAlarm::CalEvent::Types alarmTypes() const;

    signals:
        void changed();

    private:
        Ui::AlarmTypeWidget ui;
};

#endif // ALARMTYPEWIDGET_H
