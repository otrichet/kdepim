/*
  Copyright (C) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

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
#include "incidencechanger.h"
#include "incidencechanger_p.h"
#include "calendar.h"
#include "calendaradaptor.h"
#include "dndfactory.h"
#include "kcalprefs.h"
#include "mailscheduler.h"
#include "utils.h"

#include <Akonadi/ItemCreateJob>
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/ItemModifyJob>

#include <KMessageBox>

using namespace CalendarSupport;

InvitationHandler::Action actionFromStatus( InvitationHandler::SendResult result )
{
  //enum SendResult {
  //      Canceled,        /**< Sending was canceled by the user, meaning there are
  //                          local changes of which other attendees are not aware. */
  //      FailKeepUpdate,  /**< Sending failed, the changes to the incidence must be kept. */
  //      FailAbortUpdate, /**< Sending failed, the changes to the incidence must be undone. */
  //      NoSendingNeeded, /**< In some cases it is not needed to send an invitation
  //                          (e.g. when we are the only attendee) */
  //      Success
  switch ( result ) {
  case InvitationHandler::ResultCanceled:
    return InvitationHandler::ActionDontSendMessage;
  case InvitationHandler::ResultSuccess:
    return InvitationHandler::ActionSendMessage;
  default:
    return InvitationHandler::ActionAsk;
  }
}

bool IncidenceChanger::Private::myAttendeeStatusChanged( const KCalCore::Incidence::Ptr &newInc,
                                                         const KCalCore::Incidence::Ptr &oldInc )
{
  Q_ASSERT( newInc );
  Q_ASSERT( oldInc );
  KCalCore::Attendee::Ptr oldMe = oldInc->attendeeByMails( KCalPrefs::instance()->allEmails() );
  KCalCore::Attendee::Ptr newMe = newInc->attendeeByMails( KCalPrefs::instance()->allEmails() );

  return oldMe && newMe && ( oldMe->status() != newMe->status() );
}

void IncidenceChanger::Private::queueChange( Change *change )
{
  Q_ASSERT( change );
  // If there's already a change queued we just discard it
  // and send the newer change, which already includes
  // previous modifications
  const Akonadi::Item::Id id = change->newItem.id();
  if ( mQueuedChanges.contains( id ) ) {
    delete mQueuedChanges.take( id );
  }

  mQueuedChanges[id] = change;
}

void IncidenceChanger::Private::cancelChanges( Akonadi::Item::Id id )
{
  delete mQueuedChanges.take( id );
  delete mCurrentChanges.take( id );
}

void IncidenceChanger::Private::performNextChange( Akonadi::Item::Id id )
{
  delete mCurrentChanges.take( id );

  if ( mQueuedChanges.contains( id ) ) {
    performChange( mQueuedChanges.take( id ) );
  }
}

