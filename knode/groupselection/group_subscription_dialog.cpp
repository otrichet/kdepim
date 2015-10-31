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

#include "groupselection/group_subscription_dialog.h"

#include <QtCore/QTimer>
#include <KDE/KDebug>

#include "groupselection/checked_state_proxy_model.h"
#include "groupselection/enums.h"
#include "groupselection/group_model.h"
#include "groupselection/subscription_state_proxy_model.h"
#include "groupselection/subscription_state_grouping_proxy_model.h"


namespace KNode {
namespace GroupSelection {

SubscriptionDialog::SubscriptionDialog(QWidget* parent, KNNntpAccount::Ptr account)
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

    QTimer::singleShot(5, this, SLOT(init()));
}


SubscriptionDialog::~SubscriptionDialog()
{
}

void SubscriptionDialog::toSubscribe(QList<KNGroupInfo>& list)
{
    list << mSubscriptionModel->subscribed();
}

void SubscriptionDialog::toUnsubscribe(QStringList& list)
{
    Q_FOREACH(const KNGroupInfo& gi, mSubscriptionModel->unsubscribed()) {
        list << gi.name;
    }
}

void SubscriptionDialog::slotReceiveList(KNGroupListData::Ptr data)
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
}


void SubscriptionDialog::init()
{
    // Group model
    mGroupModel = new GroupModel(this);
    // Proxy that keeps trace of (un)subscription changes
    mSubscriptionModel = new SubscriptionStateProxyModel(this);
    mSubscriptionModel->setSourceModel(mGroupModel);

    // View of all groups and its dedicated proxy models
    QSortFilterProxyModel* searchProxy = new QSortFilterProxyModel(this);
    searchProxy->setSourceModel(mSubscriptionModel);
    CheckedStateConvertionProxyModel* checkableConvertionProxyModel = new CheckedStateConvertionProxyModel(this);
    checkableConvertionProxyModel->setSourceModel(searchProxy);
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
    searchProxy->sort(GroupModelColumn_Name, Qt::DescendingOrder);

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


void SubscriptionDialog::revertSelectionStateChange()
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

void SubscriptionDialog::slotSelectionChange()
{
    mAddChangeButton->setEnabled(mGroupsView->selectionModel()->hasSelection());
    mRevertChangeButton->setEnabled(mChangeView->selectionModel()->hasSelection());
}


}
}
