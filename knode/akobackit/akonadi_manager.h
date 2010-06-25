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

#ifndef KNODE_AKOBACKIT_AKOMANAGER_H
#define KNODE_AKOBACKIT_AKOMANAGER_H

#include "akobackit/constant.h"
#include "knode_export.h"

#include <QtCore/QObject>

namespace Akonadi {
  class ChangeRecorder;
  class Collection;
  class EntityTreeModel;
  class Session;
}

namespace KNode {
namespace Akobackit {

class AkoManager;
class FolderManager;
class GroupManager;

/**
 * Returns the single AkoManager instance.
 */
KNODE_EXPORT AkoManager * manager();


/**
 * This singleton is the main point of access to Akonadi.
 */
class AkoManager : public QObject
{
  friend class AkoManagerPrivate;

  private:
    /**
     * Create a new AkoManager object, should only be called by the friend class AkoManagerPrivate.
     */
    AkoManager( QObject *parent = 0 );
    /**
     * Destroy the AkoManager.
     */
    virtual ~AkoManager();

  public:
    /**
     * Returns the change recorder for use in EntityTreeModel.
     */
    Akonadi::ChangeRecorder * monitor();
    /**
     * Returns the base model of the collection tree view.
     * It includes both the nntp accounts and the local folders.
     */
    Akonadi::EntityTreeModel * collectionModel();

    /**
     * Returns the type of the given collection.
     */
    CollectionType type( const Akonadi::Collection &col );

    /**
     * Returns the manager responsible of local folders.
     */
    Akobackit::FolderManager * folderManager()
    {
      return mFolderManager;
    };
    /**
     * Returns the manager responsible of newsgroups.
     */
    Akobackit::GroupManager * groupManager()
    {
      return mGroupManager;
    };

  private:
    Akonadi::ChangeRecorder *mMonitor;
    Akonadi::EntityTreeModel *mBaseModel;
    Akonadi::Session *mSession;

    FolderManager *mFolderManager;
    GroupManager *mGroupManager;
};

}
}

#endif
