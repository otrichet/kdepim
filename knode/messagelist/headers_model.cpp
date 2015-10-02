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


#include "headers_model.h"

#include <QFont>
#include <KDE/KDebug>

#include "settings.h"


namespace KNode
{
namespace MessageList
{

static const int INVALID_ID = -1;

HeadersModel::HeadersModel(QObject* parent)
    : QAbstractItemModel(parent),
      mChildren(),
      mGroup()
{
    mDateFormatter.setCustomFormat(KNGlobals::self()->settings()->customDateFormat());
    mDateFormatter.setFormat(KNGlobals::self()->settings()->dateFormat());
}

HeadersModel::~HeadersModel()
{
    mChildren.clear();
    mGroup.reset();
}

void HeadersModel::setGroup(const KNGroup::Ptr group)
{
    QMultiHash<qint64, qint64> newChildren;
    QHash<QByteArray, int> msgIdIndex;

    if(group) {
        for(int i = 0 ; i < group->length() ; ++i) {
            const KNArticle::Ptr art = group->at(i);
            msgIdIndex.insert(art->messageID()->as7BitString(false), i);
        }

        for(int i = 0 ; i < group->length() ; ++i) {
            const KNArticle::Ptr art = group->at(i);
            int parentId = INVALID_ID;
            KMime::Headers::References* refs = art->references();
            if(refs && !refs->identifiers().isEmpty()) {
                const QByteArray parentMsgId = '<' + refs->identifiers().last() + '>';
                parentId = msgIdIndex.value(parentMsgId, INVALID_ID);
                //if(parentId == INVALID_ID) {
                //    kDebug() << "No parent found for" << art->messageID()->as7BitString(false) << "References:" << refs->as7BitString(false);
                //}
            }
            newChildren.insertMulti(parentId, i);
        }
    }

    emit layoutAboutToBeChanged();
    mChildren = newChildren;
    mGroup = group;
    emit layoutChanged();
}


int HeadersModel::rowCount(const QModelIndex& parent) const
{
    qint64 parentId = (parent.isValid() ? parent.internalId() : INVALID_ID);
    return mChildren.count(parentId);
}

int HeadersModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return COLUMN_COUNT;
}

QVariant HeadersModel::data(const QModelIndex& index, int role) const
{
    if(!mGroup) {
        return QVariant();
    }

    qint64 id = index.internalId();
    const KNRemoteArticle::Ptr art = mGroup->at(id);
    if(!art) {
        return QVariant();
    }

    switch(role) {
    case Qt::DisplayRole:
        switch(index.column()) {
            case COLUMN_SUBJECT:
                if(art->subject()) {
                    return art->subject()->asUnicodeString();
                }
                break;
            case COLUMN_FROM: {
                KMime::Headers::From* from = art->from();
                if(from && !from->mailboxes().isEmpty()) {
                    const KMime::Types::Mailbox mb = from->mailboxes().first();
                    if(mb.hasName()) {
                        return mb.name();
                    } else {
                        return mb.prettyAddress(KMime::Types::Mailbox::QuoteNever);
                    }
                }
                break;
            }
            case COLUMN_DATE:
                if(art->date()) {
                    return mDateFormatter.dateString(art->date()->dateTime().dateTime());
                }
                break;
        }
        break;
    case Qt::FontRole:
        if(!art->isRead()) {
            QFont font;
            font.setBold(true);
            return font;
        }
        break;
    }

    return QVariant();
}

QModelIndex HeadersModel::index(int row, int column, const QModelIndex& parent) const
{
    qint64 parentId = (parent.isValid() ? parent.internalId() : INVALID_ID);
    if(row >= mChildren.count(parentId)) {
        return QModelIndex();
    }
    QHash<qint64, qint64>::const_iterator it = mChildren.find(parentId) + row;
    return createIndex(row, column, (int)it.value());
}

QModelIndex HeadersModel::parent(const QModelIndex& child) const
{
    if(!child.isValid()) {
        return QModelIndex();
    }
    qint64 childId = child.internalId();

    // NOTE: review if calling key() is efficient enough.
    qint64 parentId = mChildren.key(childId);
    if(parentId == INVALID_ID) {
        return QModelIndex();
    }

    qint64 grandParentId = mChildren.key(parentId);
    QHash<qint64, qint64>::const_iterator begin = mChildren.constFind(grandParentId);
    QHash<qint64, qint64>::const_iterator it    = mChildren.constFind(grandParentId, parentId);
    int row = 0;
    while(it != begin) {
        ++row;
        --it;
    }
    return createIndex(row, 0, (int)parentId);
}


}
}
