/*
  Requires the Qt and KDE widget libraries, available at no cost at
  http://www.trolltech.com and http://www.kde.org respectively

  Copyright (c) 2002-2004 Klar�vdalens Datakonsult AB
        <info@klaralvdalens-datakonsult.se>
  Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute liinitialCheckForChangesnked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt.  If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.
*/

#include "freebusymanager.h"
#include "calendar.h"
#include "freebusydownloadjob.h"
#include "kcalprefs.h"
#include "mailscheduler.h"
#include "publishdialog.h"

#include <Akonadi/Contact/ContactSearchJob>

#include <KCalCore/Event>
#include <KCalCore/FreeBusy>
#include <KCalCore/ICalFormat>
#include <KCalCore/Person>

#include <KDebug>
#include <KMessageBox>
#include <KStandardDirs>
#include <KTemporaryFile>
#include <KUrl>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KIO/NetAccess>

#include <QDir>
#include <QFile>
#include <QPointer>
#include <QRegExp>
#include <QTextStream>
#include <QTimer>
#include <QTimerEvent>

using namespace CalendarSupport;

/// Free helper functions

KUrl replaceVariablesUrl( const KUrl &url, const QString &email )
{
  QString emailName;
  QString emailHost;

  const int atPos = email.indexOf( '@' );
  if ( atPos >= 0 ) {
    emailName = email.left( atPos );
    emailHost = email.mid( atPos + 1 );
  }

  QString saveStr = url.path();
  saveStr.replace( QRegExp( "%[Ee][Mm][Aa][Ii][Ll]%" ), email );
  saveStr.replace( QRegExp( "%[Nn][Aa][Mm][Ee]%" ), emailName );
  saveStr.replace( QRegExp( "%[Ss][Ee][Rr][Vv][Ee][Rr]%" ), emailHost );

  KUrl retUrl( url );
  retUrl.setPath( saveStr );
  return retUrl;
}

bool fbExists( const KUrl &url )
{
  // We need this function because using KIO::NetAccess::exists()
  // is useless for the http and https protocols. And getting back
  // arbitrary data is also useless because a server can respond back
  // with a "no such document" page.  So we need smart checking.

  KIO::Job *job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
  QByteArray data;
  if ( KIO::NetAccess::synchronousRun( job, 0, &data ) ) {
    QString dataStr ( data );
    if ( dataStr.contains( "BEGIN:VCALENDAR" ) ) {
      return true;
    }
  }
  return false;
}

/// FreeBusyManagerPrivate

namespace CalendarSupport {

class FreeBusyManagerPrivate
{
  FreeBusyManager *const q_ptr;
  Q_DECLARE_PUBLIC( FreeBusyManager )

  public: /// Members
    CalendarSupport::Calendar *mCalendar;
    KCalCore::ICalFormat mFormat;

    QStringList mRetrieveQueue;
    QMap<KUrl, QString> mFreeBusyUrlEmailMap;

    // Free/Busy uploading
    QDateTime mNextUploadTime;
    int mTimerID;
    bool mUploadingFreeBusy;
    bool mBrokenUrl;

    // the parentWidget to use while doing our "recursive" retrieval
    QPointer<QWidget>  mParentWidgetForRetrieval;

  public: /// Functions
    FreeBusyManagerPrivate( FreeBusyManager *q );
    void checkFreeBusyUrl();
    QString freeBusyDir() const;
    KUrl freeBusyUrl( const QString &email ) const;
    QString freeBusyToIcal( const KCalCore::FreeBusy::Ptr & );
    KCalCore::FreeBusy::Ptr iCalToFreeBusy( const QByteArray &freeBusyData );
    KCalCore::FreeBusy::Ptr ownerFreeBusy();
    QString ownerFreeBusyAsString();
    void processFreeBusyDownloadResult( KJob *_job );
    void processFreeBusyUploadResult( KJob *_job );
    bool processRetrieveQueue();
    void uploadFreeBusy();
};

}

