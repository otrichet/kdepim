/*  -*- c++ -*-
    keyresolver.cpp

    This file is part of libkleopatra, the KDE keymanagement library
    Copyright (c) 2004 Klarälvdalens Datakonsult AB

    Based on kpgp.cpp
    Copyright (C) 2001,2002 the KPGP authors
    See file libkdenetwork/AUTHORS.kpgp for details

    Libkleopatra is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    Libkleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
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

#include "keyresolver.h"

#ifndef QT_NO_CURSOR
#include "messageviewer/kcursorsaver.h"
#endif
#include "kleo_util.h"

#include <kpimutils/email.h>
#include "libkleo/ui/keyselectiondialog.h"
#include "kleo/cryptobackendfactory.h"
#include "kleo/keylistjob.h"
#include "kleo/dn.h"

#include <gpgme++/key.h>
#include <gpgme++/keylistresult.h>

#include <akonadi/collectiondialog.h>
#include <akonadi/contact/contactsearchjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemmodifyjob.h>
#include <klocale.h>
#include <kdebug.h>
#include <kinputdialog.h>
#include <kmessagebox.h>

#include <QStringList>
#include <QTextDocument>
#include <time.h>

#include <algorithm>
#include <memory>
#include <iterator>
#include <functional>
#include <map>
#include <set>
#include <iostream>
#include <cassert>

// this should go into stl_util.h, which has since moved into messageviewer.
// for lack of a better place put it in here for now.
namespace kdtools {
template <typename Iterator, typename UnaryPredicate>
bool any( Iterator first, Iterator last, UnaryPredicate p )
{
  while ( first != last )
    if ( p( *first ) )
      return true;
    else
      ++first;
  return false;
}
} // namespace kdtools

//
// some predicates to be used in STL algorithms:
//

static inline bool EmptyKeyList( const Kleo::KeyApprovalDialog::Item & item ) {
  return item.keys.empty();
}

static inline QString ItemDotAddress( const Kleo::KeyResolver::Item & item ) {
  return item.address;
}

static inline bool ApprovalNeeded( const Kleo::KeyResolver::Item & item ) {
  return item.pref == Kleo::UnknownPreference || item.pref == Kleo::NeverEncrypt || item.keys.empty() ;
}

static inline Kleo::KeyResolver::Item
CopyKeysAndEncryptionPreferences( const Kleo::KeyResolver::Item & oldItem,
				  const Kleo::KeyApprovalDialog::Item & newItem ) {
  return Kleo::KeyResolver::Item( oldItem.address, newItem.keys, newItem.pref, oldItem.signPref, oldItem.format );
}

static inline bool ByKeyID( const GpgME::Key & left, const GpgME::Key & right ) {
  return qstrcmp( left.keyID(), right.keyID() ) < 0 ;
}

static inline bool WithRespectToKeyID( const GpgME::Key & left, const GpgME::Key & right ) {
  return qstrcmp( left.keyID(), right.keyID() ) == 0 ;
}

static bool ValidOpenPGPEncryptionKey( const GpgME::Key & key ) {
  if ( key.protocol() != GpgME::OpenPGP ) {
    return false;
  }
#if 1
  if ( key.isRevoked() )
    kWarning() << "is revoked";
  if ( key.isExpired() )
    kWarning() << "is expired";
  if ( key.isDisabled() )
    kWarning() << "is disabled";
  if ( !key.canEncrypt() )
    kWarning() << "can't encrypt";
#endif
  if ( key.isRevoked() || key.isExpired() || key.isDisabled() || !key.canEncrypt() )
    return false;
  return true;
}

static bool ValidTrustedOpenPGPEncryptionKey( const GpgME::Key & key ) {
    if ( !ValidOpenPGPEncryptionKey( key ) )
        return false;
  const std::vector<GpgME::UserID> uids = key.userIDs();
  for ( std::vector<GpgME::UserID>::const_iterator it = uids.begin() ; it != uids.end() ; ++it ) {
    if ( !it->isRevoked() && it->validity() >= GpgME::UserID::Marginal )
      return true;
#if 1
    else
      if ( it->isRevoked() )
        kWarning() <<"a userid is revoked";
      else
        kWarning() <<"bad validity" << int( it->validity() );
#endif
  }
  return false;
}

static bool ValidSMIMEEncryptionKey( const GpgME::Key & key ) {
  if ( key.protocol() != GpgME::CMS )
    return false;
  if ( key.isRevoked() || key.isExpired() || key.isDisabled() || !key.canEncrypt() )
    return false;
  return true;
}

static bool ValidTrustedSMIMEEncryptionKey( const GpgME::Key & key ) {
  if ( !ValidSMIMEEncryptionKey( key ) )
    return false;
  return true;
}

static inline bool ValidTrustedEncryptionKey( const GpgME::Key & key ) {
  switch ( key.protocol() ) {
  case GpgME::OpenPGP:
    return ValidTrustedOpenPGPEncryptionKey( key );
  case GpgME::CMS:
    return ValidTrustedSMIMEEncryptionKey( key );
  default:
    return false;
  }
}

static inline bool ValidEncryptionKey( const GpgME::Key & key ) {
    switch ( key.protocol() ) {
    case GpgME::OpenPGP:
        return ValidOpenPGPEncryptionKey( key );
    case GpgME::CMS:
        return ValidSMIMEEncryptionKey( key );
    default:
        return false;
    }
}

static inline bool ValidSigningKey( const GpgME::Key & key ) {
  if ( key.isRevoked() || key.isExpired() || key.isDisabled() || !key.canSign() )
    return false;
  return key.hasSecret();
}

static inline bool ValidOpenPGPSigningKey( const GpgME::Key & key ) {
  return key.protocol() == GpgME::OpenPGP && ValidSigningKey( key );
}

static inline bool ValidSMIMESigningKey( const GpgME::Key & key ) {
  return key.protocol() == GpgME::CMS && ValidSigningKey( key );
}

static inline bool NotValidTrustedOpenPGPEncryptionKey( const GpgME::Key & key ) {
  return !ValidTrustedOpenPGPEncryptionKey( key );
}

static inline bool NotValidOpenPGPEncryptionKey( const GpgME::Key & key ) {
  return !ValidOpenPGPEncryptionKey( key );
}

static inline bool NotValidTrustedSMIMEEncryptionKey( const GpgME::Key & key ) {
  return !ValidTrustedSMIMEEncryptionKey( key );
}

static inline bool NotValidSMIMEEncryptionKey( const GpgME::Key & key ) {
  return !ValidSMIMEEncryptionKey( key );
}

static inline bool NotValidTrustedEncryptionKey( const GpgME::Key & key ) {
  return !ValidTrustedEncryptionKey( key );
}

static inline bool NotValidEncryptionKey( const GpgME::Key & key ) {
  return !ValidEncryptionKey( key );
}

static inline bool NotValidSigningKey( const GpgME::Key & key ) {
  return !ValidSigningKey( key );
}

static inline bool NotValidOpenPGPSigningKey( const GpgME::Key & key ) {
  return !ValidOpenPGPSigningKey( key );
}

static inline bool NotValidSMIMESigningKey( const GpgME::Key & key ) {
  return !ValidSMIMESigningKey( key );
}

namespace {
  struct ByTrustScore {
    static int score( const GpgME::UserID & uid )
    {
      return uid.isRevoked() || uid.isInvalid() ? -1 : uid.validity() ;
    }
    bool operator()( const GpgME::UserID & lhs, const GpgME::UserID & rhs ) const
    {
      return score( lhs ) < score( rhs ) ;
    }
  };
}

static std::vector<GpgME::UserID> matchingUIDs( const std::vector<GpgME::UserID> & uids, const QString & address ) {
    if ( address.isEmpty() )
        return std::vector<GpgME::UserID>();

    std::vector<GpgME::UserID> result;
    result.reserve( uids.size() );
    for ( std::vector<GpgME::UserID>::const_iterator it = uids.begin(), end = uids.end() ; it != end ; ++it )
        // PENDING(marc) check DN for an EMAIL, too, in case of X.509 certs... :/
        if ( const char * email = it->email() )
            if ( *email && QString::fromUtf8( email ).simplified().toLower() == address )
                result.push_back( *it );
    return result;
}

static GpgME::UserID findBestMatchUID( const GpgME::Key & key, const QString & address ) {
    const std::vector<GpgME::UserID> all = key.userIDs();
    if ( all.empty() )
        return GpgME::UserID();
    const std::vector<GpgME::UserID> matching = matchingUIDs( all, address.toLower() );
    const std::vector<GpgME::UserID> & v = matching.empty() ? all : matching ;
    return *std::max_element( v.begin(), v.end(), ByTrustScore() );
}

static QStringList keysAsStrings( const std::vector<GpgME::Key>& keys ) {
  QStringList strings;
  for ( std::vector<GpgME::Key>::const_iterator it = keys.begin() ; it != keys.end() ; ++it ) {
    assert( !(*it).userID(0).isNull() );
    QString keyLabel = QString::fromUtf8( (*it).userID(0).email() );
    if ( keyLabel.isEmpty() )
      keyLabel = QString::fromUtf8( (*it).userID(0).name() );
    if ( keyLabel.isEmpty() )
      keyLabel = QString::fromUtf8( (*it).userID(0).id() );
    strings.append( keyLabel );
  }
  return strings;
}

static std::vector<GpgME::Key> trustedOrConfirmed( const std::vector<GpgME::Key> & keys, const QString & address, bool & canceled ) {
  // PENDING(marc) work on UserIDs here?
  std::vector<GpgME::Key> fishies;
  std::vector<GpgME::Key> ickies;
  std::vector<GpgME::Key> rewookies;
  std::vector<GpgME::Key>::const_iterator it = keys.begin();
  const std::vector<GpgME::Key>::const_iterator end = keys.end();
  for ( ; it != end ; ++it ) {
    const GpgME::Key & key = *it;
    assert( ValidEncryptionKey( key ) );
    const GpgME::UserID uid = findBestMatchUID( key, address );
    if ( uid.isRevoked() ) {
        rewookies.push_back( key );
    }
    if ( !uid.isRevoked()  && uid.validity() == GpgME::UserID::Marginal ) {
        fishies.push_back( key );
    }
    if ( !uid.isRevoked()  && uid.validity() < GpgME::UserID::Never ) {
        ickies.push_back( key );
    }
  }

  if ( fishies.empty() && ickies.empty() && rewookies.empty() )
    return keys;

  // if  some keys are not fully trusted, let the user confirm their use
  QString msg = address.isEmpty()
      ? i18n("One or more of your configured OpenPGP encryption "
             "keys or S/MIME certificates is not fully trusted "
             "for encryption.")
      : i18n("One or more of the OpenPGP encryption keys or S/MIME "
             "certificates for recipient \"%1\" is not fully trusted "
             "for encryption.", address) ;

  if ( !fishies.empty() ) {
    // certificates can't have marginal trust
    msg += i18n( "\nThe following keys are only marginally trusted: \n");
    msg += keysAsStrings( fishies ).join( QLatin1String(",") );
  }
  if ( !ickies.empty() ) {
    msg += i18n( "\nThe following keys or certificates have unknown trust level: \n");
    msg += keysAsStrings( ickies ).join( QLatin1String(",") );
  }
  if ( !rewookies.empty() ) {
    msg += i18n( "\nThe following keys or certificates are <b>revoked</b>: \n");
    msg += keysAsStrings( rewookies ).join( QLatin1String(",") );
  }

  if( KMessageBox::warningContinueCancel( 0, msg, i18n("Not Fully Trusted Encryption Keys"),
                                          KStandardGuiItem::cont(), KStandardGuiItem::cancel(),
                                          QLatin1String("not fully trusted encryption key warning") )
          == KMessageBox::Continue )
    return keys;
  else
    canceled = true;
    return std::vector<GpgME::Key>();
}

namespace {
  struct IsNotForFormat : public std::unary_function<GpgME::Key,bool> {
    IsNotForFormat( Kleo::CryptoMessageFormat f ) : format( f ) {}

    bool operator()( const GpgME::Key & key ) const {
      return
	( isOpenPGP( format ) && key.protocol() != GpgME::OpenPGP ) ||
	( isSMIME( format )   && key.protocol() != GpgME::CMS );
    }

    const Kleo::CryptoMessageFormat format;
  };

  struct IsForFormat : std::unary_function<GpgME::Key,bool> {
    explicit IsForFormat( Kleo::CryptoMessageFormat f )
      : protocol( isOpenPGP( f ) ? GpgME::OpenPGP :
                  isSMIME( f )   ? GpgME::CMS :
                                   GpgME::UnknownProtocol ) {}

    bool operator()( const GpgME::Key & key ) const {
      return key.protocol() == protocol;
    }

    const GpgME::Protocol protocol;
  };

}

class Kleo::KeyResolver::SigningPreferenceCounter : public std::unary_function<Kleo::KeyResolver::Item,void> {
public:
  SigningPreferenceCounter()
    : mTotal( 0 ),
      mUnknownSigningPreference( 0 ),
      mNeverSign( 0 ),
      mAlwaysSign( 0 ),
      mAlwaysSignIfPossible( 0 ),
      mAlwaysAskForSigning( 0 ),
      mAskSigningWheneverPossible( 0 )
  {

  }
  void operator()( const Kleo::KeyResolver::Item & item );
#define make_int_accessor(x) unsigned int num##x() const { return m##x; }
  make_int_accessor(UnknownSigningPreference)
  make_int_accessor(NeverSign)
  make_int_accessor(AlwaysSign)
  make_int_accessor(AlwaysSignIfPossible)
  make_int_accessor(AlwaysAskForSigning)
  make_int_accessor(AskSigningWheneverPossible)
  make_int_accessor(Total)
#undef make_int_accessor
private:
  unsigned int mTotal;
  unsigned int mUnknownSigningPreference, mNeverSign, mAlwaysSign,
    mAlwaysSignIfPossible, mAlwaysAskForSigning, mAskSigningWheneverPossible;
};

void Kleo::KeyResolver::SigningPreferenceCounter::operator()( const Kleo::KeyResolver::Item & item ) {
  switch ( item.signPref ) {
#define CASE(x) case x: ++m##x; break
    CASE(UnknownSigningPreference);
    CASE(NeverSign);
    CASE(AlwaysSign);
    CASE(AlwaysSignIfPossible);
    CASE(AlwaysAskForSigning);
    CASE(AskSigningWheneverPossible);
#undef CASE
  }
  ++mTotal;
}



class Kleo::KeyResolver::EncryptionPreferenceCounter : public std::unary_function<Item,void> {
  const Kleo::KeyResolver * _this;
public:
  EncryptionPreferenceCounter( const Kleo::KeyResolver * kr, EncryptionPreference defaultPreference )
    : _this( kr ),
      mDefaultPreference( defaultPreference ),
      mTotal( 0 ),
      mNoKey( 0 ),
      mNeverEncrypt( 0 ),
      mUnknownPreference( 0 ),
      mAlwaysEncrypt( 0 ),
      mAlwaysEncryptIfPossible( 0 ),
      mAlwaysAskForEncryption( 0 ),
      mAskWheneverPossible( 0 )
  {

  }
  void operator()( Item & item );

  template <typename Container>
  void process( Container & c ) {
    *this = std::for_each( c.begin(), c.end(), *this );
  }

#define make_int_accessor(x) unsigned int num##x() const { return m##x; }
  make_int_accessor(NoKey)
  make_int_accessor(NeverEncrypt)
  make_int_accessor(UnknownPreference)
  make_int_accessor(AlwaysEncrypt)
  make_int_accessor(AlwaysEncryptIfPossible)
  make_int_accessor(AlwaysAskForEncryption)
  make_int_accessor(AskWheneverPossible)
  make_int_accessor(Total)
#undef make_int_accessor
private:
  EncryptionPreference mDefaultPreference;
  bool mNoOps;
  unsigned int mTotal;
  unsigned int mNoKey;
  unsigned int mNeverEncrypt, mUnknownPreference, mAlwaysEncrypt,
    mAlwaysEncryptIfPossible, mAlwaysAskForEncryption, mAskWheneverPossible;
};

void Kleo::KeyResolver::EncryptionPreferenceCounter::operator()( Item & item ) {
  if ( _this ) {
  if ( item.needKeys )
    item.keys = _this->getEncryptionKeys( item.address, true );
  if ( item.keys.empty() ) {
    ++mNoKey;
    return;
  }
  }
  switch ( !item.pref ? mDefaultPreference : item.pref ) {
#define CASE(x) case Kleo::x: ++m##x; break
    CASE(NeverEncrypt);
    CASE(UnknownPreference);
    CASE(AlwaysEncrypt);
    CASE(AlwaysEncryptIfPossible);
    CASE(AlwaysAskForEncryption);
    CASE(AskWheneverPossible);
#undef CASE
  }
  ++mTotal;
}

namespace {

  class FormatPreferenceCounterBase : public std::unary_function<Kleo::KeyResolver::Item,void> {
  public:
    FormatPreferenceCounterBase()
      : mTotal( 0 ),
	mInlineOpenPGP( 0 ),
	mOpenPGPMIME( 0 ),
	mSMIME( 0 ),
	mSMIMEOpaque( 0 )
    {

    }

#define make_int_accessor(x) unsigned int num##x() const { return m##x; }
    make_int_accessor(Total)
    make_int_accessor(InlineOpenPGP)
    make_int_accessor(OpenPGPMIME)
    make_int_accessor(SMIME)
    make_int_accessor(SMIMEOpaque)
#undef make_int_accessor

    unsigned int numOf( Kleo::CryptoMessageFormat f ) const {
      switch ( f ) {
#define CASE(x) case Kleo::x##Format: return m##x
	CASE(InlineOpenPGP);
	CASE(OpenPGPMIME);
	CASE(SMIME);
	CASE(SMIMEOpaque);
#undef CASE
      default: return 0;
      }
    }

  protected:
    unsigned int mTotal;
    unsigned int mInlineOpenPGP, mOpenPGPMIME, mSMIME, mSMIMEOpaque;
  };

  class EncryptionFormatPreferenceCounter : public FormatPreferenceCounterBase {
  public:
    EncryptionFormatPreferenceCounter() : FormatPreferenceCounterBase() {}
    void operator()( const Kleo::KeyResolver::Item & item );
  };

  class SigningFormatPreferenceCounter : public FormatPreferenceCounterBase {
  public:
    SigningFormatPreferenceCounter() : FormatPreferenceCounterBase() {}
    void operator()( const Kleo::KeyResolver::Item & item );
  };

#define CASE(x) if ( item.format & Kleo::x##Format ) ++m##x;
  void EncryptionFormatPreferenceCounter::operator()( const Kleo::KeyResolver::Item & item ) {
    if ( item.format & (Kleo::InlineOpenPGPFormat|Kleo::OpenPGPMIMEFormat) &&
	 std::find_if( item.keys.begin(), item.keys.end(),
		       ValidTrustedOpenPGPEncryptionKey ) != item.keys.end() ) {  // -= trusted?
      CASE(OpenPGPMIME);
      CASE(InlineOpenPGP);
    }
    if ( item.format & (Kleo::SMIMEFormat|Kleo::SMIMEOpaqueFormat) &&
	 std::find_if( item.keys.begin(), item.keys.end(),
		       ValidTrustedSMIMEEncryptionKey ) != item.keys.end() ) {    // -= trusted?
      CASE(SMIME);
      CASE(SMIMEOpaque);
    }
    ++mTotal;
  }

  void SigningFormatPreferenceCounter::operator()( const Kleo::KeyResolver::Item & item ) {
    CASE(InlineOpenPGP);
    CASE(OpenPGPMIME);
    CASE(SMIME);
    CASE(SMIMEOpaque);
    ++mTotal;
  }
#undef CASE

} // anon namespace

static QString canonicalAddress( const QString & _address ) {
  const QString address = KPIMUtils::extractEmailAddress( _address );
  if ( !address.contains( QLatin1Char('@') ) ) {
    // local address
    //return address + '@' + KNetwork::KResolver::localHostName();
    return address + QLatin1String("@localdomain");
  }
  else
    return address;
}


struct FormatInfo {
  std::vector<Kleo::KeyResolver::SplitInfo> splitInfos;
  std::vector<GpgME::Key> signKeys;
};

struct Kleo::KeyResolver::Private {
  std::set<QByteArray> alreadyWarnedFingerprints;

  std::vector<GpgME::Key> mOpenPGPSigningKeys; // signing
  std::vector<GpgME::Key> mSMIMESigningKeys; // signing

  std::vector<GpgME::Key> mOpenPGPEncryptToSelfKeys; // encryption to self
  std::vector<GpgME::Key> mSMIMEEncryptToSelfKeys; // encryption to self

  std::vector<Item> mPrimaryEncryptionKeys; // encryption to To/CC
  std::vector<Item> mSecondaryEncryptionKeys; // encryption to BCC

  std::map<CryptoMessageFormat,FormatInfo> mFormatInfoMap;

  // key=email address, value=crypto preferences for this contact (from kabc)
  typedef std::map<QString, ContactPreferences> ContactPreferencesMap;
  ContactPreferencesMap mContactPreferencesMap;
};


Kleo::KeyResolver::KeyResolver( bool encToSelf, bool showApproval, bool oppEncryption,
				unsigned int f,
				int encrWarnThresholdKey, int signWarnThresholdKey,
				int encrWarnThresholdRootCert, int signWarnThresholdRootCert,
				int encrWarnThresholdChainCert, int signWarnThresholdChainCert )
  : mEncryptToSelf( encToSelf ),
    mShowApprovalDialog( showApproval ),
    mOpportunisticEncyption( oppEncryption ),
    mCryptoMessageFormats( f ),
    mEncryptKeyNearExpiryWarningThreshold( encrWarnThresholdKey ),
    mSigningKeyNearExpiryWarningThreshold( signWarnThresholdKey ),
    mEncryptRootCertNearExpiryWarningThreshold( encrWarnThresholdRootCert ),
    mSigningRootCertNearExpiryWarningThreshold( signWarnThresholdRootCert ),
    mEncryptChainCertNearExpiryWarningThreshold( encrWarnThresholdChainCert ),
    mSigningChainCertNearExpiryWarningThreshold( signWarnThresholdChainCert )
{
  d = new Private();
}

Kleo::KeyResolver::~KeyResolver() {
  delete d; d = 0;
}

Kpgp::Result Kleo::KeyResolver::checkKeyNearExpiry( const GpgME::Key & key, const char * dontAskAgainName,
                                                    bool mine, bool sign, bool ca,
                                                    int recur_limit, const GpgME::Key & orig ) const
{
  if ( recur_limit <= 0 ) {
    kDebug() << "Key chain too long (>100 certs)";
    return Kpgp::Ok;
  }
  const GpgME::Subkey subkey = key.subkey(0);
  if ( d->alreadyWarnedFingerprints.count( subkey.fingerprint() ) )
    return Kpgp::Ok; // already warned about this one (and so about it's issuers)

  if ( subkey.neverExpires() )
    return Kpgp::Ok;
  static const double secsPerDay = 24 * 60 * 60;
  const double secsTillExpiry = ::difftime( subkey.expirationTime(), time(0) );
  if ( secsTillExpiry <= 0 ) {
      const int daysSinceExpiry = 1 + int( -secsTillExpiry / secsPerDay );
      kDebug() << "Key 0x" << key.shortKeyID() << " expired less than "
                   << daysSinceExpiry << " days ago";
      const QString msg =
          key.protocol() == GpgME::OpenPGP
          ? ( mine ? sign
              ? ki18np("<p>Your OpenPGP signing key</p><p align=center><b>%2</b> (KeyID 0x%3)</p>"
                       "<p>expired less than a day ago.</p>",
                       "<p>Your OpenPGP signing key</p><p align=center><b>%2</b> (KeyID 0x%3)</p>"
                       "<p>expired %1 days ago.</p>")
              : ki18np("<p>Your OpenPGP encryption key</p><p align=center><b>%2</b> (KeyID 0x%3)</p>"
                       "<p>expired less than a day ago.</p>",
                       "<p>Your OpenPGP encryption key</p><p align=center><b>%2</b> (KeyID 0x%3)</p>"
                       "<p>expired %1 days ago.</p>")
              : ki18np("<p>The OpenPGP key for</p><p align=center><b>%2</b> (KeyID 0x%3)</p>"
                       "<p>expired less than a day ago.</p>",
                       "<p>The OpenPGP key for</p><p align=center><b>%2</b> (KeyID 0x%3)</p>"
                       "<p>expired %1 days ago.</p>") )
		  .subs( daysSinceExpiry )
		  .subs( QString::fromUtf8( key.userID(0).id() ) )
		  .subs( QString::fromLatin1( key.shortKeyID() ) )
		  .toString()
          : ( ca
              ? ( key.isRoot()
                  ? ( mine ? sign
                      ? ki18np("<p>The root certificate</p><p align=center><b>%4</b></p>"
                               "<p>for your S/MIME signing certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                               "<p>expired less than a day ago.</p>",
                               "<p>The root certificate</p><p align=center><b>%4</b></p>"
                               "<p>for your S/MIME signing certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                               "<p>expired %1 days ago.</p>")
                      : ki18np("<p>The root certificate</p><p align=center><b>%4</b></p>"
                               "<p>for your S/MIME encryption certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                               "<p>expired less than a day ago.</p>",
                               "<p>The root certificate</p><p align=center><b>%4</b></p>"
                               "<p>for your S/MIME encryption certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                               "<p>expired %1 days ago.</p>")
                      : ki18np("<p>The root certificate</p><p align=center><b>%4</b></p>"
                               "<p>for S/MIME certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                               "<p>expired less than a day ago.</p>",
                               "<p>The root certificate</p><p align=center><b>%4</b></p>"
                               "<p>for S/MIME certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                               "<p>expired %1 days ago.</p>") )
                  : ( mine ? sign
                      ? ki18np("<p>The intermediate CA certificate</p><p align=center><b>%4</b></p>"
                               "<p>for your S/MIME signing certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                               "<p>expired less than a day ago.</p>",
                               "<p>The intermediate CA certificate</p><p align=center><b>%4</b></p>"
                               "<p>for your S/MIME signing certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                               "<p>expired %1 days ago.</p>")
                      : ki18np("<p>The intermediate CA certificate</p><p align=center><b>%4</b></p>"
                               "<p>for your S/MIME encryption certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                               "<p>expired less than a day ago.</p>",
                               "<p>The intermediate CA certificate</p><p align=center><b>%4</b></p>"
                               "<p>for your S/MIME encryption certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                               "<p>expired %1 days ago.</p>")
                      : ki18np("<p>The intermediate CA certificate</p><p align=center><b>%4</b></p>"
                               "<p>for S/MIME certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                               "<p>expired less than a day ago.</p>",
                               "<p>The intermediate CA certificate</p><p align=center><b>%4</b></p>"
                               "<p>for S/MIME certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                               "<p>expired %1 days ago.</p>") ) )
			   .subs( daysSinceExpiry )
			   .subs( Kleo::DN( orig.userID(0).id() ).prettyDN() )
			   .subs( QString::fromLatin1( orig.issuerSerial() ) )
			   .subs( Kleo::DN( key.userID(0).id() ).prettyDN() )
			   .toString()
              : ( mine ? sign
                  ? ki18np("<p>Your S/MIME signing certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                           "<p>expired less than a day ago.</p>",
                           "<p>Your S/MIME signing certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                           "<p>expired %1 days ago.</p>")
                  : ki18np("<p>Your S/MIME encryption certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                           "<p>expired less than a day ago.</p>",
                           "<p>Your S/MIME encryption certificate</p><p align=center><b>%2</b> (serial number %3)</p>"
                           "<p>expired %1 days ago.</p>")
                  : ki18np("<p>The S/MIME certificate for</p><p align=center><b>%2</b> (serial number %3)</p>"
                           "<p>expired less than a day ago.</p>",
                           "<p>The S/MIME certificate for</p><p align=center><b>%2</b> (serial number %3)</p>"
                           "<p>expired %1 days ago.</p>" ) )
		       .subs( daysSinceExpiry )
		       .subs( Kleo::DN( key.userID(0).id() ).prettyDN() )
		       .subs( QString::fromLatin1( key.issuerSerial() ) )
		       .toString() );
      d->alreadyWarnedFingerprints.insert( subkey.fingerprint() );
      if ( KMessageBox::warningContinueCancel( 0, msg,
                                               key.protocol() == GpgME::OpenPGP
                                               ? i18n("OpenPGP Key Expired" )
                                               : i18n("S/MIME Certificate Expired" ),
                                               KStandardGuiItem::cont(), KStandardGuiItem::cancel(), QLatin1String(dontAskAgainName) ) == KMessageBox::Cancel )
          return Kpgp::Canceled;
  } else {
  const int daysTillExpiry = 1 + int( secsTillExpiry / secsPerDay );
  kDebug() << "Key 0x" << key.shortKeyID() <<"expires in less than"
	           << daysTillExpiry << "days";
  const int threshold =
    ca
    ? ( key.isRoot()
	? ( sign
	    ? signingRootCertNearExpiryWarningThresholdInDays()
	    : encryptRootCertNearExpiryWarningThresholdInDays() )
	: ( sign
	    ? signingChainCertNearExpiryWarningThresholdInDays()
	    : encryptChainCertNearExpiryWarningThresholdInDays() ) )
    : ( sign
	? signingKeyNearExpiryWarningThresholdInDays()
	: encryptKeyNearExpiryWarningThresholdInDays() );
  if ( threshold > -1 && daysTillExpiry <= threshold ) {
    const QString msg =
      key.protocol() == GpgME::OpenPGP
      ? ( mine ? sign
	  ? ki18np("<p>Your OpenPGP signing key</p><p align=\"center\"><b>%2</b> (KeyID 0x%3)</p>"
		   "<p>expires in less than a day.</p>",
		   "<p>Your OpenPGP signing key</p><p align=\"center\"><b>%2</b> (KeyID 0x%3)</p>"
		   "<p>expires in less than %1 days.</p>")
	  : ki18np("<p>Your OpenPGP encryption key</p><p align=\"center\"><b>%2</b> (KeyID 0x%3)</p>"
		   "<p>expires in less than a day.</p>",
		   "<p>Your OpenPGP encryption key</p><p align=\"center\"><b>%2</b> (KeyID 0x%3)</p>"
		   "<p>expires in less than %1 days.</p>")
	  : ki18np("<p>The OpenPGP key for</p><p align=\"center\"><b>%2</b> (KeyID 0x%3)</p>"
		   "<p>expires in less than a day.</p>",
		   "<p>The OpenPGP key for</p><p align=\"center\"><b>%2</b> (KeyID 0x%3)</p>"
		   "<p>expires in less than %1 days.</p>") )
		  .subs( daysTillExpiry )
		  .subs( QString::fromUtf8( key.userID(0).id() ) )
		  .subs( QString::fromLatin1( key.shortKeyID() ) )
		  .toString()
      : ( ca
	  ? ( key.isRoot()
	      ? ( mine ? sign
		  ? ki18np("<p>The root certificate</p><p align=\"center\"><b>%4</b></p>"
			   "<p>for your S/MIME signing certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
			   "<p>expires in less than a day.</p>",
			   "<p>The root certificate</p><p align=\"center\"><b>%4</b></p>"
			   "<p>for your S/MIME signing certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
			   "<p>expires in less than %1 days.</p>")
		  : ki18np("<p>The root certificate</p><p align=\"center\"><b>%4</b></p>"
			   "<p>for your S/MIME encryption certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
			   "<p>expires in less than a day.</p>",
			   "<p>The root certificate</p><p align=\"center\"><b>%4</b></p>"
			   "<p>for your S/MIME encryption certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
			   "<p>expires in less than %1 days.</p>")
		  : ki18np("<p>The root certificate</p><p align=\"center\"><b>%4</b></p>"
			   "<p>for S/MIME certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
			   "<p>expires in less than a day.</p>",
			   "<p>The root certificate</p><p align=\"center\"><b>%4</b></p>"
			   "<p>for S/MIME certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
			   "<p>expires in less than %1 days.</p>") )
	      : ( mine ? sign
		  ? ki18np("<p>The intermediate CA certificate</p><p align=\"center\"><b>%4</b></p>"
			   "<p>for your S/MIME signing certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
			   "<p>expires in less than a day.</p>",
			   "<p>The intermediate CA certificate</p><p align=\"center\"><b>%4</b></p>"
			   "<p>for your S/MIME signing certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
			   "<p>expires in less than %1 days.</p>")
		  : ki18np("<p>The intermediate CA certificate</p><p align=\"center\"><b>%4</b></p>"
			   "<p>for your S/MIME encryption certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
			   "<p>expires in less than a day.</p>",
			   "<p>The intermediate CA certificate</p><p align=\"center\"><b>%4</b></p>"
			   "<p>for your S/MIME encryption certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
			   "<p>expires in less than %1 days.</p>")
		  : ki18np("<p>The intermediate CA certificate</p><p align=\"center\"><b>%4</b></p>"
			   "<p>for S/MIME certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
			   "<p>expires in less than a day.</p>",
			   "<p>The intermediate CA certificate</p><p align=\"center\"><b>%4</b></p>"
			   "<p>for S/MIME certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
			   "<p>expires in less than %1 days.</p>") ) )
			   .subs( daysTillExpiry )
			   .subs( Kleo::DN( orig.userID(0).id() ).prettyDN() )
			   .subs( QString::fromLatin1( orig.issuerSerial() ) )
			   .subs( Kleo::DN( key.userID(0).id() ).prettyDN() )
			   .toString()
	  : ( mine ? sign
	      ? ki18np("<p>Your S/MIME signing certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
		       "<p>expires in less than a day.</p>",
		       "<p>Your S/MIME signing certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
		       "<p>expires in less than %1 days.</p>")
	      : ki18np("<p>Your S/MIME encryption certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
		       "<p>expires in less than a day.</p>",
		       "<p>Your S/MIME encryption certificate</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
		       "<p>expires in less than %1 days.</p>")
	      : ki18np("<p>The S/MIME certificate for</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
		       "<p>expires in less than a day.</p>",
		       "<p>The S/MIME certificate for</p><p align=\"center\"><b>%2</b> (serial number %3)</p>"
		       "<p>expires in less than %1 days.</p>" ) )
		       .subs( daysTillExpiry )
		       .subs( Kleo::DN( key.userID(0).id() ).prettyDN() )
		       .subs( QString::fromLatin1( key.issuerSerial() ) )
		       .toString() );
    d->alreadyWarnedFingerprints.insert( subkey.fingerprint() );
    if ( KMessageBox::warningContinueCancel( 0, msg,
					     key.protocol() == GpgME::OpenPGP
					     ? i18n("OpenPGP Key Expires Soon" )
					     : i18n("S/MIME Certificate Expires Soon" ),
					     KStandardGuiItem::cont(), KStandardGuiItem::cancel(),
					     QLatin1String( dontAskAgainName ) )
	 == KMessageBox::Cancel )
      return Kpgp::Canceled;
  }
  }
  if ( key.isRoot() )
    return Kpgp::Ok;
  else if ( const char * chain_id = key.chainID() ) {
    const std::vector<GpgME::Key> issuer = lookup( QStringList( QLatin1String( chain_id ) ), false );
    if ( issuer.empty() )
      return Kpgp::Ok;
    else
      return checkKeyNearExpiry( issuer.front(), dontAskAgainName, mine, sign,
				 true, recur_limit-1, ca ? orig : key );
  }
  return Kpgp::Ok;
}

Kpgp::Result Kleo::KeyResolver::setEncryptToSelfKeys( const QStringList & fingerprints ) {
  if ( !encryptToSelf() )
    return Kpgp::Ok;

  std::vector<GpgME::Key> keys = lookup( fingerprints );
  std::remove_copy_if( keys.begin(), keys.end(),
		       std::back_inserter( d->mOpenPGPEncryptToSelfKeys ),
		       NotValidTrustedOpenPGPEncryptionKey ); // -= trusted?
  std::remove_copy_if( keys.begin(), keys.end(),
		       std::back_inserter( d->mSMIMEEncryptToSelfKeys ),
		       NotValidTrustedSMIMEEncryptionKey );   // -= trusted?

  if ( d->mOpenPGPEncryptToSelfKeys.size() + d->mSMIMEEncryptToSelfKeys.size()
       < keys.size() ) {
    // too few keys remain...
    const QString msg = i18n("One or more of your configured OpenPGP encryption "
			     "keys or S/MIME certificates is not usable for "
			     "encryption. Please reconfigure your encryption keys "
			     "and certificates for this identity in the identity "
			     "configuration dialog.\n"
			     "If you choose to continue, and the keys are needed "
			     "later on, you will be prompted to specify the keys "
			     "to use.");
    return KMessageBox::warningContinueCancel( 0, msg, i18n("Unusable Encryption Keys"),
					       KStandardGuiItem::cont(), KStandardGuiItem::cancel(),
					       QLatin1String("unusable own encryption key warning") )
      == KMessageBox::Continue ? Kpgp::Ok : Kpgp::Canceled ;
  }

  // check for near-expiry:

  for ( std::vector<GpgME::Key>::const_iterator it = d->mOpenPGPEncryptToSelfKeys.begin() ; it != d->mOpenPGPEncryptToSelfKeys.end() ; ++it ) {
    const Kpgp::Result r = checkKeyNearExpiry( *it, "own encryption key expires soon warning",
					       true, false );
    if ( r != Kpgp::Ok )
      return r;
  }

  for ( std::vector<GpgME::Key>::const_iterator it = d->mSMIMEEncryptToSelfKeys.begin() ; it != d->mSMIMEEncryptToSelfKeys.end() ; ++it ) {
    const Kpgp::Result r = checkKeyNearExpiry( *it, "own encryption key expires soon warning",
					       true, false );
    if ( r != Kpgp::Ok )
      return r;
  }

  return Kpgp::Ok;
}

Kpgp::Result Kleo::KeyResolver::setSigningKeys( const QStringList & fingerprints ) {
  std::vector<GpgME::Key> keys = lookup( fingerprints, true ); // secret keys
  std::remove_copy_if( keys.begin(), keys.end(),
		       std::back_inserter( d->mOpenPGPSigningKeys ),
		       NotValidOpenPGPSigningKey );
  std::remove_copy_if( keys.begin(), keys.end(),
		       std::back_inserter( d->mSMIMESigningKeys ),
		       NotValidSMIMESigningKey );

  if ( d->mOpenPGPSigningKeys.size() + d->mSMIMESigningKeys.size() < keys.size() ) {
    // too few keys remain...
    const QString msg = i18n("One or more of your configured OpenPGP signing keys "
			     "or S/MIME signing certificates is not usable for "
			     "signing. Please reconfigure your signing keys "
			     "and certificates for this identity in the identity "
			     "configuration dialog.\n"
			     "If you choose to continue, and the keys are needed "
			     "later on, you will be prompted to specify the keys "
			     "to use.");
    return KMessageBox::warningContinueCancel( 0, msg, i18n("Unusable Signing Keys"),
					       KStandardGuiItem::cont(), KStandardGuiItem::cancel(),
					       QLatin1String("unusable signing key warning") )
      == KMessageBox::Continue ? Kpgp::Ok : Kpgp::Canceled ;
  }

  // check for near expiry:

  for ( std::vector<GpgME::Key>::const_iterator it = d->mOpenPGPSigningKeys.begin() ; it != d->mOpenPGPSigningKeys.end() ; ++it ) {
    const Kpgp::Result r = checkKeyNearExpiry( *it, "signing key expires soon warning",
					       true, true );
    if ( r != Kpgp::Ok )
      return r;
  }

  for ( std::vector<GpgME::Key>::const_iterator it = d->mSMIMESigningKeys.begin() ; it != d->mSMIMESigningKeys.end() ; ++it ) {
    const Kpgp::Result r = checkKeyNearExpiry( *it, "signing key expires soon warning",
					       true, true );
    if ( r != Kpgp::Ok )
      return r;
  }

  return Kpgp::Ok;
}

void Kleo::KeyResolver::setPrimaryRecipients( const QStringList & addresses ) {
  d->mPrimaryEncryptionKeys = getEncryptionItems( addresses );
}

void Kleo::KeyResolver::setSecondaryRecipients( const QStringList & addresses ) {
  d->mSecondaryEncryptionKeys = getEncryptionItems( addresses );
}

std::vector<Kleo::KeyResolver::Item> Kleo::KeyResolver::getEncryptionItems( const QStringList & addresses ) {
  std::vector<Item> items;
  items.reserve( addresses.size() );
  for ( QStringList::const_iterator it = addresses.begin() ; it != addresses.end() ; ++it ) {
    QString addr = canonicalAddress( *it ).toLower();
    const ContactPreferences pref = lookupContactPreferences( addr );

    items.push_back( Item( *it, /*getEncryptionKeys( *it, true ),*/
			   pref.encryptionPreference,
			   pref.signingPreference,
			   pref.cryptoMessageFormat ) );
  }
  return items;
}

