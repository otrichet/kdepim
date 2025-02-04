/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "contactselectiondialog.h"

#include "contactselectionwidget.h"

#include <klocale.h>

ContactSelectionDialog::ContactSelectionDialog( QItemSelectionModel *selectionModel, QWidget *parent )
  : KDialog( parent )
{
  setCaption( i18n( "Select Contacts" ) );
  setButtons( Ok | Cancel );

  mSelectionWidget = new ContactSelectionWidget( selectionModel, this );
  setMainWidget( mSelectionWidget );

  setInitialSize( QSize( 450, 220 ) );
}

void ContactSelectionDialog::setMessageText( const QString &message )
{
  mSelectionWidget->setMessageText( message );
}

void ContactSelectionDialog::setDefaultAddressBook( const Akonadi::Collection &addressBook )
{
  mSelectionWidget->setDefaultAddressBook( addressBook );
}

KABC::Addressee::List ContactSelectionDialog::selectedContacts() const
{
  return mSelectionWidget->selectedContacts();
}

#include "contactselectiondialog.moc"