FreeBusyManagerPrivate::FreeBusyManagerPrivate( FreeBusyManager *q )
  : q_ptr( q ),
    mCalendar( 0 ),
    mTimerID( 0 ),
    mUploadingFreeBusy( false ),
    mBrokenUrl( false ),
    mParentWidgetForRetrieval( 0 )
{
}

void FreeBusyManagerPrivate::checkFreeBusyUrl()
{
  KUrl targetURL( KCalPrefs::instance()->freeBusyPublishUrl() );
  mBrokenUrl = targetURL.isEmpty() || !targetURL.isValid();
}

QString FreeBusyManagerPrivate::freeBusyDir() const
{
  return KStandardDirs::locateLocal( "data", QLatin1String( "korganizer/freebusy" ) );
}

KUrl FreeBusyManagerPrivate::freeBusyUrl( const QString &email ) const
{
  // First check if there is a specific FB url for this email
  QString configFile = KStandardDirs::locateLocal( "data",
                                                   QLatin1String( "korganizer/freebusyurls" ) );
  KConfig cfg( configFile );
  KConfigGroup group = cfg.group( email );
  QString url = group.readEntry( QLatin1String( "url" ) );
  if ( !url.isEmpty() ) {
    kDebug() << "Found cached url:" << url;
    KUrl cachedUrl( url );
    if ( KCalPrefs::instance()->thatIsMe( email ) ) {
      cachedUrl.setUser( KCalPrefs::instance()->mFreeBusyRetrieveUser );
      cachedUrl.setPass( KCalPrefs::instance()->mFreeBusyRetrievePassword );
    }
    return replaceVariablesUrl( cachedUrl, email );
  }
  // Try with the url configurated by preferred email in kcontactmanager
  Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob();
  job->setQuery( Akonadi::ContactSearchJob::Email, email );
  if ( !job->exec() ) {
    return KUrl();
  }

  QString pref;
  const KABC::Addressee::List contacts = job->contacts();
  foreach ( const KABC::Addressee &contact, contacts ) {
    pref = contact.preferredEmail();
    if ( !pref.isEmpty() && pref != email ) {
      kDebug() << "Preferred email of" << email << "is" << pref;
      group = cfg.group( pref );
      url = group.readEntry ( "url" );
      if ( !url.isEmpty() ) {
        kDebug() << "Taken url from preferred email:" << url;
        return replaceVariablesUrl( KUrl( url ), email );
      }
    }
  }
  // None found. Check if we do automatic FB retrieving then
  if ( !KCalPrefs::instance()->mFreeBusyRetrieveAuto ) {
    // No, so no FB list here
    return KUrl();
  }

  // Sanity check: Don't download if it's not a correct email
  // address (this also avoids downloading for "(empty email)").
  int emailpos = email.indexOf( QLatin1Char( '@' ) );
  if( emailpos == -1 ) {
     kDebug() << "No '@' found in" << email;
     return KUrl();
  }

  const QString emailHost = email.mid( emailpos + 1 );

  // Build the URL
  if ( KCalPrefs::instance()->mFreeBusyCheckHostname ) {
    // Don't try to fetch free/busy data for users not on the specified servers
    // This tests if the hostnames match, or one is a subset of the other
    const QString hostDomain = KUrl( KCalPrefs::instance()->mFreeBusyRetrieveUrl ).host();
    if ( hostDomain != emailHost &&
         !hostDomain.endsWith( QLatin1Char( '.' ) + emailHost ) &&
         !emailHost.endsWith( QLatin1Char( '.' ) + hostDomain ) ) {
      // Host names do not match
      kDebug() << "Host '" << hostDomain << "' doesn't match email '" << email << '\'';
      return KUrl();
    }
  }

  if ( KCalPrefs::instance()->mFreeBusyRetrieveUrl.contains( QRegExp( "\\.[xiv]fb$" ) ) ) {
    // user specified a fullpath
    // do variable string replacements to the URL (MS Outlook style)
    const KUrl sourceUrl( KCalPrefs::instance()->mFreeBusyRetrieveUrl );
    KUrl fullpathURL = replaceVariablesUrl( sourceUrl, email );

    // set the User and Password part of the URL
    fullpathURL.setUser( KCalPrefs::instance()->mFreeBusyRetrieveUser );
    fullpathURL.setPass( KCalPrefs::instance()->mFreeBusyRetrievePassword );

    // no need to cache this URL as this is pretty fast to get from the config value.
    // return the fullpath URL
    return fullpathURL;
 }

 // else we search for a fb file in the specified URL with known possible extensions

 const QStringList extensions = QStringList() << "xfb" << "ifb" << "vfb";
 QStringList::ConstIterator ext;
 for ( ext = extensions.constBegin(); ext != extensions.constEnd(); ++ext ) {
   // build a url for this extension
   const KUrl sourceUrl = KCalPrefs::instance()->mFreeBusyRetrieveUrl;
   KUrl dirURL = replaceVariablesUrl( sourceUrl, email );
   if ( KCalPrefs::instance()->mFreeBusyFullDomainRetrieval ) {
     dirURL.addPath( email + '.' + (*ext) );
   } else {
     // Cut off everything left of the @ sign to get the user name.
     const QString emailName = email.left( emailpos );
     dirURL.addPath( emailName + '.' + (*ext ) );
   }
   dirURL.setUser( KCalPrefs::instance()->mFreeBusyRetrieveUser );
   dirURL.setPass( KCalPrefs::instance()->mFreeBusyRetrievePassword );
   if ( fbExists( dirURL ) ) {
     // write the URL to the cache
     KConfigGroup group = cfg.group( email );
     group.writeEntry( "url", dirURL.prettyUrl() ); // prettyURL() does not write user nor password
     return dirURL;
   }
 }

 return KUrl();
}