static Kleo::Action action( bool doit, bool ask, bool donot, bool requested ) {
  if ( requested && !donot )
    return Kleo::DoIt;
  if ( doit && !ask && !donot )
    return Kleo::DoIt;
  if ( !doit && ask && !donot )
    return Kleo::Ask;
  if ( !doit && !ask && donot )
    return requested ? Kleo::Conflict : Kleo::DontDoIt ;
  if ( !doit && !ask && !donot )
    return Kleo::DontDoIt ;
  return Kleo::Conflict;
}

Kleo::Action Kleo::KeyResolver::checkSigningPreferences( bool signingRequested ) const {

  if ( signingRequested && d->mOpenPGPSigningKeys.empty() && d->mSMIMESigningKeys.empty() )
    return Impossible;

  SigningPreferenceCounter count;
  count = std::for_each( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
			 count );
  count = std::for_each( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
			 count );

  unsigned int sign = count.numAlwaysSign();
  unsigned int ask = count.numAlwaysAskForSigning();
  const unsigned int dontSign = count.numNeverSign();
  if ( signingPossible() ) {
    sign += count.numAlwaysSignIfPossible();
    ask += count.numAskSigningWheneverPossible();
  }

  return action( sign, ask, dontSign, signingRequested );
}

bool Kleo::KeyResolver::signingPossible() const {
  return !d->mOpenPGPSigningKeys.empty() || !d->mSMIMESigningKeys.empty() ;
}

