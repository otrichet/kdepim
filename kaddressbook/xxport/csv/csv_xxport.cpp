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

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "csv_xxport.h"

#include "contactfields.h"
#include "csvimportdialog.h"

#include <kfiledialog.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktemporaryfile.h>
#include <kurl.h>

#include <QtCore/QPointer>
#include <QtCore/QTextCodec>
#include <QtCore/QTextStream>

CsvXXPort::CsvXXPort( QWidget *parent )
  : XXPort( parent )
{
}

bool CsvXXPort::exportContacts( const KABC::Addressee::List &contacts ) const
{
  KUrl url = KFileDialog::getSaveUrl( KUrl( "addressbook.csv" ) );
  if ( url.isEmpty() )
      return true;

  if ( QFileInfo(url.isLocalFile() ? url.toLocalFile() : url.path()).exists() ) {
    if ( KMessageBox::questionYesNo( parentWidget(), i18n( "Do you want to overwrite file \"%1\"", url.isLocalFile() ? url.toLocalFile() : url.path() ) ) == KMessageBox::No )
      return true;
  }

  if ( !url.isLocalFile() ) {
    KTemporaryFile tmpFile;
    if ( !tmpFile.open() ) {
      const QString msg = i18n( "<qt>Unable to open file <b>%1</b></qt>", url.url() );
      KMessageBox::error( parentWidget(), msg );
      return false;
    }

    exportToFile( &tmpFile, contacts );
    tmpFile.flush();

    return KIO::NetAccess::upload( tmpFile.fileName(), url, parentWidget() );

  } else {
    QFile file( url.toLocalFile() );
    if ( !file.open( QIODevice::WriteOnly ) ) {
      const QString msg = i18n( "<qt>Unable to open file <b>%1</b>.</qt>", url.toLocalFile() );
      KMessageBox::error( parentWidget(), msg );
      return false;
    }

    exportToFile( &file, contacts );
    file.close();

    return true;
  }
}

void CsvXXPort::exportToFile( QFile *file, const KABC::Addressee::List &contacts ) const
{
  QTextStream stream( file );
  stream.setCodec( QTextCodec::codecForLocale() );

  ContactFields::Fields fields = ContactFields::allFields();
  fields.remove( ContactFields::Undefined );

  bool first = true;

  // First output the column headings
  for ( int i = 0; i < fields.count(); ++i ) {
    if ( !first )
      stream << ",";

    // add quoting as defined in RFC 4180
    QString label = ContactFields::label( fields.at( i ) );
    label.replace( QLatin1Char( '"' ), QLatin1String( "\"\"" ) );

    stream << "\"" << label << "\"";
    first = false;
  }
  stream << "\n";

  // Then all the contacts
  for ( int i = 0; i < contacts.count(); ++i ) {

    const KABC::Addressee contact = contacts.at( i );
    first = true;

    for ( int j = 0; j < fields.count(); ++j ) {
      if ( !first )
        stream << ",";

      QString content;
      if ( fields.at( j ) == ContactFields::Birthday ||
           fields.at( j ) == ContactFields::Anniversary ) {
        const QDateTime dateTime = QDateTime::fromString( ContactFields::value( fields.at( j ), contact ), Qt::ISODate );
        if ( dateTime.isValid() )
          content = dateTime.date().toString( Qt::ISODate );
      } else {
        content = ContactFields::value( fields.at( j ), contact ).replace( '\n', "\\n" );
      }

      // add quoting as defined in RFC 4180
      content.replace( QLatin1Char( '"' ), QLatin1String( "\"\"" ) );

      stream << '\"' << content << '\"';
      first = false;
    }

    stream << "\n";
  }
}

KABC::Addressee::List CsvXXPort::importContacts() const
{
  KABC::Addressee::List contacts;

  QPointer<CSVImportDialog> dlg = new CSVImportDialog( parentWidget() );
  if ( dlg->exec() && dlg )
    contacts = dlg->contacts();

  delete dlg;

  return contacts;
}
