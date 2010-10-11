/*
  Copyright 2010 Olivier Trichet <nive@nivalis.org>

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include "akobackit/nntpaccount.h"

#include "akobackit/constant.h"
#include "knglobals.h"

#include <Akonadi/AgentManager>
#include <kio/ioslave_defaults.h>
#include <KPIMIdentities/Identity>
#include <KPIMIdentities/IdentityManager>
#include <QDBusInterface>
#include <QDBusReply>


namespace KNode {

NntpAccount::NntpAccount( const Akonadi::AgentInstance &agent )
  : SettingsContainerInterface(),
    mAgent( agent ),
    mProperties(),
    mIdentityUoid( -1 )
{
  Q_ASSERT( mAgent.identifier().startsWith( Akobackit::NNTP_RESOURCE_AGENTTYPE )
             || !mAgent.isValid() );
}

NntpAccount::~NntpAccount()
{
}



Akonadi::AgentInstance NntpAccount::agent()
{
  return mAgent;
}



template< class T >
QVariant NntpAccount::loadProperty( const QString &key )
{
  if ( !mProperties.contains( key ) && mAgent.isValid() ) {
    QDBusInterface remoteAgent( QString( "org.freedesktop.Akonadi.Agent.%1" ).arg( mAgent.identifier() ),
                                "/Settings",
                                "org.kde.Akonadi.NNTP.Settings" );
    QDBusReply<T> reply = remoteAgent.call( key );
    Q_ASSERT_X( reply.isValid(), "NntpAccount::loadProperty", reply.error().message().toUtf8() );
    if ( reply.isValid() ) {
      mProperties.insert( key, QVariant( reply.value() ) );
    }
  }

  return mProperties.value( key );
}


void NntpAccount::save()
{
  if ( mProperties.contains( "name" ) ) {
    const QVariant name = mProperties.take( "name" );
    mAgent.setName( name.toString() );
  }


  QDBusInterface remoteAgent( QString( "org.freedesktop.Akonadi.Agent.%1" ).arg( mAgent.identifier() ),
                              "/Settings",
                              "org.kde.Akonadi.NNTP.Settings" );

  QHash<QString,QVariant>::ConstIterator it = mProperties.constBegin();
  while ( it != mProperties.constEnd() ) {
    const QString &name = it.key();
    const QString method = QLatin1String( "set" ) + name.at( 0 ).toUpper() + name.mid( 1 );
    QDBusReply<void> reply = remoteAgent.call( method, it.value() );
    kDebug() << "Save" << name << " by calling " << method << "with value" << it.value();
    Q_ASSERT_X( reply.isValid(), "NntpAccount::save", reply.error().message().toUtf8() );
    ++it;
  }
}



// ---- Properties getter and setter ----

QString NntpAccount::name()
{
  if ( mProperties.contains( "name" ) ) {
    QVariant v = mProperties.value( "name" );
    return v.toString();
  }
  return mAgent.name();
}
void NntpAccount::setName( const QString &name )
{
  mProperties.insert( "name", name );
}



QString NntpAccount::server()
{
  QVariant v = loadProperty<QString>( "server" );
  return v.toString();
}
void NntpAccount::setServer( const QString &server )
{
  mProperties.insert( "server", server );
}

uint NntpAccount::port()
{
  QVariant v = loadProperty<uint>( "port" );
  bool ok = false;
  uint port = v.toUInt( &ok );
  return ( ok ? port : DEFAULT_NNTP_PORT );
}
void NntpAccount::setPort( uint port )
{
  mProperties.insert( "port", port );
}

NntpAccount::Encryption NntpAccount::encryption()
{
  QVariant v = loadProperty<int>( "encryption" );
  bool ok = false;
  int encryption = v.toInt( &ok );
  return ( ok ? static_cast<Encryption>( encryption ) : None );
}
void NntpAccount::setEncryption( NntpAccount::Encryption encryption )
{
  mProperties.insert( "encryption", static_cast<int>( encryption ) );
}




bool NntpAccount::fetchDescriptions()
{
  QVariant v = loadProperty<bool>( "fetchDescriptions" );
  return v.toBool();
}
void NntpAccount::setFetchDescriptions( bool fetch )
{
  mProperties.insert( "fetchDescriptions", fetch );
}



bool NntpAccount::requiresAuthentication()
{
  QVariant v = loadProperty<bool>( "requiresAuthentication" );
  return v.toBool();
}
void NntpAccount::setRequiresAuthentication( bool required )
{
  mProperties.insert( "requiresAuthentication", required );
}

QString NntpAccount::user()
{
  QVariant v = loadProperty<QString>( "userName" );
  return v.toString();
}
void NntpAccount::setUser( const QString &user )
{
  mProperties.insert( "userName", user );
}

QString NntpAccount::password()
{
  kDebug() << "AKONADI PORT: Not implemented:" << Q_FUNC_INFO;
  return QString();
}
void NntpAccount::setPassword( const QString &password )
{
  kDebug() << "AKONADI PORT: Not implemented:" << Q_FUNC_INFO;
}




// ---- Implementation of SettingsContainerInterface ----

const KPIMIdentities::Identity& NntpAccount::identity() const
{
  kDebug() << "AKONADI PORT: Needs to read the identity from the configuration:" << Q_FUNC_INFO;
  if ( mIdentityUoid < 0 ) {
    return KPIMIdentities::Identity::null();
  }
  return KNGlobals::self()->identityManager()->identityForUoid( mIdentityUoid );
}
void NntpAccount::setIdentity( const KPIMIdentities::Identity& identity )
{
  kDebug() << "AKONADI PORT: Needs to write the identity from the configuration:" << Q_FUNC_INFO;
  mIdentityUoid = ( identity.isNull() ? -1 : identity.uoid() );
}
void NntpAccount::writeConfig()
{
  kDebug() << "AKONADI PORT: Not implemented:" << Q_FUNC_INFO;
}



}