Kleo::Action Kleo::KeyResolver::checkEncryptionPreferences( bool encryptionRequested ) const {

  if ( d->mPrimaryEncryptionKeys.empty() && d->mSecondaryEncryptionKeys.empty() )
    return DontDoIt;

  if ( encryptionRequested && encryptToSelf() &&
       d->mOpenPGPEncryptToSelfKeys.empty() && d->mSMIMEEncryptToSelfKeys.empty() )
    return Impossible;

  if ( !encryptionRequested && !mOpportunisticEncyption ) {
    // try to minimize crypto ops (including key lookups) by only
    // looking up keys when at least one the the encryption
    // preferences needs it:
    EncryptionPreferenceCounter count( 0, UnknownPreference );
    count.process( d->mPrimaryEncryptionKeys );
    count.process( d->mSecondaryEncryptionKeys );
    if ( !count.numAlwaysEncrypt() &&
         !count.numAlwaysAskForEncryption() && // this guy might not need a lookup, when declined, but it's too complex to implement that here
         !count.numAlwaysEncryptIfPossible() &&
         !count.numAskWheneverPossible() )
        return DontDoIt;
  }

  EncryptionPreferenceCounter count( this, mOpportunisticEncyption ? AskWheneverPossible : UnknownPreference );
  count = std::for_each( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
			 count );
  count = std::for_each( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
			 count );

  unsigned int encrypt = count.numAlwaysEncrypt();
  unsigned int ask = count.numAlwaysAskForEncryption();
  const unsigned int dontEncrypt = count.numNeverEncrypt() + count.numNoKey();
  if ( encryptionPossible() ) {
    encrypt += count.numAlwaysEncryptIfPossible();
    ask += count.numAskWheneverPossible();
  }

  const Action act = action( encrypt, ask, dontEncrypt, encryptionRequested );
  if ( act != Ask ||
       std::for_each( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
       std::for_each( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
		      EncryptionPreferenceCounter( this, UnknownPreference ) ) ).numAlwaysAskForEncryption() )
    return act;
  else
    return AskOpportunistic;
}

