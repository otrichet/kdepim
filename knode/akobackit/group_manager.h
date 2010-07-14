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

#ifndef KNODE_AKOBACKIT_GROUPMANAGER_H
#define KNODE_AKOBACKIT_GROUPMANAGER_H

#include "akobackit/group.h"
#include "akobackit/nntpaccount.h"

#include <QtCore/QObject>

namespace Akonadi {
  class Collection;
}

namespace KNode {
namespace Akobackit {

class AkoManager;

/**
 * Manager of news groups.
 */
class GroupManager : public QObject
{
  Q_OBJECT

  public:
    GroupManager( AkoManager *parent );
    virtual ~GroupManager();

    /**
     * Check if the collection @p col is a folder.
     */
    bool isGroup( const Akonadi::Collection &col );

    /**
     * Returns the list of news groups of an account
     */
    Group::List groups( NntpAccount::Ptr account );

    /**
     * Returns the group for @p collection.
     */
    Group::Ptr group( const Akonadi::Collection &collection );



    /**
     * Edits the property of a group. This opens the edition dialog of the group.
     * @param group The group that have to be configured (must be valid).
     * @param parentWidget the parent widget of the edition dialog.
     */
    void editGroup( Group::Ptr group, QWidget *parentWidget );

    /**
     * Save modification made to @p group.
     */
    void saveGroup( Group::Ptr group );

  private:
    AkoManager *mMainManager;
};

}
}

#endif
