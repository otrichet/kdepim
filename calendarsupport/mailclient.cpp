/*
  Copyright (c) 1998 Barry D Benowitz <b.benowitz@telesciences.com>
  Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2009 Allen Winter <winter@kde.org>

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

#include "mailclient.h"

#include "config-enterprise.h"
#include "kdepim-version.h"

#include <Akonadi/Collection>

#include <KCalCore/Attendee>
#include <KCalCore/Incidence>
#include <KCalCore/IncidenceBase>

#include <KCalUtils/IncidenceFormatter>

#include <KMime/Message>

#include <KPIMIdentities/Identity>

#include <KPIMUtils/Email>

#include <Mailtransport/MessageQueueJob>
#include <Mailtransport/Transport>
#include <Mailtransport/TransportManager>

#include <KDebug>
#include <KLocale>
#include <KProtocolManager>
#include <KSystemTimeZone>

using namespace CalendarSupport;

MailClient::MailClient() : QObject()
{
}

MailClient::~MailClient()
{
}

bool MailClient::mailAttendees( const KCalCore::IncidenceBase::Ptr &incidence,
                                const KPIMIdentities::Identity &identity,
                                bool bccMe, const QString &attachment,
                                const QString &mailTransport )
{
  KCalCore::Attendee::List attendees = incidence->attendees();
  if ( attendees.count() == 0 ) {
    return false;
  }

  const QString from = incidence->organizer()->fullName();
  const QString organizerEmail = incidence->organizer()->email();

  QStringList toList;
  QStringList ccList;
  for ( int i=0; i<attendees.count(); ++i ) {
    KCalCore::Attendee::Ptr a = attendees.at(i);

    const QString email = a->email();
    if ( email.isEmpty() ) {
      continue;
    }

    // In case we (as one of our identities) are the organizer we are sending
    // this mail. We could also have added ourselves as an attendee, in which
    // case we don't want to send ourselves a notification mail.
    if ( organizerEmail == email ) {
      continue;
    }

    // Build a nice address for this attendee including the CN.
    QString tname, temail;
    const QString username = KPIMUtils::quoteNameIfNecessary( a->name() );
    // ignore the return value from extractEmailAddressAndName() because
    // it will always be false since tusername does not contain "@domain".
    KPIMUtils::extractEmailAddressAndName( username, temail, tname );
    tname += QLatin1String( " <" ) + email + QLatin1Char( '>' );

    // Optional Participants and Non-Participants are copied on the email
    if ( a->role() == KCalCore::Attendee::OptParticipant ||
         a->role() == KCalCore::Attendee::NonParticipant ) {
      ccList << tname;
    } else {
      toList << tname;
    }
  }
  if( toList.count() == 0 && ccList.count() == 0 ) {
    // Not really to be called a groupware meeting, eh
    return false;
  }
  QString to;
  if ( toList.count() > 0 ) {
    to = toList.join( QLatin1String( ", " ) );
  }
  QString cc;
  if ( ccList.count() > 0 ) {
    cc = ccList.join( QLatin1String( ", " ) );
  }

  QString subject;
  if ( incidence->type() != KCalCore::Incidence::TypeFreeBusy ) {
    KCalCore::Incidence::Ptr inc = incidence.staticCast<KCalCore::Incidence>();
    subject = inc->summary();
  } else {
    subject = i18n( "Free Busy Object" );
  }

  QString body = KCalUtils::IncidenceFormatter::mailBodyStr( incidence, KSystemTimeZones::local() );

  return send( identity, from, to, cc, subject, body, false,
               bccMe, attachment, mailTransport );
}

bool MailClient::mailOrganizer( const KCalCore::IncidenceBase::Ptr &incidence,
                                const KPIMIdentities::Identity &identity,
                                const QString &from, bool bccMe,
                                const QString &attachment,
                                const QString &sub, const QString &mailTransport )
{
  QString to = incidence->organizer()->fullName();

  QString subject = sub;

  if ( incidence->type() != KCalCore::Incidence::TypeFreeBusy ) {
    KCalCore::Incidence::Ptr inc = incidence.staticCast<KCalCore::Incidence>();
    if ( subject.isEmpty() ) {
      subject = inc->summary();
    }
  } else {
    subject = i18n( "Free Busy Message" );
  }

  QString body = KCalUtils::IncidenceFormatter::mailBodyStr( incidence, KSystemTimeZones::local() );

  return send( identity, from, to, QString(), subject, body, false,
               bccMe, attachment, mailTransport );
}

bool MailClient::mailTo( const KCalCore::IncidenceBase::Ptr &incidence,
                         const KPIMIdentities::Identity &identity,
                         const QString &from, bool bccMe,
                         const QString &recipients, const QString &attachment,
                         const QString &mailTransport )
{
  QString subject;

  if ( incidence->type() != KCalCore::Incidence::TypeFreeBusy ) {
    KCalCore::Incidence::Ptr inc = incidence.staticCast<KCalCore::Incidence>() ;
    subject = inc->summary();
  } else {
    subject = i18n( "Free Busy Message" );
  }
  QString body = KCalUtils::IncidenceFormatter::mailBodyStr( incidence, KSystemTimeZones::local() );

  return send( identity, from, recipients, QString(), subject, body, false,
               bccMe, attachment, mailTransport );
}

bool MailClient::send( const KPIMIdentities::Identity &identity,
                       const QString &from, const QString &_to,
                       const QString &cc, const QString &subject,
                       const QString &body, bool hidden, bool bccMe,
                       const QString &attachment, const QString &mailTransport )
{
  Q_UNUSED( identity );
  Q_UNUSED( hidden );

  // We must have a recipients list for most MUAs. Thus, if the 'to' list
  // is empty simply use the 'from' address as the recipient.
  QString to = _to;
  if ( to.isEmpty() ) {
    to = from;
  }
  kDebug() << "\nFrom:" << from
           << "\nTo:" << to
           << "\nCC:" << cc
           << "\nSubject:" << subject << "\nBody: \n" << body
           << "\nAttachment:\n" << attachment
           << "\nmailTransport: " << mailTransport;

  QTime timer;
  timer.start();

  MailTransport::Transport *transport =
    MailTransport::TransportManager::self()->transportByName( mailTransport );

  if ( !transport ) {
    transport =
      MailTransport::TransportManager::self()->transportByName(
        MailTransport::TransportManager::self()->defaultTransportName() );
  }

  if ( !transport ) {
    // TODO: we need better error handling. Currently korganizer just says "Error sending invitation".
    // Using a boolean for errors isn't granular enough.
    kWarning() << "Error fetching transport; mailTransport"
               << mailTransport << MailTransport::TransportManager::self()->defaultTransportName();
    return false;
  }

  const int transportId = transport->id();

  // gather config values
  KConfig config( "mailviewerrc" );

  KConfigGroup configGroup( &config, QLatin1String( "Invitations" ) );
  const bool outlookConformInvitation = configGroup.readEntry( "LegacyBodyInvites",
#ifdef KDEPIM_ENTERPRISE_BUILD
                                                               true
#else
                                                               false
#endif
                                                             );

  // Now build the message we like to send. The message KMime::Message::Ptr instance
  // will be the root message that has 2 additional message. The body itself and
  // the attached cal.ics calendar file.
  KMime::Message::Ptr message = KMime::Message::Ptr( new KMime::Message );
  message->contentTransferEncoding()->clear();  // 7Bit, decoded.

  // Set the headers
  message->userAgent()->fromUnicodeString(
    KProtocolManager::userAgentForApplication(
      QLatin1String( "KOrganizer" ), QLatin1String( KDEPIM_VERSION ) ), "utf-8" );
  message->from()->fromUnicodeString( from, "utf-8" );
  message->to()->fromUnicodeString( to, "utf-8" );
  message->cc()->fromUnicodeString( cc, "utf-8" );
  if( bccMe ) {
    message->bcc()->fromUnicodeString( from, "utf-8" ); //from==me, right?
  }
  message->date()->setDateTime( KDateTime::currentLocalDateTime() );
  message->subject()->fromUnicodeString( subject, "utf-8" );

  if ( outlookConformInvitation ) {
    message->contentType()->setMimeType( "text/calendar" );
    message->contentType()->setCharset( "utf-8" );
    message->contentType()->setName( QLatin1String( "cal.ics" ), "utf-8" );
    message->contentType()->setParameter( QLatin1String( "method" ), QLatin1String( "request" ) );

    if ( !attachment.isEmpty() ) {
      KMime::Headers::ContentDisposition *disposition =
        new KMime::Headers::ContentDisposition( message.get() );
      disposition->setDisposition( KMime::Headers::CDinline );
      message->setHeader( disposition );
      message->contentTransferEncoding()->setEncoding( KMime::Headers::CEquPr );
      message->setBody( attachment.toUtf8() );
    }
  } else {
    // We need to set following 4 lines by hand else KMime::Content::addContent
    // will create a new Content instance for us to attach the main message
    // what we don't need cause we already have the main message instance where
    // 2 additional messages are attached.
    KMime::Headers::ContentType *ct = message->contentType();
    ct->setMimeType( "multipart/mixed" );
    ct->setBoundary( KMime::multiPartBoundary() );
    ct->setCategory( KMime::Headers::CCcontainer );

    // Set the first multipart, the body message.
    KMime::Content *bodyMessage = new KMime::Content;
    KMime::Headers::ContentDisposition *bodyDisposition =
      new KMime::Headers::ContentDisposition( bodyMessage );
    bodyDisposition->setDisposition( KMime::Headers::CDinline );
    bodyMessage->contentType()->setMimeType( "text/plain" );
    bodyMessage->contentType()->setCharset( "utf-8" );
    bodyMessage->contentTransferEncoding()->setEncoding( KMime::Headers::CEquPr );
    bodyMessage->setBody( body.toUtf8() );
    message->addContent( bodyMessage );

    // Set the sedcond multipart, the attachment.
    if ( !attachment.isEmpty() ) {
      KMime::Content *attachMessage = new KMime::Content;
      KMime::Headers::ContentDisposition *attachDisposition =
        new KMime::Headers::ContentDisposition( attachMessage );
      attachDisposition->setDisposition( KMime::Headers::CDattachment );
      attachMessage->contentType()->setMimeType( "text/calendar" );
      attachMessage->contentType()->setCharset( "utf-8" );
      attachMessage->contentType()->setName( QLatin1String( "cal.ics" ), "utf-8" );
      attachMessage->contentType()->setParameter( QLatin1String( "method" ), QLatin1String( "request" ) );
      attachMessage->setHeader( attachDisposition );
      attachMessage->contentTransferEncoding()->setEncoding( KMime::Headers::CEquPr );
      attachMessage->setBody( attachment.toUtf8() );
      message->addContent( attachMessage );
    }
  }

  // Job done, attach the both multiparts and assemble the message.
  message->assemble();

  // Put the newly created item in the MessageQueueJob.
  MailTransport::MessageQueueJob *qjob = new MailTransport::MessageQueueJob( this );
  qjob->transportAttribute().setTransportId( transportId );
  qjob->sentBehaviourAttribute().setSentBehaviour(
           MailTransport::SentBehaviourAttribute::MoveToDefaultSentCollection );
  qjob->sentBehaviourAttribute().setMoveToCollection( Akonadi::Collection( -1 ) );
  qjob->addressAttribute().setFrom( from );
  qjob->addressAttribute().setTo( KPIMUtils::splitAddressList( to ) );
  qjob->addressAttribute().setCc( KPIMUtils::splitAddressList( cc ) );
  if( bccMe ) {
    qjob->addressAttribute().setBcc( KPIMUtils::splitAddressList( from ) );
  }
  qjob->setMessage( message );
  if( ! qjob->exec() ) {
    kWarning() << "Error queuing message in outbox:" << qjob->errorText();
    return false;
  }

  // Everything done successful now.
  kDebug() << "Send mail finished. Time elapsed in ms:" << timer.elapsed();
  return true;
}