bool Kleo::KeyResolver::encryptionPossible() const {
  return std::find_if( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
		       EmptyKeyList ) == d->mPrimaryEncryptionKeys.end()
    &&   std::find_if( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
		       EmptyKeyList ) == d->mSecondaryEncryptionKeys.end() ;
}

Kpgp::Result Kleo::KeyResolver::resolveAllKeys( bool& signingRequested, bool& encryptionRequested ) {
  if ( !encryptionRequested && !signingRequested ) {
    // make a dummy entry with all recipients, but no signing or
    // encryption keys to avoid special-casing on the caller side:
    dump();
    d->mFormatInfoMap[OpenPGPMIMEFormat].splitInfos.push_back( SplitInfo( allRecipients() ) );
    dump();
    return Kpgp::Ok;
  }
  Kpgp::Result result = Kpgp::Ok;
  if ( encryptionRequested )
    result = resolveEncryptionKeys( signingRequested );
  if ( result != Kpgp::Ok )
    return result;
  if ( signingRequested ) {
    if ( encryptionRequested ) {
      result = resolveSigningKeysForEncryption();
    }
    else {
      result = resolveSigningKeysForSigningOnly();
      if ( result == Kpgp::Failure ) {
        signingRequested = false;
        return Kpgp::Ok;
      }
    }
  }
  return result;
}

