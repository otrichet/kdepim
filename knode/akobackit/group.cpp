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


#include "akobackit/group.h"
#include "knglobals.h"

#include <Akonadi/CollectionModifyJob>
#include <Akonadi/EntityDisplayAttribute>
#include <KPIMIdentities/Identity>
#include <KPIMIdentities/IdentityManager>


namespace KNode {


Group::Group( const Akonadi::Collection &collection )
  : SettingsContainerInterface(),
    mCollection( collection )
{
  Q_ASSERT( mCollection.isValid() );
}

Group::~Group()
{
}


QString Group::name()
{
  const QString display = displayName();
  return ( display.isEmpty() ? groupName() : display );
}

QString Group::groupName()
{
  return mCollection.name();
}

QString Group::displayName()
{
  if ( mProperties.contains( "displayName" ) ) {
    return mProperties.value( "displayName" ).toString();
  }

  Akonadi::EntityDisplayAttribute *attribute = mCollection.attribute<Akonadi::EntityDisplayAttribute>();
  if ( attribute ) {
    return attribute->displayName();
  }
  return QString();
}

void Group::setDisplayName( const QString &name )
{
  mProperties.insert( "displayName", name );
}


QString Group::description()
{
  kDebug() << "AKONADI PORT: Not implemented";
  return QString();
}

Group::PostingStatus Group::postingStatus()
{
  kDebug() << "AKONADI PORT: Not implemented";
  return Unknown;
}


bool Group::useCharset()
{
  return loadProperty( "useCharset", false ).toBool();
}

void Group::setUseCharset( bool useCharset )
{
  mProperties.insert( "useCharset", useCharset );
}

QByteArray Group::defaultCharset()
{
  return loadProperty( "charset", QByteArray( "UTF-8" ) ).toByteArray();
}

void Group::setDefaultCharset( const QByteArray &charset )
{
  mProperties.insert( "charset", charset );
}





template<class T>
QVariant Group::loadProperty( const QString &key, const T &defaultValue ) const
{
  if ( mProperties.contains( key ) ) {
    return mProperties.value( key );
  }
  const KConfigGroup conf = KNGlobals::self()->config()->group( "Akonadi" )
                                                        .group( "Group" )
                                                        .group( QString::number( mCollection.id() ) );
  return conf.readEntry( key, defaultValue );
}

void Group::save()
{
  // config stored inside Akonadi
  if ( mProperties.contains( "displayName" ) ) {
    const QString displayName = mProperties.value( "displayName" ).toString().trimmed();
    Akonadi::EntityDisplayAttribute *attribute = 0;
    if ( displayName.isEmpty() ) {
      attribute = mCollection.attribute<Akonadi::EntityDisplayAttribute>();
    } else {
      attribute = mCollection.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Entity::AddIfMissing );
    }
    if ( attribute ) {
      attribute->setDisplayName( displayName );
    }

    Akonadi::CollectionModifyJob *modify = new Akonadi::CollectionModifyJob( mCollection );
    modify->start(); // TODO: handles error

    mProperties.remove( "displayName" );
  }


  // config stored in knoderc
  KConfigGroup conf = KNGlobals::self()->config()->group( "Akonadi" )
                                                  .group( "Group" )
                                                  .group( QString::number( mCollection.id() ) );
  QHash<QString,QVariant>::ConstIterator it = mProperties.constBegin();
  while ( it != mProperties.constEnd() ) {
    const QString &name = it.key();
    conf.writeEntry( name, it.value() );
    ++it;
  }
  conf.sync();
}




// ---- Implementation of SettingsContainerInterface ----

const KPIMIdentities::Identity& Group::identity() const
{
  int uoid = loadProperty( "identity", -1 ).toInt();
  if ( uoid < 0 ) {
    return KPIMIdentities::Identity::null();
  }
  return KNGlobals::self()->identityManager()->identityForUoid( uoid );
}

void Group::setIdentity( const KPIMIdentities::Identity &identity )
{
  int uoid = identity.isNull() ? -1 : identity.uoid();
  mProperties.insert( "identity", uoid );
}

void Group::writeConfig()
{
  // FIXME: do not write every thing to the backend!
  save();
}



}