bool IncidenceChanger::Private::performChange( Change *change )
{
  Q_ASSERT( change );
  Akonadi::Item newItem = change->newItem;
  const KCalCore::Incidence::Ptr oldinc =  change->oldInc;
  const KCalCore::Incidence::Ptr newinc = CalendarSupport::incidence( newItem );

  kDebug() << "id="                  << newItem.id()
           << "uid="                 << newinc->uid()
           << "version="             << newItem.revision()
           << "summary="             << newinc->summary()
           << "old summary"          << oldinc->summary()
           << "type="                << int( newinc->type() )
           << "storageCollectionId=" << newItem.storageCollectionId();

  // There's not any job modifying this item, so mCurrentChanges[item.id] can't exist
  Q_ASSERT( !mCurrentChanges.contains( newItem.id() ) );

  // Check if the item was deleted, we already check in changeIncidence() but
  // this change could be already in the queue when the item was deleted
  if ( !mCalendar->incidence( newItem.id() ).isValid() ||
       mDeletedItemIds.contains( newItem.id() ) ) {
    kDebug() << "Incidence deleted";
    // return true, the user doesn't want to see errors because he was too fast
    return true;
  }

  if ( newinc.data() == oldinc.data() ) {
    // Don't do anything
    kDebug() << "Incidence not changed";
    return true;
  } else {

    if ( mLatestRevisionByItemId.contains( newItem.id() ) &&
         mLatestRevisionByItemId[newItem.id()] > newItem.revision() ) {
      /* When a ItemModifyJob ends, the application can still modify the old items if the user
       * is quick because the ETM wasn't updated yet, and we'll get a STORE error, because
       * we are not modifying the latest revision.
       *
       * When a job ends, we keep the new revision in m_latestVersionByItemId
       * so we can update the item's revision
       */
      newItem.setRevision( mLatestRevisionByItemId[newItem.id()] );
    }

    kDebug() << "Changing incidence";
    const bool attendeeStatusChanged = myAttendeeStatusChanged( oldinc, newinc );
    const int revision = newinc->revision();
    newinc->setRevision( revision + 1 );
    // FIXME: Use a generic method for this! Ideally, have an interface class
    //        for group cheduling. Each implementation could then just do what
    //        it wants with the event. If no groupware is used,use the null
    //        pattern...
    if ( KCalPrefs::instance()->mUseGroupwareCommunication ) {
      InvitationHandler handler( mCalendar );
      handler.setDialogParent( change->parent );
      if ( mOperationStatus.contains( change->atomicOperationId ) ) {
        handler.setDefaultAction( actionFromStatus( mOperationStatus.value( change->atomicOperationId ) ) );
      }

      const InvitationHandler::SendResult result = handler.sendIncidenceModifiedMessage( KCalCore::iTIPRequest,
                                                                                          newinc,
                                                                                          attendeeStatusChanged );
      if ( result == InvitationHandler::ResultFailAbortUpdate ) {
        kDebug() << "Sending invitations failed. Reverting changes.";
        if ( newinc->type() == oldinc->type() ) {
          KCalCore::IncidenceBase *i1 = newinc.data();
          KCalCore::IncidenceBase *i2 = oldinc.data();

          *i1 = *i2;
        }
        return false;
      } else if ( change->atomicOperationId ) {
        mOperationStatus.insert( change->atomicOperationId, result );
      }
    }
  }

  // FIXME: if that's a groupware incidence, and I'm not the organizer,
  // send out a mail to the organizer with a counterproposal instead
  // of actually changing the incidence. Then no locking is needed.
  // FIXME: if that's a groupware incidence, and the incidence was
  // never locked, we can't unlock it with endChange().

  mCurrentChanges[newItem.id()] = change;

  // Don't write back remote revision since we can't make sure it is the current one
  // fixes problems with DAV resource
  newItem.setRemoteRevision( QString() );

  Akonadi::ItemModifyJob *job = new Akonadi::ItemModifyJob( newItem );
  connect( job, SIGNAL(result( KJob *)), this, SLOT(changeIncidenceFinished(KJob *)) );
  return true;
}

void IncidenceChanger::Private::changeIncidenceFinished( KJob *j )
{
  // we should probably update the revision number here,or internally in the Event
  // itself when certain things change. need to verify with ical documentation.
  const Akonadi::ItemModifyJob* job = qobject_cast<const Akonadi::ItemModifyJob*>( j );
  Q_ASSERT( job );

  const Akonadi::Item newItem = job->item();

  if ( !mCurrentChanges.contains( newItem.id() ) ) {
    kDebug() << "Item was deleted? Great.";
    cancelChanges( newItem.id() );
    emit incidenceChangeFinished( Akonadi::Item(), newItem, UNKNOWN_MODIFIED, true );
    return;
  }

  const Private::Change *change = mCurrentChanges[newItem.id()];
  const KCalCore::Incidence::Ptr oldInc = change->oldInc;

  Akonadi::Item oldItem;
  oldItem.setPayload<KCalCore::Incidence::Ptr>( oldInc );
  oldItem.setMimeType( oldInc->mimeType() );
  oldItem.setId( newItem.id() );

  if ( job->error() ) {
    kWarning() << "Item modify failed:" << job->errorString();

    const KCalCore::Incidence::Ptr newInc = CalendarSupport::incidence( newItem );
    KMessageBox::sorry( change->parent,
                        i18n( "Unable to save changes for incidence %1 \"%2\": %3",
                              i18n( newInc->typeStr() ),
                              newInc->summary(),
                              job->errorString() ) );
    emit incidenceChangeFinished( oldItem, newItem, change->action, false );
  } else {
    emit incidenceChangeFinished( oldItem, newItem, change->action, true );
  }

  mLatestRevisionByItemId[newItem.id()] = newItem.revision();

  // execute any other modification if it exists
  qRegisterMetaType<Akonadi::Item::Id>( "Akonadi::Item::Id" );
  QMetaObject::invokeMethod( this, "performNextChange",
                             Qt::QueuedConnection,
                             Q_ARG( Akonadi::Item::Id, newItem.id() ) );
}