Kpgp::Result Kleo::KeyResolver::resolveEncryptionKeys( bool signingRequested ) {
  //
  // 1. Get keys for all recipients:
  //
  kDebug() << "resolving enc keys";
  for ( std::vector<Item>::iterator it = d->mPrimaryEncryptionKeys.begin() ; it != d->mPrimaryEncryptionKeys.end() ; ++it ) {
    kDebug() << "checking primary:" << it->address;
    if ( !it->needKeys )
      continue;
    it->keys = getEncryptionKeys( it->address, false );
    kDebug() << "got # keys:" << it->keys.size();
    if ( it->keys.empty() )
      return Kpgp::Canceled;
    QString addr = canonicalAddress( it->address ).toLower();
    const ContactPreferences pref = lookupContactPreferences( addr );
    it->pref = pref.encryptionPreference;
    it->signPref = pref.signingPreference;
    it->format = pref.cryptoMessageFormat;
    kDebug() << "set key data:" << int( it->pref ) << int( it->signPref ) << int( it->format );
  }

  for ( std::vector<Item>::iterator it = d->mSecondaryEncryptionKeys.begin() ; it != d->mSecondaryEncryptionKeys.end() ; ++it ) {
    if ( !it->needKeys )
      continue;
    it->keys = getEncryptionKeys( it->address, false );
    if ( it->keys.empty() )
      return Kpgp::Canceled;
    QString addr = canonicalAddress( it->address ).toLower();
    const ContactPreferences pref = lookupContactPreferences( addr );
    it->pref = pref.encryptionPreference;
    it->signPref = pref.signingPreference;
    it->format = pref.cryptoMessageFormat;
  }

  // 1a: Present them to the user

  const Kpgp::Result res = showKeyApprovalDialog();
  if ( res != Kpgp::Ok )
    return res;

  //
  // 2. Check what the primary recipients need
  //

  // 2a. Try to find a common format for all primary recipients,
  //     else use as many formats as needed

  const EncryptionFormatPreferenceCounter primaryCount
    = std::for_each( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
		     EncryptionFormatPreferenceCounter() );

  CryptoMessageFormat commonFormat = AutoFormat;
  for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i ) {
    if ( !( concreteCryptoMessageFormats[i] & mCryptoMessageFormats ) )
      continue;
    if ( signingRequested && signingKeysFor( concreteCryptoMessageFormats[i] ).empty() )
      continue;
    if ( encryptToSelf() && encryptToSelfKeysFor( concreteCryptoMessageFormats[i] ).empty() )
      continue;
    if ( primaryCount.numOf( concreteCryptoMessageFormats[i] ) == primaryCount.numTotal() ) {
      commonFormat = concreteCryptoMessageFormats[i];
      break;
    }
  }
  kDebug() << "got commonFormat for primary recipients:" << int( commonFormat );
  if ( commonFormat != AutoFormat )
    addKeys( d->mPrimaryEncryptionKeys, commonFormat );
  else
    addKeys( d->mPrimaryEncryptionKeys );

  collapseAllSplitInfos(); // these can be encrypted together

  // 2b. Just try to find _something_ for each secondary recipient,
  //     with a preference to a common format (if that exists)

  const EncryptionFormatPreferenceCounter secondaryCount
    = std::for_each( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
		     EncryptionFormatPreferenceCounter() );

  if ( commonFormat != AutoFormat &&
       secondaryCount.numOf( commonFormat ) == secondaryCount.numTotal() )
    addKeys( d->mSecondaryEncryptionKeys, commonFormat );
  else
    addKeys( d->mSecondaryEncryptionKeys );

  // 3. Check for expiry:

  for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i ) {
    const std::vector<SplitInfo> si_list = encryptionItems( concreteCryptoMessageFormats[i] );
    for ( std::vector<SplitInfo>::const_iterator sit = si_list.begin() ; sit != si_list.end() ; ++sit )
      for ( std::vector<GpgME::Key>::const_iterator kit = sit->keys.begin() ; kit != sit->keys.end() ; ++kit ) {
	const Kpgp::Result r = checkKeyNearExpiry( *kit, "other encryption key near expiry warning",
						   false, false );
	if ( r != Kpgp::Ok )
	  return r;
      }
  }

  // 4. Check that we have the right keys for encryptToSelf()

  if ( !encryptToSelf() )
    return Kpgp::Ok;

  // 4a. Check for OpenPGP keys

  kDebug() << "sizes of encryption items:" << encryptionItems( InlineOpenPGPFormat ).size() << encryptionItems( OpenPGPMIMEFormat ).size() << encryptionItems( SMIMEFormat ).size() << encryptionItems( SMIMEOpaqueFormat ).size();
  if ( !encryptionItems( InlineOpenPGPFormat ).empty() ||
       !encryptionItems( OpenPGPMIMEFormat ).empty() ) {
    // need them
    if ( d->mOpenPGPEncryptToSelfKeys.empty() ) {
      const QString msg = i18n("Examination of recipient's encryption preferences "
			       "yielded that the message should be encrypted using "
			       "OpenPGP, at least for some recipients;\n"
			       "however, you have not configured valid trusted "
			       "OpenPGP encryption keys for this identity.\n"
			       "You may continue without encrypting to yourself, "
			       "but be aware that you will not be able to read your "
			       "own messages if you do so.");
      if ( KMessageBox::warningContinueCancel( 0, msg,
					       i18n("Unusable Encryption Keys"),
					       KStandardGuiItem::cont(), KStandardGuiItem::cancel(),
					       QLatin1String("encrypt-to-self will fail warning") )
	   == KMessageBox::Cancel )
	return Kpgp::Canceled;
      // FIXME: Allow selection
    }
    addToAllSplitInfos( d->mOpenPGPEncryptToSelfKeys,
			InlineOpenPGPFormat|OpenPGPMIMEFormat );
  }

  // 4b. Check for S/MIME certs:

  if ( !encryptionItems( SMIMEFormat ).empty() ||
       !encryptionItems( SMIMEOpaqueFormat ).empty() ) {
    // need them
    if ( d->mSMIMEEncryptToSelfKeys.empty() ) {
      // don't have one
      const QString msg = i18n("Examination of recipient's encryption preferences "
			       "yielded that the message should be encrypted using "
			       "S/MIME, at least for some recipients;\n"
			       "however, you have not configured valid "
			       "S/MIME encryption certificates for this identity.\n"
			       "You may continue without encrypting to yourself, "
			       "but be aware that you will not be able to read your "
			       "own messages if you do so.");
      if ( KMessageBox::warningContinueCancel( 0, msg,
					       i18n("Unusable Encryption Keys"),
					       KStandardGuiItem::cont(), KStandardGuiItem::cancel(),
					       QLatin1String("encrypt-to-self will fail warning") )
	   == KMessageBox::Cancel )
	return Kpgp::Canceled;
      // FIXME: Allow selection
    }
    addToAllSplitInfos( d->mSMIMEEncryptToSelfKeys,
			SMIMEFormat|SMIMEOpaqueFormat );
  }

  // FIXME: Present another message if _both_ OpenPGP and S/MIME keys
  // are missing.

  return Kpgp::Ok;
}

