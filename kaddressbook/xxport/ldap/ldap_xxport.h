/*
    This file is part of KAddressBook.
    Copyright (c) 2000 - 2009 Oliver Strutynski <olistrut@gmx.de>
                              Tobias Koenig <tokoe@kde.org>
                              Sebastian Sauer <sebsauer@kdab.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#ifndef LDAP_XXPORT_H
#define LDAP_XXPORT_H

#include "xxport.h"

class LDAPXXPort : public XXPort
{
  public:
    LDAPXXPort( QWidget *parent = 0 );
    KABC::Addressee::List importContacts() const;
    bool exportContacts( const KABC::Addressee::List &contacts ) const;
};

#endif