IncidenceChanger::IncidenceChanger( CalendarSupport::Calendar *cal,
                                    QObject *parent,
                                    Akonadi::Entity::Id defaultCollectionId )
  : QObject( parent ), d( new Private( defaultCollectionId, cal ) )
{
  connect( d, SIGNAL(incidenceChangeFinished(Akonadi::Item,Akonadi::Item,CalendarSupport::IncidenceChanger::WhatChanged,bool)),
           SIGNAL(incidenceChangeFinished(Akonadi::Item,Akonadi::Item,CalendarSupport::IncidenceChanger::WhatChanged,bool)) );
}

IncidenceChanger::~IncidenceChanger()
{
  delete d;
}

bool IncidenceChanger::sendGroupwareMessage( const Akonadi::Item &aitem,
                                             KCalCore::iTIPMethod method,
                                             HowChanged action,
                                             QWidget *parent,
                                             uint atomicOperationId )
{
  const KCalCore::Incidence::Ptr incidence = CalendarSupport::incidence( aitem );
  if ( !incidence ) {
    kDebug() << "Invalid incidence";
    return false;
  }
  if ( KCalPrefs::instance()->thatIsMe( incidence->organizer()->email() ) &&
       incidence->attendeeCount() > 0 &&
       !KCalPrefs::instance()->mUseGroupwareCommunication ) {
    emit schedule( method, aitem );
    return true;
  } else if ( KCalPrefs::instance()->mUseGroupwareCommunication ) {
    InvitationHandler handler( d->mCalendar );
    handler.setDialogParent( parent );
    if ( d->mOperationStatus.contains( atomicOperationId ) ) {
      handler.setDefaultAction( actionFromStatus( d->mOperationStatus.value( atomicOperationId ) ) );
    }
    InvitationHandler::SendResult status;
    switch ( action ) {
      case INCIDENCEADDED:
        status = handler.sendIncidenceCreatedMessage( method, incidence );
        break;
      case INCIDENCEEDITED:
        status = handler.sendIncidenceModifiedMessage( method, incidence, false );
        break;
      case INCIDENCEDELETED:
        status = handler.sendIncidenceDeletedMessage( method, incidence );
      case NOCHANGE:
        break;
    };

    if ( atomicOperationId && action != NOCHANGE ) {
      d->mOperationStatus.insert( atomicOperationId, status );
    }
    return ( status != InvitationHandler::ResultFailAbortUpdate );
  }
  return true;
}

void IncidenceChanger::cancelAttendees( const Akonadi::Item &aitem )
{
  const KCalCore::Incidence::Ptr incidence = CalendarSupport::incidence( aitem );
  Q_ASSERT( incidence );
  if ( KCalPrefs::instance()->mUseGroupwareCommunication ) {
    if ( KMessageBox::questionYesNo(
           0,
           i18n( "Some attendees were removed from the incidence. "
                 "Shall cancel messages be sent to these attendees?" ),
           i18n( "Attendees Removed" ), KGuiItem( i18n( "Send Messages" ) ),
           KGuiItem( i18n( "Do Not Send" ) ) ) == KMessageBox::Yes ) {
      // don't use Akonadi::Groupware::sendICalMessage here, because that asks just
      // a very general question "Other people are involved, send message to
      // them?", which isn't helpful at all in this situation. Afterwards, it
      // would only call the Akonadi::MailScheduler::performTransaction, so do this
      // manually.
      // FIXME: Groupware scheduling should be factored out to it's own class
      //        anyway
      CalendarSupport::MailScheduler scheduler(
        static_cast<CalendarSupport::Calendar*>(d->mCalendar) );
      scheduler.performTransaction( incidence, KCalCore::iTIPCancel );
    }
  }
}

