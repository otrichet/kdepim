/* listItem.cc			KPilot
**
** Copyright (C) 1998-2001 by Dan Pilone
**
** Program description
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

static const char *listitems_id =
	"$Id$";

#include "options.h"



#ifndef _QSTRING_H
#include <qstring.h>
#endif
#ifndef _QLISTBOX_H
#include <qlistbox.h>
#endif



#ifndef _KPILOT_LISTITEMS_H
#include "listItems.h"
#endif

#ifdef DEBUG
/* static */ int PilotListItem::crt = 0;
/* static */ int PilotListItem::del = 0;
/* static */ int PilotListItem::count = 0;

/* static */ void PilotListItem::counts()
{
	FUNCTIONSETUP;
	DEBUGKPILOT << fname
		<< ": created=" << crt << " deletions=" << del << endl;
}
#endif

PilotListItem::PilotListItem(const QString & text,
	int pilotid, void *r) :
	QListBoxText(text),
	fid(pilotid),
	fr(r)
{
	// FUNCTIONSETUP;
#ifdef DEBUG
	crt++;
	count++;
	if (!(count & 0xff))
		counts();
#endif
	(void) listitems_id;
}

PilotListItem::~PilotListItem()
{
	// FUNCTIONSETUP;
#ifdef DEBUG
	del++;
	count++;
	if (!(count & 0xff))
		counts();
#endif
}