Kpgp::Result Kleo::KeyResolver::resolveSigningKeysForEncryption() {
  if ( ( !encryptionItems( InlineOpenPGPFormat ).empty() ||
	 !encryptionItems( OpenPGPMIMEFormat ).empty() )
       && d->mOpenPGPSigningKeys.empty() ) {
    const QString msg = i18n("Examination of recipient's signing preferences "
			     "yielded that the message should be signed using "
			     "OpenPGP, at least for some recipients;\n"
			     "however, you have not configured valid "
			     "OpenPGP signing certificates for this identity.");
    if ( KMessageBox::warningContinueCancel( 0, msg,
					     i18n("Unusable Signing Keys"),
					     KGuiItem(i18n("Do Not OpenPGP-Sign")),
					     KStandardGuiItem::cancel(),
					     QLatin1String("signing will fail warning") )
	 == KMessageBox::Cancel )
      return Kpgp::Canceled;
    // FIXME: Allow selection
  }
  if ( ( !encryptionItems( SMIMEFormat ).empty() ||
	 !encryptionItems( SMIMEOpaqueFormat ).empty() )
       && d->mSMIMESigningKeys.empty() ) {
    const QString msg = i18n("Examination of recipient's signing preferences "
			     "yielded that the message should be signed using "
			     "S/MIME, at least for some recipients;\n"
			     "however, you have not configured valid "
			     "S/MIME signing certificates for this identity.");
    if ( KMessageBox::warningContinueCancel( 0, msg,
					     i18n("Unusable Signing Keys"),
					     KGuiItem(i18n("Do Not S/MIME-Sign")),
					     KStandardGuiItem::cancel(),
					     QLatin1String("signing will fail warning") )
	 == KMessageBox::Cancel )
      return Kpgp::Canceled;
    // FIXME: Allow selection
  }

  // FIXME: Present another message if _both_ OpenPGP and S/MIME keys
  // are missing.

  for ( std::map<CryptoMessageFormat,FormatInfo>::iterator it = d->mFormatInfoMap.begin() ; it != d->mFormatInfoMap.end() ; ++it )
    if ( !it->second.splitInfos.empty() ) {
      dump();
      it->second.signKeys = signingKeysFor( it->first );
      dump();
    }

  return Kpgp::Ok;
}

Kpgp::Result Kleo::KeyResolver::resolveSigningKeysForSigningOnly() {
  //
  // we don't need to distinguish between primary and secondary
  // recipients here:
  //
  SigningFormatPreferenceCounter count;
  count = std::for_each( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
			 count );
  count = std::for_each( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
			 count );

  // try to find a common format that works for all (and that we have signing keys for):

  CryptoMessageFormat commonFormat = AutoFormat;

  for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i ) {
    if ( !(mCryptoMessageFormats & concreteCryptoMessageFormats[i]) )
      continue; // skip
    if ( signingKeysFor( concreteCryptoMessageFormats[i] ).empty() )
      continue; // skip
    if ( count.numOf( concreteCryptoMessageFormats[i] ) == count.numTotal() ) {
      commonFormat = concreteCryptoMessageFormats[i];
      break;
    }
  }

  if ( commonFormat != AutoFormat ) { // found
    dump();
    FormatInfo & fi = d->mFormatInfoMap[ commonFormat ];
    fi.signKeys = signingKeysFor( commonFormat );
    fi.splitInfos.resize( 1 );
    fi.splitInfos.front() = SplitInfo( allRecipients() );
    dump();
    return Kpgp::Ok;
  }

  const QString msg = i18n("Examination of recipient's signing preferences "
                           "showed no common type of signature matching your "
                           "available signing keys.\n"
                           "Send message without signing?"  );
  if ( KMessageBox::warningContinueCancel( 0, msg, i18n("No signing possible"),
                                           KStandardGuiItem::cont() )
       == KMessageBox::Continue ) {
    d->mFormatInfoMap[OpenPGPMIMEFormat].splitInfos.push_back( SplitInfo( allRecipients() ) );
    return Kpgp::Failure; // means "Ok, but without signing"
  }
  return Kpgp::Canceled;
}

std::vector<GpgME::Key> Kleo::KeyResolver::signingKeysFor( CryptoMessageFormat f ) const {
  if ( isOpenPGP( f ) )
    return d->mOpenPGPSigningKeys;
  if ( isSMIME( f ) )
    return d->mSMIMESigningKeys;
  return std::vector<GpgME::Key>();
}

std::vector<GpgME::Key> Kleo::KeyResolver::encryptToSelfKeysFor( CryptoMessageFormat f ) const {
  if ( isOpenPGP( f ) )
    return d->mOpenPGPEncryptToSelfKeys;
  if ( isSMIME( f ) )
    return d->mSMIMEEncryptToSelfKeys;
  return std::vector<GpgME::Key>();
}

QStringList Kleo::KeyResolver::allRecipients() const {
  QStringList result;
  std::transform( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
		  std::back_inserter( result ), ItemDotAddress );
  std::transform( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
		  std::back_inserter( result ), ItemDotAddress );
  return result;
}

void Kleo::KeyResolver::collapseAllSplitInfos() {
  dump();
  for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i ) {
    std::map<CryptoMessageFormat,FormatInfo>::iterator pos =
      d->mFormatInfoMap.find( concreteCryptoMessageFormats[i] );
    if ( pos == d->mFormatInfoMap.end() )
      continue;
    std::vector<SplitInfo> & v = pos->second.splitInfos;
    if ( v.size() < 2 )
      continue;
    SplitInfo & si = v.front();
    for ( std::vector<SplitInfo>::const_iterator it = v.begin() + 1; it != v.end() ; ++it ) {
      si.keys.insert( si.keys.end(), it->keys.begin(), it->keys.end() );
      qCopy( it->recipients.begin(), it->recipients.end(), std::back_inserter( si.recipients ) );
    }
    v.resize( 1 );
  }
  dump();
}

void Kleo::KeyResolver::addToAllSplitInfos( const std::vector<GpgME::Key> & keys, unsigned int f ) {
  dump();
  if ( !f || keys.empty() )
    return;
  for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i ) {
    if ( !( f & concreteCryptoMessageFormats[i] ) )
      continue;
    std::map<CryptoMessageFormat,FormatInfo>::iterator pos =
      d->mFormatInfoMap.find( concreteCryptoMessageFormats[i] );
    if ( pos == d->mFormatInfoMap.end() )
      continue;
    std::vector<SplitInfo> & v = pos->second.splitInfos;
    for ( std::vector<SplitInfo>::iterator it = v.begin() ; it != v.end() ; ++it )
      it->keys.insert( it->keys.end(), keys.begin(), keys.end() );
  }
  dump();
}

void Kleo::KeyResolver::dump() const {
#ifndef NDEBUG
  if ( d->mFormatInfoMap.empty() )
    std::cerr << "Keyresolver: Format info empty" << std::endl;
  for ( std::map<CryptoMessageFormat,FormatInfo>::const_iterator it = d->mFormatInfoMap.begin() ; it != d->mFormatInfoMap.end() ; ++it ) {
    std::cerr << "Format info for " << Kleo::cryptoMessageFormatToString( it->first )
	      << ":" << std::endl
	      << "  Signing keys: ";
    for ( std::vector<GpgME::Key>::const_iterator sit = it->second.signKeys.begin() ; sit != it->second.signKeys.end() ; ++sit )
      std::cerr << sit->shortKeyID() << " ";
    std::cerr << std::endl;
    unsigned int i = 0;
    for ( std::vector<SplitInfo>::const_iterator sit = it->second.splitInfos.begin() ; sit != it->second.splitInfos.end() ; ++sit, ++i ) {
      std::cerr << "  SplitInfo #" << i << " encryption keys: ";
      for ( std::vector<GpgME::Key>::const_iterator kit = sit->keys.begin() ; kit != sit->keys.end() ; ++kit )
	std::cerr << kit->shortKeyID() << " ";
      std::cerr << std::endl
		<< "  SplitInfo #" << i << " recipients: "
		<< qPrintable(sit->recipients.join( QLatin1String(", ") )) << std::endl;
    }
  }
#endif
}