QString FreeBusyManagerPrivate::freeBusyToIcal( const KCalCore::FreeBusy::Ptr &freebusy )
{
  return mFormat.createScheduleMessage( freebusy, KCalCore::iTIPPublish );
}

KCalCore::FreeBusy::Ptr FreeBusyManagerPrivate::iCalToFreeBusy( const QByteArray &freeBusyData )
{
  const QString freeBusyVCal( QString::fromUtf8( freeBusyData ) );
  KCalCore::FreeBusy::Ptr fb = mFormat.parseFreeBusy( freeBusyVCal );

  if ( !fb ) {
    kDebug() << "Error parsing free/busy";
    kDebug() << freeBusyVCal;
  }

  return fb;
}

KCalCore::FreeBusy::Ptr FreeBusyManagerPrivate::ownerFreeBusy()
{
  KDateTime start = KDateTime::currentUtcDateTime();
  KDateTime end = start.addDays( KCalPrefs::instance()->mFreeBusyPublishDays );

  KCalCore::Event::List events;
  Akonadi::Item::List items = mCalendar ?
                              mCalendar->rawEvents( start.date(), end.date() ) :
                              Akonadi::Item::List();
  foreach ( const Akonadi::Item &item, items ) {
    events << item.payload<KCalCore::Event::Ptr>();
  }
  KCalCore::FreeBusy::Ptr freebusy ( new KCalCore::FreeBusy( events, start, end ) );
  freebusy->setOrganizer( KCalCore::Person::Ptr(
                            new KCalCore::Person( KCalPrefs::instance()->fullName(),
                                                  KCalPrefs::instance()->email() ) ) );
  return freebusy;
}

QString FreeBusyManagerPrivate::ownerFreeBusyAsString()
{
  return freeBusyToIcal( ownerFreeBusy() );
}

