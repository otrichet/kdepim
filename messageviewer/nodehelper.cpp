/* -*- mode: C++; c-file-style: "gnu" -*-
  Copyright (C) 2009 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Copyright (c) 2009 Andras Mantia <andras@kdab.net>

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
#include <config-messageviewer.h>

#include "nodehelper.h"
#include "iconnamecache.h"
#include "globalsettings.h"
#include "partmetadata.h"
#include "interfaces/bodypart.h"
#include "util.h"

#include <messagecore/nodehelper.h>
#include <messagecore/stringutil.h>
#include "messagecore/globalsettings.h"

#include <kmime/kmime_content.h>
#include <kmime/kmime_message.h>
#include <kmimetype.h>
#include <kdebug.h>
#include <kascii.h>
#include <ktemporaryfile.h>
#include <klocale.h>
#include <kcharsets.h>
#include <kde_file.h>
#include <kpimutils/kfileio.h>

#include <QDir>
#include <QTextCodec>



namespace MessageViewer {

QStringList replySubjPrefixes(QStringList() << "Re\\s*:" << "Re\\[\\d+\\]:" << "Re\\d+:");
QStringList forwardSubjPrefixes( QStringList() << "Fwd:" << "FW:");

NodeHelper::NodeHelper()
{
  //TODO(Andras) add methods to modify these prefixes

  mLocalCodec = QTextCodec::codecForName( KGlobal::locale()->encoding() );

  // In the case of Japan. Japanese locale name is "eucjp" but
  // The Japanese mail systems normally used "iso-2022-jp" of locale name.
  // We want to change locale name from eucjp to iso-2022-jp at KMail only.

  // (Introduction to i18n, 6.6 Limit of Locale technology):
  // EUC-JP is the de-facto standard for UNIX systems, ISO 2022-JP
  // is the standard for Internet, and Shift-JIS is the encoding
  // for Windows and Macintosh.
  if ( mLocalCodec ) {
    if ( mLocalCodec->name().toLower() == "eucjp"
#if defined Q_WS_WIN || defined Q_WS_MACX
        || mLocalCodec->name().toLower() == "shift-jis" // OK?
#endif
       )
    {
      mLocalCodec = QTextCodec::codecForName("jis7");
      // QTextCodec *cdc = QTextCodec::codecForName("jis7");
      // QTextCodec::setCodecForLocale(cdc);
      // KGlobal::locale()->setEncoding(cdc->mibEnum());
    }
  }
}

NodeHelper::~NodeHelper()
{
}

void NodeHelper::setNodeProcessed(KMime::Content* node, bool recurse )
{
  if ( !node )
    return;
  mProcessedNodes.append( node );
  //kDebug() << "Node processed: " << node->index().toString() << node->contentType()->as7BitString();
           //<< " decodedContent" << node->decodedContent();
  if ( recurse ) {
    KMime::Content::List contents = node->contents();
    Q_FOREACH( KMime::Content *c, contents )
    {
      setNodeProcessed( c, true );
    }
  }
}

void NodeHelper::setNodeUnprocessed(KMime::Content* node, bool recurse )
{
  if ( !node )
    return;
  mProcessedNodes.removeAll( node );

  //avoid double addition of extra nodes, eg. encrypted attachments
  for ( QMap<KMime::Content*, QList<KMime::Content*> >::iterator it = mExtraContents.begin(); it != mExtraContents.end(); ++it) {
    if ( node == dynamic_cast<KMime::Content*>( it.key() ) ) {
      Q_FOREACH( KMime::Content* c, it.value() ) {
        KMime::Content * p = c->parent();
        if ( p )
          p->removeContent( c );
      }
      qDeleteAll( it.value() );
      //kDebug() << "mExtraContents deleted for" << it.key();
      mExtraContents.remove( it.key() );
    }
  }

  //kDebug() << "Node UNprocessed: " << node;
  if ( recurse ) {
    KMime::Content::List contents = node->contents();
    Q_FOREACH( KMime::Content *c, contents )
    {
      setNodeUnprocessed( c, true );
    }
  }
}

bool NodeHelper::nodeProcessed( KMime::Content* node ) const
{
  if ( !node )
    return true;
  return mProcessedNodes.contains( node );
}

void NodeHelper::clear()
{
  mProcessedNodes.clear();
  mEncryptionState.clear();
  mSignatureState.clear();
  mOverrideCodecs.clear();
  for ( QMap<KMime::Content*, QMap< QByteArray, Interface::BodyPartMemento*> >::iterator
        it = mBodyPartMementoMap.begin(), end = mBodyPartMementoMap.end();
        it != end; ++it ) {
    clearBodyPartMemento( it.value() );
  }
  mBodyPartMementoMap.clear();

  for ( QMap<KMime::Content*, QList<KMime::Content*> >::iterator it = mExtraContents.begin(); it != mExtraContents.end(); ++it) {
    Q_FOREACH( KMime::Content* c, it.value() ) {
      KMime::Content * p = c->parent();
      if ( p )
        p->removeContent( c );
    }
    qDeleteAll( it.value() );
    kDebug() << "mExtraContents deleted for" << it.key();
  }
  mExtraContents.clear();
  mDisplayEmbeddedNodes.clear();
  mDisplayHiddenNodes.clear();
}


void NodeHelper::clearBodyPartMemento(QMap<QByteArray, Interface::BodyPartMemento*> bodyPartMementoMap)
{
  for ( QMap<QByteArray, Interface::BodyPartMemento*>::iterator
        it = bodyPartMementoMap.begin(), end = bodyPartMementoMap.end();
        it != end; ++it ) {
    Interface::BodyPartMemento *memento = it.value();
    memento->detach();
    delete memento;
  }
  bodyPartMementoMap.clear();
}


void NodeHelper::setEncryptionState( KMime::Content* node, const KMMsgEncryptionState state )
{
  mEncryptionState[node] = state;
}

KMMsgEncryptionState NodeHelper::encryptionState( KMime::Content *node ) const
{
  if ( mEncryptionState.contains( node ) )
    return mEncryptionState[node];

  return KMMsgNotEncrypted;
}

void NodeHelper::setSignatureState( KMime::Content* node, const KMMsgSignatureState state )
{
  mSignatureState[node] = state;
}

KMMsgSignatureState NodeHelper::signatureState( KMime::Content *node ) const
{
  if ( mSignatureState.contains( node ) )
    return mSignatureState[node];

  return KMMsgNotSigned;
}

PartMetaData NodeHelper::partMetaData(KMime::Content* node)
{
  return mPartMetaDatas.value( node );
}

void NodeHelper::setPartMetaData(KMime::Content* node, const PartMetaData& metaData)
{
  mPartMetaDatas.insert( node, metaData );
}

QString NodeHelper::writeNodeToTempFile(KMime::Content* node)
{
  // If the message part is already written to a file, no point in doing it again.
  // This function is called twice actually, once from the rendering of the attachment
  // in the body and once for the header.
  KUrl existingFileName = tempFileUrlFromNode( node );
  if ( !existingFileName.isEmpty() ) {
    return existingFileName.toLocalFile();
  }

  QString fileName = NodeHelper::fileName( node );

  QString fname = createTempDir( node->index().toString() );
  if ( fname.isEmpty() )
    return QString();

  // strip off a leading path
  int slashPos = fileName.lastIndexOf( '/' );
  if( -1 != slashPos )
    fileName = fileName.mid( slashPos + 1 );
  if( fileName.isEmpty() )
    fileName = "unnamed";
  fname += '/' + fileName;

  //kDebug() << "Create temp file: " << fname;

  QByteArray data = node->decodedContent();
  if ( node->contentType()->isText() && data.size() > 0 ) {
    // convert CRLF to LF before writing text attachments to disk
    data = KMime::CRLFtoLF( data );
  }
  if( !KPIMUtils::kByteArrayToFile( data, fname, false, false, false ) )
    return QString();

  mTempFiles.append( fname );
  // make file read-only so that nobody gets the impression that he might
  // edit attached files (cf. bug #52813)
  ::chmod( QFile::encodeName( fname ), S_IRUSR );

  return fname;
}



KUrl NodeHelper::tempFileUrlFromNode( const KMime::Content *node )
{
  if (!node)
    return KUrl();

  QString index = node->index().toString();

  foreach ( const QString &path, mTempFiles ) {
    int right = path.lastIndexOf( '/' );
    int left = path.lastIndexOf( ".index.", right );
    if ( left != -1 )
        left += 7;

    QString storedIndex = path.mid( left, right - left );
    if ( left != -1 && storedIndex == index )
      return KUrl( path );
  }
  return KUrl();
}


QString NodeHelper::createTempDir( const QString &param )
{
  KTemporaryFile *tempFile = new KTemporaryFile();
  tempFile->setSuffix( ".index." + param );
  tempFile->open();
  QString fname = tempFile->fileName();
  delete tempFile;

  if ( ::access( QFile::encodeName( fname ), W_OK ) != 0 ) {
    // Not there or not writable
    if( KDE_mkdir( QFile::encodeName( fname ), 0 ) != 0 ||
        ::chmod( QFile::encodeName( fname ), S_IRWXU ) != 0 ) {
      return QString(); //failed create
    }
  }

  Q_ASSERT( !fname.isNull() );

  mTempDirs.append( fname );
  return fname;
}


void NodeHelper::removeTempFiles()
{
  for (QStringList::Iterator it = mTempFiles.begin(); it != mTempFiles.end();
    ++it)
  {
    QFile::remove(*it);
  }
  mTempFiles.clear();
  for (QStringList::Iterator it = mTempDirs.begin(); it != mTempDirs.end();
    it++)
  {
    QDir(*it).rmdir(*it);
  }
  mTempDirs.clear();
}

void NodeHelper::addTempFile( const QString& file )
{
  mTempFiles.append( file );
}

bool NodeHelper::isToltecMessage( KMime::Content* node )
{
  if ( !node->contentType( false ) )
    return false;

  if ( node->contentType()->mediaType().toLower() != "multipart" ||
       node->contentType()->subType().toLower() != "mixed" )
    return false;

  if ( node->contents().size() != 3 )
    return false;

  const KMime::Headers::Base *libraryHeader = node->headerByType( "X-Library" );
  if ( !libraryHeader )
    return false;

  if ( QString::fromLatin1( libraryHeader->as7BitString( false ) ).toLower() !=
       QLatin1String( "toltec" ) )
    return false;

  const KMime::Headers::Base *kolabTypeHeader = node->headerByType( "X-Kolab-Type" );
  if ( !kolabTypeHeader )
    return false;

  if ( !QString::fromLatin1( kolabTypeHeader->as7BitString( false ) ).toLower().startsWith(
         QLatin1String( "application/x-vnd.kolab" ) ) )
    return false;

  return true;
}

bool NodeHelper::isInEncapsulatedMessage( KMime::Content* node )
{
  const KMime::Content * const topLevel = node->topLevel();
  const KMime::Content * cur = node;
  while ( cur && cur != topLevel ) {
    const bool parentIsMessage = cur->parent() && cur->parent()->contentType( false ) &&
                                 cur->parent()->contentType()->mimeType().toLower() == "message/rfc822";
    if ( parentIsMessage && cur->parent() != topLevel ) {
      return true;
    }
    cur = cur->parent();
  }
  return false;
}


QByteArray NodeHelper::charset( KMime::Content *node )
{
  if ( node->contentType( false ) )
    return node->contentType( false )->charset();
  else
    return node->defaultCharset();
}

KMMsgEncryptionState NodeHelper::overallEncryptionState( KMime::Content *node ) const
{
    KMMsgEncryptionState myState = KMMsgEncryptionStateUnknown;
    if ( !node )
      return myState;

    if( encryptionState( node ) == KMMsgNotEncrypted ) {
        // NOTE: children are tested ONLY when parent is not encrypted
        KMime::Content *child = MessageCore::NodeHelper::firstChild( node );
        if ( child )
            myState = overallEncryptionState( child );
        else
            myState = KMMsgNotEncrypted;
    }
    else { // part is partially or fully encrypted
        myState = encryptionState( node );
    }
    // siblings are tested always
    KMime::Content * next = MessageCore::NodeHelper::nextSibling( node );
    if( next ) {
        KMMsgEncryptionState otherState = overallEncryptionState( next );
        switch( otherState ) {
        case KMMsgEncryptionStateUnknown:
            break;
        case KMMsgNotEncrypted:
            if( myState == KMMsgFullyEncrypted )
                myState = KMMsgPartiallyEncrypted;
            else if( myState != KMMsgPartiallyEncrypted )
                myState = KMMsgNotEncrypted;
            break;
        case KMMsgPartiallyEncrypted:
            myState = KMMsgPartiallyEncrypted;
            break;
        case KMMsgFullyEncrypted:
            if( myState != KMMsgFullyEncrypted )
                myState = KMMsgPartiallyEncrypted;
            break;
        case KMMsgEncryptionProblematic:
            break;
        }
    }

//kDebug() <<"\n\n  KMMsgEncryptionState:" << myState;

    return myState;
}


KMMsgSignatureState NodeHelper::overallSignatureState( KMime::Content* node ) const
{
    KMMsgSignatureState myState = KMMsgSignatureStateUnknown;
    if ( !node )
      return myState;

    if( signatureState( node ) == KMMsgNotSigned ) {
        // children are tested ONLY when parent is not signed
        KMime::Content* child = MessageCore::NodeHelper::firstChild( node );
        if( child )
            myState = overallSignatureState( child );
        else
            myState = KMMsgNotSigned;
    }
    else { // part is partially or fully signed
        myState = signatureState( node );
    }
    // siblings are tested always
    KMime::Content *next = MessageCore::NodeHelper::nextSibling( node );
    if( next ) {
        KMMsgSignatureState otherState = overallSignatureState( next );
        switch( otherState ) {
        case KMMsgSignatureStateUnknown:
            break;
        case KMMsgNotSigned:
            if( myState == KMMsgFullySigned )
                myState = KMMsgPartiallySigned;
            else if( myState != KMMsgPartiallySigned )
                myState = KMMsgNotSigned;
            break;
        case KMMsgPartiallySigned:
            myState = KMMsgPartiallySigned;
            break;
        case KMMsgFullySigned:
            if( myState != KMMsgFullySigned )
                myState = KMMsgPartiallySigned;
            break;
        case KMMsgSignatureProblematic:
            break;
        }
    }

//kDebug() <<"\n\n  KMMsgSignatureState:" << myState;

    return myState;
}

QString NodeHelper::iconName( KMime::Content *node, int size )
{
  if ( !node )
    return QString();

  QByteArray mimeType = node->contentType()->mimeType();
  kAsciiToLower( mimeType.data() );
  return Util::fileNameForMimetype( mimeType, size, node->contentDisposition()->filename(),
                                    node->contentType()->name() );
}

void NodeHelper::magicSetType( KMime::Content* node, bool aAutoDecode )
{
  const QByteArray body = ( aAutoDecode ) ? node->decodedContent() : node->body() ;
  KMimeType::Ptr mime = KMimeType::findByContent( body );

  QString mimetype = mime->name();
  node->contentType()->setMimeType( mimetype.toLatin1() );
}

// static
QString NodeHelper::replacePrefixes( const QString& str,
                                    const QStringList& prefixRegExps,
                                    bool replace,
                                    const QString& newPrefix )
{
  bool recognized = false;
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
    // try good ole Re/Fwd:
    recognized = str.startsWith( newPrefix );
  } else { // valid rx
    QString tmp = str;
    if ( rx.indexIn( tmp ) == 0 ) {
      recognized = true;
      if ( replace )
        return tmp.replace( 0, rx.matchedLength(), newPrefix + ' ' );
    }
  }
  if ( !recognized )
    return newPrefix + ' ' + str;
  else
    return str;
}

QString NodeHelper::cleanSubject( KMime::Message *message )
{
  return cleanSubject( message, replySubjPrefixes + forwardSubjPrefixes,
                       true, QString() ).trimmed();
}

QString NodeHelper::cleanSubject( KMime::Message *message, const QStringList & prefixRegExps,
                                  bool replace,
                                  const QString & newPrefix )
{
  return NodeHelper::replacePrefixes( message->subject()->asUnicodeString(), prefixRegExps, replace,
                                      newPrefix );
}

void NodeHelper::setOverrideCodec( KMime::Content* node, const QTextCodec* codec )
{
  if ( !node )
    return;

  mOverrideCodecs[node] = codec;
}

const QTextCodec * NodeHelper::codec( KMime::Content* node )
{
  if (! node )
    return mLocalCodec;

  const QTextCodec *c = mOverrideCodecs[node];
  if ( !c ) {
    // no override-codec set for this message, try the CT charset parameter:
    c = codecForName( node->contentType()->charset() );
  }
  if ( !c ) {
    // Ok, no override and nothing in the message, let's use the fallback
    // the user configured
    c = codecForName( MessageCore::GlobalSettings::self()->fallbackCharacterEncoding().toLatin1() );
  }
  if ( !c ) {
    // no charset means us-ascii (RFC 2045), so using local encoding should
    // be okay
    c = mLocalCodec;
  }
  return c;
}

const QTextCodec* NodeHelper::codecForName(const QByteArray& _str)
{
  if (_str.isEmpty())
    return 0;
  QByteArray codec = _str;
  kAsciiToLower(codec.data());
  return KGlobal::charsets()->codecForName(codec);
}

QByteArray NodeHelper::path(const KMime::Content* node)
{
  if ( !node->parent() ) {
    return QByteArray( ":" );
  }
  const KMime::Content *p = node->parent();

  // count number of siblings with the same type as us:
  int nth = 0;
  for ( KMime::Content *c = MessageCore::NodeHelper::firstChild(p); c != node; c = MessageCore::NodeHelper::nextSibling(c) ) {
    if ( c->contentType()->mediaType() == const_cast<KMime::Content*>(node)->contentType()->mediaType() && c->contentType()->subType() == const_cast<KMime::Content*>(node)->contentType()->subType() ) {
      ++nth;
    }
  }
  QString subpath;
  return NodeHelper::path(p) + subpath.sprintf( ":%s/%s[%X]", const_cast<KMime::Content*>(node)->contentType()->mediaType().constData(), const_cast<KMime::Content*>(node)->contentType()->subType().constData(), nth ).toLocal8Bit();
}

QString NodeHelper::fileName( const KMime::Content *node )
{
  QString name = const_cast<KMime::Content*>( node )->contentDisposition()->filename();
  if ( name.isEmpty() )
    name = const_cast<KMime::Content*>( node )->contentType()->name();

  name = name.trimmed();
  return name;
}

//FIXME(Andras) review it (by Marc?) to see if I got it right. This is supposed to be the partNode::internalBodyPartMemento replacement
Interface::BodyPartMemento *NodeHelper::bodyPartMemento( KMime::Content *node,
                                                         const QByteArray &which ) const
{
  if ( !mBodyPartMementoMap.contains( node ) )
    return 0;
  const QMap<QByteArray,Interface::BodyPartMemento*>::const_iterator it =
  mBodyPartMementoMap[node].find( which.toLower() );
  return it != mBodyPartMementoMap[node].end() ? it.value() : 0 ;
}

 //FIXME(Andras) review it (by Marc?) to see if I got it right. This is supposed to be the partNode::internalSetBodyPartMemento replacement
void NodeHelper::setBodyPartMemento( KMime::Content* node, const QByteArray &which,
                                     Interface::BodyPartMemento *memento )
{
  const QMap<QByteArray,Interface::BodyPartMemento*>::iterator it =
    mBodyPartMementoMap[node].lowerBound( which.toLower() );

  if ( it != mBodyPartMementoMap[node].end() && it.key() == which.toLower() ) {
    delete it.value();
    if ( memento ) {
      it.value() = memento;
    } else {
      mBodyPartMementoMap[node].erase( it );
    }
  } else {
    mBodyPartMementoMap[node].insert( which.toLower(), memento );
  }
}

bool NodeHelper::isNodeDisplayedEmbedded( KMime::Content* node ) const
{
  //kDebug() << "IS NODE: " << mDisplayEmbeddedNodes.contains( node );
  return mDisplayEmbeddedNodes.contains( node );
}

void NodeHelper::setNodeDisplayedEmbedded( KMime::Content* node, bool displayedEmbedded )
{
  //kDebug() << "SET NODE: " << node << displayedEmbedded;
  if ( displayedEmbedded )
    mDisplayEmbeddedNodes.insert( node );
  else
    mDisplayEmbeddedNodes.remove( node );
}

bool NodeHelper::isNodeDisplayedHidden( KMime::Content* node ) const
{
  return mDisplayHiddenNodes.contains( node );
}

void NodeHelper::setNodeDisplayedHidden( KMime::Content* node, bool displayedHidden )
{
  if( displayedHidden ) {
    mDisplayHiddenNodes.insert( node );
  } else {
    mDisplayEmbeddedNodes.remove( node );
  }
}

QString NodeHelper::asHREF( const KMime::Content* node, const QString &place )
{
  if ( !node )
    return QString();
  else {
    QString indexStr = node->index().toString();
    //if the node is an extra node, prepent the index of the extra node to the url
    for ( QMap<KMime::Content*, QList<KMime::Content*> >::iterator it = mExtraContents.begin(); it != mExtraContents.end(); ++it) {
      QList<KMime::Content*> extraNodes = it.value();
      for ( int i = 0; i < extraNodes.size(); ++i )  {
        if ( node->topLevel() == extraNodes[i] ) {
          indexStr.prepend( QString("%1:").arg(i) );
          it = mExtraContents.end();
          --it;
          break;
        }
      }
    }
    return QString( "attachment:%1?place=%2" ).arg( indexStr ).arg( place );
  }
}

QString NodeHelper::fixEncoding( const QString &encoding )
{
  QString returnEncoding = encoding;
  // According to http://www.iana.org/assignments/character-sets, uppercase is
  // preferred in MIME headers
  if ( returnEncoding.toUpper().contains( "ISO " ) ) {
    returnEncoding = returnEncoding.toUpper();
    returnEncoding.replace( "ISO ", "ISO-" );
  }
  return returnEncoding;
}


//-----------------------------------------------------------------------------
QString NodeHelper::encodingForName( const QString &descriptiveName )
{
  QString encoding = KGlobal::charsets()->encodingForName( descriptiveName );
  return NodeHelper::fixEncoding( encoding );
}

QStringList NodeHelper::supportedEncodings(bool usAscii)
{
  QStringList encodingNames = KGlobal::charsets()->availableEncodingNames();
  QStringList encodings;
  QMap<QString,bool> mimeNames;
  for (QStringList::Iterator it = encodingNames.begin();
    it != encodingNames.end(); ++it)
  {
    QTextCodec *codec = KGlobal::charsets()->codecForName(*it);
    QString mimeName = (codec) ? QString(codec->name()).toLower() : (*it);
    if (!mimeNames.contains(mimeName) )
    {
      encodings.append( KGlobal::charsets()->descriptionForEncoding(*it) );
      mimeNames.insert( mimeName, true );
    }
  }
  encodings.sort();
  if (usAscii)
    encodings.prepend(KGlobal::charsets()->descriptionForEncoding("us-ascii") );
  return encodings;
}


QByteArray NodeHelper::autoDetectCharset(const QByteArray &_encoding, const QStringList &encodingList, const QString &text)
{
    QStringList charsets = encodingList;
    if (!_encoding.isEmpty())
    {
       QString currentCharset = QString::fromLatin1(_encoding);
       charsets.removeAll(currentCharset);
       charsets.prepend(currentCharset);
    }

    QStringList::ConstIterator it = charsets.constBegin();
    for (; it != charsets.constEnd(); ++it)
    {
       QByteArray encoding = (*it).toLatin1();
       if (encoding == "locale")
       {
         encoding = QTextCodec::codecForName( KGlobal::locale()->encoding() )->name();
         kAsciiToLower(encoding.data());
       }
       if (text.isEmpty())
         return encoding;
       if (encoding == "us-ascii") {
         bool ok;
         (void) toUsAscii(text, &ok);
         if (ok)
            return encoding;
       }
       else
       {
         const QTextCodec *codec = codecForName(encoding);
         if (!codec) {
           kDebug() << "Auto-Charset: Something is wrong and I can not get a codec:" << encoding;
         } else {
           if (codec->canEncode(text))
              return encoding;
         }
       }
    }
    return 0;
}

QByteArray NodeHelper::toUsAscii(const QString& _str, bool *ok)
{
  bool all_ok =true;
  QString result = _str;
  int len = result.length();
  for (int i = 0; i < len; i++)
    if (result.at(i).unicode() >= 128) {
      result[i] = '?';
      all_ok = false;
    }
  if (ok)
    *ok = all_ok;
  return result.toLatin1();
}

QString NodeHelper::fromAsString( KMime::Content* node )
{
  KMime::Message* topLevel = dynamic_cast<KMime::Message*>( node->topLevel() );
  if ( topLevel )
    return topLevel->from()->asUnicodeString();
  return QString();
}

void NodeHelper::attachExtraContent( KMime::Content *topLevelNode, KMime::Content* content )
{
  //kDebug() << "mExtraContents added for" << topLevelNode << " extra content: " << content;
  mExtraContents[topLevelNode].append( content );
}

void NodeHelper::removeAllExtraContent( KMime::Content *topLevelNode )
{
  if ( mExtraContents.contains( topLevelNode ) ) {
    qDeleteAll( mExtraContents[topLevelNode] );
    mExtraContents.remove( topLevelNode );
  }    
}

QList< KMime::Content* > NodeHelper::extraContents( KMime::Content *topLevelnode )
{
 if ( mExtraContents.contains( topLevelnode ) ) {
    return mExtraContents[topLevelnode];
 } else {
  return QList< KMime::Content* >();
 }
}

bool NodeHelper::isPermanentwWithExtraContent( KMime::Content* node )
{
  return mExtraContents.contains( node ) && mExtraContents[ node ].size() > 0;
}


void NodeHelper::mergeExtraNodes( KMime::Content *node )
{
  if ( !node )
    return;

  QList<KMime::Content* > extraNodes = extraContents( node );
  Q_FOREACH( KMime::Content* extra, extraNodes ) {
    if( node->bodyIsMessage() ) {
      kWarning() << "Asked to attach extra content to a kmime::message, this does not make sense. Attaching to:" << node <<
                    node->encodedContent() << "\n====== with =======\n" <<  extra << extra->encodedContent();
        continue;
      }
      KMime::Content *c = new KMime::Content( node );
      c->setContent( extra->encodedContent() );
      c->parse();
      node->addContent( c );
  }

  Q_FOREACH( KMime::Content* child, node->contents() ) {
    mergeExtraNodes( child );
  }
}

void NodeHelper::cleanFromExtraNodes( KMime::Content* node )
{
  if ( !node )
    return;
  QList<KMime::Content* > extraNodes = extraContents( node );
  Q_FOREACH( KMime::Content* extra, extraNodes ) {
     QByteArray s = extra->encodedContent();
     QList<KMime::Content* > children = node->contents();
     Q_FOREACH( KMime::Content *c, children ) {
       if ( c->encodedContent() == s ) {
         node->removeContent( c );
       }
     }
  }
  Q_FOREACH( KMime::Content* child, node->contents() ) {
    cleanFromExtraNodes( child );
  }
}


KMime::Message* NodeHelper::messageWithExtraContent( KMime::Content* topLevelNode )
{
  /*The merge is done in several steps:
    1) merge the extra nodes into topLevelNode
    2) copy the modified (merged) node tree into a new node tree
    3) restore the original node tree in topLevelNode by removing the extra nodes from it

    The reason is that extra nodes are assigned by pointer value to the nodes in the original tree.
  */
  if (!topLevelNode)
    return 0;

  mergeExtraNodes( topLevelNode );

  KMime::Message *m = new KMime::Message;
  m->setContent( topLevelNode->encodedContent() );
  m->parse();

  cleanFromExtraNodes( topLevelNode );
