/*
 * Copyright (c) 2015 Olivier Trichet <olivier@trichet.fr>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef KNODE_GROUPSELECTION_SUBSCRIPTIONDIALOG_H
#define KNODE_GROUPSELECTION_SUBSCRIPTIONDIALOG_H

#include "ui_group_subscription_dialog.h"

#include <KDE/KDialog>

#include "kngroupmanager.h"
#include "knnntpaccount.h"


namespace KNode {
namespace GroupSelection {

class GroupModel;
class SubscriptionStateProxyModel;

class SubscriptionDialog : public KDialog, private Ui_GroupSelectionDialog
{
    Q_OBJECT

    public:
        SubscriptionDialog(QWidget* parent, KNNntpAccount::Ptr account);
        virtual ~SubscriptionDialog();


        /**
         * Returns the list of groups that were subscribed.
         */
        void toSubscribe(QList<KNGroupInfo>& list);
        /**
         * Returns the list of groups that were unsubscribed.
         */
        void toUnsubscribe(QStringList& list);

    public Q_SLOTS:
        void slotReceiveList(KNGroupListData::Ptr data);

    Q_SIGNALS:
        void loadList(KNNntpAccount::Ptr account);
        void fetchList(KNNntpAccount::Ptr account);
        void checkNew(KNNntpAccount::Ptr account, QDate since);

    private Q_SLOTS:
        void init();

        /**
         * Revert change to the subscription state of the current selection
         * in the view associated with the click button.
         */
        void revertSelectionStateChange();

        /**
         * Called when the items selection in any of the view is modified.
         */
        void slotSelectionChange();

        void slotRequestNewList();
        void slotRequestGroupSince();

    private:
        KNNntpAccount::Ptr mAccount;
        GroupModel* mGroupModel;
        SubscriptionStateProxyModel* mSubscriptionModel;
};

}
}

#endif
