/* -*- mode: C++; c-file-style: "gnu" -*-
  kmsearchpattern.cpp
  Author: Marc Mutz <Marc@Mutz.com>

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

#include "searchpattern.h"
#include "filterlog.h"
using MailCommon::FilterLog;
#include <kpimutils/email.h>

#ifndef KDEPIM_NO_NEPOMUK

#include <nie.h>
#include <nmo.h>
#include <nco.h>

#include <Nepomuk/Tag>
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/AndTerm>
#include <Nepomuk/Query/OrTerm>
#include <Nepomuk/Query/LiteralTerm>
#include <Nepomuk/Query/ResourceTerm>
#include <Nepomuk/Query/NegationTerm>
#include <Nepomuk/Query/ResourceTypeTerm>

#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/RDF>

#endif

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <kmime/kmime_message.h>
#include <kmime/kmime_util.h>

#include <akonadi/contact/contactsearchjob.h>

#include <QRegExp>
#include <QByteArray>
#include <QDataStream>
#include <QXmlStreamWriter>


#include <assert.h>

using namespace MailCommon;

static const char* funcConfigNames[] =
  { "contains", "contains-not", "equals", "not-equal", "regexp",
    "not-regexp", "greater", "less-or-equal", "less", "greater-or-equal",
    "is-in-addressbook", "is-not-in-addressbook", "is-in-category", "is-not-in-category",
    "has-attachment", "has-no-attachment"};
static const int numFuncConfigNames = sizeof funcConfigNames / sizeof *funcConfigNames;

struct _statusNames {
  const char* name;
  Akonadi::MessageStatus status;
};

static struct _statusNames statusNames[] = {
  { "Important", Akonadi::MessageStatus::statusImportant() },
  { "Unread", Akonadi::MessageStatus::statusUnread() },
  { "Read", Akonadi::MessageStatus::statusRead() },
  { "Deleted", Akonadi::MessageStatus::statusDeleted() },
  { "Replied", Akonadi::MessageStatus::statusReplied() },
  { "Forwarded", Akonadi::MessageStatus::statusForwarded() },
  { "Queued", Akonadi::MessageStatus::statusQueued() },
  { "Sent", Akonadi::MessageStatus::statusSent() },
  { "Watched", Akonadi::MessageStatus::statusWatched() },
  { "Ignored", Akonadi::MessageStatus::statusIgnored() },
  { "Action Item", Akonadi::MessageStatus::statusToAct() },
  { "Spam", Akonadi::MessageStatus::statusSpam() },
  { "Ham", Akonadi::MessageStatus::statusHam() },
  { "Has Attachment", Akonadi::MessageStatus::statusHasAttachment() }
};

static const int numStatusNames = sizeof statusNames / sizeof ( struct _statusNames );

//==================================================
//
// class SearchRule (was: KMFilterRule)
//
//==================================================

SearchRule::SearchRule( const QByteArray & field, Function func, const QString & contents )
  : mField( field ),
    mFunction( func ),
    mContents( contents )
{
}

SearchRule::SearchRule( const SearchRule & other )
  : mField( other.mField ),
    mFunction( other.mFunction ),
    mContents( other.mContents )
{
}

const SearchRule & SearchRule::operator=( const SearchRule & other ) {
  if ( this == &other )
    return *this;

  mField = other.mField;
  mFunction = other.mFunction;
  mContents = other.mContents;

  return *this;
}

SearchRule::Ptr SearchRule::createInstance( const QByteArray & field,
                                                Function func,
                                                const QString & contents )
{
  SearchRule::Ptr ret;
  if (field == "<status>")
    ret = SearchRule::Ptr( new SearchRuleStatus( field, func, contents ) );
  else if ( field == "<age in days>" || field == "<size>" )
    ret = SearchRule::Ptr( new SearchRuleNumerical( field, func, contents ) );
  else
    ret = SearchRule::Ptr( new SearchRuleString( field, func, contents ) );

  return ret;
}

SearchRule::Ptr SearchRule::createInstance( const QByteArray & field,
                                                const char *func,
                                                const QString & contents )
{
  return ( createInstance( field, configValueToFunc( func ), contents ) );
}

SearchRule::Ptr SearchRule::createInstance( const SearchRule & other )
{
  return ( createInstance( other.field(), other.function(), other.contents() ) );
}

SearchRule::Ptr SearchRule::createInstanceFromConfig( const KConfigGroup & config, int aIdx )
{
  const char cIdx = char( int('A') + aIdx );

  static const QString & field = KGlobal::staticQString( "field" );
  static const QString & func = KGlobal::staticQString( "func" );
  static const QString & contents = KGlobal::staticQString( "contents" );

  const QByteArray &field2 = config.readEntry( field + cIdx, QString() ).toLatin1();
  Function func2 = configValueToFunc( config.readEntry( func + cIdx, QString() ).toLatin1() );
  const QString & contents2 = config.readEntry( contents + cIdx, QString() );

  if ( field2 == "<To or Cc>" ) // backwards compat
    return SearchRule::createInstance( "<recipients>", func2, contents2 );
  else
    return SearchRule::createInstance( field2, func2, contents2 );
}

SearchRule::Ptr SearchRule::createInstance( QDataStream &s )
{
  QByteArray field;
  s >> field;
  QString function;
  s >> function;
  Function func = configValueToFunc( function.toUtf8() );
  QString contents;
  s >> contents;
  return createInstance( field, func, contents );
}

SearchRule::~SearchRule()
{
}

SearchRule::Function SearchRule::configValueToFunc( const char * str ) {
  if ( !str )
    return FuncNone;

  for ( int i = 0 ; i < numFuncConfigNames ; ++i )
    if ( qstricmp( funcConfigNames[i], str ) == 0 ) return (Function)i;

  return FuncNone;
}

QString SearchRule::functionToString( Function function )
{
  if ( function != FuncNone )
    return funcConfigNames[int( function )];
  else
    return "invalid";
}

void SearchRule::writeConfig( KConfigGroup & config, int aIdx ) const {
  const char cIdx = char('A' + aIdx);
  static const QString & field = KGlobal::staticQString( "field" );
  static const QString & func = KGlobal::staticQString( "func" );
  static const QString & contents = KGlobal::staticQString( "contents" );

  config.writeEntry( field + cIdx, QString(mField) );
  config.writeEntry( func + cIdx, functionToString( mFunction ) );
  config.writeEntry( contents + cIdx, mContents );
}

void SearchRule::setFunction( Function function )
{
  mFunction = function;
}

SearchRule::Function SearchRule::function() const
{
  return mFunction;
}

void SearchRule::setField( const QByteArray &field )
{
  mField = field;
}

QByteArray SearchRule::field() const
{
  return mField;
}

void SearchRule::setContents( const QString &contents )
{
  mContents = contents;
}

QString SearchRule::contents() const
{
  return mContents;
}

const QString SearchRule::asString() const
{
  QString result  = "\"" + mField + "\" <";
  result += functionToString( mFunction );
  result += "> \"" + mContents + "\"";

  return result;
}

bool SearchRule::requiresBody() const
{
  return true;
}

#ifndef KDEPIM_NO_NEPOMUK

Nepomuk::Query::ComparisonTerm::Comparator SearchRule::nepomukComparator() const
{
  switch ( function() ) {
    case SearchRule::FuncContains:
    case SearchRule::FuncContainsNot:
      return Nepomuk::Query::ComparisonTerm::Contains;
    case SearchRule::FuncEquals:
    case SearchRule::FuncNotEqual:
      return Nepomuk::Query::ComparisonTerm::Equal;
    case SearchRule::FuncIsGreater:
      return Nepomuk::Query::ComparisonTerm::Greater;
    case SearchRule::FuncIsGreaterOrEqual:
      return Nepomuk::Query::ComparisonTerm::GreaterOrEqual;
    case SearchRule::FuncIsLess:
      return Nepomuk::Query::ComparisonTerm::Smaller;
    case SearchRule::FuncIsLessOrEqual:
      return Nepomuk::Query::ComparisonTerm::SmallerOrEqual;
    case SearchRule::FuncRegExp:
    case SearchRule::FuncNotRegExp:
      return Nepomuk::Query::ComparisonTerm::Regexp;
    default:
      kDebug() << "Unhandled function type: " << function();
  }
  return Nepomuk::Query::ComparisonTerm::Equal;
}

bool SearchRule::isNegated() const
{
  bool negate = false;
  switch ( function() ) {
    case SearchRule::FuncContainsNot:
    case SearchRule::FuncNotEqual:
    case SearchRule::FuncNotRegExp:
    case SearchRule::FuncHasNoAttachment:
    case SearchRule::FuncIsNotInCategory:
    case SearchRule::FuncIsNotInAddressbook:
      negate = true;
    default:
      break;
  }
  return negate;
}

void SearchRule::addAndNegateTerm(const Nepomuk::Query::Term& term, Nepomuk::Query::GroupTerm& termGroup) const
{
  if ( isNegated() ) {
    Nepomuk::Query::NegationTerm neg;
    neg.setSubTerm( term );
    termGroup.addSubTerm( neg );
  } else {
    termGroup.addSubTerm( term );
  }
}

#endif

QString SearchRule::xesamComparator() const
{
  switch ( function() ) {
    case SearchRule::FuncContains:
    case SearchRule::FuncContainsNot:
    return QLatin1String("contains");
    case SearchRule::FuncEquals:
    case SearchRule::FuncNotEqual:
    return QLatin1String("equals");
    case SearchRule::FuncIsGreater:
    return QLatin1String("greaterThan");
    case SearchRule::FuncIsGreaterOrEqual:
      return QLatin1String("greaterThanEquals");
    case SearchRule::FuncIsLess:
      return QLatin1String("lessThan");
    case SearchRule::FuncIsLessOrEqual:
      return QLatin1String("lessThanEquals");
      // FIXME how to handle the below? full text?
    case SearchRule::FuncRegExp:
    case SearchRule::FuncNotRegExp:
    default:
      kDebug() << "Unhandled function type: " << function();
  }
  return QLatin1String("equals");
}

QDataStream& SearchRule::operator >>( QDataStream& s ) const
{
  s << mField << functionToString( mFunction ) << mContents;
  return s;
}


//==================================================
//
// class SearchRuleString
//
//==================================================

SearchRuleString::SearchRuleString( const QByteArray & field,
                                        Function func, const QString & contents )
          : SearchRule(field, func, contents)
{
}

SearchRuleString::SearchRuleString( const SearchRuleString & other )
  : SearchRule( other )
{
}

const SearchRuleString & SearchRuleString::operator=( const SearchRuleString & other )
{
  if ( this == &other )
    return *this;

  setField( other.field() );
  setFunction( other.function() );
  setContents( other.contents() );

  return *this;
}

SearchRuleString::~SearchRuleString()
{
}

bool SearchRuleString::isEmpty() const
{
  return field().trimmed().isEmpty() || contents().isEmpty();
}

bool SearchRuleString::requiresBody() const
{
  if ( !field().startsWith( '<' ) || field() == "<recipients>" )
    return false;

  return true;
}

bool SearchRuleString::matches( const Akonadi::Item &item ) const
{
  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  assert( msg.get() );

  if ( isEmpty() )
    return false;

  QString msgContents;
  // Show the value used to compare the rules against in the log.
  // Overwrite the value for complete messages and all headers!
  bool logContents = true;

  if( field() == "<message>" ) {
    msgContents = msg->encodedContent();
    logContents = false;
  } else if ( field() == "<body>" ) {
    msgContents = msg->body();
    logContents = false;
  } else if ( field() == "<any header>" ) {
    msgContents = msg->head();
    logContents = false;
  } else if ( field() == "<recipients>" ) {
    // (mmutz 2001-11-05) hack to fix "<recipients> !contains foo" to
    // meet user's expectations. See FAQ entry in KDE 2.2.2's KMail
    // handbook
    if ( function() == FuncEquals || function() == FuncNotEqual )
      // do we need to treat this case specially? Ie.: What shall
      // "equality" mean for recipients.
      return matchesInternal( msg->to()->asUnicodeString() )
          || matchesInternal( msg->cc()->asUnicodeString() )
          || matchesInternal( msg->bcc()->asUnicodeString() ) ;

    msgContents = msg->to()->asUnicodeString();
    msgContents += ", " + msg->cc()->asUnicodeString();
    msgContents += ", " + msg->bcc()->asUnicodeString();
  } else if ( field() == "<tag>" ) {
#ifndef KDEPIM_NO_NEPOMUK    
    const Nepomuk::Resource res( item.url() );
    foreach ( const Nepomuk::Tag &tag, res.tags() )
      msgContents += tag.label();
    logContents = false;
#endif    
  } else {
    // make sure to treat messages with multiple header lines for
    // the same header correctly
    msgContents = msg->headerByType( field() ) ? msg->headerByType( field() )->asUnicodeString() : "";
  }

  if ( function() == FuncIsInAddressbook ||
       function() == FuncIsNotInAddressbook ) {
    // I think only the "from"-field makes sense.
    msgContents = msg->headerByType( field() ) ? msg->headerByType( field() )->asUnicodeString() : "";
    if ( msgContents.isEmpty() )
      return ( function() == FuncIsInAddressbook ) ? false : true;
  }

  // these two functions need the kmmessage therefore they don't call matchesInternal
  if ( function() == FuncHasAttachment )
    return ( msg->attachments().size() > 0 );
  if ( function() == FuncHasNoAttachment )
    return ( msg->attachments().size() == 0 );

  bool rc = matchesInternal( msgContents );
  if ( FilterLog::instance()->isLogging() ) {
    QString msg = ( rc ? "<font color=#00FF00>1 = </font>"
                       : "<font color=#FF0000>0 = </font>" );
    msg += FilterLog::recode( asString() );
    // only log headers bcause messages and bodies can be pretty large
    if ( logContents )
      msg += " (<i>" + FilterLog::recode( msgContents ) + "</i>)";
    FilterLog::instance()->add( msg, FilterLog::RuleResult );
  }
  return rc;
}

#ifndef KDEPIM_NO_NEPOMUK
void SearchRuleString::addPersonTerm(Nepomuk::Query::GroupTerm& groupTerm, const QUrl& field) const
{
  // TODO split contents() into address/name and adapt the query accordingly
  const Nepomuk::Query::ComparisonTerm valueTerm( Vocabulary::NCO::emailAddress(), Nepomuk::Query::LiteralTerm( contents() ), nepomukComparator() );
  const Nepomuk::Query::ComparisonTerm addressTerm( Vocabulary::NCO::hasEmailAddress(), valueTerm, Nepomuk::Query::ComparisonTerm::Equal );
  const Nepomuk::Query::ComparisonTerm personTerm( field, addressTerm, Nepomuk::Query::ComparisonTerm::Equal );
  groupTerm.addSubTerm( personTerm );
}

void SearchRuleString::addQueryTerms(Nepomuk::Query::GroupTerm& groupTerm) const
{
  Nepomuk::Query::OrTerm termGroup;
  if ( field().toLower() == "to" || field() == "<recipients>" || field() == "<any header>" || field() == "<message>" )
    addPersonTerm( termGroup, Vocabulary::NMO::to() );
  if ( field().toLower() == "cc" || field() == "<recipients>" || field() == "<any header>" || field() == "<message>" )
    addPersonTerm( termGroup, Vocabulary::NMO::cc() );
  if ( field().toLower() == "bcc" || field() == "<recipients>" || field() == "<any header>" || field() == "<message>" )
    addPersonTerm( termGroup, Vocabulary::NMO::bcc() );

  if ( field().toLower() == "from" || field() == "<any header>" || field() == "<message>" )
    addPersonTerm( termGroup, Vocabulary::NMO::from() );

  if ( field().toLower() == "subject" || field() == "<any header>" || field() == "<message>" ) {
    const Nepomuk::Query::ComparisonTerm subjectTerm( Vocabulary::NMO::messageSubject(), Nepomuk::Query::LiteralTerm( contents() ), nepomukComparator() );
    termGroup.addSubTerm( subjectTerm );
  }

  // TODO complete for other headers, generic headers

  if ( field() == "<tag>" ) {
    const Nepomuk::Tag tag( contents() );
    addAndNegateTerm( Nepomuk::Query::ComparisonTerm( Soprano::Vocabulary::NAO::hasTag(),
                                                      Nepomuk::Query::ResourceTerm( tag ),
                                                      Nepomuk::Query::ComparisonTerm::Equal ),
                                                      groupTerm );
  }

  if ( field() == "<body>" || field() == "<message>" ) {
    const Nepomuk::Query::ComparisonTerm bodyTerm( Vocabulary::NMO::plainTextMessageContent(), Nepomuk::Query::LiteralTerm( contents() ), nepomukComparator() );
    termGroup.addSubTerm( bodyTerm );

    const Nepomuk::Query::ComparisonTerm attachmentBodyTerm( Vocabulary::NMO::plainTextMessageContent(), Nepomuk::Query::LiteralTerm( contents() ), nepomukComparator() );
    const Nepomuk::Query::ComparisonTerm attachmentTerm( Vocabulary::NIE::isPartOf(), attachmentBodyTerm, Nepomuk::Query::ComparisonTerm::Equal );
    termGroup.addSubTerm( attachmentTerm );
  }

  if ( !termGroup.subTerms().isEmpty() )
    addAndNegateTerm( termGroup, groupTerm );
}
#endif

// helper, does the actual comparing
bool SearchRuleString::matchesInternal( const QString & msgContents ) const
{
  switch ( function() ) {
  case SearchRule::FuncEquals:
      return ( QString::compare( msgContents.toLower(), contents().toLower() ) == 0 );

  case SearchRule::FuncNotEqual:
      return ( QString::compare( msgContents.toLower(), contents().toLower() ) != 0 );

  case SearchRule::FuncContains:
    return ( msgContents.contains( contents(), Qt::CaseInsensitive ) );

  case SearchRule::FuncContainsNot:
    return ( !msgContents.contains( contents(), Qt::CaseInsensitive ) );

  case SearchRule::FuncRegExp:
    {
      QRegExp regexp( contents(), Qt::CaseInsensitive );
      return ( regexp.indexIn( msgContents ) >= 0 );
    }

  case SearchRule::FuncNotRegExp:
    {
      QRegExp regexp( contents(), Qt::CaseInsensitive );
      return ( regexp.indexIn( msgContents ) < 0 );
    }

  case FuncIsGreater:
      return ( QString::compare( msgContents.toLower(), contents().toLower() ) > 0 );

  case FuncIsLessOrEqual:
      return ( QString::compare( msgContents.toLower(), contents().toLower() ) <= 0 );

  case FuncIsLess:
      return ( QString::compare( msgContents.toLower(), contents().toLower() ) < 0 );

  case FuncIsGreaterOrEqual:
      return ( QString::compare( msgContents.toLower(), contents().toLower() ) >= 0 );

  case FuncIsInAddressbook: {
    const QStringList addressList = KPIMUtils::splitAddressList( msgContents.toLower() );
    for ( QStringList::ConstIterator it = addressList.constBegin(); ( it != addressList.constEnd() ); ++it ) {
      Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob();
      job->setLimit( 1 );
      job->setQuery( Akonadi::ContactSearchJob::Email, KPIMUtils::extractEmailAddress( *it ) );
      job->exec();

      if ( !job->contacts().isEmpty() )
        return true;
    }
    return false;
  }

  case FuncIsNotInAddressbook: {
    const QStringList addressList = KPIMUtils::splitAddressList( msgContents.toLower() );
    for ( QStringList::ConstIterator it = addressList.constBegin(); ( it != addressList.constEnd() ); ++it ) {
      Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob();
      job->setLimit( 1 );
      job->setQuery( Akonadi::ContactSearchJob::Email, KPIMUtils::extractEmailAddress( *it ) );
      job->exec();

      if ( job->contacts().isEmpty() )
        return true;
    }
    return false;
  }

  case FuncIsInCategory: {
    QString category = contents();
    const QStringList addressList =  KPIMUtils::splitAddressList( msgContents.toLower() );

    for ( QStringList::ConstIterator it = addressList.constBegin(); it != addressList.constEnd(); ++it ) {
      Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob();
      job->setQuery( Akonadi::ContactSearchJob::Email, KPIMUtils::extractEmailAddress( *it ) );
      job->exec();

      const KABC::Addressee::List contacts = job->contacts();

      foreach ( const KABC::Addressee &contact, contacts ) {
        if ( contact.hasCategory( category ) )
          return true;
      }
    }
    return false;
  }

  case FuncIsNotInCategory: {
    QString category = contents();
    const QStringList addressList =  KPIMUtils::splitAddressList( msgContents.toLower() );

    for ( QStringList::ConstIterator it = addressList.constBegin(); it != addressList.constEnd(); ++it ) {
      Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob();
      job->setQuery( Akonadi::ContactSearchJob::Email, KPIMUtils::extractEmailAddress( *it ) );
      job->exec();

      const KABC::Addressee::List contacts = job->contacts();

      foreach ( const KABC::Addressee &contact, contacts ) {
        if ( contact.hasCategory( category ) )
          return false;
      }

    }
    return true;
  }
  default:
    ;
  }

  return false;
}

void SearchRuleString::addXesamClause( QXmlStreamWriter& stream ) const
{
  const QString func = xesamComparator();

  stream.writeStartElement( func );

  if ( field().toLower() == "subject" ||
       field().toLower() == "to" ||
       field().toLower() == "cc" ||
       field().toLower() == "bcc" ||
       field().toLower() == "from" ||
       field().toLower() == "sender" ) {
    stream.writeStartElement( QLatin1String("field") );
    stream.writeAttribute( QLatin1String("name"), field().toLower() );
  } else {
    stream.writeStartElement( QLatin1String("fullTextFields") );
  }
  stream.writeEndElement();
  stream.writeTextElement( QLatin1String("string"), contents() );

  stream.writeEndElement();
}


//==================================================
//
// class SearchRuleNumerical
//
//==================================================

SearchRuleNumerical::SearchRuleNumerical( const QByteArray & field,
                                        Function func, const QString & contents )
          : SearchRule(field, func, contents)
{
}

bool SearchRuleNumerical::isEmpty() const
{
  bool ok = false;
  contents().toInt( &ok );

  return !ok;
}


bool SearchRuleNumerical::matches( const Akonadi::Item &item ) const
{
  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();

  QString msgContents;
  qint64 numericalMsgContents = 0;
  qint64 numericalValue = 0;

  if ( field() == "<size>" ) {
    numericalMsgContents = item.size();
    numericalValue = contents().toLongLong();
    msgContents.setNum( numericalMsgContents );
  } else if ( field() == "<age in days>" ) {
    QDateTime msgDateTime = msg->date()->dateTime().dateTime();
    numericalMsgContents = msgDateTime.daysTo( QDateTime::currentDateTime() );
    numericalValue = contents().toInt();
    msgContents.setNum( numericalMsgContents );
  }
  bool rc = matchesInternal( numericalValue, numericalMsgContents, msgContents );
  if ( FilterLog::instance()->isLogging() ) {
    QString msg = ( rc ? "<font color=#00FF00>1 = </font>"
                       : "<font color=#FF0000>0 = </font>" );
    msg += FilterLog::recode( asString() );
    msg += " ( <i>" + QString::number( numericalMsgContents ) + "</i> )";
    FilterLog::instance()->add( msg, FilterLog::RuleResult );
  }
  return rc;
}

bool SearchRuleNumerical::matchesInternal( long numericalValue,
    long numericalMsgContents, const QString & msgContents ) const
{
  switch ( function() ) {
  case SearchRule::FuncEquals:
      return ( numericalValue == numericalMsgContents );

  case SearchRule::FuncNotEqual:
      return ( numericalValue != numericalMsgContents );

  case SearchRule::FuncContains:
    return ( msgContents.contains( contents(), Qt::CaseInsensitive ) );

  case SearchRule::FuncContainsNot:
    return ( !msgContents.contains( contents(), Qt::CaseInsensitive ) );

  case SearchRule::FuncRegExp:
    {
      QRegExp regexp( contents(), Qt::CaseInsensitive );
      return ( regexp.indexIn( msgContents ) >= 0 );
    }

  case SearchRule::FuncNotRegExp:
    {
      QRegExp regexp( contents(), Qt::CaseInsensitive );
      return ( regexp.indexIn( msgContents ) < 0 );
    }

  case FuncIsGreater:
      return ( numericalMsgContents > numericalValue );

  case FuncIsLessOrEqual:
      return ( numericalMsgContents <= numericalValue );

  case FuncIsLess:
      return ( numericalMsgContents < numericalValue );

  case FuncIsGreaterOrEqual:
      return ( numericalMsgContents >= numericalValue );

  case FuncIsInAddressbook:  // since email-addresses are not numerical, I settle for false here
    return false;

  case FuncIsNotInAddressbook:
    return false;

  default:
    ;
  }

  return false;
}

#ifndef KDEPIM_NO_NEPOMUK

void SearchRuleNumerical::addQueryTerms(Nepomuk::Query::GroupTerm& groupTerm) const
{
  if ( field() == "<size>" ) {
    const Nepomuk::Query::ComparisonTerm sizeTerm( Vocabulary::NIE::byteSize(),
                                                   Nepomuk::Query::LiteralTerm( contents().toInt() ),
                                                   nepomukComparator() );
    addAndNegateTerm( sizeTerm, groupTerm );
  } else if ( field() == "<age in days>" ) {
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
    // TODO
  }
}
#endif

void SearchRuleNumerical::addXesamClause( QXmlStreamWriter & stream ) const
{

}


//==================================================
//
// class SearchRuleStatus
//
//==================================================
QString englishNameForStatus( const Akonadi::MessageStatus &status )
{
  for ( int i=0; i< numStatusNames; i++ ) {
    if ( statusNames[i].status == status ) {
      return statusNames[i].name;
    }
  }
  return QString();
}

SearchRuleStatus::SearchRuleStatus( const QByteArray & field,
                                        Function func, const QString & aContents )
          : SearchRule(field, func, aContents)
{
  // the values are always in english, both from the conf file as well as
  // the patternedit gui
  mStatus = statusFromEnglishName( aContents );
}

SearchRuleStatus::SearchRuleStatus( Akonadi::MessageStatus status, Function func )
 : SearchRule( "<status>", func, englishNameForStatus( status ) )
{
  mStatus = status;
}

Akonadi::MessageStatus SearchRuleStatus::statusFromEnglishName( const QString &aStatusString )
{
  for ( int i=0; i< numStatusNames; i++ ) {
    if ( !aStatusString.compare( statusNames[i].name ) ) {
      return statusNames[i].status;
    }
  }
  Akonadi::MessageStatus unknown;
  return unknown;
}

bool SearchRuleStatus::isEmpty() const
{
  return field().trimmed().isEmpty() || contents().isEmpty();
}

bool SearchRuleStatus::matches( const Akonadi::Item &item ) const
{
  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  Akonadi::MessageStatus status;
  status.setStatusFromFlags( item.flags() );
  bool rc = false;
  switch ( function() ) {
    case FuncEquals: // fallthrough. So that "<status> 'is' 'read'" works
    case FuncContains:
      if (status & mStatus)
        rc = true;
      break;
    case FuncNotEqual: // fallthrough. So that "<status> 'is not' 'read'" works
    case FuncContainsNot:
      if (! (status & mStatus) )
        rc = true;
      break;
    // FIXME what about the remaining funcs, how can they make sense for
    // stati?
    default:
      break;
  }
  if ( FilterLog::instance()->isLogging() ) {
    QString msg = ( rc ? "<font color=#00FF00>1 = </font>"
                       : "<font color=#FF0000>0 = </font>" );
    msg += FilterLog::recode( asString() );
    FilterLog::instance()->add( msg, FilterLog::RuleResult );
  }
  return rc;
}

#ifndef KDEPIM_NO_NEPOMUK
void SearchRuleStatus::addTagTerm( Nepomuk::Query::GroupTerm &groupTerm, const QString &tagId ) const
{
  // TODO handle function() == NOT
  const Nepomuk::Tag tag( tagId );
  addAndNegateTerm( Nepomuk::Query::ComparisonTerm( Soprano::Vocabulary::NAO::hasTag(),
                                                    Nepomuk::Query::ResourceTerm( tag.resourceUri() ),
                                                    Nepomuk::Query::ComparisonTerm::Equal ),
                                                    groupTerm );
}

void SearchRuleStatus::addQueryTerms(Nepomuk::Query::GroupTerm& groupTerm) const
{
  bool read = false;
  if ( function() == FuncContains || function() == FuncEquals )
    read = true;

  if ( !mStatus.isRead() )
    read = !read;

  groupTerm.addSubTerm( Nepomuk::Query::ComparisonTerm( Vocabulary::NMO::isRead(), Nepomuk::Query::LiteralTerm( read ), Nepomuk::Query::ComparisonTerm::Equal ) );

  if ( mStatus.isImportant() )
    addTagTerm( groupTerm, "important" );
  if ( mStatus.isToAct() )
    addTagTerm( groupTerm, "todo" );
  if ( mStatus.isWatched() )
    addTagTerm( groupTerm, "watched" );

  // TODO
}
#endif

void SearchRuleStatus::addXesamClause( QXmlStreamWriter & stream ) const
{

}


// ----------------------------------------------------------------------------

//==================================================
//
// class SearchPattern
//
//==================================================

SearchPattern::SearchPattern()
  : QList<SearchRule::Ptr>()
{
  init();
}

SearchPattern::SearchPattern( const KConfigGroup & config )
  : QList<SearchRule::Ptr>()
{
  readConfig( config );
}

SearchPattern::~SearchPattern()
{
}

bool SearchPattern::matches( const Akonadi::Item &item, bool ignoreBody ) const
{
  if ( isEmpty() )
    return true;
  if ( !item.hasPayload<KMime::Message::Ptr>() )
    return false;

  QList<SearchRule::Ptr>::const_iterator it;
  switch ( mOperator ) {
  case OpAnd: // all rules must match
    for ( it = begin() ; it != end() ; ++it )
      if ( !((*it)->requiresBody() && ignoreBody) )
        if ( !(*it)->matches( item ) )
          return false;
    return true;
  case OpOr:  // at least one rule must match
    for ( it = begin() ; it != end() ; ++it )
      if ( !((*it)->requiresBody() && ignoreBody) )
        if ( (*it)->matches( item ) )
          return true;
    // fall through
  default:
    return false;
  }
}

bool SearchPattern::requiresBody() const {
  QList<SearchRule::Ptr>::const_iterator it;
    for ( it = begin() ; it != end() ; ++it )
      if ( (*it)->requiresBody() )
	return true;
  return false;
}

void SearchPattern::purify() {
  QList<SearchRule::Ptr>::iterator it = end();
  while ( it != begin() ) {
    --it;
    if ( (*it)->isEmpty() ) {
#ifndef NDEBUG
      kDebug() << "Removing" << (*it)->asString();
#endif
      erase( it );
      it = end();
    }
  }
}

void SearchPattern::readConfig( const KConfigGroup & config ) {
  init();

  mName = config.readEntry("name");
  if ( !config.hasKey("rules") ) {
    kDebug() << "Found legacy config! Converting.";
    importLegacyConfig( config );
    return;
  }

  mOperator = config.readEntry("operator") == "or" ? OpOr : OpAnd;

  const int nRules = config.readEntry( "rules", 0 );

  for ( int i = 0 ; i < nRules ; i++ ) {
    SearchRule::Ptr r = SearchRule::createInstanceFromConfig( config, i );
    if ( !r->isEmpty() )
      append( r );
  }
}

void SearchPattern::importLegacyConfig( const KConfigGroup & config ) {
  SearchRule::Ptr rule = SearchRule::createInstance( config.readEntry("fieldA").toLatin1(),
					  config.readEntry("funcA").toLatin1(),
					  config.readEntry("contentsA") );
  if ( rule->isEmpty() ) {
    // if the first rule is invalid,
    // we really can't do much heuristics...
    return;
  }
  append( rule );

  const QString sOperator = config.readEntry("operator");
  if ( sOperator == "ignore" ) return;

  rule = SearchRule::createInstance( config.readEntry("fieldB").toLatin1(),
			   config.readEntry("funcB").toLatin1(),
			   config.readEntry("contentsB") );
  if ( rule->isEmpty() ) {
    return;
  }
  append( rule );

  if ( sOperator == "or"  ) {
    mOperator = OpOr;
    return;
  }
  // This is the interesting case...
  if ( sOperator == "unless" ) { // meaning "and not", ie we need to...
    // ...invert the function (e.g. "equals" <-> "doesn't equal")
    // We simply toggle the last bit (xor with 0x1)... This assumes that
    // SearchRule::Function's come in adjacent pairs of pros and cons
    SearchRule::Function func = last()->function();
    unsigned int intFunc = (unsigned int)func;
    func = SearchRule::Function( intFunc ^ 0x1 );

    last()->setFunction( func );
  }

  // treat any other case as "and" (our default).
}

void SearchPattern::writeConfig( KConfigGroup & config ) const {
  config.writeEntry("name", mName);
  config.writeEntry("operator", (mOperator == SearchPattern::OpOr) ? "or" : "and" );

  int i = 0;
  QList<SearchRule::Ptr>::const_iterator it;
  for ( it = begin() ; it != end() && i < FILTER_MAX_RULES ; ++i, ++it )
    // we could do this ourselves, but we want the rules to be extensible,
    // so we give the rule it's number and let it do the rest.
    (*it)->writeConfig( config, i );

  // save the total number of rules.
  config.writeEntry( "rules", i );
}

void SearchPattern::init() {
  clear();
  mOperator = OpAnd;
  mName = '<' + i18nc("name used for a virgin filter","unknown") + '>';
}

QString SearchPattern::asString() const {
  QString result;
  if ( mOperator == OpOr )
    result = i18n("(match any of the following)");
  else
    result = i18n("(match all of the following)");

  QList<SearchRule::Ptr>::const_iterator it;
  for ( it = begin() ; it != end() ; ++it )
    result += "\n\t" + FilterLog::recode( (*it)->asString() );

  return result;
}

#ifndef KDEPIM_NO_NEPOMUK

static Nepomuk::Query::GroupTerm makeGroupTerm( SearchPattern::Operator op )
{
  if ( op == SearchPattern::OpOr )
    return Nepomuk::Query::OrTerm();
  return Nepomuk::Query::AndTerm();
}
#endif

QString SearchPattern::asSparqlQuery() const
{
#ifndef KDEPIM_NO_NEPOMUK

  Nepomuk::Query::Query query;

  Nepomuk::Query::AndTerm outerGroup;
  const Nepomuk::Types::Class cl( Vocabulary::NMO::Email() );
  const Nepomuk::Query::ResourceTypeTerm typeTerm( cl );
  const Nepomuk::Query::Query::RequestProperty itemIdProperty( Akonadi::ItemSearchJob::akonadiItemIdUri(), false );

  Nepomuk::Query::GroupTerm innerGroup = makeGroupTerm( mOperator );
  for ( const_iterator it = begin(); it != end(); ++it )
    (*it)->addQueryTerms( innerGroup );

  if ( innerGroup.subTerms().isEmpty() )
    return QString();
  outerGroup.addSubTerm( innerGroup );
  outerGroup.addSubTerm( typeTerm );
  query.setTerm( outerGroup );
  query.addRequestProperty( itemIdProperty );
  return query.toSparqlQuery();
#else
  return QString(); //TODO what to return in this case?
#endif  
}

QString MailCommon::SearchPattern::asXesamQuery() const
{
  QString query;
  QXmlStreamWriter stream( &query );
  stream.setAutoFormatting(true);
  stream.writeStartDocument();
  stream.writeStartElement( QLatin1String("request" ) );
  stream.writeAttribute( QLatin1String("xmlns"), QLatin1String("http://freedesktop.org/standards/xesam/1.0/query") );
  stream.writeStartElement( QLatin1String("query") );

  const bool needsOperator = count() > 1;
  if ( needsOperator ) {
    if ( mOperator == SearchPattern::OpOr ) {
      stream.writeStartElement( QLatin1String("or") );
    } else if ( mOperator == SearchPattern::OpAnd ) {
      stream.writeStartElement( QLatin1String("and") );
    } else {
      Q_ASSERT(false); // can't happen (TM)
    }
  }

  QListIterator<SearchRule::Ptr> it( *this );
  while( it.hasNext() ) {
    const SearchRule::Ptr rule = it.next();
    rule->addXesamClause( stream );
  }

  if ( needsOperator )
    stream.writeEndElement(); // operator
  stream.writeEndElement(); // query
  stream.writeEndElement(); // request
  stream.writeEndDocument();
  return query;
}

const SearchPattern & SearchPattern::operator=( const SearchPattern & other ) {
  if ( this == &other )
    return *this;

  setOp( other.op() );
  setName( other.name() );

  clear(); // ###
  QList<SearchRule::Ptr>::const_iterator it;
  for ( it = other.begin() ; it != other.end() ; ++it )
    append( SearchRule::createInstance( **it ) ); // deep copy

  return *this;
}

QByteArray SearchPattern::serialize() const
{
  QByteArray out;
  QDataStream stream( &out, QIODevice::WriteOnly );
  *this >> stream;
  return out;
}

void SearchPattern::deserialize( const QByteArray &str )
{
  QDataStream stream( str );
  *this << stream;
}

QDataStream & SearchPattern::operator>>( QDataStream &s ) const
{
  if ( op() == SearchPattern::OpAnd ) {
    s << QString::fromLatin1( "and" );
  } else {
    s << QString::fromLatin1( "or" );
  }
  Q_FOREACH( const SearchRule::Ptr rule, *this ) {
    *rule >> s;
  }
  return s;
}

QDataStream & SearchPattern::operator <<( QDataStream &s )
{
  QString op;
  s >> op;
  setOp( op == QLatin1String( "and" ) ? OpAnd : OpOr );

  while ( !s.atEnd() ) {
    SearchRule::Ptr rule = SearchRule::createInstance( s );
    append( rule );
  }
  return s;
}

