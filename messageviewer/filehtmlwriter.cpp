/*  -*- c++ -*-
    filehtmlwriter.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include <config-messageviewer.h>

#include "filehtmlwriter.h"

#include <kdebug.h>

#include <QTextStream>


namespace MessageViewer {

  FileHtmlWriter::FileHtmlWriter( const QString & filename )
    : HtmlWriter(),
      mFile( filename.isEmpty() ? QString( "filehtmlwriter.out" ) : filename )
  {
    mStream.setCodec( "UTF-8" );
  }

  FileHtmlWriter::~FileHtmlWriter() {
    if ( mFile.isOpen() ) {
      kWarning() << "FileHtmlWriter: file still open!";
      mStream.setDevice(0);
      mFile.close();
    }
  }

  void FileHtmlWriter::begin( const QString & css ) {
    openOrWarn();
    if ( !css.isEmpty() )
      write( "<!-- CSS Definitions \n" + css + "-->\n" );
  }

  void FileHtmlWriter::end() {
    flush();
    mStream.setDevice(0);
    mFile.close();
  }

  void FileHtmlWriter::reset() {
    if ( mFile.isOpen() ) {
      mStream.setDevice( 0 );
      mFile.close();
    }
  }

  void FileHtmlWriter::write( const QString & str ) {
    mStream << str;
    flush();
  }

  void FileHtmlWriter::queue( const QString & str ) {
    write( str );
  }

  void FileHtmlWriter::flush() {
    mStream.flush();
    mFile.flush();
  }

  void FileHtmlWriter::openOrWarn() {
    if ( mFile.isOpen() ) {
      kWarning() << "FileHtmlWriter: file still open!";
      mStream.setDevice( 0 );
      mFile.close();
    }
    if ( !mFile.open( QIODevice::WriteOnly ) )
      kWarning() << "FileHtmlWriter: Cannot open file" << mFile.fileName();
    else
      mStream.setDevice( &mFile );
  }

  void FileHtmlWriter::embedPart( const QByteArray & contentId, const QString & url ) {
    mStream << "<!-- embedPart(contentID=" << contentId << ", url=" << url << ") -->" << endl;
    flush();
  }


} // namespace KMail