void FreeBusyManagerPrivate::processFreeBusyDownloadResult( KJob *_job )
{
  Q_ASSERT( dynamic_cast<FreeBusyDownloadJob *>( _job ) );
  Q_Q( FreeBusyManager );

  FreeBusyDownloadJob *job = static_cast<FreeBusyDownloadJob *>( _job );
  if ( job->error() ) {
    KMessageBox::sorry(
      mParentWidgetForRetrieval,
      i18n( "Failed to download free/busy data from: %1\nReason: %2",
            job->url().prettyUrl(), job->errorText() ),
      i18n( "Free/busy retrieval error" ) );

    // TODO: Ask for a retry? (i.e. queue  the email again when the user wants it).

    // Make sure we don't fill up the map with unneeded data on failures.
    mFreeBusyUrlEmailMap.take( job->url() );
  } else {
    KCalCore::FreeBusy::Ptr fb = iCalToFreeBusy( job->rawFreeBusyData() );

    Q_ASSERT( mFreeBusyUrlEmailMap.contains( job->url() ) );
    const QString email = mFreeBusyUrlEmailMap.take( job->url() );

    if ( fb ) {
      KCalCore::Person::Ptr p = fb->organizer();
      p->setEmail( email );
      q->saveFreeBusy( fb, p );

      emit q->freeBusyRetrieved( fb, email );
    } else {
      KMessageBox::sorry(
        mParentWidgetForRetrieval,
        i18n( "Failed to parse free/busy information that was retrieved from: %1",
              job->url().prettyUrl() ),
        i18n( "Free/busy retrieval error" ) );
    }
  }

  // When downloading failed or finished, start a job for the next one in the
  // queue if needed.
  processRetrieveQueue();
}

void FreeBusyManagerPrivate::processFreeBusyUploadResult( KJob *_job )
{
  KIO::FileCopyJob *job = static_cast<KIO::FileCopyJob *>( _job );
  if ( job->error() ) {
    KMessageBox::sorry(
        job->ui()->window(),
        i18n( "<qt><p>The software could not upload your free/busy list to "
              "the URL '%1'. There might be a problem with the access "
              "rights, or you specified an incorrect URL. The system said: "
              "<em>%2</em>.</p>"
              "<p>Please check the URL or contact your system administrator."
              "</p></qt>", job->destUrl().prettyUrl(),
              job->errorString() ) );
  }
  // Delete temp file
  KUrl src = job->srcUrl();
  Q_ASSERT( src.isLocalFile() );
  if ( src.isLocalFile() ) {
    QFile::remove( src.toLocalFile() );
  }
  mUploadingFreeBusy = false;
}

bool FreeBusyManagerPrivate::processRetrieveQueue()
{
  Q_Q( FreeBusyManager );

  if ( mRetrieveQueue.isEmpty() ) {
    return true;
  }

  QString email = mRetrieveQueue.takeFirst();
  KUrl freeBusyUrlForEmail = freeBusyUrl( email );

  if ( !freeBusyUrlForEmail.isValid() ) {
    kDebug() << "Invalid FreeBusy URL" << freeBusyUrlForEmail.prettyUrl() << email;
    return false;
  }

  if ( mFreeBusyUrlEmailMap.contains( freeBusyUrlForEmail ) ) {
    return true; // Already in progress.
  }

  mFreeBusyUrlEmailMap.insert( freeBusyUrlForEmail, email );

  FreeBusyDownloadJob *job = new FreeBusyDownloadJob( freeBusyUrlForEmail,
                                                      mParentWidgetForRetrieval );
  q->connect( job, SIGNAL(result(KJob *)), SLOT(processFreeBusyDownloadResult(KJob *)) );
  job->start();

  return true;
}

void FreeBusyManagerPrivate::uploadFreeBusy()
{
  Q_Q( FreeBusyManager );

  // user has automatic uploading disabled, bail out
  if ( !KCalPrefs::instance()->freeBusyPublishAuto() ||
       KCalPrefs::instance()->freeBusyPublishUrl().isEmpty() ) {
     return;
  }

  if( mTimerID != 0 ) {
    // A timer is already running, so we don't need to do anything
    return;
  }

  int now = static_cast<int>( QDateTime::currentDateTime().toTime_t() );
  int eta = static_cast<int>( mNextUploadTime.toTime_t() ) - now;

  if ( !mUploadingFreeBusy ) {
    // Not currently uploading
    if ( mNextUploadTime.isNull() ||
         QDateTime::currentDateTime() > mNextUploadTime ) {
      // No uploading have been done in this session, or delay time is over
      q->publishFreeBusy();
      return;
    }

    // We're in the delay time and no timer is running. Start one
    if ( eta <= 0 ) {
      // Sanity check failed - better do the upload
      q->publishFreeBusy();
      return;
    }
  } else {
    // We are currently uploading the FB list. Start the timer
    if ( eta <= 0 ) {
      kDebug() << "This shouldn't happen! eta <= 0";
      eta = 10; // whatever
    }
  }

  // Start the timer
  mTimerID = q->startTimer( eta * 1000 );

  if ( mTimerID == 0 ) {
    // startTimer failed - better do the upload
    q->publishFreeBusy();
  }
}

