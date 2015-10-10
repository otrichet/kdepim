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

#include "headers_view.h"

#include "headers_model.h"


static void extendedNext(QModelIndex& index);

namespace KNode {
namespace MessageList {

HeadersView::HeadersView(QWidget* parent)
    : QTreeView(parent)
{
    setAlternatingRowColors(true);
}

HeadersView::~HeadersView()
{
}

void HeadersView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    KNArticle::List articles;
    Q_FOREACH(const QModelIndex& idx, selected.indexes()) {
        const KNArticle::Ptr& art = model()->data(idx, HeadersModel::ArticleRole).value<KNRemoteArticle::Ptr>();
        if(art) {
            articles.append(art);
        }
    }
    emit articlesSelected(articles);

    QTreeView::selectionChanged(selected, deselected);
}


bool HeadersView::selectNextUnread()
{
    QModelIndex index = currentIndex();
    if(!index.isValid()) {
        // No current selection => search the whole group
        index = model()->index(0, 0, rootIndex());
    } else {
        const QModelIndex child = index.child(0, index.column());
        if(child.isValid()) {
            index = child;
        } else {
            extendedNext(index);
            if(!index.isValid()) {
                return false;
            }
        }
    }

    index = find(index, HeadersModel::ReadRole, false);
    if(index.isValid()) {
        setCurrentIndex(index);
    }
    return index.isValid();
}

bool HeadersView::selectNextUnreadThread()
{
    QModelIndex index = currentIndex();
    if(index.isValid()) {
        // Find the root of the thread
        QModelIndex parent;
        while((parent = index.parent()).isValid()) {
            index = parent;
        }
        // Go to next thread
        index = index.sibling(index.row() + 1, index.column());
        if(!index.isValid()) {
            return false;
        }
    } else {
        index = model()->index(0, 0, rootIndex());
    }

    index = find(index, HeadersModel::ReadRole, false);
    if(index.isValid()) {
        setCurrentIndex(index);
    }
    return index.isValid();
}


QModelIndex HeadersView::find(const QModelIndex& from, int role, const QVariant& value)
{
    QModelIndex index = from;
    do {
        const QModelIndexList& match = model()->match(index, role, value, 1 /* hit count*/,
                                                      Qt::MatchExactly | Qt::MatchRecursive);
        if(!match.isEmpty()) {
            index = match.at(0);
            break;
        }
        extendedNext(index);
    } while(index.isValid());

    return index;
}


void HeadersView::selectPreviousMessage()
{
    QModelIndex index = currentIndex();
    if(!index.isValid()) {
        // No current selection => search the whole group
        index = model()->index(0, 0, rootIndex());
    } else {
        // In the previous thread (if it exists), try to find the deepest child.
        QModelIndex prev = index.sibling(index.row() - 1, index.column());
        while(prev.isValid()) {
            index = prev;
            prev = prev.child(model()->rowCount(index) -1 , index.column());
        }
        // Otherwise select the parent.
        if(index == currentIndex()) {
            index = index.parent();
        }
    }

    if(index.isValid()) {
        setCurrentIndex(index);
    }
}

void HeadersView::selectNextMessage()
{
    QModelIndex index = currentIndex();
    if(!index.isValid()) {
        // No current selection => search the whole group
        index = model()->index(0, 0, rootIndex());
    } else {
        if(model()->hasChildren(index)) {
            index = index.child(0, index.column());
        } else {
            extendedNext(index);
        }
    }

    if(index.isValid()) {
        setCurrentIndex(index);
    }
}


}
}


/**
 * Find the next sibling or if it does not exists, the next sibling
 * of the nearest ancestor.
 * An invalid index is returned if no such index is found.
 */
static void extendedNext(QModelIndex& index)
{
    QModelIndex next;
    do {
        next = index.sibling(index.row() + 1, index.column());
        if(!next.isValid()) {
            index = index.parent();
        }
    } while(!next.isValid() && index.isValid());
    index = next;
}
