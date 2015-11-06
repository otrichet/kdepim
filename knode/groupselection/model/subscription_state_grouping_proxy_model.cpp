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

#include "subscription_state_grouping_proxy_model.h"

#include "../enums.h"

#include <QtGui/QFont>
#include <KDE/KLocalizedString>
#include <KDE/KDebug>

namespace KNode {
namespace GroupSelection {

static const int ROW_TITLE_SUBSCRIBE = 0;
static const int ROW_TITLE_UNSUBSCRIBE = 1;
static const int ROW_TITLE_EXISTING = 2;

static const int COLUMN = 0;

// Use the internalId of QModelIndex to store the type of item.
static const quint32 INDEX_TYPE_TITLE = 0;
static const quint32 INDEX_TYPE_SUBSCRIBED = 1;
static const quint32 INDEX_TYPE_UNSUBSCRIBED = 2;
static const quint32 INDEX_TYPE_EXISTING = 3;


SubscriptionStateGroupingProxyModel::SubscriptionStateGroupingProxyModel(QObject*parent)
    : QAbstractProxyModel(parent),
      mSubscribed(), mUnsubscribed(), mExisting()
{
}

SubscriptionStateGroupingProxyModel::~SubscriptionStateGroupingProxyModel()
{
}


QModelIndex SubscriptionStateGroupingProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    if(!sourceIndex.isValid()) {
        return QModelIndex();
    }

    const QPersistentModelIndex index(sourceIndex);

    int idx = mSubscribed.indexOf(index);
    if(idx != -1) {
        return createIndex(idx, COLUMN, INDEX_TYPE_SUBSCRIBED);
    }

    idx = mUnsubscribed.indexOf(index);
    if(idx != -1) {
        return createIndex(idx, COLUMN, INDEX_TYPE_UNSUBSCRIBED);
    }

    idx = mExisting.indexOf(index);
    if(idx != -1) {
        return createIndex(idx, COLUMN, INDEX_TYPE_EXISTING);
    }

    return QModelIndex();
}

QModelIndex SubscriptionStateGroupingProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
    if(!proxyIndex.isValid()) {
        return QModelIndex();
    }

    switch(proxyIndex.internalId()) {
        case INDEX_TYPE_SUBSCRIBED:
            Q_ASSERT(proxyIndex.row() < mSubscribed.size());
            return mSubscribed.at(proxyIndex.row());
        case INDEX_TYPE_UNSUBSCRIBED:
            Q_ASSERT(proxyIndex.row() < mUnsubscribed.size());
            return mUnsubscribed.at(proxyIndex.row());
        case INDEX_TYPE_EXISTING:
            Q_ASSERT(proxyIndex.row() < mExisting.size());
            return mExisting.at(proxyIndex.row());
    }

    return QModelIndex();
}




QVariant SubscriptionStateGroupingProxyModel::data(const QModelIndex& proxyIndex, int role) const
{
    if(proxyIndex.internalId() == INDEX_TYPE_TITLE) {
        switch(role) {
        case Qt::DisplayRole:
            switch(proxyIndex.row()) {
            case ROW_TITLE_SUBSCRIBE:
                return i18nc("@title %1:number of groups", "Subscribed groups (%1)", mSubscribed.size());
            case ROW_TITLE_UNSUBSCRIBE:
                return i18nc("@title %1:number of groups", "Unsubscribed groups (%1)", mUnsubscribed.size());
            case ROW_TITLE_EXISTING:
                return i18nc("@title %1:number of groups", "Current subscription (%1)", mExisting.size());
            }
            break;

        case Qt::FontRole: {
                QFont f = QAbstractProxyModel::data(proxyIndex, role).value<QFont>();
                f.setBold(true);
                return QVariant::fromValue(f);
            }
            break;

        default:
            return QVariant();
            break;
        }
    }

  return QAbstractProxyModel::data(proxyIndex, role);
}


