#include "loging.h"
#include "task.h"
#include "preferences.h"
#include <qdatetime.h>
#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qregexp.h>

#include <kdebug.h>

#include <iostream>

#define LOG_START				1
#define LOG_STOP				2
#define LOG_NEW_TOTAL_TIME		3
#define LOG_NEW_SESSION_TIME	4

Loging *Loging::_instance = 0;

Loging::Loging()
{
	_preferences = Preferences::instance();
}

void Loging::start( Task * task)
{
	log(task, LOG_START);
}

void Loging::stop( Task * task)
{
	log(task, LOG_STOP);
}

// when time is reset...
void Loging::newTotalTime( Task * task, long minutes)
{
	log(task, LOG_NEW_TOTAL_TIME, minutes);
}
void Loging::newSessionTime( Task * task, long minutes)
{
	log(task, LOG_NEW_SESSION_TIME, minutes);
}

void Loging::log( Task * task, short type, long minutes)
{

	if(_preferences->timeLoging()) {
		QFile f(_preferences->timeLog());

		if ( f.open( IO_WriteOnly | IO_Append) ) {
			QTextStream out( &f );        // use a text stream

			if( type == LOG_START) {
				out << "<starting         ";
			} else if( type == LOG_STOP ) {
				out << "<stopping         ";
			} else if( type == LOG_NEW_TOTAL_TIME) {
				out << "<new_total_time   ";
			} else if( type == LOG_NEW_SESSION_TIME) {
				out << "<new_session_time ";
			} else {
				kdError() << "Programming error!" << endl;
			}

			out << "task=\"" << constructTaskName(task) << "\" "
			    << "date=\"" << QDateTime::currentDateTime().toString() << "\" ";
			  
			if ( type == LOG_NEW_TOTAL_TIME || type == LOG_NEW_SESSION_TIME) {
				out << "new_total=\"" << minutes  << "\" ";
			}
			
			out << "/>\n";

			f.close();
		} else {
			std::cerr << "Couldn't write to time-log file";
		}
	}
}

Loging *Loging::instance()
{
  if (_instance == 0) {
    _instance = new Loging();
  }
  return _instance;
}

QString Loging::constructTaskName(Task *task)
{
	QListViewItem *item = task;
	
	QString name = escapeXML(task->name());
	
	while( ( item = item->parent() ) )
	{
		name = escapeXML(((Task *)item)->name()) + name.prepend('/');
	}

	return name;
}

// why the hell do I need to do this?!?
#define QS(c) QString::fromLatin1(c)

QString Loging::escapeXML( QString string)
{
	QString result = QString(string);
	result.replace( QRegExp(QS("&")),  QS("&amp;")  );
	result.replace( QRegExp(QS("<")),  QS("&lt;")   );
	result.replace( QRegExp(QS(">")),  QS("&gt;")   );
	result.replace( QRegExp(QS("'")),  QS("&apos;") );
	result.replace( QRegExp(QS("\"")), QS("&quot;") );
	// protect also our task-separator
	result.replace( QRegExp(QS("/")),  QS("&slash;") );

	return result;
}

Loging::~Loging() {
}
