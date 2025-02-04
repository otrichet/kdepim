/*******************************************************************************
**
** Filename   : util
** Created on : 03 April, 2005
** Copyright  : (c) 2005 Till Adam
** Email      : <adam@kde.org>
**
*******************************************************************************/

/*******************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   It is distributed in the hope that it will be useful, but
**   WITHOUT ANY WARRANTY; without even the implied warranty of
**   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**   General Public License for more details.
**
**   You should have received a copy of the GNU General Public License
**   along with this program; if not, write to the Free Software
**   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
**   In addition, as a special exception, the copyright holders give
**   permission to link the code of this program with any edition of
**   the Qt library by Trolltech AS, Norway (or with modified versions
**   of Qt that use the same license as Qt), and distribute linked
**   combinations including the two.  You must obey the GNU General
**   Public License in all respects for all of the code used other than
**   Qt.  If you modify this file, you may extend this exception to
**   your version of the file, but you are not obligated to do so.  If
**   you do not wish to do so, delete this exception statement from
**   your version.
**
*******************************************************************************/


#include "util.h"
#include "imapsettings.h"
#include "settings.h"

#include <akonadi/agentmanager.h>
#include <kimap/loginjob.h>
#include <kmime/kmime_message.h>
#include <mailtransport/transport.h>

using namespace KSieveUi;

KUrl KSieveUi::Util::findSieveUrlForAccount( const QString &identifier )
{
  QScopedPointer<OrgKdeAkonadiImapSettingsInterface> interface( new OrgKdeAkonadiImapSettingsInterface( "org.freedesktop.Akonadi.Resource." + identifier,
                                                                                                        "/Settings", QDBusConnection::sessionBus() ) );
  if ( !interface->sieveSupport() )
    return KUrl();

  if ( interface->sieveReuseConfig() ) {
    // assemble Sieve url from the settings of the account:
    KUrl u;
    u.setProtocol( "sieve" );
    QString server;
    QDBusReply<QString> reply = interface->imapServer();
    if ( reply.isValid() ) {
      server = reply;
      server = server.section( ':', 0, 0 );
    } else {
      return KUrl();
    }
    u.setHost( server );
    u.setUser( interface->userName() );

    QDBusInterface resourceSettings( QLatin1String( "org.freedesktop.Akonadi.Resource." ) + identifier, "/Settings", "org.kde.Akonadi.Imap.Wallet" );

    QString pwd;
    QDBusReply<QString> replyPass = resourceSettings.call( "password" );
    if ( replyPass.isValid() ) {
      pwd = replyPass;
    }
    u.setPass( pwd );
    u.setPort( interface->sievePort() );
    QString authStr;
    switch( interface->authentication() ) {
    case MailTransport::Transport::EnumAuthenticationType::CLEAR:
    case MailTransport::Transport::EnumAuthenticationType::PLAIN:
      authStr = "PLAIN";
      break;
    case MailTransport::Transport::EnumAuthenticationType::LOGIN:
      authStr = "LOGIN";
      break;
    case MailTransport::Transport::EnumAuthenticationType::CRAM_MD5:
      authStr = "CRAM-MD5";
      break;
    case MailTransport::Transport::EnumAuthenticationType::DIGEST_MD5:
      authStr = "DIGEST-MD5";
      break;
    case MailTransport::Transport::EnumAuthenticationType::GSSAPI:
      authStr = "GSSAPI";
      break;
    case MailTransport::Transport::EnumAuthenticationType::ANONYMOUS:
      authStr = "ANONYMOUS";
      break;
    default:
      authStr = "PLAIN";
      break;
    }
    u.addQueryItem( "x-mech", authStr );
    if ( interface->safety() == ( int )( KIMAP::LoginJob::Unencrypted ))
      u.addQueryItem( "x-allow-unencrypted", "true" );
    u.setFileName( interface->sieveVacationFilename() );
    return u;
  } else {
    KUrl u( interface->sieveAlternateUrl() );
    if ( u.protocol().toLower() == "sieve" && ( interface->safety() == ( int )( KIMAP::LoginJob::Unencrypted ) ) && u.queryItem("x-allow-unencrypted").isEmpty() )
      u.addQueryItem( "x-allow-unencrypted", "true" );
    u.setFileName( interface->sieveVacationFilename() );
    return u;
  }
}

#define IMAP_RESOURCE_IDENTIFIER "akonadi_imap_resource"

Akonadi::AgentInstance::List KSieveUi::Util::imapAgentInstances()
{
  Akonadi::AgentInstance::List relevantInstances;
  foreach ( const Akonadi::AgentInstance &instance, Akonadi::AgentManager::self()->instances() ) {
    if ( instance.type().mimeTypes().contains( KMime::Message::mimeType() ) &&
         instance.type().capabilities().contains( "Resource" ) &&
         !instance.type().capabilities().contains( "Virtual" ) ) {

      if ( instance.identifier().contains( IMAP_RESOURCE_IDENTIFIER ) )
        relevantInstances << instance;
    }
  }

  return relevantInstances;
}

bool KSieveUi::Util::checkOutOfOfficeOnStartup()
{
  return Settings::self()->checkOutOfOfficeOnStartup();
}

bool KSieveUi::Util::allowOutOfOfficeSettings()
{
  return Settings::self()->allowOutOfOfficeSettings();
}
