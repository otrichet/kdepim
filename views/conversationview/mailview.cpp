/*
 * mailview.cpp
 *
 * copyright (c) Aron Bostrom <Aron.Bostrom at gmail.com>, 2006 
 *
 * this library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <QTextFrame>
#include <QTextFrameFormat>
#include <QTextLength>
#include <QtDebug>
#include <QTextDocument>

#include "mailview.h"

//MailView::MailView(QWidget *parent)
//{
//	QTextEdit(parent);
//	setReadOnly(true);
//}

int MailView::getNeededHeight() const
{
	qDebug() << "Dbg: " << document()->rootFrame()->frameFormat().height().rawValue();
	return document()->rootFrame()->frameFormat().height().rawValue();
}

void MailView::updateHeight()
{
	setMinimumHeight(getNeededHeight());
}

void MailView::setConversation(const QModelIndex &index)
{
	setHtml("");
	int conversationId = index.row();
	int max = backend->messageCount(conversationId)-1;
	for (int count = 0; count < max; ++count) {
                QString tmp = "<B>";
                tmp.append(backend->messageAuthor(conversationId, count));
                tmp.append("</B><BR>");
                tmp.append(backend->messageContent(conversationId, count));
                tmp.append("</HR>");
                append(tmp);
	}
        QString tmp = "<B>";
        tmp.append(backend->messageAuthor(conversationId, max));
        tmp.append("</B><BR>");
        tmp.append(backend->messageContent(conversationId, max));
        append(tmp);
}
