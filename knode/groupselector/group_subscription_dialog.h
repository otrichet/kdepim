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


#ifndef KNODE_GROUPSUBSCRIPTIONDIALOG_H
#define KNODE_GROUPSUBSCRIPTIONDIALOG_H

#include "akobackit/nntpaccount.h"
#include "ui_group_subscription_dialog.h"

#include <Akonadi/Collection>
#include <KDialog>

namespace Akonadi {
  class ChangeRecorder;
  class Collection;
}
class KJob;

namespace KNode {

class SubscriptionStateModel;

/**
 * A dialog to (un)subscribe to groups of an account.
 */
class GroupSubscriptionDialog : public KDialog, private Ui::GroupSelectionDialog
{
  Q_OBJECT

  public:
    /**
     * Create a new dialog window to subscribe to group of @p account.
     * It will auto-delete when it is closed.
     */
    GroupSubscriptionDialog( QWidget *parent, NntpAccount::Ptr account );
    virtual ~GroupSubscriptionDialog();

  protected slots:
    /**
     * Validate subscription changes made by the user.
     */
    virtual void slotButtonClicked( int button );

  private slots:
    void fetchResult( KJob *job );
    /**
     * Calls revertSelectionStateChange() on selection items from the change view.
     */
    void revertStateChangeFromChangeView();
    /**
     * Calls revertSelectionStateChange() on selection items from the groups view.
     */
    void revertStateChangeFromGroupView();

    /**
     * Called when the items selection in any of the view is modified.
     */
    void slotSelectionChange();

  private:
    void initModelView();

    /**
     * Revert change to the subscription state of the current selection
     * in @p view.
     */
    void revertSelectionStateChange( QAbstractItemView *view );

    SubscriptionStateModel *mSubscriptionModel;

    /**
     * True when the root collection of the account
     * passed to the constructor has been fetched.
     */
    bool mRootFetched;
    /**
     * True when subscribed collection has been fetched.
     */
    bool mSubscribedFetched;

    Akonadi::Collection mRoot;
    Akonadi::Collection::List mSubscribedCollections;
};

}

#endif
