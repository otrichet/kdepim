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

#ifndef KNODE_AKOBACKIT_LOCALFOLDERSSETUPJOB_H
#define KNODE_AKOBACKIT_LOCALFOLDERSSETUPJOB_H

#include <Akonadi/AgentInstance>
#include <Akonadi/Collection>
#include <KJob>

namespace Akonadi {
  class AgentInstance;
}

namespace KNode {
namespace Akobackit {

/**
 * This job is responsible for:
 * @li creating the Akonadi resource for KNode's local folders, if it does not
 * exist yet.
 * @li configuring this resource (or reconfiguring it if the configuration was lost/messed up).
 * @li creating/recreating the special folders (outbox, sent-mail, draft). They
 * are recover based on their name if the folder exists but it does not have its special
 * flags in Akonadi.
 */
class LocalFoldersSetupJob : public KJob
{
  Q_OBJECT

  public:
    /**
     * Create a new job to setup the local folders.
     * @param resource The existing folders resource or an invalid resource if none
     * is found in KNode's configuration.
     * @param parent The parent object.
     * @param ensureSpecialFoldersExistOnly When true, the resource is not created or configured; this
     * job only check if special folders are still present.
     */
    explicit LocalFoldersSetupJob( const Akonadi::AgentInstance &resource, QObject *parent = 0, bool ensureSpecialFoldersExistOnly = false );
    /**
     * Destructor.
     */
    virtual ~LocalFoldersSetupJob();

    /**
     * @reimplemented
     */
    virtual void start();

    /**
     * Returns the local folders resource. It may be invalid if an error occurs.
     */
    Akonadi::AgentInstance instance() const;

    enum {
      ResourceCreationError        = KJob::UserDefinedError + 1,
      RootFolderSetupError         = KJob::UserDefinedError + 2,
      SpecialCollectionSetupError  = KJob::UserDefinedError + 3
    };

  signals:
    /**
     * This signal is emitted when the local folders resource is correctly
     * configured. This allow the parent of this job to keep track of the
     * resource identifier for future reconfiguration.
     * @note This signal may be emitted even if this job is in error.
     */
    void resourceCreated( const Akonadi::AgentInstance &resource );

  private:
    /**
     * Configure the resource if this has not been done yet.
     * (name, maildir path).
     * @param resourceIsNew when true, the resource existed before this job
     * start.
     */
    void configureResource( bool resourceIsNew );
    /**
     * Start a synchronization job of the maildir resource.
     */
    void startCollectionTreeSynchronization();
    /**
     * Fetch all collections of the resource for setting up
     * the root folder and the special collections.
     */
    void fetchCollections();
    /**
     * Setup the root folder (name, icon
     */
    void setupRootFolder();
    /**
     * Ensures that special collections exist in @p resource.
     * These are outbox, sent-mail and drafts.
     */
    void ensureSpecialCollectionExists();



    /**
     * The maildir resource that is created/worked on.
     */
    Akonadi::AgentInstance mResource;
    /**
     * Collections found in the resource.
     */
    Akonadi::Collection::List mCollections;

    bool mOnlySpecialFolder;


  private slots:
    /**
     * Called by start().
     */
    void doStart();

    /**
     * Result slot for the job creating the local folders resource.
     */
    void foldersResourceCreated( KJob *job );
    /**
     * Result slot for the job synchronizing the resource.
     */
    void synchronizationFinished( KJob *job );
    /**
     * Result slot the for the collections fetch.
     */
    void collectionsFetched( KJob *job );

};

}
}

#endif