/// FreeBusyManager::Singleton

namespace CalendarSupport {

struct FreeBusyManagerStatic
{
  FreeBusyManager instance;
};

}

K_GLOBAL_STATIC( FreeBusyManagerStatic, sManagerInstance )

/// FreeBusyManager

FreeBusyManager::FreeBusyManager()
  : d_ptr( new FreeBusyManagerPrivate( this ) )
{
  setObjectName( QLatin1String( "FreeBusyManager" ) );
  connect( KCalPrefs::instance(), SIGNAL(configChanged()),
           SLOT(checkFreeBusyUrl()) );
}

FreeBusyManager::~FreeBusyManager()
{
  delete d_ptr;
}

FreeBusyManager *FreeBusyManager::self()
{
  return &sManagerInstance->instance;
}

void FreeBusyManager::setCalendar( CalendarSupport::Calendar *c )
{
  Q_D( FreeBusyManager );

  if ( d->mCalendar ) {
    disconnect( d->mCalendar, SIGNAL(calendarChanged()) );
  }

  d->mCalendar = c;
  if ( d->mCalendar ) {
    d->mFormat.setTimeSpec( d->mCalendar->timeSpec() );
  }

  connect( d->mCalendar, SIGNAL(calendarChanged()),
           SLOT(uploadFreeBusy()) );

  // Lets see if we need to update our published
  QTimer::singleShot( 0, this, SLOT(uploadFreeBusy()) );
}

