#ifndef _ABBROWSER_FACTORY_H
#define _ABBROWSER_FACTORY_H
/* abbrowser-factory.h                       KPilot
**
** Copyright (C) 2001 by Dan Pilone
**
** This file defines the factory for the abbrowser-conduit plugin.
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

#include <klibloader.h>

class KInstance;
class KAboutData;

class AbbrowserConduitFactory : public KLibFactory
{
Q_OBJECT

public:
	AbbrowserConduitFactory(QObject * = 0L,const char * = 0L);
	virtual ~AbbrowserConduitFactory();

	static KAboutData *about() { return fAbout; } ;
	static const char *group() { return fGroup; } ;
	static const char *smartMerge() { return fSmartMerge; } ;
	static const char *conflictResolution() { return fResolution; } ;
	static const char *archiveDeletedRecs() { return fArchive; };
	static const char *streetType() { return fStreetType; } ;
	static const char *faxType() { return fFaxType; } ;
	static const char *syncMode() { return fSyncMode;};
	static const char *firstSync() { return fFirstSync; } ;
	static const char *fullSyncOnPCChange() {return fFullSyncOnPCChange; } ;
	static const char *otherField() { return fOtherField; } ;
	
protected:
	virtual QObject* createObject( QObject* parent = 0,
		const char* name = 0,
		const char* classname = "QObject",
		const QStringList &args = QStringList() );
private:
	KInstance *fInstance;
	static KAboutData *fAbout;
	static const char *fGroup;

	// KConfig entry keys.
	//
	//
	static const char *fSmartMerge,
		*fResolution,
		*fArchive,
		*fStreetType,
		*fFaxType,
		*fSyncMode, 
		*fFirstSync,
		*fOtherField,
		*fFullSyncOnPCChange;
} ;

extern "C"
{

void *init_libaddressconduit();

} ;

#endif
