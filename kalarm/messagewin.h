/*
 *  messagewin.h  -  displays an alarm message
 *  Program:  kalarm
 *  (C) 2001 by David Jarvie  software@astrojar.org.uk
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef MESSAGEWIN_H
#define MESSAGEWIN_H

#include "mainwindowbase.h"

#include "msgevent.h"
using namespace KCal;

class QPushButton;
class AlarmTimeWidget;

/**
 * MessageWin: A window to display an alarm message
 */
class MessageWin : public MainWindowBase
{
		Q_OBJECT
	public:
		MessageWin();     // for session management restoration only
		explicit MessageWin(const KAlarmEvent&, const KAlarmAlarm&, bool reschedule_event = true);
		~MessageWin();
		static int        instanceCount()  { return nInstances; }

	protected:
		virtual void showEvent(QShowEvent*);
		virtual void resizeEvent(QResizeEvent*);
		virtual void saveProperties(KConfig*);
		virtual void readProperties(KConfig*);

	protected slots:
		void              slotShowDefer();
		void              slotDefer();
		void              slotKAlarm();

	private:
		QSize             initView();
		// KAlarmEvent properties
		KAlarmEvent       event;            // the whole event, for updating the calendar file
		QString           message;
		QFont             font;
		QColor            colour;
		QDateTime         dateTime;
		QString           eventID;
		QString           audioFile;
		int               alarmID;
		int               flags;
		bool              beep;
		bool              file;
		bool              noDefer;          // don't display a Defer option
		// Miscellaneous
		QPushButton*      deferButton;
		AlarmTimeWidget*  deferTime;
		int               deferHeight;      // height of defer dialog
		int               restoreHeight;
		bool              rescheduleEvent;  // true to delete event after message has been displayed
		bool              shown;            // true once the window has been displayed
		bool              deferDlgShown;    // true if defer dialog is visible
		bool              fileError;        // true if initView() couldn't open the file to display
		static int        nInstances;       // number of current instances
};

#endif // MESSAGEWIN_H