/*!
  This method is called when the user has selected to publish its
  free/busy list or when the delay have passed.
*/
void FreeBusyManager::publishFreeBusy( QWidget *parentWidget )
{
  Q_D( FreeBusyManager );
  // Already uploading? Skip this one then.
  if ( d->mUploadingFreeBusy ) {
    return;
  }

  // No calendar set yet? Don't upload to prevent losing published information that
  // might still be valid.
  if ( !d->mCalendar ) {
    return;
  }

  KUrl targetURL( KCalPrefs::instance()->freeBusyPublishUrl() );
  if ( targetURL.isEmpty() )  {
    KMessageBox::sorry(
      parentWidget,
      i18n( "<qt><p>No URL configured for uploading your free/busy list. "
            "Please set it in KOrganizer's configuration dialog, on the "
            "\"Free/Busy\" page.</p>"
            "<p>Contact your system administrator for the exact URL and the "
            "account details.</p></qt>" ),
      i18n( "No Free/Busy Upload URL" ) );
    return;
  }

  if ( d->mBrokenUrl ) {
     // Url is invalid, don't try again
    return;
  }
  if ( !targetURL.isValid() ) {
    KMessageBox::sorry(
      parentWidget,
      i18n( "<qt>The target URL '%1' provided is invalid.</qt>", targetURL.prettyUrl() ),
      i18n( "Invalid URL" ) );
    d->mBrokenUrl = true;
    return;
  }
  targetURL.setUser( KCalPrefs::instance()->mFreeBusyPublishUser );
  targetURL.setPass( KCalPrefs::instance()->mFreeBusyPublishPassword );

  d->mUploadingFreeBusy = true;

  // If we have a timer running, it should be stopped now
  if ( d->mTimerID != 0 ) {
    killTimer( d->mTimerID );
    d->mTimerID = 0;
  }

  // Save the time of the next free/busy uploading
  d->mNextUploadTime = QDateTime::currentDateTime();
  if ( KCalPrefs::instance()->mFreeBusyPublishDelay > 0 ) {
    d->mNextUploadTime =
      d->mNextUploadTime.addSecs( KCalPrefs::instance()->mFreeBusyPublishDelay * 60 );
  }

  QString messageText = d->ownerFreeBusyAsString();

  // We need to massage the list a bit so that Outlook understands
  // it.
  messageText = messageText.replace( QRegExp( QLatin1String( "ORGANIZER\\s*:MAILTO:" ) ),
                                     QLatin1String( "ORGANIZER:" ) );

  // Create a local temp file and save the message to it
  KTemporaryFile tempFile;
  tempFile.setAutoRemove( false );
  if ( tempFile.open() ) {
    QTextStream textStream ( &tempFile );
    textStream << messageText;
    textStream.flush();

#if 0
    QString defaultEmail = KOCore()::self()->email();
    QString emailHost = defaultEmail.mid( defaultEmail.indexOf( '@' ) + 1 );

    // Put target string together
    KUrl targetURL;
    if( KCalPrefs::instance()->mPublishKolab ) {
      // we use Kolab
      QString server;
      if ( KCalPrefs::instance()->mPublishKolabServer == QLatin1String( "%SERVER%" ) ||
           KCalPrefs::instance()->mPublishKolabServer.isEmpty() ) {
        server = emailHost;
      } else {
        server = KCalPrefs::instance()->mPublishKolabServer;
      }

      targetURL.setProtocol( "webdavs" );
      targetURL.setHost( server );

      QString fbname = KCalPrefs::instance()->mPublishUserName;
      int at = fbname.indexOf( '@' );
      if ( at > 1 && fbname.length() > (uint)at ) {
        fbname = fbname.left(at);
      }
      targetURL.setPath( "/freebusy/" + fbname + ".ifb" );
      targetURL.setUser( KCalPrefs::instance()->mPublishUserName );
      targetURL.setPass( KCalPrefs::instance()->mPublishPassword );
    } else {
      // we use something else
      targetURL = KCalPrefs::instance()->mPublishAnyURL.replace( "%SERVER%", emailHost );
      targetURL.setUser( KCalPrefs::instance()->mPublishUserName );
      targetURL.setPass( KCalPrefs::instance()->mPublishPassword );
    }
#endif

    KUrl src;
    src.setPath( tempFile.fileName() );

    kDebug() << targetURL;

    KIO::Job *job = KIO::file_copy( src, targetURL, -1, KIO::Overwrite | KIO::HideProgressInfo );

    job->ui()->setWindow( parentWidget );

    connect( job, SIGNAL(result(KJob *)), SLOT(slotUploadFreeBusyResult(KJob *)) );
  }
}

void FreeBusyManager::mailFreeBusy(int daysToPublish, QWidget* parentWidget)
{
   Q_D( FreeBusyManager );
 // No calendar set yet?
  if ( !d->mCalendar ) {
    return;
  }
  
  KDateTime start = KDateTime::currentUtcDateTime().toTimeSpec( d->mCalendar->timeSpec() );
  KDateTime end = start.addDays( daysToPublish );

  Event::List events;
  Akonadi::Item::List items = d->mCalendar->rawEvents( start.date(), end.date() );
  foreach(const Akonadi::Item &item, items) { //FIXME
    events << item.payload<Event::Ptr>();
  }

  FreeBusy::Ptr freebusy( new FreeBusy( events, start, end ) );
  freebusy->setOrganizer( Person::Ptr( new Person( CalendarSupport::KCalPrefs::instance()->fullName(),
                                                   CalendarSupport::KCalPrefs::instance()->email() ) ) );

  QPointer<PublishDialog> publishdlg = new PublishDialog();
  if ( publishdlg->exec() == QDialog::Accepted ) {
    // Send the mail
    CalendarSupport::MailScheduler scheduler( d->mCalendar );
    if ( scheduler.publish( freebusy, publishdlg->addresses() ) ) {
      KMessageBox::information(
        parentWidget,
        i18n( "The free/busy information was successfully sent." ),
        i18n( "Sending Free/Busy" ),
        "FreeBusyPublishSuccess" );
    } else {
      KMessageBox::error(
        parentWidget,
        i18n( "Unable to publish the free/busy data." ) );
    }
  }
  delete publishdlg;
}