//   qDebug() << "MESSAGE WITH EXTRA: " << m->encodedContent();
//   qDebug() << "MESSAGE WITHOUT EXTRA: " << topLevelNode->encodedContent();

  return m;
}

NodeHelper::AttachmentDisplayInfo NodeHelper::attachmentDisplayInfo( KMime::Content* node )
{
  AttachmentDisplayInfo info;
  info.icon = iconName( node, KIconLoader::Small );
  info.label = fileName( node );
  if( info.label.isEmpty() ) {
    info.label = node->contentDescription()->asUnicodeString();
  }

  bool typeBlacklisted = node->contentType()->mediaType().toLower() == "multipart";
  if ( !typeBlacklisted ) {
    typeBlacklisted = MessageCore::StringUtil::isCryptoPart( node->contentType()->mediaType(),
                                                             node->contentType()->subType(),
                                                             node->contentDisposition()->filename() );
  }
  typeBlacklisted = typeBlacklisted || node == node->topLevel();
  const bool firstTextChildOfEncapsulatedMsg =
        node->contentType()->mediaType().toLower() == "text" &&
        node->contentType()->subType().toLower() == "plain" &&
        node->parent() && node->parent()->contentType()->mediaType().toLower() == "message";
  typeBlacklisted = typeBlacklisted || firstTextChildOfEncapsulatedMsg;
  info.displayInHeader = !info.label.isEmpty() && !info.icon.isEmpty() && !typeBlacklisted;
  return info;
}

