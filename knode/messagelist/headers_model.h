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


#ifndef KNODE_HEADERS_MODEL_H
#define KNODE_HEADERS_MODEL_H

#include <QtCore/QAbstractItemModel>
#include <QtCore/QByteArray>

#include <kmime/kmime_dateformatter.h>

#include "knarticle.h"
#include "kngroup.h"

class KNArticleFilter;

namespace KNode
{
namespace MessageList
{

class HeadersModel : public QAbstractItemModel
{
    private:
        /**
         * Column index.
         */
        enum HeaderColumnIndex
        {
            COLUMN_SUBJECT = 0,
            COLUMN_FROM,
            COLUMN_DATE,

            COLUMN_COUNT
        };

    public:
        /**
         * Custom role for the #data() method.
         */
        enum HeadersRole
        {
            InvalidRole = Qt::UserRole,
            ArticleRole,                 ///< The Article::Ptr.
            ReadRole,                    ///< Indicate if the article is read (bool).
        };

    public:
        explicit HeadersModel(QObject* parent = 0);
        virtual ~HeadersModel();

        /**
         * Change the group.
         * @param group The new group.
         */
        void setGroup(const KNGroup::Ptr group);
        /**
         * Change the filter.
         */
        void setFilter(KNArticleFilter* filter);

        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
        virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
        virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
        virtual QModelIndex parent(const QModelIndex& child) const;

    private:
        /**
         * Reload the internal data structure.
         */
        void reload(KNGroup::Ptr group);

        // { parent -> [ child_1, ... ] }
        QMultiHash<qint64, qint64> mChildren;
        KNGroup::Ptr mGroup;
        KNArticleFilter* mFilter;
        KMime::DateFormatter mDateFormatter;
};


}
}


#endif
