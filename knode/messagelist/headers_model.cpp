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
#include <KDE/KLocalizedString>

#include "knarticlefilter.h"
#include "knfiltermanager.h"
#include "settings.h"

namespace KNode
{
namespace MessageList
{

static const int INVALID_ID = -1;

struct Header
{
    Header(KNRemoteArticle::Ptr a)
        : article(a), parent(0), children()
    {};
    ~Header()
    {
        article.reset();
        parent = 0;
        qDeleteAll(children);
        children.clear();
    }

    KNRemoteArticle::Ptr article;
    Header* parent;
    QList<Header*> children;
};

static QVariant extractFrom(const KNArticle::Ptr& art)
{
    KMime::Headers::From* from = art->from();
    if(from && !from->mailboxes().isEmpty()) {
        const KMime::Types::Mailbox mb = from->mailboxes().first();
        if(mb.hasName()) {
            return mb.name();
        } else {
            return mb.prettyAddress(KMime::Types::Mailbox::QuoteNever);
        }
    }
    return QVariant();
}



HeadersModel::HeadersModel(QObject* parent)
    : QAbstractItemModel(parent),
      mRoot(new Header(KNRemoteArticle::Ptr())),
      mGroup(),
      mSortByThreadChangeDate(false)
{
    mDateFormatter.setCustomFormat(KNGlobals::self()->settings()->customDateFormat());
    mDateFormatter.setFormat(KNGlobals::self()->settings()->dateFormat());

    mFilter = KNGlobals::self()->filterManager()->currentFilter();

    mNormlizeSubject = new QRegExp("^\\s*(?:tr|re[.f]?|fwd?)\\s*:\\s*", Qt::CaseInsensitive);
}

HeadersModel::~HeadersModel()
{
    delete mRoot;
    mGroup.reset();
}


void HeadersModel::setSortedByThreadChangeDate(bool b)
{
    if(mSortByThreadChangeDate != b) {
        // TODO
    }
}

bool HeadersModel::sortedByThreadChangeDate()
{
    return mSortByThreadChangeDate;
}


void HeadersModel::setFilter(KNArticleFilter* filter)
{
    mFilter = filter;
    reload(mGroup);
}


void HeadersModel::setGroup(const KNGroup::Ptr group)
{
    if(group != mGroup) {
        reload(group);
    }
}

void HeadersModel::reload(const KNGroup::Ptr group)
{
    Header* root = new Header(KNRemoteArticle::Ptr());
    QHash<QByteArray, Header*> msgIdIndex;

    if(group) {
        if(mFilter) {
            mFilter->doFilter(group);
        }


        for(int i = 0 ; i < group->length() ; ++i) {
            const KNRemoteArticle::Ptr art = group->at(i);
            if(art->filterResult()) {
                msgIdIndex.insert(art->messageID()->as7BitString(false),
                                  new Header(art));
            }
        }

        Q_FOREACH(Header* hdr, msgIdIndex) {
            Header* parent = root;
            KMime::Headers::References* refs = hdr->article->references();
            if(refs && !refs->identifiers().isEmpty()) {
                const QByteArray parentMsgId = '<' + refs->identifiers().last() + '>';
                parent = msgIdIndex.value(parentMsgId, root);
            }
            hdr->parent = parent;
            parent->children.append(hdr);
        }
    }

    Header* oldRoot = mRoot;
    beginResetModel();
    mRoot = root;
    mGroup = group;
    endResetModel();
    delete oldRoot;
}


int HeadersModel::rowCount(const QModelIndex& parent) const
{
    Header* p = parent.isValid() ? static_cast<Header*>(parent.internalPointer()) : mRoot;
    return p->children.count();
}

int HeadersModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return COLUMN_COUNT;
}

QVariant HeadersModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid()) {
        return QVariant();
    }

    const Header* hdr = static_cast<Header*>(index.internalPointer());
    const KNRemoteArticle::Ptr art = hdr->article;

    switch(role) {
    case Qt::DisplayRole:
        switch(index.column()) {
            case COLUMN_SUBJECT:
                if(art->subject()) {
                    return art->subject()->asUnicodeString();
                }
                break;
            case COLUMN_FROM:
                return extractFrom(art);
                break;
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
    case ArticleRole:
        return QVariant::fromValue(art);
        break;
    case ReadRole:
        return QVariant::fromValue(art->isRead());
        break;
    case SortRole:
        switch(index.column()) {
            case COLUMN_SUBJECT:
                if(art->subject()) {
                    return art->subject()->asUnicodeString().replace(*mNormlizeSubject, "");
                }
                break;
            case COLUMN_FROM:
                return extractFrom(art);
                break;
            case COLUMN_DATE:
                return art->date()->dateTime().dateTime();
                break;
        }
        break;
    }

    return QVariant();
}

QVariant HeadersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal) {
        switch(role) {
            case Qt::DisplayRole:
                switch(section) {
                    case COLUMN_SUBJECT:
                        return i18n("Subject");
                    case COLUMN_FROM:
                        return i18n("From");
                    case COLUMN_DATE:
                        return i18n("Date");
                }
                break;
        }
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}


QModelIndex HeadersModel::index(int row, int column, const QModelIndex& parent) const
{
    Header* p = parent.isValid() ? static_cast<Header*>(parent.internalPointer()) : mRoot;
    if(row >= p->children.count()) {
        return QModelIndex();
    }
    return createIndex(row, column, p->children.at(row));
}

QModelIndex HeadersModel::parent(const QModelIndex& child) const
{
    if(!child.isValid()) {
        return QModelIndex();
    }

    Header* c = static_cast<Header*>(child.internalPointer());
    Header* p = c->parent;

    // Parent is the root
    if(p->parent == 0) {
        return QModelIndex();
    }

    int row = p->parent->children.indexOf(p);
    return createIndex(row, 0, p);
}


}
}
