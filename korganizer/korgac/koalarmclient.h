/*
  KOrganizer Alarm Daemon Client.

  This file is part of KOrganizer.

  Copyright (c) 2002,2003 Cornelius Schumacher <schumacher@kde.org>

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
#ifndef KOALARMCLIENT_H
#define KOALARMCLIENT_H

#ifndef _WIN32_WCE
#include <KSessionManager>
#endif

#include <QTimer>
#include <QDateTime>

class AlarmDialog;
class AlarmDockWindow;

namespace CalendarSupport {
  class Calendar;
}

namespace Akonadi {
  class Item;
}

#ifndef _WIN32_WCE
class KOAlarmClient : public QObject, public KSessionManager
#else
class KOAlarmClient : public QObject
#endif
{
  Q_OBJECT
  public:
    KOAlarmClient( QObject *parent = 0 );
    ~KOAlarmClient();

#ifndef _WIN32_WCE
    bool commitData( QSessionManager & );
#endif

    // DBUS interface
    void quit();
    void hide();
    void show();
    void forceAlarmCheck();
    void dumpDebug();
    QStringList dumpAlarms();

    void debugShowDialog();

  public slots:
    void slotQuit();

  protected slots:
    void checkAlarms();

  signals:
    void reminderCount( int );
    void saveAllSignal();

  private:
    void createReminder( CalendarSupport::Calendar *calendar, const Akonadi::Item &incidence,
                         const QDateTime &dt, const QString &displayText );
    void saveLastCheckTime();

    AlarmDockWindow *mDocker;  // the panel icon
    CalendarSupport::Calendar *mCalendar;

    QDateTime mLastChecked;
    QTimer mCheckTimer;

    AlarmDialog *mDialog;
};

#endif