Kpgp::Result Kleo::KeyResolver::showKeyApprovalDialog() {
  const bool showKeysForApproval = showApprovalDialog()
    || std::find_if( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
		     ApprovalNeeded ) != d->mPrimaryEncryptionKeys.end()
    || std::find_if( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
		     ApprovalNeeded ) != d->mSecondaryEncryptionKeys.end() ;

  if ( !showKeysForApproval )
    return Kpgp::Ok;

  std::vector<Kleo::KeyApprovalDialog::Item> items;
  items.reserve( d->mPrimaryEncryptionKeys.size() +
	         d->mSecondaryEncryptionKeys.size() );
  std::copy( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
	     std::back_inserter( items ) );
  std::copy( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
	     std::back_inserter( items ) );

  std::vector<GpgME::Key> senderKeys;
  senderKeys.reserve( d->mOpenPGPEncryptToSelfKeys.size() +
	              d->mSMIMEEncryptToSelfKeys.size() );
  std::copy( d->mOpenPGPEncryptToSelfKeys.begin(), d->mOpenPGPEncryptToSelfKeys.end(),
	     std::back_inserter( senderKeys ) );
  std::copy( d->mSMIMEEncryptToSelfKeys.begin(), d->mSMIMEEncryptToSelfKeys.end(),
	     std::back_inserter( senderKeys ) );

#ifndef QT_NO_CURSOR
  const MessageViewer::KCursorSaver idle( MessageViewer::KBusyPtr::idle() );
#endif

  Kleo::KeyApprovalDialog dlg( items, senderKeys );

  if ( dlg.exec() == QDialog::Rejected )
    return Kpgp::Canceled;

  items = dlg.items();
  senderKeys = dlg.senderKeys();

  if ( dlg.preferencesChanged() ) {
    for ( uint i = 0; i < items.size(); ++i ) {
      ContactPreferences pref = lookupContactPreferences( items[i].address );
      pref.encryptionPreference = items[i].pref;
      pref.pgpKeyFingerprints.clear();
      pref.smimeCertFingerprints.clear();
      const std::vector<GpgME::Key> & keys = items[i].keys;
      for ( std::vector<GpgME::Key>::const_iterator it = keys.begin(), end = keys.end() ; it != end ; ++it ) {
        if ( it->protocol() == GpgME::OpenPGP ) {
          if ( const char * fpr = it->primaryFingerprint() )
            pref.pgpKeyFingerprints.push_back( QLatin1String( fpr ) );
        } else if ( it->protocol() == GpgME::CMS ) {
          if ( const char * fpr = it->primaryFingerprint() )
            pref.smimeCertFingerprints.push_back( QLatin1String( fpr ) );
        }
      }
      saveContactPreference( items[i].address, pref );
    }
  }

  // show a warning if the user didn't select an encryption key for
  // herself:
  if ( encryptToSelf() && senderKeys.empty() ) {
    const QString msg = i18n("You did not select an encryption key for yourself "
			     "(encrypt to self). You will not be able to decrypt "
			     "your own message if you encrypt it.");
    if ( KMessageBox::warningContinueCancel( 0, msg,
					     i18n("Missing Key Warning"),
					     KGuiItem(i18n("&Encrypt")) )
	 == KMessageBox::Cancel )
      return Kpgp::Canceled;
    else
      mEncryptToSelf = false;
  }

  // count empty key ID lists
  const unsigned int emptyListCount =
    std::count_if( items.begin(), items.end(), EmptyKeyList );

  // show a warning if the user didn't select an encryption key for
  // some of the recipients
  if ( items.size() == emptyListCount  ) {
    const QString msg = ( d->mPrimaryEncryptionKeys.size() +
			  d->mSecondaryEncryptionKeys.size() == 1 )
                  ? i18n("You did not select an encryption key for the "
                         "recipient of this message; therefore, the message "
                         "will not be encrypted.")
                  : i18n("You did not select an encryption key for any of the "
                         "recipients of this message; therefore, the message "
                         "will not be encrypted.");
    if ( KMessageBox::warningContinueCancel( 0, msg,
					     i18n("Missing Key Warning"),
					     KGuiItem(i18n("Send &Unencrypted")) )
	 == KMessageBox::Cancel )
      return Kpgp::Canceled;
  } else if ( emptyListCount > 0 ) {
    const QString msg = ( emptyListCount == 1 )
                  ? i18n("You did not select an encryption key for one of "
                         "the recipients: this person will not be able to "
                         "decrypt the message if you encrypt it.")
                  : i18n("You did not select encryption keys for some of "
                         "the recipients: these persons will not be able to "
                         "decrypt the message if you encrypt it." );
#ifndef QT_NO_CURSOR
    MessageViewer::KCursorSaver idle( MessageViewer::KBusyPtr::idle() );
#endif
    if ( KMessageBox::warningContinueCancel( 0, msg,
                                             i18n("Missing Key Warning"),
                                             KGuiItem(i18n("&Encrypt")) )
         == KMessageBox::Cancel )
      return Kpgp::Canceled;
  }

  std::transform( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
                  items.begin(),
                  d->mPrimaryEncryptionKeys.begin(),
                  CopyKeysAndEncryptionPreferences );
  std::transform( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
                  items.begin() + d->mPrimaryEncryptionKeys.size(),
                  d->mSecondaryEncryptionKeys.begin(),
                  CopyKeysAndEncryptionPreferences );

  d->mOpenPGPEncryptToSelfKeys.clear();
  d->mSMIMEEncryptToSelfKeys.clear();

  std::remove_copy_if( senderKeys.begin(), senderKeys.end(),
                       std::back_inserter( d->mOpenPGPEncryptToSelfKeys ),
                       NotValidTrustedOpenPGPEncryptionKey ); // -= trusted (see above, too)?
  std::remove_copy_if( senderKeys.begin(), senderKeys.end(),
                       std::back_inserter( d->mSMIMEEncryptToSelfKeys ),
                       NotValidTrustedSMIMEEncryptionKey );   // -= trusted (see above, too)?

  return Kpgp::Ok;
}

std::vector<Kleo::KeyResolver::SplitInfo> Kleo::KeyResolver::encryptionItems( Kleo::CryptoMessageFormat f ) const {
  dump();
  std::map<CryptoMessageFormat,FormatInfo>::const_iterator it =
    d->mFormatInfoMap.find( f );
  return it != d->mFormatInfoMap.end() ? it->second.splitInfos : std::vector<SplitInfo>() ;
}

std::vector<GpgME::Key> Kleo::KeyResolver::signingKeys( CryptoMessageFormat f ) const {
  dump();
  std::map<CryptoMessageFormat,FormatInfo>::const_iterator it =
    d->mFormatInfoMap.find( f );
  return it != d->mFormatInfoMap.end() ? it->second.signKeys : std::vector<GpgME::Key>() ;
}

//
//
// Private helper methods below:
//
//


std::vector<GpgME::Key> Kleo::KeyResolver::selectKeys( const QString & person, const QString & msg, const std::vector<GpgME::Key> & selectedKeys ) const {
  const bool opgp = containsOpenPGP( mCryptoMessageFormats );
  const bool x509 = containsSMIME( mCryptoMessageFormats );

  Kleo::KeySelectionDialog dlg( i18n("Encryption Key Selection"),
				msg, KPIMUtils::extractEmailAddress( person ), selectedKeys,
                                Kleo::KeySelectionDialog::ValidEncryptionKeys
                                & ~(opgp ? 0 : Kleo::KeySelectionDialog::OpenPGPKeys)
                                & ~(x509 ? 0 : Kleo::KeySelectionDialog::SMIMEKeys),
				true, true ); // multi-selection and "remember choice" box

  if ( dlg.exec() != QDialog::Accepted )
    return std::vector<GpgME::Key>();
  std::vector<GpgME::Key> keys = dlg.selectedKeys();
  keys.erase( std::remove_if( keys.begin(), keys.end(),
                              NotValidTrustedEncryptionKey ), // -= trusted?
                              keys.end() );
  if ( !keys.empty() && dlg.rememberSelection() )
    setKeysForAddress( person, dlg.pgpKeyFingerprints(), dlg.smimeFingerprints() );
  return keys;
}


std::vector<GpgME::Key> Kleo::KeyResolver::getEncryptionKeys( const QString & person, bool quiet ) const {

  const QString address = canonicalAddress( person ).toLower();

  // First look for this person's address in the address->key dictionary
  const QStringList fingerprints = keysForAddress( address );

  if ( !fingerprints.empty() ) {
    kDebug() << "Using encryption keys 0x"
	             << fingerprints.join( QLatin1String(", 0x") )
	             << "for" << person;
    std::vector<GpgME::Key> keys = lookup( fingerprints );
    if ( !keys.empty() ) {
      // Check if all of the keys are trusted and valid encryption keys
      if ( std::find_if( keys.begin(), keys.end(),
                         NotValidTrustedEncryptionKey ) != keys.end() ) { // -= trusted?
        // not ok, let the user select: this is not conditional on !quiet,
        // since it's a bug in the configuration and the user should be
        // notified about it as early as possible:
        keys = selectKeys( person,
            i18nc("if in your language something like "
              "'certificate(s)' is not possible please "
              "use the plural in the translation",
              "There is a problem with the "
              "encryption certificate(s) for \"%1\".\n\n"
              "Please re-select the certificate(s) which should "
              "be used for this recipient.", person),
            keys );
      }
      bool canceled = false;
      keys = trustedOrConfirmed( keys, address, canceled );
      if ( canceled )
          return std::vector<GpgME::Key>();

      if ( !keys.empty() )
        return keys;
      // keys.empty() is considered cancel by callers, so go on
    }
  }

  // Now search all public keys for matching keys
  std::vector<GpgME::Key> matchingKeys = lookup( QStringList( address ) );
  matchingKeys.erase( std::remove_if( matchingKeys.begin(), matchingKeys.end(),
                      NotValidEncryptionKey ), matchingKeys.end() );

  // if called with quite == true (from EncryptionPreferenceCounter), we only want to
  // check if there are keys for this recipients, not (yet) their validity, so
  // don't show the untrusted encryption key warning in that case
  bool canceled = false;
  if ( !quiet )
    matchingKeys = trustedOrConfirmed( matchingKeys, address, canceled );
  if ( canceled )
    return std::vector<GpgME::Key>();
  if ( quiet || matchingKeys.size() == 1 )
    return matchingKeys;

  // no match until now, or more than one key matches; let the user
  // choose the key(s)
  // FIXME: let user get the key from keyserver
  return trustedOrConfirmed( selectKeys( person,
          matchingKeys.empty()
          ? i18nc( "if in your language something like "
                   "'certificate(s)' is not possible please "
                   "use the plural in the translation",
                   "<qt>No valid and trusted encryption certificate was "
                   "found for \"%1\".<br/><br/>"
                   "Select the certificate(s) which should "
                   "be used for this recipient. If there is no suitable certificate in the list "
                   "you can also search for external certificates by clicking the button: "
                   "search for external certificates.</qt>",
                   Qt::escape( person ) )
          : i18nc( "if in your language something like "
                   "'certificate(s)' is not possible please "
                   "use the plural in the translation",
                   "More than one certificate matches \"%1\".\n\n"
                   "Select the certificate(s) which should "
                   "be used for this recipient.", Qt::escape( person ) ),
          matchingKeys ), address, canceled );
  // we can ignore 'canceled' here, since trustedOrConfirmed() returns
  // an empty vector when canceled == true, and we'd just do the same
}


