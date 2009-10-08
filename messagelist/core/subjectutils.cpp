/******************************************************************************
 *
 *  Copyright 2009 Kevin Ottens <ervin@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "subjectutils_p.h"

#include <KConfigGroup>
#include <KDebug>

#include <QRegExp>
#include <QStringList>

#include "configprovider.h"

QString MessageList::Core::SubjectUtils::stripOffPrefixes( const QString &subject )
{
  KConfigGroup composerGroup( ConfigProvider::self()->config(), "Composer" );

  QStringList replyPrefixes = composerGroup.readEntry( "reply-prefixes", QStringList() );
  if ( replyPrefixes.isEmpty() ) {
    replyPrefixes << "Re\\s*:" << "Re\\[\\d+\\]:" << "Re\\d+:";
  }

  QStringList forwardPrefixes = composerGroup.readEntry( "forward-prefixes", QStringList() );
  if ( forwardPrefixes.isEmpty() ) {
    forwardPrefixes << "Fwd:" << "FW:";
  }

  QString str = subject;
  const QStringList prefixRegExps = replyPrefixes + forwardPrefixes;

  // construct a big regexp that
  // 1. is anchored to the beginning of str (sans whitespace)
  // 2. matches at least one of the part regexps in prefixRegExps
  QString bigRegExp = QString::fromLatin1("^(?:\\s+|(?:%1))+\\s*")
                      .arg( prefixRegExps.join(")|(?:") );
  QRegExp rx( bigRegExp, Qt::CaseInsensitive );
  if ( !rx.isValid() ) {
    kWarning() << "bigRegExp = \""
               << bigRegExp << "\"\n"
               << "prefix regexp is invalid!";
  } else { // valid rx
    QString tmp = str;
    if ( rx.indexIn( tmp ) == 0 ) {
      return tmp.replace( 0, rx.matchedLength(), QString() );
    }
  }

  return str;
}