Qt::ItemFlags SubscriptionStateGroupingProxyModel::flags(const QModelIndex& index) const
{
    qint64 internalId = index.internalId();
    if(internalId == INDEX_TYPE_TITLE) {
        return Qt::NoItemFlags; // Inactive, unselectable, etc.
    }
    return QAbstractProxyModel::flags(index);
}




int SubscriptionStateGroupingProxyModel::rowCount(const QModelIndex& parent) const
{
    if(!parent.isValid()) {
        // Only one title: subscribed groups
        return (mExisting.isEmpty() && mUnsubscribed.isEmpty() ? 1 : 3);
    }

    if(parent.internalId() == INDEX_TYPE_TITLE) {
        switch(parent.row()) {
        case ROW_TITLE_SUBSCRIBE:
            return mSubscribed.size();
        case ROW_TITLE_UNSUBSCRIBE:
            return mUnsubscribed.size();
        case ROW_TITLE_EXISTING:
            return mExisting.size();
        }
    }

    return 0;
}

int SubscriptionStateGroupingProxyModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QModelIndex SubscriptionStateGroupingProxyModel::parent(const QModelIndex& child) const
{
    switch(child.internalId()) {
    case INDEX_TYPE_TITLE:
        return QModelIndex();
    case INDEX_TYPE_SUBSCRIBED:
        return createIndex(ROW_TITLE_SUBSCRIBE, COLUMN, INDEX_TYPE_TITLE);
    case INDEX_TYPE_UNSUBSCRIBED:
        return createIndex(ROW_TITLE_UNSUBSCRIBE, COLUMN, INDEX_TYPE_TITLE);
    case INDEX_TYPE_EXISTING:
        return createIndex(ROW_TITLE_EXISTING, COLUMN, INDEX_TYPE_TITLE);
    }

    return QModelIndex();
}

QModelIndex SubscriptionStateGroupingProxyModel::index(int row, int column, const QModelIndex& parent) const
{
    if(!parent.isValid()) {
        if(row == ROW_TITLE_SUBSCRIBE || row == ROW_TITLE_UNSUBSCRIBE || row == ROW_TITLE_EXISTING) {
            return createIndex(row, column, 0);
        }
        return QModelIndex();
    }

    if(parent.internalId() == INDEX_TYPE_TITLE) {
        if(parent.row() == ROW_TITLE_SUBSCRIBE && row < mSubscribed.size()) {
            return createIndex(row, column, INDEX_TYPE_SUBSCRIBED);
        }
        if(parent.row() == ROW_TITLE_UNSUBSCRIBE && row < mUnsubscribed.size()) {
            return createIndex(row, column, INDEX_TYPE_UNSUBSCRIBED);
        }
        if(parent.row() == ROW_TITLE_EXISTING && row < mExisting.size()) {
            return createIndex(row, column, INDEX_TYPE_EXISTING);
        }
        return QModelIndex();
    }

    return QModelIndex();
}

void SubscriptionStateGroupingProxyModel::setSourceModel(QAbstractItemModel* source)
{
    if(sourceModel()) {
        sourceModel()->disconnect(this);
    }
    connect(source, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(sourceModelDataChanged(QModelIndex,QModelIndex)));
    connect(source, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(sourceModelRowsInserted(QModelIndex,int,int)));
    connect(source, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(sourceModelRowsAboutToBeRemoved(QModelIndex,int,int)));
    connect(source, SIGNAL(modelReset()),
            this, SLOT(refreshInternal()));

    QAbstractProxyModel::setSourceModel(source);

    refreshInternal();
}

