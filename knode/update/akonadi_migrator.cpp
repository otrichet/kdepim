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


#include "akonadi_migrator.h"

#include "akobackit/akonadi_manager.h"
#include "akobackit/constant.h"
#include "akobackit/folder_manager.h"
#include "akobackit/item_merge_job.h"
#include "akobackit/localfolders_setup_job.h"
#include "update/legacy/knfoldermanager.h"

#include <Akonadi/CollectionCreateJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ItemCreateJob>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

namespace KNode {
namespace Update {

AkonadiMigrator::AkonadiMigrator( QObject *parent )
  : UpdaterBase( parent ),
    mError( NoError )
{
}

AkonadiMigrator::~AkonadiMigrator()
{
}

QString AkonadiMigrator::name() const
{
  return i18n( "Migration of local folders and newsgroup accounts" );
}



UpdaterBase::Status AkonadiMigrator::error() const
{
  return mError;
}

void AkonadiMigrator::update()
{
  KConfigGroup config = migratorConfig()->group( "AkonadiMigrator" ).group( "Folders" );
  if ( !config.readEntry( "done", false ) ) {
    migrateFolders( config );
    config.sync();
  }
}



void AkonadiMigrator::migrateFolders( KConfigGroup &config )
{
  const int PROGRESS_AFTER_RESOURCE_CREATION = 1;
  const int PROGRESS_AFTER_FOLDERS_CREATION = 10;
  const int PROGRESS_AFTER_MESSAGES_MIGRATION = 40;
  const int MAX_PROGRESS = PROGRESS_AFTER_MESSAGES_MIGRATION;

  KNFolderManager *oldFolderManager = new KNFolderManager();
  Akobackit::FolderManager *folderManager = Akobackit::manager()->folderManager();

  emit progress( 0 );

  KNFolder::List oldFolders = oldFolderManager->folders();
  if ( oldFolders.isEmpty() ) {
    emit message( i18n( "No local folder found" ), LogWarn );
    emit progress( MAX_PROGRESS );
    markDone( config, true );
    return;
  }

  bool needsToBeRunAgain = false;


  // Create the instance and special folders if they do not exists yet
  {
    Akonadi::AgentInstance resource = folderManager->foldersResource( false );
    Akobackit::LocalFoldersSetupJob *job = new Akobackit::LocalFoldersSetupJob( resource, this );
    if ( !job->exec() ) {
      switch ( job->error() ) {
        case Akobackit::LocalFoldersSetupJob::ResourceCreationError:
          // Abort
          emit progress( MAX_PROGRESS );
          emit message( i18nc( "%1: error code", "Unable to create the local folders resource (error %1).", "AKM01" ), LogError );
          mError = Error;
          return;
          break;
        case Akobackit::LocalFoldersSetupJob::RootFolderSetupError:
          emit progress( MAX_PROGRESS );
          emit message( i18nc( "%1: error code", "Unable to create the local folders resource (error %1).", "AKM02" ), LogError );
          mError = Error;
          break;
        case Akobackit::LocalFoldersSetupJob::SpecialCollectionSetupError:
          needsToBeRunAgain = true;
          emit message( i18n( "Unable to create the outbox, drafts or sent-mail folder" ), LogWarn );
          mError = Warning;
          break;
        default:
          emit progress( MAX_PROGRESS );
          emit message( i18nc( "%1:original error message, %2:original error code, %3: error code",
                              "An unknow error occurs: %1 (error %3/%2).",
                              job->errorString(), job->error(), "AKM03" ),
                        LogError );
          mError = Error;
          return;
          break;
      }
    }

    folderManager->foldersResourceCreated( job->instance() );

    emit progress( PROGRESS_AFTER_RESOURCE_CREATION );
  }


  // Create folders
  QMap<int, Akonadi::Collection> idMatching; // key=old folder ID ; value=new folder collection
  {
    int folderCount = oldFolders.size(); // used calculate progress
    QList<int> errorFolderIds; // List of folder's id that can not be created.
    while ( !oldFolders.isEmpty() ) {
      const KNFolder::Ptr &oldFolder = oldFolders.takeFirst();

      if ( oldFolder->isRootFolder() ) {
        idMatching.insert( oldFolder->id(), folderManager->rootFolder() );
        continue;
      }

      const QString configKey = QString( "collection for folder %1" ).arg( oldFolder->id() );

      // Already done ?
      {
        Akonadi::Entity::Id id = config.readEntry( configKey , -1 );
        if ( id != -1 ) {
          idMatching.insert( oldFolder->id(), Akonadi::Collection( id ) );
          continue;
        }
      }

      // Check the parent
      Akonadi::Collection parent = idMatching.value( oldFolder->parentId() );
      if ( !parent.isValid() ) {
        // Parent not created yet: let's check it exists somewhere
        bool foundParent = false;
        foreach ( const KNFolder::Ptr &f, oldFolders ) {
          if ( oldFolder->parentId() == f->id() ) {
            foundParent = true;
            break;
          }
        }
        if ( !foundParent ) {
          // The parent is in error or does not exists: reparenting.
          oldFolder->setParent( oldFolderManager->root() );
        }

        // see you next loop
        oldFolders.append( oldFolder );
        continue;
      }

      Akonadi::Collection newFolder;

      // Map special folders
      if ( oldFolder == oldFolderManager->drafts() ) {
        newFolder = folderManager->draftsFolder();
      } else if ( oldFolder == oldFolderManager->outbox() ) {
        newFolder = folderManager->outboxFolder();
      } else if ( oldFolder == oldFolderManager->sent() ) {
        newFolder = folderManager->sentmailFolder();
      }

      // Create a new collection
      if ( !newFolder.isValid() ) {
        newFolder.setParentCollection( parent );

        newFolder.setName( oldFolder->name() );
        // Avoid sibling with the same name
        Akonadi::CollectionFetchJob *fetchJob = new Akonadi::CollectionFetchJob( parent, Akonadi::CollectionFetchJob::FirstLevel, this );
        if ( fetchJob->exec() ) {
          Akonadi::Collection::List siblings = fetchJob->collections();
          bool needNewName = false;
          do {
            needNewName = false;
            foreach ( const Akonadi::Collection &col, siblings ) {
              if ( col.name() == newFolder.name() ) {
                kDebug() << "Modifying name of collection" << newFolder.name();
                newFolder.setName( newFolder.name() + ' ' );
                needNewName = true;
              }
            }
          } while ( needNewName );
        } else {
          kWarning() << "Unable to fetch siblings of collection" << newFolder.name();
          // Assume there is no sibling with this name. If there is one, the next CollectionCreateJob will
          // fail anyway.
        }

        Akonadi::CollectionCreateJob *job = new Akonadi::CollectionCreateJob( newFolder, this );
        if ( !job->exec() ) {
          needsToBeRunAgain = true;
          emit message( i18nc( "%1:original error message, %2:original error code, %3: error code",
                              "<qt>Unable to migrate the folder <em>%4</em>: %1 (error %3/%2).</qt>",
                              job->errorString(), job->error(), "AKM04", oldFolder->name() ),
                        LogWarn );
          mError = Warning;
          continue;
        }
        newFolder = job->collection();
        Q_ASSERT( newFolder.isValid() );
      }

      Q_ASSERT( newFolder.isValid() );
      idMatching.insert( oldFolder->id(), newFolder );
      config.writeEntry( configKey, newFolder.id() );
      config.sync();

      const int done = ( ( PROGRESS_AFTER_FOLDERS_CREATION - PROGRESS_AFTER_RESOURCE_CREATION ) * ( folderCount - oldFolders.size() ) ) / folderCount;
      emit progress ( PROGRESS_AFTER_RESOURCE_CREATION + done );
    }

    emit progress( PROGRESS_AFTER_FOLDERS_CREATION );
  }

  // Migrate message and their flags
  {
    QList<int> oldFoldersId = idMatching.keys();
    int folderCount = oldFoldersId.size();
    int folderDoneCount = 0;
    foreach ( int id, oldFoldersId ) {
      const KNFolder::Ptr &oldFolder = oldFolderManager->folder( id );
      const Akonadi::Collection &newFolder = idMatching[ id ];

      if ( oldFolder == oldFolderManager->root() ) {
        continue;
      }

      emit message( i18n( "<qt>Migrating message from the folder <em>%1</em></qt>", oldFolder->name() ) );

      oldFolderManager->loadHeaders( oldFolder );

      LocalArticle::List articles;
      for ( int i = 0 ; i < oldFolder->length() ; ++i ) {
        if ( oldFolder->at( i )->type() != KNArticle::ATlocal ) {
          // That's not even possible...
          kError() << "An article was not a KNLocalArticle in a folder";
          continue;
        }
        const KNLocalArticle::Ptr &oldLocalArticle = boost::static_pointer_cast<KNLocalArticle>( oldFolder->at( i ) );

        const LocalArticle::Ptr &article = LocalArticle::Ptr( new LocalArticle( Akonadi::Item() ) );
        // content
        oldFolder->loadArticle( oldLocalArticle ); // Load the full article content
        oldLocalArticle->assemble(); // mandated by encodedContent()
        article->setContent( oldLocalArticle->encodedContent() );
        article->parse();

        // flags
        article->setDoPost( oldLocalArticle->doPost() );
        article->setPosted( oldLocalArticle->posted() );
        article->setDoMail( oldLocalArticle->doMail() );
        article->setMailed( oldLocalArticle->mailed() );
        article->setEditDisabled( oldLocalArticle->editDisabled() );
        article->setCanceled( oldLocalArticle->canceled() );
        // TODO: restore serverid once the account are migrated!

        articles.append( article );
      }
      if ( !articles.isEmpty() ) {
        Akobackit::ItemsMergeJob *job = new Akobackit::ItemsMergeJob( articles, newFolder, this );
        if ( !job->exec() ) {
          needsToBeRunAgain = true;
          // TODO: deals with error
          continue;
        }
      }

      const QString configKey = QString( "message for folder %1" ).arg( oldFolder->id() );
      config.writeEntry( configKey , true );
      config.sync();

      // Save memory: unload articles.
      oldFolder->clear();


      ++folderDoneCount;
      const int done = ( ( PROGRESS_AFTER_MESSAGES_MIGRATION - PROGRESS_AFTER_FOLDERS_CREATION ) * folderDoneCount ) / folderCount;
      emit progress ( PROGRESS_AFTER_FOLDERS_CREATION + done );

    }

  }

  emit progress( PROGRESS_AFTER_MESSAGES_MIGRATION );


  markDone( config, !needsToBeRunAgain );
}








void AkonadiMigrator::markDone( KConfigGroup &config, bool done )
{
  config.writeEntry( "done", done );
  config.sync();
}



}
}