std::vector<GpgME::Key> Kleo::KeyResolver::lookup( const QStringList & patterns, bool secret ) const {
  if ( patterns.empty() )
    return std::vector<GpgME::Key>();
  kDebug() << "( \"" << patterns.join( QLatin1String("\", \"") ) << "\"," << secret << ")";
  std::vector<GpgME::Key> result;
  if ( mCryptoMessageFormats & (InlineOpenPGPFormat|OpenPGPMIMEFormat) )
    if ( const Kleo::CryptoBackend::Protocol * p = Kleo::CryptoBackendFactory::instance()->openpgp() ) {
      std::auto_ptr<Kleo::KeyListJob> job( p->keyListJob( false, false, true ) ); // use validating keylisting
      if ( job.get() ) {
	std::vector<GpgME::Key> keys;
	job->exec( patterns, secret, keys );
	result.insert( result.end(), keys.begin(), keys.end() );
      }
    }
  if ( mCryptoMessageFormats & (SMIMEFormat|SMIMEOpaqueFormat) )
    if ( const Kleo::CryptoBackend::Protocol * p = Kleo::CryptoBackendFactory::instance()->smime() ) {
      std::auto_ptr<Kleo::KeyListJob> job( p->keyListJob( false, false, true ) ); // use validating keylisting
      if ( job.get() ) {
	std::vector<GpgME::Key> keys;
	job->exec( patterns, secret, keys );
	result.insert( result.end(), keys.begin(), keys.end() );
      }
    }
  kDebug() << " returned" << result.size() << "keys";
  return result;
}

void Kleo::KeyResolver::addKeys( const std::vector<Item> & items, CryptoMessageFormat f ) {
  dump();
  for ( std::vector<Item>::const_iterator it = items.begin() ; it != items.end() ; ++it ) {
    SplitInfo si( QStringList( it->address ) );
    std::remove_copy_if( it->keys.begin(), it->keys.end(),
			 std::back_inserter( si.keys ), IsNotForFormat( f ) );
    dump();
    kWarning( si.keys.empty() )
      << "Kleo::KeyResolver::addKeys(): Fix EncryptionFormatPreferenceCounter."
      << "It detected a common format, but the list of such keys for recipient \""
      << it->address << "\" is empty!";
    d->mFormatInfoMap[ f ].splitInfos.push_back( si );
  }
  dump();
}

void Kleo::KeyResolver::addKeys( const std::vector<Item> & items ) {
  dump();
  for ( std::vector<Item>::const_iterator it = items.begin() ; it != items.end() ; ++it ) {
    SplitInfo si( QStringList( it->address ) );
    CryptoMessageFormat f = AutoFormat;
    for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i ) {
      const CryptoMessageFormat fmt = concreteCryptoMessageFormats[i];
      if ( ( fmt & it->format ) &&
           kdtools::any( it->keys.begin(), it->keys.end(), IsForFormat( fmt ) ) )
      {
        f = fmt;
        break;
      }
    }
    if ( f == AutoFormat )
      kWarning() << "Something went wrong. Didn't find a format for \""
                  << it->address << "\"";
    else
      std::remove_copy_if( it->keys.begin(), it->keys.end(),
                           std::back_inserter( si.keys ), IsNotForFormat( f ) );
    d->mFormatInfoMap[ f ].splitInfos.push_back( si );
  }
  dump();
}

Kleo::KeyResolver::ContactPreferences Kleo::KeyResolver::lookupContactPreferences( const QString& address ) const
{
  const Private::ContactPreferencesMap::iterator it =
    d->mContactPreferencesMap.find( address );
  if ( it != d->mContactPreferencesMap.end() )
    return it->second;

  Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob();
  job->setLimit( 1 );
  job->setQuery( Akonadi::ContactSearchJob::Email, address );
  job->exec();

  const KABC::Addressee::List res = job->contacts();
  ContactPreferences pref;
  if ( !res.isEmpty() ) {
    KABC::Addressee addr = res.first();
    QString encryptPref = addr.custom( QLatin1String("KADDRESSBOOK"), QLatin1String("CRYPTOENCRYPTPREF") );
    pref.encryptionPreference = Kleo::stringToEncryptionPreference( encryptPref );
    QString signPref = addr.custom( QLatin1String("KADDRESSBOOK"), QLatin1String("CRYPTOSIGNPREF") );
    pref.signingPreference = Kleo::stringToSigningPreference( signPref );
    QString cryptoFormats = addr.custom( QLatin1String("KADDRESSBOOK"), QLatin1String("CRYPTOPROTOPREF") );
    pref.cryptoMessageFormat = Kleo::stringToCryptoMessageFormat( cryptoFormats );
    pref.pgpKeyFingerprints = addr.custom( QLatin1String("KADDRESSBOOK"), QLatin1String("OPENPGPFP") ).split( QLatin1Char(','), QString::SkipEmptyParts );
    pref.smimeCertFingerprints = addr.custom( QLatin1String("KADDRESSBOOK"), QLatin1String("SMIMEFP") ).split( QLatin1Char(','), QString::SkipEmptyParts );
  }
  // insert into map and grab resulting iterator
  d->mContactPreferencesMap.insert( std::make_pair( address, pref ) );
  return pref;
}

void Kleo::KeyResolver::writeCustomContactProperties( KABC::Addressee &contact, const ContactPreferences& pref ) const
{
  contact.insertCustom( QLatin1String("KADDRESSBOOK"), QLatin1String("CRYPTOENCRYPTPREF"), QLatin1String( Kleo::encryptionPreferenceToString( pref.encryptionPreference ) ) );
  contact.insertCustom( QLatin1String("KADDRESSBOOK"), QLatin1String("CRYPTOSIGNPREF"), QLatin1String( Kleo::signingPreferenceToString( pref.signingPreference ) ) );
  contact.insertCustom( QLatin1String("KADDRESSBOOK"), QLatin1String("CRYPTOPROTOPREF"), QLatin1String( cryptoMessageFormatToString( pref.cryptoMessageFormat ) ) );
  contact.insertCustom( QLatin1String("KADDRESSBOOK"), QLatin1String("OPENPGPFP"), pref.pgpKeyFingerprints.join( QLatin1String(",") ) );
  contact.insertCustom( QLatin1String("KADDRESSBOOK"), QLatin1String("SMIMEFP"), pref.smimeCertFingerprints.join( QLatin1String(",") ) );
}

void Kleo::KeyResolver::saveContactPreference( const QString& email, const ContactPreferences& pref ) const
{
  d->mContactPreferencesMap.insert( std::make_pair( email, pref ) );

  Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob();
  job->setLimit( 1 );
  job->setQuery( Akonadi::ContactSearchJob::Email, email );
  job->exec();

  const Akonadi::Item::List items = job->items();

  if ( items.isEmpty() ) {
    bool ok = true;
    const QString fullName = KInputDialog::getText( i18n( "Name Selection" ), i18n( "Which name shall the contact '%1' have in your address book?", email ), QString(), &ok );
    if ( !ok )
      return;

    Akonadi::CollectionDialog dlg;
    dlg.setMimeTypeFilter( QStringList() << KABC::Addressee::mimeType() );
    dlg.setAccessRightsFilter( Akonadi::Collection::CanCreateItem );
    dlg.setDescription( i18n( "Select the address book folder to store the new contact in:" ) );
    if ( !dlg.exec() )
      return;

    const Akonadi::Collection targetCollection = dlg.selectedCollection();

    KABC::Addressee contact;
    contact.setNameFromString( fullName );
    contact.insertEmail( email, true );
    writeCustomContactProperties( contact, pref );

    Akonadi::Item item( KABC::Addressee::mimeType() );
    item.setPayload<KABC::Addressee>( contact );

    new Akonadi::ItemCreateJob( item, targetCollection );
  } else {
    Akonadi::Item item = items.first();

    KABC::Addressee contact = item.payload<KABC::Addressee>();
    writeCustomContactProperties( contact, pref );

    item.setPayload<KABC::Addressee>( contact );

    new Akonadi::ItemModifyJob( item );
  }

  // Assumption: 'pref' comes from d->mContactPreferencesMap already, no need to update that
}

Kleo::KeyResolver::ContactPreferences::ContactPreferences()
  : encryptionPreference( UnknownPreference ),
    signingPreference( UnknownSigningPreference ),
    cryptoMessageFormat( AutoFormat )
{
}

QStringList Kleo::KeyResolver::keysForAddress( const QString & address ) const {
  if( address.isEmpty() ) {
    return QStringList();
  }
  QString addr = canonicalAddress( address ).toLower();
  const ContactPreferences pref = lookupContactPreferences( addr );
  return pref.pgpKeyFingerprints + pref.smimeCertFingerprints;
}

void Kleo::KeyResolver::setKeysForAddress( const QString& address, const QStringList& pgpKeyFingerprints, const QStringList& smimeCertFingerprints ) const {
  if( address.isEmpty() ) {
    return;
  }
  QString addr = canonicalAddress( address ).toLower();
  ContactPreferences pref = lookupContactPreferences( addr );
  pref.pgpKeyFingerprints = pgpKeyFingerprints;
  pref.smimeCertFingerprints = smimeCertFingerprints;
  saveContactPreference( addr, pref );
}