void SubscriptionStateGroupingProxyModel::refreshInternal()
{
    beginResetModel();

    mSubscribed.clear();
    mUnsubscribed.clear();
    mExisting.clear();

    QVector<QModelIndex> stack;
    stack << sourceModel()->index(0, 0).parent();
    while(!stack.isEmpty()) {
        const QModelIndex index = stack.first();

        QVector<QPersistentModelIndex>* dst = 0;
        switch(index.data(SubscriptionStateRole).value<SubscriptionState>()) {
        case ExistingSubscription:
            dst = &mExisting;
            break;
        case NewSubscription:
            dst = &mSubscribed;
            break;
        case NewUnsubscription:
            dst = &mUnsubscribed;
            break;
        case NoStateChange:
        case Invalid:
            break;
        }
        if(dst) {
            const int i = findInsertionPlace(*dst, index);
            dst->insert(i, index);
        }

        stack.remove(0);

        if(sourceModel()->hasChildren(index)) {
            const int rowCount = sourceModel()->rowCount(index);
            for(int r = 0 ; r < rowCount ; ++r) {
                stack << sourceModel()->index(r, 0, index);
            }
        }
    }

    endResetModel();

    emit changed();
}



void SubscriptionStateGroupingProxyModel::sourceModelRowsInserted(const QModelIndex& parent, int start, int end)
{
    Q_ASSERT(start <= end);

#define INSERT_SOURCE_INDEX(SourceModelIndexList, TitleRow) \
    { \
        int i = findInsertionPlace(SourceModelIndexList, index); \
        const QModelIndex parent = createIndex(TitleRow, COLUMN, INDEX_TYPE_TITLE); \
        beginInsertRows(parent, i, i); \
        SourceModelIndexList.insert(i, index); \
        endInsertRows(); \
        emit dataChanged(parent, parent); \
    } \
    break;

    for(int row = start ; row <= end ; ++row) {
        const QModelIndex index = sourceModel()->index(row, 0, parent);
        const QVariant value = index.data(SubscriptionStateRole);
        SubscriptionState state = value.value<SubscriptionState>();
        switch(state) {
        case NewSubscription:
            INSERT_SOURCE_INDEX(mSubscribed, ROW_TITLE_SUBSCRIBE)
        case NewUnsubscription:
            INSERT_SOURCE_INDEX(mUnsubscribed, ROW_TITLE_UNSUBSCRIBE)
        case ExistingSubscription:
            INSERT_SOURCE_INDEX(mExisting, ROW_TITLE_EXISTING)
        case NoStateChange:
        case Invalid:
            break;
        }
    }

    emit changed();
}

void SubscriptionStateGroupingProxyModel::sourceModelRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    Q_ASSERT(start <= end);

#define TRY_REMOVE_INDEX_ROW(SourceModelIndexList, TitleRow) \
    { \
        int i = SourceModelIndexList.indexOf(index); \
        if(i != -1) { \
            const QModelIndex parent = createIndex(TitleRow, COLUMN, INDEX_TYPE_TITLE); \
            beginRemoveRows(parent, i, i); \
            SourceModelIndexList.remove(i); \
            endRemoveRows(); \
            emit dataChanged(parent, parent); \
            continue; \
        } \
    }

    for(int row = start ; row <= end ; ++row) {
        const QPersistentModelIndex index(sourceModel()->index(row, 0, parent));
        TRY_REMOVE_INDEX_ROW(mSubscribed, ROW_TITLE_SUBSCRIBE);
        TRY_REMOVE_INDEX_ROW(mUnsubscribed, ROW_TITLE_UNSUBSCRIBE);
        TRY_REMOVE_INDEX_ROW(mExisting, ROW_TITLE_EXISTING);
    }

    emit changed();
}

void SubscriptionStateGroupingProxyModel::sourceModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    Q_ASSERT(topLeft.row() <= bottomRight.row());
    Q_ASSERT(topLeft.parent() == bottomRight.parent());

    sourceModelRowsAboutToBeRemoved(topLeft.parent(), topLeft.row(), bottomRight.row());
    sourceModelRowsInserted(topLeft.parent(), topLeft.row(), bottomRight.row());
}


int SubscriptionStateGroupingProxyModel::findInsertionPlace(QVector<QPersistentModelIndex>& list, const QModelIndex& idx) const
{
    const QString name = idx.data(Qt::DisplayRole).toString();
    int i = 0;
    while(i < list.size()) {
        const QString listItemName = list.at(i).data(Qt::DisplayRole).toString();
        if(name < listItemName) {
            break;
        }
        ++i;
    }
    return i;
}


}
}
