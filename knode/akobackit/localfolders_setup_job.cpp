/*
  Copyright 2010 Olivier Trichet <nive@nivalis.org>

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

#include "akobackit/localfolders_setup_job.h"

#include "akobackit/constant.h"

#include <Akonadi/AgentInstanceCreateJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/KMime/SpecialMailCollections>
#include <Akonadi/KMime/SpecialMailCollectionsRequestJob>
#include <akonadi/resourcesynchronizationjob.h>
#include <KLocalizedString>
#include <KStandardDirs>
#include <QDBusInterface>
#include <QDBusReply>
#include <QTimer>

namespace KNode {
namespace Akobackit {


LocalFoldersSetupJob::LocalFoldersSetupJob( const Akonadi::AgentInstance &resource, QObject *parent, bool ensureSpecialFoldersExistOnly )
  : KJob( parent ),
    mResource( resource ),
    mOnlySpecialFolder( ensureSpecialFoldersExistOnly )
{
  kDebug();
}

LocalFoldersSetupJob::~LocalFoldersSetupJob()
{
  kDebug();
}


Akonadi::AgentInstance LocalFoldersSetupJob::instance() const
{
  return mResource;
}



void LocalFoldersSetupJob::start()
{
  QTimer::singleShot( 0, this, SLOT( doStart() ) );
}

void LocalFoldersSetupJob::doStart()
{
  kDebug() << mResource.identifier();

  if ( mOnlySpecialFolder ) {
    if ( mResource.isValid() ) {
      ensureSpecialCollectionExists();
    } else {
      emitResult();
    }
    return;
  }

  if ( !mResource.isValid() ) {
    Akonadi::AgentInstanceCreateJob *job = new Akonadi::AgentInstanceCreateJob( MAILDIR_RESOURCE_AGENTTYPE );
    connect( job, SIGNAL( result( KJob * ) ), this, SLOT( foldersResourceCreated( KJob * ) ) );
    job->start();
  } else {
    configureResource( false );
  }
}

void LocalFoldersSetupJob::foldersResourceCreated( KJob *job )
{
  kDebug() << job->error() << job->errorString();
  if ( job->error() ) {
    setError( ResourceCreationError );
    setErrorText( i18nc( "%1: original error message", "Unable to setup the local folders (%1).", job->errorString() ) );
    emitResult();
  } else {
    Akonadi::AgentInstanceCreateJob *aicJob = qobject_cast<Akonadi::AgentInstanceCreateJob*>( job );
    Q_ASSERT( aicJob );

    mResource = aicJob->instance();
    Q_ASSERT( mResource.isValid() );

    emit resourceCreated( mResource );

    configureResource( true );
  }
}

void LocalFoldersSetupJob::configureResource( bool resourceIsNew )
{
  mResource.setName( i18nc( "Akonadi resource name", "KNode's local folders" ) );

  QDBusInterface remoteAgent( QString( "org.freedesktop.Akonadi.Agent.%1" ).arg( mResource.identifier() ),
                              "/Settings",
                              "org.kde.Akonadi.Maildir.Settings" );

  QString path;
  if ( !resourceIsNew ) { // the path is always empty when the resource is new
    QDBusReply<QString> reply = remoteAgent.call( "path" );
    path = reply.value();
  }
  if ( path.isEmpty() ) {
    // note: pre-Akonadi versions of KNode stored their folders in apps/knode/folders/
    // as mbox. So don't use this folder too.
    kDebug() << KStandardDirs::locateLocal( "data", "knode/local-folders/" );
    QDBusReply<void> reply = remoteAgent.call( "setPath", KStandardDirs::locateLocal( "data", "knode/local-folders/" ) );
    kDebug() << "Reply for Dbus call to setPath(QString):" << reply.error();
    Q_ASSERT( reply.isValid() );

    mResource.reconfigure();
  }
  startCollectionTreeSynchronization();
}


void LocalFoldersSetupJob::startCollectionTreeSynchronization()
{
  // Use a job instead of calling "mResource.synchronizeCollectionTree();"
  // because we need to wait for the synchronisation to finish.

  Akonadi::ResourceSynchronizationJob *job = new Akonadi::ResourceSynchronizationJob( mResource );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( synchronizationFinished( KJob* ) ) );
  job->start();
}

void LocalFoldersSetupJob::synchronizationFinished( KJob *job )
{
  kDebug() << job->error() << job->errorString();
  // We are not interested in result: we were waiting.

  mResource.synchronize();

  fetchCollections();
}

void LocalFoldersSetupJob::fetchCollections()
{
  kDebug() << mResource.identifier();
  Q_ASSERT( mResource.isValid() );

  Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob( Akonadi::Collection::root(),
                                                                      Akonadi::CollectionFetchJob::Recursive );
  job->fetchScope().setResource( mResource.identifier() );
  connect( job, SIGNAL( result( KJob * ) ), this, SLOT( collectionsFetched( KJob * ) ) );
  //job->start(); // Not needed for Akonadi::Job
}

void LocalFoldersSetupJob::collectionsFetched( KJob *job )
{
  kDebug() << job->error() << job->errorString();
  if ( job->error() ) {
    setError( RootFolderSetupError );
    setErrorText( i18nc( "%1: original error message", "Unable to find the local folders (%1).", job->errorString() ) );
    emitResult();
  } else {
    Akonadi::CollectionFetchJob *cfJob = qobject_cast<Akonadi::CollectionFetchJob*>( job );
    Q_ASSERT( cfJob );
    mCollections = cfJob->collections();
    Q_ASSERT_X( mCollections.size() > 0, "LocalFoldersSetupJob::setupRootFolder",
                "Found 0 collections for the local folders resource." );

    setupRootFolder();
  }
}


void LocalFoldersSetupJob::setupRootFolder()
{
  Akonadi::Collection baseCol;
  foreach ( const Akonadi::Collection &c, mCollections ) {
    if ( c.parentCollection() == Akonadi::Collection::root() ) {
      baseCol = c;
      break;
    }
  }
  mCollections.removeAll( baseCol );
  Q_ASSERT( baseCol.isValid() );

  // Change the attribute of the base collection (not interested in the result)
  Akonadi::EntityDisplayAttribute *attribute = baseCol.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Entity::AddIfMissing );
  attribute->setDisplayName( i18nc( "Name of the root folder", "Local folders" ) );
  attribute->setIconName( "folder" );
  Akonadi::CollectionModifyJob *cmJob = new Akonadi::CollectionModifyJob( baseCol ); // fire and forget.
  Q_UNUSED( cmJob );
  //cmJob->start(); // Not needed for Akonadi::Job

  // Register the base collection to the type SpecialMailCollections::Root
  Akonadi::SpecialMailCollections *smc = Akonadi::SpecialMailCollections::self();
  bool res = smc->registerCollection( Akonadi::SpecialMailCollections::Root, baseCol );
  if ( !res ) {
    setError( RootFolderSetupError );
    setErrorText( i18n( "Unable to setup the local folders correctly." ) );
    emitResult();
  } else {
    ensureSpecialCollectionExists();
  }
}


void LocalFoldersSetupJob::ensureSpecialCollectionExists()
{
  QList<Akonadi::SpecialMailCollections::Type> specialCollections;
  specialCollections << Akonadi::SpecialMailCollections::Outbox
                     << Akonadi::SpecialMailCollections::SentMail
                     << Akonadi::SpecialMailCollections::Drafts;

  Akonadi::SpecialMailCollections *smc = Akonadi::SpecialMailCollections::self();

  // Remove existing type
  QList<Akonadi::SpecialMailCollections::Type>::Iterator it = specialCollections.begin();
  while ( it != specialCollections.end() ) {
    if ( smc->hasCollection( *it, mResource ) ) {
      it = specialCollections.erase( it );
    } else {
      ++it;
    }
  }

  if ( specialCollections.isEmpty() ) {
    emitResult();
    return;
  }

  // Find existing collection with suitable name and reused them
  foreach ( const Akonadi::Collection &c, mCollections ) {
    Akonadi::SpecialMailCollections::Type type = Akonadi::SpecialMailCollections::Invalid;
    if ( c.remoteId().compare( "outbox", Qt::CaseInsensitive ) == 0 ) {
      type = Akonadi::SpecialMailCollections::Outbox;
    } else if ( c.remoteId().compare( "sent-mail", Qt::CaseInsensitive ) == 0 ) {
      type = Akonadi::SpecialMailCollections::SentMail;
    } else if ( c.remoteId().compare( "drafts", Qt::CaseInsensitive ) == 0 ) {
      type = Akonadi::SpecialMailCollections::Drafts;
    }

    if ( type != Akonadi::SpecialMailCollections::Invalid
         && specialCollections.contains( type ) ) {
      smc->registerCollection( type, c );
      specialCollections.removeAll( type );
    }
  }

  if ( specialCollections.isEmpty() ) {
    emitResult();
    return;
  }

  // create special collection
  Akonadi::SpecialMailCollectionsRequestJob *job = new Akonadi::SpecialMailCollectionsRequestJob();
  foreach ( Akonadi::SpecialMailCollections::Type type, specialCollections ) {
    job->requestCollection( type, mResource );
  }
  connect( job, SIGNAL( result( KJob * ) ), this, SLOT( ensureSpecialCollectionExistsResult( KJob * ) ) );
  job->start();
}

void LocalFoldersSetupJob::ensureSpecialCollectionExistsResult( KJob *job )
{
  if ( job->error() ) {
    setError( SpecialCollectionSetupError );
    setErrorText( i18n( "Unable to setup some local folders correctly." ) );
  }

  emitResult();
}



}
}

#include "localfolders_setup_job.moc"
