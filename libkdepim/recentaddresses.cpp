/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  Copyright (c) 2001-2003 Carsten Pfeiffer <pfeiffer@kde.org>
 *  Copyright (c) 2003 Zack Rusin <zack@kde.org>
 *
 *  KMail is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License, version 2, as
 *  published by the Free Software Foundation.
 *
 *  KMail is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */
#include "recentaddresses.h"
#include <kpimutils/email.h>

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KLocale>
#if KDE_IS_VERSION( 4, 5, 74 )
#include <KEditListWidget>
#else
#include <KEditListBox>
#endif

#include <QCoreApplication>
#include <QLayout>
#include <QVBoxLayout>

using namespace KPIM;

RecentAddresses *s_self = 0;

void deleteGlobalRecentAddresses()
{
  delete s_self;
  s_self = 0;
}

RecentAddresses *RecentAddresses::self( KConfig *config )
{
  if ( !s_self ) {
    s_self = new RecentAddresses( config );
    qAddPostRoutine( deleteGlobalRecentAddresses );
  }
  return s_self;
}

bool RecentAddresses::exists()
{
  return s_self != 0;
}

RecentAddresses::RecentAddresses( KConfig *config )
{
  if ( !config ) {
    load( KGlobal::config().data() );
  } else {
    load( config );
  }
}

RecentAddresses::~RecentAddresses()
{
  // if you want this destructor to get called, use a K3StaticDeleter
  // on s_self
}

void RecentAddresses::load( KConfig *config )
{
  QStringList addresses;
  QString name;
  QString email;

  m_addresseeList.clear();
  KConfigGroup cg( config, "General" );
  m_maxCount = cg.readEntry( "Maximum Recent Addresses", 40 );
  addresses = cg.readEntry( "Recent Addresses", QStringList() );
  for ( QStringList::Iterator it = addresses.begin(); it != addresses.end(); ++it ) {
    KABC::Addressee::parseEmailAddress( *it, name, email );
    if ( !email.isEmpty() ) {
      KABC::Addressee addr;
      addr.setNameFromString( name );
      addr.insertEmail( email, true );
      m_addresseeList.append( addr );
    }
  }

  adjustSize();
}

void RecentAddresses::save( KConfig *config )
{
  KConfigGroup cg( config, "General" );
  cg.writeEntry( "Recent Addresses", addresses() );
}

void RecentAddresses::add( const QString &entry )
{
  if ( !entry.isEmpty() && m_maxCount > 0 ) {
    const QStringList list = KPIMUtils::splitAddressList( entry );
    for ( QStringList::const_iterator e_it = list.constBegin(); e_it != list.constEnd(); ++e_it ) {
      KPIMUtils::EmailParseResult errorCode = KPIMUtils::isValidAddress( *e_it );
      if ( errorCode != KPIMUtils::AddressOk ) {
        continue;
      }
      QString email;
      QString fullName;
      KABC::Addressee addr;

      KABC::Addressee::parseEmailAddress( *e_it, fullName, email );

      for ( KABC::Addressee::List::Iterator it = m_addresseeList.begin();
          it != m_addresseeList.end(); ++it ) {
        if ( email == (*it).preferredEmail() ) {
          //already inside, remove it here and add it later at pos==1
          m_addresseeList.erase( it );
          break;
        }
      }
      addr.setNameFromString( fullName );
      addr.insertEmail( email, true );
      m_addresseeList.prepend( addr );
      adjustSize();
    }
  }
}

void RecentAddresses::setMaxCount( int count )
{
    m_maxCount = count;
    adjustSize();
}

void RecentAddresses::adjustSize()
{
  while ( m_addresseeList.count() > m_maxCount ) {
    m_addresseeList.takeLast();
  }
}

void RecentAddresses::clear()
{
  m_addresseeList.clear();
  adjustSize();
}

QStringList RecentAddresses::addresses() const
{
  QStringList addresses;
  for ( KABC::Addressee::List::ConstIterator it = m_addresseeList.constBegin();
        it != m_addresseeList.constEnd(); ++it ) {
    addresses.append( (*it).fullEmail() );
  }
  return addresses;
}

RecentAddressDialog::RecentAddressDialog( QWidget *parent )
  : KDialog( parent )
{
  setCaption( i18n( "Edit Recent Addresses" ) );
  setButtons( Ok|Cancel );
  setDefaultButton( Ok );
  setModal( true );
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QVBoxLayout *layout = new QVBoxLayout( page );
  layout->setSpacing( spacingHint() );
  layout->setMargin( 0 );

#if KDE_IS_VERSION( 4, 5, 74 )
  mEditor = new KEditListWidget( page );
  mEditor->setButtons( KEditListWidget::Add | KEditListWidget::Remove );
#else
  mEditor = new KEditListBox( i18n( "Recent Addresses" ), page );
  mEditor->setButtons( KEditListBox::Add | KEditListBox::Remove );
#endif
  layout->addWidget( mEditor );
}

void RecentAddressDialog::setAddresses( const QStringList &addrs )
{
  mEditor->clear();
  mEditor->insertStringList( addrs );
}

QStringList RecentAddressDialog::addresses() const
{
  return mEditor->items();
}