static KMime::Content* decryptedNodeForContent( KMime::Content *content, NodeHelper *nodeHelper )
{
  if ( !nodeHelper->extraContents( content ).isEmpty() ) {
    if ( nodeHelper->extraContents( content ).size() == 1 ) {
      return nodeHelper->extraContents( content ).first();
    } else {
      kWarning() << "WTF, encrypted node has multiple extra contents?";
    }
  }
  return 0;
}

bool NodeHelper::unencryptedMessage_helper( KMime::Content *node, QByteArray &resultingData, bool addHeaders,
                                            int recursionLevel )
{
  bool returnValue = false;
  if ( node ) {
    KMime::Content *curNode = node;
    KMime::Content *decryptedNode = 0;
    const QByteArray type = node->contentType( false ) ? QByteArray( node->contentType()->mediaType() ).toLower() : "text";
    const QByteArray subType = node->contentType( false ) ? node->contentType()->subType().toLower() : "plain";
    const bool isMultipart = node->contentType( false ) && node->contentType()->isMultipart();
    bool isSignature = false;

    //kDebug() << "(" << recursionLevel << ") Looking at" << type << "/" << subType;

    if ( isMultipart ) {
      if ( subType == "signed" ) {
        isSignature = true;
      } else if ( subType == "encrypted" ) {
        decryptedNode = decryptedNodeForContent( curNode, this );
      }
    } else if ( type == "application" ) {
      if ( subType == "octet-stream" ) {
        decryptedNode = decryptedNodeForContent( curNode, this );
      } else if ( subType == "pkcs7-signature" ) {
        isSignature = true;
      } else if ( subType == "pkcs7-mime" ) {
        // note: subtype pkcs7-mime can also be signed
        //       and we do NOT want to remove the signature!
        if ( encryptionState( curNode ) != KMMsgNotEncrypted ) {
        decryptedNode = decryptedNodeForContent( curNode, this );
        }
      }
    }

    if ( decryptedNode ) {
      //kDebug() << "Current node has an associated decrypted node, adding a modified header "
      //            "and then processing the children.";

      Q_ASSERT( addHeaders );
      KMime::Content headers;
      headers.setHead( curNode->head() );
      headers.parse();
      if ( decryptedNode->contentType( false ) ) {
        headers.contentType()->from7BitString( decryptedNode->contentType()->as7BitString( false ) );
      } else {
        headers.removeHeader( headers.contentType()->type() );
      }
      if ( decryptedNode->contentTransferEncoding( false ) ) {
        headers.contentTransferEncoding()->from7BitString( decryptedNode->contentTransferEncoding()->as7BitString( false ) );
      } else {
        headers.removeHeader( headers.contentTransferEncoding()->type() );
      }
      if ( decryptedNode->contentDisposition( false ) ) {
        headers.contentDisposition()->from7BitString( decryptedNode->contentDisposition()->as7BitString( false ) );
      } else {
        headers.removeHeader( headers.contentDisposition()->type() );
      }
      if ( decryptedNode->contentDescription( false ) ) {
        headers.contentDescription()->from7BitString( decryptedNode->contentDescription()->as7BitString( false ) );
      } else {
        headers.removeHeader( headers.contentDescription()->type() );
      }
      headers.assemble();

      resultingData += headers.head() + '\n';
      unencryptedMessage_helper( decryptedNode, resultingData, false, recursionLevel + 1 );

      returnValue = true;
    }

    else if ( isSignature ) {
      //kDebug() << "Current node is a signature, adding it as-is.";
      // We can't change the nodes under the signature, as that would invalidate it. Add the signature
      // and its child as-is
      if ( addHeaders ) {
        resultingData += curNode->head() + '\n';
      }
      resultingData += curNode->encodedBody();
      returnValue = false;
    }

    else if ( isMultipart ) {
      //kDebug() << "Current node is a multipart node, adding its header and then processing all children.";
      // Normal multipart node, add the header and all of its children
      bool somethingChanged = false;
      if ( addHeaders ) {
        resultingData += curNode->head() + '\n';
      }
      const QByteArray boundary = curNode->contentType()->boundary();
      foreach( KMime::Content *child, curNode->contents() ) {
        resultingData += "\n--" + boundary + '\n';
        const bool changed = unencryptedMessage_helper( child, resultingData, true, recursionLevel + 1 );
        if ( changed ) {
          somethingChanged = true;
        }
      }
      resultingData += "\n--" + boundary + "--\n\n";
      returnValue = somethingChanged;
    }

    else if ( curNode->bodyIsMessage() ) {
      //kDebug() << "Current node is a message, adding the header and then processing the child.";
      if ( addHeaders ) {
        resultingData += curNode->head() + '\n';
      }

      returnValue = unencryptedMessage_helper( curNode->bodyAsMessage().get(), resultingData, true, recursionLevel + 1 );
    }

    else {
      //kDebug() << "Current node is an ordinary leaf node, adding it as-is.";
      if ( addHeaders ) {
        resultingData += curNode->head() + '\n';
      }
      resultingData += curNode->body();
      returnValue = false;
    }
  }

  //kDebug() << "(" << recursionLevel << ") done.";
  return returnValue;
}

KMime::Message::Ptr NodeHelper::unencryptedMessage( const KMime::Message::Ptr& originalMessage )
{
  QByteArray resultingData;
  const bool messageChanged = unencryptedMessage_helper( originalMessage.get(), resultingData, true );
  if ( messageChanged ) {
#if 0
    kDebug() << "Resulting data is:" << resultingData;
    QFile bla("stripped.mbox");
    bla.open(QIODevice::WriteOnly);
    bla.write(resultingData);
    bla.close();
#endif
    KMime::Message::Ptr newMessage( new KMime::Message );
    newMessage->setContent( resultingData );
    newMessage->parse();
    return newMessage;
  } else {
    return KMime::Message::Ptr();
  }
}

}
