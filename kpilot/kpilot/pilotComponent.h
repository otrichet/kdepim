#ifndef _KPILOT_PILOTCOMPONENT_H
#define _KPILOT_PILOTCOMPONENT_H
/* pilotComponent.h			KPilot
**
** Copyright (C) 1998-2001 by Dan Pilone
**
** See the .cc file for an explanation of what this file is for.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program in a file called COPYING; if not, write to
** the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
** MA 02111-1307, USA.
*/

/*
** Bug reports and questions can be sent to kde-pim@kde.org
*/

/**
  * Base class for any module to KPilot
  */
#ifndef QWIDGET_H
#include <qwidget.h>
#endif

#ifndef QSTRING_H
#include <qstring.h>
#endif

struct CategoryAppInfo;
class QComboBox;

class PilotComponent : public QWidget
{
Q_OBJECT

public:
	PilotComponent(QWidget* parent, 
		const char *id,
		const QString& dbPath);

	/**
	* Load data from files, etc. Always called
	* before the component is made visible the first time.
	*/
	virtual void initialize() = 0;

	/**
	* Get ready for a hotsync -- write any unflushed records
	* to disk, close windows, whatever. Returns false if it
	* is impossible to go into a sync now (due to open windows
	* or strange state.).
	*
	* The default implementation returns true.
	*
	* If the function returns false, it can also put a string
	* stating the reason why into @p s. This string will be 
	* displayed to the user:
	*     "Can't start HotSync. %1" 
	* where %1 is replaced by s.
	*/
	virtual bool preHotSync(QString &s) ;

	/**
	* Reload data (possibly changed by the hotsync) etc. etc.
	*/
	virtual void postHotSync() { } ;

protected:
	/**
	* Look up the selected category from the combo box in the
	* Pilot's register of categories. We need this functon because
	* the combo box doesn't contain any reference to the category
	* ID, and we need that ID to do anything with the Pilot.
	*
	* If AllIsUnfiled is true, then when the user selects the
	* category "All" in the combo box (always the first category),
	* Unfiled (0) is returned. Otherwise if the category "All"
	* is selected -1 is returned. For all other categories
	* selected, their ID is returned. If nothing is selected,
	* behave as if "All" is selected.
	*/
	int findSelectedCategory(QComboBox *,
		CategoryAppInfo *,
		bool AllIsUnfiled=false);

	/**
	* Populate the combo box with the categories found in
	* the Pilot's application categories block. Erases
	* combo box's contents first. 
	*
	* Always includes the category "All" as the first
	* entry in the combo box.
	*
	* If info is a NULL pointer, just put "All" in the combo box.
	*/
	void populateCategories(QComboBox *,
		CategoryAppInfo *info=0);

	const QString& dbPath() const { return fDBPath; } ;

public slots:
	void slotShowComponent();

signals:
	void showComponent(PilotComponent *);

private:
	QString fDBPath;
} ;

#endif