bool IncidenceChanger::deleteIncidence( const Akonadi::Item &aitem,
                                        uint atomicOperationId,
                                        QWidget *parent )
{
  const KCalCore::Incidence::Ptr incidence = CalendarSupport::incidence( aitem );
  if ( !incidence ) {
    kDebug() << "Invalid incidence";
    return false;
  }

  kDebug() << "Deleting incidence " << incidence->summary() << "; id = " << aitem.id();

  if ( !isNotDeleted( aitem.id() ) ) {
    kDebug() << "Item already deleted, skipping and returning true";
    return true;
  }

  if ( !( d->mCalendar->hasDeleteRights( aitem ) ) ) {
    kWarning() << "insufficient rights to delete incidence";
    return false;
  }

  const bool doDelete = sendGroupwareMessage( aitem, KCalCore::iTIPCancel,
                                              INCIDENCEDELETED, parent, atomicOperationId );
  if( !doDelete ) {
    kDebug() << "Groupware says no";
    return false;
  }

  d->mDeletedItemIds.append( aitem.id() );

  emit incidenceToBeDeleted( aitem );
  d->cancelChanges( aitem.id() ); //abort changes to this incidence cause we will just delete it
  Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob( aitem );
  connect( job, SIGNAL(result(KJob *)), this, SLOT(deleteIncidenceFinished(KJob *)) );
  return true;
}

void IncidenceChanger::deleteIncidenceFinished( KJob *j )
{
  // todo, cancel changes?
  kDebug();
  const Akonadi::ItemDeleteJob *job = qobject_cast<const Akonadi::ItemDeleteJob*>( j );
  Q_ASSERT( job );
  const Akonadi::Item::List items = job->deletedItems();
  Q_ASSERT( items.count() == 1 );
  KCalCore::Incidence::Ptr tmp = CalendarSupport::incidence( items.first() );
  Q_ASSERT( tmp );
  if ( job->error() ) {
    KMessageBox::sorry( 0, //PENDING(AKONADI_PORT) set parent
                        i18n( "Unable to delete incidence %1 \"%2\": %3",
                              i18n( tmp->typeStr() ),
                              tmp->summary(),
                              job->errorString() ) );
    d->mDeletedItemIds.removeOne( items.first().id() );
    emit incidenceDeleteFinished( items.first(), false );
    return;
  }
  if ( !KCalPrefs::instance()->thatIsMe( tmp->organizer()->email() ) ) {
    const QStringList myEmails = KCalPrefs::instance()->allEmails();
    bool notifyOrganizer = false;
    for ( QStringList::ConstIterator it = myEmails.begin(); it != myEmails.end(); ++it ) {
      QString email = *it;
      KCalCore::Attendee::Ptr me( tmp->attendeeByMail( email ) );
      if ( me ) {
        if ( me->status() == KCalCore::Attendee::Accepted ||
             me->status() == KCalCore::Attendee::Delegated ) {
          notifyOrganizer = true;
        }
        KCalCore::Attendee::Ptr newMe( new KCalCore::Attendee( *me ) );
        newMe->setStatus( KCalCore::Attendee::Declined );
        tmp->clearAttendees();
        tmp->addAttendee( newMe );
        break;
      }
    }

    if ( KCalPrefs::instance()->useGroupwareCommunication() && notifyOrganizer ) {
      CalendarSupport::MailScheduler scheduler( d->mCalendar );
      scheduler.performTransaction( tmp, KCalCore::iTIPReply );
    }
  }
  d->mLatestRevisionByItemId.remove( items.first().id() );
  emit incidenceDeleteFinished( items.first(), true );
}