bool FreeBusyManager::retrieveFreeBusy( const QString &email, bool forceDownload,
                                        QWidget *parentWidget )
{
  Q_D( FreeBusyManager );

  kDebug() << email;
  if ( email.isEmpty() ) {
    return false;
  }

  d->mParentWidgetForRetrieval = parentWidget;

  if ( KCalPrefs::instance()->thatIsMe( email ) ) {
    // Don't download our own free-busy list from the net
    kDebug() << "freebusy of owner";
    emit freeBusyRetrieved( d->ownerFreeBusy(), email );
    return true;
  }

  // Check for cached copy of free/busy list
  KCalCore::FreeBusy::Ptr fb = loadFreeBusy( email );
  if ( fb ) {
    emit freeBusyRetrieved( fb, email );
    return true;
  }

  // Don't download free/busy if the user does not want it.
  if ( !KCalPrefs::instance()->mFreeBusyRetrieveAuto && !forceDownload ) {
    return false;
  }

  d->mRetrieveQueue.append( email );

  if ( d->mRetrieveQueue.count() > 1 ) {
    return true;
  }

  return d->processRetrieveQueue();
}

void FreeBusyManager::cancelRetrieval()
{
  Q_D( FreeBusyManager );
  d->mRetrieveQueue.clear();
}

KCalCore::FreeBusy::Ptr FreeBusyManager::loadFreeBusy( const QString &email )
{
  Q_D( FreeBusyManager );
  const QString fbd = d->freeBusyDir();

  QFile f( fbd + QLatin1Char( '/' ) + email + QLatin1String( ".ifb" ) );
  if ( !f.exists() ) {
    kDebug() << f.fileName() << "doesn't exist.";
    return KCalCore::FreeBusy::Ptr();
  }

  if ( !f.open( QIODevice::ReadOnly ) ) {
    kDebug() << "Unable to open file" << f.fileName();
    return KCalCore::FreeBusy::Ptr();
  }

  QTextStream ts( &f );
  QString str = ts.readAll();

  return d->iCalToFreeBusy( str.toUtf8() );
}

bool FreeBusyManager::saveFreeBusy( const KCalCore::FreeBusy::Ptr &freebusy,
                                    const KCalCore::Person::Ptr &person )
{
  Q_D( FreeBusyManager );
  Q_ASSERT( person );
  kDebug() << person->fullName();

  QString fbd = d->freeBusyDir();

  QDir freeBusyDirectory( fbd );
  if ( !freeBusyDirectory.exists() ) {
    kDebug() << "Directory" << fbd <<" does not exist!";
    kDebug() << "Creating directory:" << fbd;

    if( !freeBusyDirectory.mkpath( fbd ) ) {
      kDebug() << "Could not create directory:" << fbd;
      return false;
    }
  }

  QString filename( fbd );
  filename += QLatin1Char( '/' );
  filename += person->email();
  filename += QLatin1String( ".ifb" );
  QFile f( filename );

  kDebug() << "filename:" << filename;

  freebusy->clearAttendees();
  freebusy->setOrganizer( person );

  QString messageText = d->mFormat.createScheduleMessage( freebusy, KCalCore::iTIPPublish );

  if ( !f.open( QIODevice::ReadWrite ) ) {
    kDebug() << "acceptFreeBusy: Can't open:" << filename << "for writing";
    return false;
  }
  QTextStream t( &f );
  t << messageText;
  f.close();

  return true;
}

void FreeBusyManager::timerEvent( QTimerEvent * )
{
  publishFreeBusy();
}

#include "freebusymanager.moc"
