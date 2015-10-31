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

#ifndef KNODE_GROUPSELECTION_SUBSCRIPTIONSTATEGROUPINGPROXYMODEL_H
#define KNODE_GROUPSELECTION_SUBSCRIPTIONSTATEGROUPINGPROXYMODEL_H

#include <QtCore/QVector>
#include <QtGui/QSortFilterProxyModel>

namespace KNode {
namespace GroupSelection {

/**
 * This model places the subscribed (respectively unsubscribed) groups
 * together under a common ancestor.
 *
 * It is assumed that a SubscriptionChangeFilterProxyModel is a source model.
 */
class SubscriptionStateGroupingProxyModel : public QAbstractProxyModel
{
    Q_OBJECT

    public:
        SubscriptionStateGroupingProxyModel(QObject* parent);
        virtual ~SubscriptionStateGroupingProxyModel();

    public:
        /**
         * Reimplemented to provide data for titles.
         */
        virtual QVariant data(const QModelIndex& proxyIndex, int role = Qt::DisplayRole) const;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const;
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
        virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
        virtual QModelIndex parent(const QModelIndex& child) const;
        virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

        virtual QModelIndex mapFromSource(const QModelIndex& sourceIndex) const;
        virtual QModelIndex mapToSource(const QModelIndex& proxyIndex) const;

        virtual void setSourceModel(QAbstractItemModel* sourceModel);

    Q_SIGNALS:
        /**
         * Emitted to inform the view that.
         */
        void changed();

    private Q_SLOTS:
        /**
         * Connected to the source model to update mSubscribed and mUnsubscribed.
         */
        void sourceModelRowsInserted(const QModelIndex& parent, int start, int end);
        /**
         * Connected to the source model to update mSubscribed and mUnsubscribed.
         */
        void sourceModelRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);
        /**
         * Connected to the source model to update mSubscribed and mUnsubscribed.
         */
        void sourceModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

        /**
         * Refresh the internal state when the source model is resetted.
         */
        void refreshInternal();

    private:
        QVector<QPersistentModelIndex> mSubscribed;
        QVector<QPersistentModelIndex> mUnsubscribed;
        QVector<QPersistentModelIndex> mExisting;


        /**
         * Returns the index of @p list to insert @p idx such as the QModelIndex in the list
         * are sorted by their display role.
         */
        int findInsertionPlace(QVector<QPersistentModelIndex>& list, const QModelIndex& idx) const;
};

}
}

#endif