bool IncidenceChanger::cutIncidences( const Akonadi::Item::List &list, QWidget *parent )
{
  Akonadi::Item::List::ConstIterator it;
  bool doDelete = true;
  Akonadi::Item::List itemsToCut;
  const uint atomicOperationId = startAtomicOperation();
  for ( it = list.constBegin(); it != list.constEnd(); ++it ) {
    if ( CalendarSupport::hasIncidence( ( *it ) ) ) {
      doDelete = sendGroupwareMessage( *it, KCalCore::iTIPCancel,
                                       INCIDENCEDELETED, parent, atomicOperationId );

      if ( doDelete ) {
        emit incidenceToBeDeleted( *it );
        itemsToCut.append( *it );
      }
    }
   }

  endAtomicOperation( atomicOperationId );

#ifndef QT_NO_DRAGANDDROP
  CalendarAdaptor::Ptr cal( new CalendarAdaptor( d->mCalendar, parent ) );
  CalendarSupport::DndFactory factory( cal, true/*delete calendarAdaptor*/ );

  if ( factory.cutIncidences( itemsToCut ) ) {
#endif
    for ( it = itemsToCut.constBegin(); it != itemsToCut.constEnd(); ++it ) {
      emit incidenceDeleteFinished( *it, true );
    }
    return !itemsToCut.isEmpty();
#ifndef QT_NO_DRAGANDDROP
  } else {
    return false;
  }
#endif
}

bool IncidenceChanger::cutIncidence( const Akonadi::Item &item, QWidget *parent )
{
  Akonadi::Item::List items;
  items.append( item );
  return cutIncidences( items, parent );
}

void IncidenceChanger::setDefaultCollectionId( Akonadi::Entity::Id defaultCollectionId )
{
  d->mDefaultCollectionId = defaultCollectionId;
}

bool IncidenceChanger::changeIncidence( const KCalCore::Incidence::Ptr &oldinc,
                                        const Akonadi::Item &newItem,
                                        WhatChanged action,
                                        QWidget *parent,
                                        uint atomicOperationId )
{
  if ( !CalendarSupport::hasIncidence( newItem ) || !newItem.isValid() ) {
    kDebug() << "Skipping invalid item id=" << newItem.id();
    return false;
  }

  if ( !d->mCalendar->hasChangeRights( newItem ) ) {
    kWarning() << "insufficient rights to change incidence";
    return false;
  }

  if ( !isNotDeleted( newItem.id() ) ) {
    kDebug() << "Skipping change, the item got deleted";
    return false;
  }

  Private::Change *change = new Private::Change();
  change->action = action;
  change->newItem = newItem;
  change->oldInc = oldinc;
  change->parent = parent;
  change->atomicOperationId = atomicOperationId;

  if ( d->mCurrentChanges.contains( newItem.id() ) ) {
    d->queueChange( change );
  } else {
    d->performChange( change );
  }
  return true;
}

bool IncidenceChanger::addIncidence( const KCalCore::Incidence::Ptr &incidence,
                                     QWidget *parent,
                                     Akonadi::Collection &selectedCollection,
                                     int &dialogCode,
                                     uint atomicOperationId )
{
  const Akonadi::Collection defaultCollection = d->mCalendar->collection( d->mDefaultCollectionId );

  const QString incidenceMimeType = incidence->mimeType();
  const bool defaultIsOk = defaultCollection.contentMimeTypes().contains( incidenceMimeType ) &&
                           defaultCollection.rights() & Akonadi::Collection::CanCreateItem;

  if ( d->mDestinationPolicy == ASK_DESTINATION ||
       !defaultCollection.isValid() ||
       !defaultIsOk ) {
    QStringList mimeTypes( incidenceMimeType );
    selectedCollection = CalendarSupport::selectCollection( parent,
                                                            dialogCode,
                                                            mimeTypes,
                                                            defaultCollection );
  } else {
    dialogCode = QDialog::Accepted;
    selectedCollection = defaultCollection;
  }

  if ( selectedCollection.isValid() ) {
    return addIncidence( incidence, selectedCollection, parent, atomicOperationId );
  } else {
    kError() << "Selected collection isn't valid.";
    return false;
  }
}

