
#include <calformat.h>

#include "eventsyncee.h"


using namespace KSync;

SyncEntry* EventSyncEntry::clone()  {
    return new EventSyncEntry( *this );
}

EventSyncee::EventSyncee()
    : SyncTemplate<EventSyncEntry>(DtEnd+1) {

};
QString EventSyncee::type() const{
    return QString::fromLatin1( "EventSyncee" );
}
Syncee* EventSyncee::clone() {
    EventSyncee* temp = new EventSyncee();
    temp->setSyncMode( syncMode() );
    temp->setFirstSync( firstSync() );
    EventSyncEntry* entry;
    for ( entry = mList.first(); entry != 0; entry = mList.next() ) {
        temp->addEntry( entry->clone() );
    }
    return temp;
}
QString EventSyncee::newId() const {
    return KCal::CalFormat::createUniqueId();
}
