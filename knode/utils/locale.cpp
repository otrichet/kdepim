/*
  Copyright 2009 Olivier Trichet <nive@nivalis.org>

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

#include "locale.h"

#include "knglobals.h"
#include "settings.h"

#include <KCharsets>
#include <KGlobal>
#include <kmime/kmime_charfreq.h>
#include <KLocale>

#include <QTextCodec>


using namespace KNode::Utilities;


QString Locale::toMimeCharset( const QString &charset )
{
#if 0
  QString c = charset;

  // First, get the user preferred encoding
  if ( c.isEmpty() ) {
    c = KGlobal::locale()->encoding();
    if ( c.isEmpty() ) { // Let's test to be really sure...
      return "UTF-8";
    }
  }

  // Second, convert to something sensible
  bool ok;
  QTextCodec *codec = KGlobal::charsets()->codecForName( c, ok );
  if ( ok && !codec->name().isEmpty() ) {
    c = codec->name();
  } else {
    return "UTF-8";
  }

  // special logic for japanese users:
  // "euc-jp" is default encoding for them, but in the news
  // "iso-2022-jp" is used (#36638)
  if ( c.toUpper() == "EUC-JP" ) {
    c = "ISO-2022-JP";
  }

  // Uppercase is preferred in MIME headers
  c = c.toUpper();

  return c;
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



QByteArray Locale::defaultCharset()
{
#if 0
  return toMimeCharset( knGlobals.settings()->charset() ).toLatin1();
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}

QByteArray Locale::defaultCharset( KNGroup::Ptr g )
{
#if 0
  if ( g && g->useCharset() ) {
    return toMimeCharset( g->defaultCharset() ).toLatin1();
  } else {
    return defaultCharset();
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}




void Locale::recodeString( const QString &s, KNGroup::Ptr g, QByteArray &result )
{
#if 0
  Q_ASSERT( g );

  encodeTo7Bit( s.toLatin1(), defaultCharset( g ), result );
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void Locale::encodeTo7Bit( const QByteArray &raw, const QByteArray &charset, QByteArray &result )
{
#if 0
  if ( raw.isEmpty() ) {
    result = raw;
    return;
  }

  KMime::CharFreq cf( raw );
  if ( cf.isSevenBitText() ) {
    result = raw;
    return;
  }

  // Transform 8-bit data
  QString properData = QTextCodec::codecForName( charset )->toUnicode( raw );
  result = KMime::encodeRFC2047String( properData, "UTF-8" );
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


QStringList Locale::encodings()
{
#if 0
  QStringList encodings = KGlobal::charsets()->availableEncodingNames();
  QStringList ret;
  QStringList seenEncs;

  // Blacklist 'UTF-16' which gives garbage (bug #168327).
  seenEncs << "UTF-16";

  foreach ( const QString &enc, encodings ) {
    // Valid codec only
    bool ok;

    KGlobal::charsets()->codecForName( enc, ok );
    if ( !ok ) {
      continue;
    }

    // One encoding description per MIME-charset
    QString mimeEnc = toMimeCharset( enc );
    if ( !seenEncs.contains( mimeEnc ) ) {
      seenEncs << mimeEnc;
      ret << KGlobal::charsets()->descriptionForEncoding( enc );
    }
  }

  ret.sort();
  return ret;
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}

