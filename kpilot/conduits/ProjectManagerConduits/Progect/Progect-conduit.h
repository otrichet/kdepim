#ifndef _ProgectCONDUIT_H
#define _ProgectCONDUIT_H

/* Progect-conduit.h			KPilot
**
** Copyright (C) 1998-2001 Dan Pilone
** Copyright (C) 1998-2000 Preston Brown
** Copyright (C) 1998 Herwin-Jan Steehouwer
** Copyright (C) 1998 Reinhold Kainhofer
**
** This file is part of the Progect conduit, a conduit for KPilot that
** synchronises the Pilot's Progect application with the outside world,
** which currently means KOrganizer.
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
** Bug reports and questions can be sent to groot@kde.org
*/

#include "options.h"
using namespace KCal;

class ProgectConduit : public GenericOrganizerConduit {
	Q_OBJECT
public:
	ProgectConduit(KPilotDeviceLink *, const char *n=0L, const QStringList &l=QStringList());
	virtual ~ProgectConduit() {};

protected:
	virtual long dbtype() { return 0x44415441; }
	virtual long dbcreator() { return 0x6c625047; }
};


// $Log$
// Revision 1.1  2002/04/07 12:09:42  kainhofe
// Initial checkin of the conduit. The gui works mostly, but syncing crashes KPilot...
//
// Revision 1.3  2002/04/05 21:17:01  reinhold
// *** empty log message ***
//
// Revision 1.2  2002/03/23 21:46:43  reinhold
// config  dlg works, but the last changes crash the plugin itself
//
// Revision 1.1  2002/03/09 15:45:48  reinhold
// Moved the files around
//
// Revision 1.1.1.1  2002/03/09 15:38:45  reinhold
// Initial checin of the generic project manager / List manager conduit.
//
//
//
#endif