bool IncidenceChanger::addIncidence( const KCalCore::Incidence::Ptr &incidence,
                                     const Akonadi::Collection &collection,
                                     QWidget *parent,
                                     uint atomicOperationId )
{
  Q_UNUSED( parent );

  if ( !incidence || !collection.isValid() ) {
    kError() << "Incidence or collection isn't valid. collection.isValid() == " << collection.isValid();
    return false;
  }

  if ( !( collection.rights() & Akonadi::Collection::CanCreateItem ) ) {
    kWarning() << "insufficient rights to create incidence";
    return false;
  }

  Akonadi::Item item;
  item.setPayload<KCalCore::Incidence::Ptr>( incidence );

  item.setMimeType( incidence->mimeType() );
  Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( item, collection );

  Private::AddInfo addInfo;
  addInfo.parent = parent;
  addInfo.atomicOperationId = atomicOperationId;

  // so the jobs sees this info
  d->mAddInfoForJob.insert( job, addInfo );

  // The connection needs to be queued to be sure addIncidenceFinished
  // is called after the kjob finished it's eventloop. That's needed
  // because Akonadi::Groupware uses synchronous job->exec() calls.
  connect( job, SIGNAL(result(KJob*)),
           this, SLOT(addIncidenceFinished(KJob*)), Qt::QueuedConnection );
  return true;
}

void IncidenceChanger::addIncidenceFinished( KJob *j )
{
  kDebug();
  const Akonadi::ItemCreateJob *job = qobject_cast<const Akonadi::ItemCreateJob*>( j );
  Q_ASSERT( job );
  KCalCore::Incidence::Ptr incidence = CalendarSupport::incidence( job->item() );

  if  ( job->error() ) {
    KMessageBox::sorry(
      0, //PENDING(AKONADI_PORT) set parent, ideally the one passed in addIncidence...
      i18n( "Unable to save %1 \"%2\": %3",
            i18n( incidence->typeStr() ),
            incidence->summary(),
            job->errorString() ) );
    emit incidenceAddFinished( job->item(), false );
    return;
  }

  if ( KCalPrefs::instance()->useGroupwareCommunication() ) {
    Q_ASSERT( incidence );
    InvitationHandler handler( d->mCalendar );
    //handler.setDialogParent( 0 ); // PENDING(AKONADI_PORT) set parent, ideally the one passed in addIncidence...
    const InvitationHandler::SendResult status =
        handler.sendIncidenceCreatedMessage( KCalCore::iTIPRequest, incidence );

    if ( status == InvitationHandler::ResultFailAbortUpdate ) {
      // TODO: At this point we'd need to delete the incidence again.
      kError() << "Sending invitations failed, but did not delete the incidence";
    }

    const uint atomicOperationId = d->mAddInfoForJob[j].atomicOperationId;
    if ( atomicOperationId ) {
      d->mOperationStatus.insert( atomicOperationId, status );
    }
  }
  emit incidenceAddFinished( job->item(), true );
}

void IncidenceChanger::setDestinationPolicy( DestinationPolicy destinationPolicy )
{
  d->mDestinationPolicy = destinationPolicy;
}

IncidenceChanger::DestinationPolicy IncidenceChanger::destinationPolicy() const
{
  return d->mDestinationPolicy;
}

bool IncidenceChanger::isNotDeleted( Akonadi::Item::Id id ) const
{
  if ( d->mCalendar->incidence( id ).isValid() ) {
    // it's inside the calendar, but maybe it's being deleted by a job or was
    // deleted but the ETM doesn't know yet
    return !d->mDeletedItemIds.contains( id );
  } else {
    // not inside the calendar, i don't know it
    return false;
  }
}

void IncidenceChanger::setCalendar( CalendarSupport::Calendar *calendar )
{
  d->mCalendar = calendar;
}

uint IncidenceChanger::startAtomicOperation()
{
  static unsigned int latestAtomicOperationId = 0;

  return ++latestAtomicOperationId;
}

void IncidenceChanger::endAtomicOperation( uint atomicOperationId )
{
  d->mOperationStatus.remove( atomicOperationId );
}

bool IncidenceChanger::changeInProgress( Akonadi::Item::Id id )
{
  return d->mCurrentChanges.contains( id );
}
