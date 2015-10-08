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

#ifndef KNODE_HEADERS_WIDGET_H
#define KNODE_HEADERS_WIDGET_H

#include <QtGui/QWidget>

#include "knarticle.h"
#include "kngroup.h"

class KFilterProxySearchLine;
class KNArticleFilter;

namespace KNode {
namespace MessageList {

class HeadersModel;
class HeadersView;

class HeadersWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit HeadersWidget(QWidget* parent = 0);
        ~HeadersWidget();

        /**
         * Select the next unread article.
         * @return @code false if no unread article is found.
         */
        bool selectNextUnreadMessage();
        /**
         * Select the first unread article starting from the next thread.
         * @return @code false if no unread article is found.
         */
        bool selectNextUnreadThread();

        /**
         * Returns the list of currently selected articles.
         */
        KNRemoteArticle::List getSelectedMessages();
        /**
         * Returns the list of threads containing selected articles.
         */
        KNRemoteArticle::List getSelectedThreads();

    public Q_SLOTS:
        void showGroup(const KNGroup::Ptr group);
        void setFilter(KNArticleFilter* filter);

    Q_SIGNALS:
        void articlesSelected(const KNArticle::List article);

    private:
      KFilterProxySearchLine* mSearch;
      HeadersView* mView;
      HeadersModel* mModel;
};

}
}

#endif
