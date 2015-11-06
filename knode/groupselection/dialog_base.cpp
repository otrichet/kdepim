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

#include "dialog_base.h"

#include <QtCore/QTimer>
#include <KDE/KRecursiveFilterProxyModel>
#include <KDE/KDebug>

#include "enums.h"
#include "helper/group_list_date_picker.h"
#include "model/checked_state_proxy_model.h"
#include "model/group_model.h"
#include "model/recent_group_proxy_model.h"
#include "model/subscription_state_proxy_model.h"
#include "model/subscription_state_grouping_proxy_model.h"

#include "scheduler.h"

namespace KNode {
namespace GroupSelection {

BaseDialog::BaseDialog(QWidget* parent, KNNntpAccount::Ptr account)
    : KDialog(parent),
      mAccount(account),
      mGroupModel(0),
      mSubscriptionModel(0)
{
    setupUi(this);
    setCaption(i18nc("@title:window", "Subscribe to Newsgroups"));
    setMainWidget(page);
    if(QApplication::isLeftToRight()) {
        mAddChangeButton->setIcon(KIcon("arrow-right"));
        mRevertChangeButton->setIcon(KIcon("arrow-left"));
    } else {
        mAddChangeButton->setIcon(KIcon("arrow-left"));
        mRevertChangeButton->setIcon(KIcon("arrow-right"));
    }

    setButtons(Ok | Cancel | Help | User1 | User2);
    setHelp("anc-fetch-group-list");
    setButtonText(User1, i18nc("@action:button Fetch the list of groups from the server", "New &List"));
    connect(this, SIGNAL(user1Clicked()), this, SLOT(slotRequestNewList()));
    setButtonText(User2, i18nc("@action:button Fetch the list of groups from the server", "New &Groups..."));
    connect(this, SIGNAL(user2Clicked()), this, SLOT(slotRequestGroupSince()));


    QTimer::singleShot(5, this, SLOT(init()));
}



BaseDialog::~BaseDialog()
{
    KNode::Scheduler* s = KNGlobals::self()->scheduler();
    s->cancelJobs(KNJobData::JTLoadGroups);
    s->cancelJobs(KNJobData::JTFetchGroups);
}

void BaseDialog::toSubscribe(QList<KNGroupInfo>& list)
{
    list << mSubscriptionModel->subscribed();
}

void BaseDialog::toUnsubscribe(QStringList& list)
{
    Q_FOREACH(const KNGroupInfo& gi, mSubscriptionModel->unsubscribed()) {
        list << gi.name;
    }
}

void BaseDialog::slotReceiveList(KNGroupListData::Ptr data)
{
    QStringList subscribed;
    QList<KNGroupInfo>* groups = 0;

    if(data) {
        groups = data->extractList();

        Q_FOREACH(const KNGroupInfo& gi, *groups) {
            if(gi.subscribed) {
                subscribed << gi.name;
            }
        }
    }
    mSubscriptionModel->setOriginalSubscriptions(subscribed);
    mGroupModel->newList(groups);

    enableButton(User1, true);
    enableButton(User2, true);
}


void BaseDialog::init()
{
    // Group model
    mGroupModel = new GroupModel(this);
    // Proxy that keeps trace of (un)subscription changes
    mSubscriptionModel = new SubscriptionStateProxyModel(this);
    mSubscriptionModel->setSourceModel(mGroupModel);

    // View of all groups and its dedicated proxy models
    KRecursiveFilterProxyModel* searchProxy = new KRecursiveFilterProxyModel(this);
    searchProxy->setSourceModel(mSubscriptionModel);
    RecentGroupProxyModel* filterRecentGroup = new RecentGroupProxyModel(this);
    filterRecentGroup->setSourceModel(searchProxy);
    CheckedStateConvertionProxyModel* checkableConvertionProxyModel = new CheckedStateConvertionProxyModel(this);
    checkableConvertionProxyModel->setSourceModel(filterRecentGroup);
    mGroupsView->setModel(checkableConvertionProxyModel);

    // View of subscription changes and its models
    SubscriptionStateGroupingProxyModel* groupByStateProxy = new SubscriptionStateGroupingProxyModel(this);
    groupByStateProxy->setSourceModel(mSubscriptionModel);
    mChangeView->setModel(groupByStateProxy);
    connect(groupByStateProxy, SIGNAL(changed()),
            mChangeView, SLOT(expandAll()));

    // Filter & sort
    mSearchLine->setProxy(searchProxy);
    searchProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    searchProxy->setSortLocaleAware(true);
    searchProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    searchProxy->sort(GroupModelColumn_Name, Qt::AscendingOrder);
    mNewOnlyCheckbox->setChecked(filterRecentGroup->isNewOnlyEnabled());
    connect(mNewOnlyCheckbox, SIGNAL(toggled(bool)),
            filterRecentGroup, SLOT(setEnable(bool)));
    mTreeviewCheckbox->setChecked(mGroupModel->modelAsTree());
    connect(mTreeviewCheckbox, SIGNAL(toggled(bool)),
            mGroupModel, SLOT(modelAsTree(bool)));




    // Operation on selection
    connect(mAddChangeButton, SIGNAL(clicked(bool)),
            this, SLOT(revertSelectionStateChange()));
    connect(mRevertChangeButton, SIGNAL(clicked(bool)),
            this, SLOT(revertSelectionStateChange()));
    connect(mChangeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(slotSelectionChange()));
    connect(mGroupsView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotSelectionChange()));

    // Request the list of groups
    emit loadList(mAccount);
}


void BaseDialog::revertSelectionStateChange()
{
    QAbstractItemView* view = (sender() == mAddChangeButton ? mGroupsView : mChangeView);

    // Operate on QPersistentModelIndex because call
    // to setData() below will invalidates QModelIndex.

    QList<QPersistentModelIndex> persistentIndices;
    const QModelIndexList& indexes = view->selectionModel()->selectedIndexes();
    foreach(const QModelIndex& index, indexes) {
        persistentIndices << index;
    }

    QAbstractItemModel* model = view->model();
    foreach(const QPersistentModelIndex& index, persistentIndices) {
        QVariant v = index.data(SubscriptionStateRole);
        if (!v.isValid()) {
            continue;
        }
        SubscriptionState state = v.value<SubscriptionState>();
        if(view == mChangeView) {
            if(state == NewSubscription || state == ExistingSubscription) {
                state = NewUnsubscription;
            } else if(state == NewUnsubscription) {
                state = NewSubscription;
            } else {
                continue;
            }
        } else if(view == mGroupsView) {
            if(state == ExistingSubscription) {
                state = NewUnsubscription;
            } else if(state == NoStateChange) {
                state = NewSubscription;
            } else {
                continue;
            }
        } else {
            continue;
        }

        model->setData(index, QVariant::fromValue(state), SubscriptionStateRole);
    }
}

void BaseDialog::slotSelectionChange()
{
    mAddChangeButton->setEnabled(mGroupsView->selectionModel()->hasSelection());
    mRevertChangeButton->setEnabled(mChangeView->selectionModel()->hasSelection());
}


void BaseDialog::slotRequestNewList()
{
    enableButton(User1, false);
    enableButton(User2, false);
    emit fetchList(mAccount);
}

void BaseDialog::slotRequestGroupSince()
{
    QPointer<GroupListDatePicker> diag = new GroupListDatePicker(this, mAccount);
    if(diag->exec() == QDialog::Accepted) {
        enableButton(User1, false);
        enableButton(User2, false);
        emit checkNew(mAccount, diag->selectedDate());
    }
    delete diag;
}


}
}
