/***************************************************************************
        konsolekalendarexports.cpp  -  description
           -------------------
    begin                : Sun May 25 2003
    copyright            : (C) 2003 by Tuukka Pasanen
    copyright            : (C) 2003 by Allen Winter
    email                : illuusio@mailcity.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdlib.h>
#include <iostream>

#include <qdatetime.h>

#include <kdebug.h>
#include <klocale.h>

#include <libkcal/calendarlocal.h>
#include <libkcal/calendar.h>
#include <libkcal/event.h>
#include <libkcal/htmlexport.h>


#include "konsolekalendarexports.h"

using namespace KCal;
using namespace std;

KonsoleKalendarExports::KonsoleKalendarExports( KonsoleKalendarVariables *variables )
{
  m_variables = variables;
  m_firstEntry = true;
}


KonsoleKalendarExports::~KonsoleKalendarExports()
{
}

bool KonsoleKalendarExports::exportAsTxt( QTextStream *ts, Event *event ){

  if( m_firstEntry == true ||
      m_lastDate.day() != event->dtStart().date().day() ||
      m_lastDate.month() != event->dtStart().date().month() ||
      m_lastDate.year() != event->dtStart().date().year() ){


    m_firstEntry=false;
    int len = event->dtStartStr().length();
    QString date = event->dtStartStr();
    date.truncate( len - 5 );
    *ts << I18N_NOOP("Date:") << "\t" <<  date << endl;
    m_lastDate = event->dtStart().date();

  }

  if ( !event->doesFloat() ) {
    *ts << "\t";
    *ts <<  event->dtStartStr().remove(0, (event->dtStartStr().find(' ', 0, false) + 1) );
    *ts << " - ";
    *ts << event->dtEndStr().remove(0, (event->dtEndStr().find(' ', 0, false) + 1) );
  }


  *ts << endl << I18N_NOOP("Summary:") << endl;
  *ts << "\t" << event->summary() << endl;
  *ts << I18N_NOOP("Description:") << endl;
  if( !event->description().isEmpty() ) {
    *ts << "\t" << event->description() << endl;
  } else {
    *ts << "\t" << I18N_NOOP("(no description available)") << endl;
  }
  *ts << I18N_NOOP("UID:") << endl;
  *ts << "\t" << event->uid() << endl;
  *ts << "----------------------------------" << endl;

  return true;
}

bool KonsoleKalendarExports::exportAsCSV( QTextStream *ts, Event *event ){

// startdate,starttime,enddate,endtime,summary,description,UID

  QString delim = ",";  //one day maybe the delim character can be an option??

  if ( !event->doesFloat() ) {
    *ts <<          event->dtStart().date().toString("yyyy-M-d");
    *ts << delim << event->dtStart().time().toString("hh:mm");
    *ts << delim << event->dtEnd().date().toString("yyyy-M-d");
    *ts << delim << event->dtEnd().time().toString("hh:mm");
  } else {
    *ts << ",,,";
  }

  QString rdelim = "\\" + delim;
  *ts << delim << event->summary().replace(delim,rdelim);
  *ts << delim << event->description().replace(delim,rdelim);
  *ts << delim << event->uid();
  *ts << endl;

  return true;
}
