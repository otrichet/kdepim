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

#ifndef KNODE_AKOBACKIT_FOLDERMANAGER_H
#define KNODE_AKOBACKIT_FOLDERMANAGER_H

#include "akobackit/item_local_article.h"

#include <QtCore/QObject>

namespace Akonadi {
  class AgentInstance;
  class Collection;
}
class KJob;

namespace KNode {
namespace Akobackit {

class AkoManager;

class FolderManager : public QObject
{
  Q_OBJECT

  public:
    FolderManager( AkoManager *parent );
    virtual ~FolderManager();

    /**
     * Returns the Akonadi agent responsible for KNode's local folders.
     * @param setup When @em true, the resource is created @em asynchronously if it does
     * not exists yet. An invalid AgentInstance is returned then.
     * @return the Akonadi resource managing the local folders or an invalid resource if none
     * is created.
     */
    Akonadi::AgentInstance foldersResource( bool setup = false ) const;

    /**
     * Returns the root folder or an invalid collection if it is not found.
     * @note this is not Akonadi::Collection::root() but the resource root.
     */
    Akonadi::Collection rootFolder() const;
    /**
     * Returns the outbox folder or an invalid collection if it is not found.
     */
    Akonadi::Collection outboxFolder() const;
      /**
     * Returns the sent-mail folder or an invalid collection if it is not found.
     */
    Akonadi::Collection sentmailFolder() const;
    /**
     * Returns the drafts folder or an invalid collection if it is not found.
     */
    Akonadi::Collection draftsFolder() const;

    /**
     * Check if the collection @p col is a folder.
     */
    bool isFolder( const Akonadi::Collection &col );

    /**
     * Check if a folder @p parent has a child folder named @p childName.
     */
    bool hasChild( const Akonadi::Collection &parent, const QString &childName );

    /**
     * Create a new folder under the existing folder @p parent.
     * The new folder can be retrieved via the folderCreated()
     * signal.
     * @param parent The parent of the new folder.
     * @param name The name of the new folder.
     */
    void createNewFolder( const Akonadi::Collection &parent, const QString &name );

    /**
     * Delete the folder @p f and all its children.
     *
     */
    void removeFolder( const Akonadi::Collection &f );

    /**
     * Empty the folder @p folder.
     */
    void emptyFolder( const Akonadi::Collection &folder );

    /**
     * Move some articles into a folder.
     */
    void moveIntoFolder( const LocalArticle::List &articles, const Akonadi::Collection &folder );

  signals:
    /**
     * This signal is emitted when a new folder is created.
     */
    void folderCreated( const Akonadi::Collection &folder );

  private slots:
    /**
     * Result slot for the job setting up the local folders resource.
     */
    void foldersResourceSetupResult( KJob *job );
    /**
     * Connected to the signal emitted when the resource is created.
     * This store in the config the identifier of this resource.
     */
    void foldersResourceCreated( const Akonadi::AgentInstance &resource );
    /**
     * Result slot for the job that creates a new folder.
     * It emits the createNewFolder() signals.
     */
    void folderCreationResult( KJob *job );
    /**
     * Result for the job that deletes a folder.
     */
    void folderDeletionResult( KJob *job );
    /**
     * Result for the job that empty a folder.
     */
    void folderEmptyingResult( KJob *job );
  private:
    AkoManager *mMainManager;
};

}
}

#endif
