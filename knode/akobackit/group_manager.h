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
     * Returns a group in @p account whose name is @p groupName.
     * @return a null pointer if none is found or a group named @p groupNamed.
     */
    Group::Ptr group( const QString &groupName, NntpAccount::Ptr account );

    /**
     * Returns the account that contains the group @p group.
     */
    NntpAccount::Ptr account( const Group::Ptr &group );


    /**
     * Fetch new headers of a group.
     * @param group The group whose headers should be checked.
     * @param silent If true, no error feedback is presented to the user.
     */
    void fetchNewHeaders( Group::Ptr group, bool silent = false );

    /**
     * Fetch new headers in an account
     * @param account All groups of the this account are checked.
     * @param silent If true, no error feedback is presented to the user.
     */
    void fetchNewHeaders( NntpAccount::Ptr account, bool silent = false );

    /**
     * Fetch new headers in all groups of all accounts.
     * @param silent If true, no error is presented to the user.
     */
    void fetchNewHeaders( bool silent = false);


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


    /**
     * Show the dialog use to (un)subscribe to news groups.
     * @param account The account whose groups will be available for subscription.
     */
    void showSubscriptionDialog( NntpAccount::Ptr account, QWidget *parentWidget );
    /**
     * Unsubscribed from the group represented by @p collection.
     */
    void unsubscribeGroup( const Group::Ptr &group );

  private:
    AkoManager *mMainManager;
};

}
}

#endif
