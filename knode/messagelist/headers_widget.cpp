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

#include "headers_widget.h"

#include <QtGui/QVBoxLayout>
#include <KDE/KFilterProxySearchLine>

#include "headers_model.h"
#include "headers_view.h"

#include "knarticlemanager.h"
#include "knglobals.h"

namespace KNode {
namespace MessageList {

HeadersWidget::HeadersWidget(QWidget* parent)
  : QWidget(parent)
{
    mSearch = new KFilterProxySearchLine(this);
    mView = new HeadersView(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(mSearch);
    layout->addWidget(mView);

    mModel = new HeadersModel();
    mView->setModel(mModel);

    connect(KNGlobals::self()->articleManager(), SIGNAL(groupChanged(const KNGroup::Ptr)),
            this, SLOT(showGroup(const KNGroup::Ptr)));
    connect(KNGlobals::self()->articleManager(), SIGNAL(filterChanged(KNArticleFilter*)),
            this, SLOT(setFilter(KNArticleFilter*)));
    connect(mView, SIGNAL(articlesSelected(const KNArticle::List)),
            this, SIGNAL(articlesSelected(const KNArticle::List)));
}

HeadersWidget::~HeadersWidget()
{
}


void HeadersWidget::showGroup(const KNGroup::Ptr group)
{
    mModel->setGroup(group);
}

void HeadersWidget::setFilter(KNArticleFilter* filter)
{
    mModel->setFilter(filter);
}

bool HeadersWidget::selectNextUnreadMessage()
{
    return mView->selectNextUnread();
}

bool HeadersWidget::selectNextUnreadThread()
{
    return mView->selectNextUnreadThread();
}


}
}
