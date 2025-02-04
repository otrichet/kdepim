/*
    This file is part of KAddressBook.
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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
*/

#include "xxportfactory.h"

#include "csv/csv_xxport.h"
#include "ldif/ldif_xxport.h"
#include "ldap/ldap_xxport.h"
#include "vcard/vcard_xxport.h"
#include "gmx/gmx_xxport.h"

XXPort* XXPortFactory::createXXPort( const QString &identifier, QWidget *parentWidget ) const
{
  if ( identifier == "vcard21" || identifier == "vcard30" ) {
    XXPort *xxport = new VCardXXPort( parentWidget );

    if ( identifier == "vcard21" )
      xxport->setOption( "version", "v21" );

    return xxport;
  } else if ( identifier == "csv" )
    return new CsvXXPort( parentWidget );
  else if ( identifier == "ldif" )
    return new LDIFXXPort( parentWidget );
  else if ( identifier == "ldap" )
    return new LDAPXXPort( parentWidget );
  else if ( identifier == "gmx" )
    return new GMXXXPort( parentWidget );
  else
    return 0;
}
