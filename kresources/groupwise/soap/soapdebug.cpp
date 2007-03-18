#include "groupwiseserver.h"

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kdebug.h>
#include <ktemporaryfile.h>

#include <kabcresourcecached.h>

#include <kcal/icalformat.h>
#include <kcal/resourcelocal.h>
#include <kcal/calendarresources.h>

#include <iostream>

namespace KABC {

class ResourceMemory : public ResourceCached
{
  public:
    ResourceMemory() : ResourceCached() {}
    
    Ticket *requestSaveTicket() { return 0; }
    bool load() { return true; }
    bool save( Ticket * ) { return true; }
    void releaseSaveTicket( Ticket * ) {}
};

}

static const KCmdLineOptions options[] =
{
  { "s", 0, 0 },
  { "server <hostname>", I18N_NOOP("Server"), 0 },
  { "u", 0, 0 },
  { "user <string>", I18N_NOOP("User"), 0 },
  { "p", 0, 0 },
  { "password <string>", I18N_NOOP("Password"), 0 },
  { "f", 0, 0 },
  { "freebusy-user <string>", I18N_NOOP("Free/Busy user name"), 0 },
  { "addressbook-id <string>", I18N_NOOP("Addressbook identifier"), 0 },
  KCmdLineLastOption
};

int main( int argc, char **argv )
{
  KAboutData aboutData( "soapdebug", I18N_NOOP("Groupwise Soap Debug"), "0.1" );
  aboutData.addAuthor( "Cornelius Schumacher", 0, "schumacher@kde.org" );

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication app;

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  QString user = args->getOption( "user" );
  QString pass = args->getOption( "password" );
  QString url = args->getOption( "server" );

#if 1
  if ( user.isEmpty() ) {
    kError() << "Need user." << endl;
    return 1; 
  }
  if ( pass.isEmpty() ) {
    kError() << "Need password." << endl;
    return 1; 
  }
  if ( url.isEmpty() ) {
    kError() << "Need server." << endl;
    return 1; 
  }
#endif

  GroupwiseServer server( url, user, pass, 0 );

#if 1
  if ( !server.login() ) {
    kError() << "Unable to login to server " << url << endl;
    return 1;
  }
#endif

#if 0
  server.dumpData();
#endif

#if 0
  server.getCategoryList();
#endif

#if 0
  server.dumpFolderList();
#endif

#if 0
  QString fbUser = args->getOption( "freebusy-user" );
  if ( fbUser.isEmpty() ) {
    kError() << "Need user for which the freebusy data should be retrieved."
              << endl;
  } else {
    KCal::FreeBusy *fb = new KCal::FreeBusy;

    server.readFreeBusy( "user1",
      QDate( 2004, 9, 1 ), QDate( 2004, 10, 31 ), fb );
  }
#endif

#if 0
  KTemporaryFile temp;
  temp.setautoRemove(false);
  temp.open();
  KCal::ResourceLocal resource( temp.fileName() );
  resource.setActive( true );
  KCal::CalendarResources calendar;
  calendar.resourceManager()->add( &resource );
  kDebug() << "Login" << endl;

  if ( !server.login() ) {
    kDebug() << "Unable to login." << endl;
  } else {
    kDebug() << "Read calendar" << endl;
    if ( !server.readCalendarSynchronous( &resource ) ) {
      kDebug() << "Unable to read calendar data." << endl;
    }
    kDebug() << "Logout" << endl;
    server.logout();
  }
  KCal::ICalFormat format;

  QString ical = format.toString( &calendar );

  kDebug() << "ICALENDAR: " << ical << endl;
#endif

#if 0
  QString id = args->getOption( "addressbook-id" );

  kDebug() << "ADDRESSBOOK ID: " << id << endl;

  QStringList ids;
  ids.append( id );

  KABC::ResourceMemory resource;

  kDebug() << "Login" << endl;
  if ( !server.login() ) {
    kError() << "Unable to login." << endl;
  } else {
    kDebug() << "Read Addressbook" << endl;
    if ( !server.readAddressBooksSynchronous( ids, &resource ) ) {
      kError() << "Unable to read addressbook data." << endl;
    }
    kDebug() << "Logout" << endl;
    server.logout();
  }

  KABC::Addressee::List addressees;
  KABC::Resource::Iterator it2;
  for( it2 = resource.begin(); it2 != resource.end(); ++it2 ) {
    kDebug() << "ADDRESSEE: " << (*it2).fullEmail() << endl;
    addressees.append( *it2 );
  }
#endif

#if 0    
  server.getDelta();
#endif

  server.logout();

  return 0;
}

