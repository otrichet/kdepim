/*
    Empath - Mailer for KDE
    
    Copyright 1999, 2000
        Rik Hemsley <rik@kde.org>
        Wilco Greven <j.w.greven@student.utwente.nl>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef __GNUG__
# pragma implementation "EmpathHeaderSpecWidget.h"
#endif

// Qt includes
#include <klineedit.h>
#include <qlabel.h>

// Local includes
#include "EmpathHeaderSpecWidget.h"
#include "EmpathHeaderBodyWidget.h"
#include "EmpathAddressSelectionWidget.h"
#include "EmpathTextSelectionWidget.h"
#include "EmpathUIUtils.h"
#include "Empath.h"

#include <RMM_Enum.h>

EmpathHeaderSpecWidget::EmpathHeaderSpecWidget(
        RMM::RHeader header, 
        QWidget * parent)
    :
        QHBox(parent, "HeaderSpecWidget")
{
    RMM::HeaderDataType t = RMM::headerTypesTable[header.headerType()];
    
    headerNameWidget_ = new QLabel(this);
    
    headerBodyWidget_ = (t == RMM::AddressList || t == RMM::Address) 
        ? new EmpathAddressSelectionWidget(this)
        : new EmpathTextSelectionWidget(this);

    headerNameWidget_->setText(header.headerName() + ": ");
    headerBodyWidget_->setText(header.headerBody()->asString());
    
    headerNameWidget_->setFocusProxy(headerBodyWidget_);
    setFocusProxy(headerBodyWidget_);
}

EmpathHeaderSpecWidget::~EmpathHeaderSpecWidget()
{
    // Empty.
}

    RMM::RHeader
EmpathHeaderSpecWidget::header()
{
    QCString headerName = headerNameWidget_->text().local8Bit();
    QCString headerBody = headerBodyWidget_->text().local8Bit();
    RMM::RHeader h(headerName + ": " + headerBody);
    return h;
}

    int
EmpathHeaderSpecWidget::sizeOfColumnOne()
{
    return (headerNameWidget_->sizeHint().width());
}

    void
EmpathHeaderSpecWidget::setColumnOneSize(int i)
{
    headerNameWidget_->setFixedWidth(i);
}

    QString
EmpathHeaderSpecWidget::headerName()
{
    QString s = headerNameWidget_->text();
    s.truncate(s.length() - 1); // Remove trailing ':'
    return s;
}

    QString
EmpathHeaderSpecWidget::headerBody()
{
    return headerBodyWidget_->text();
}

    void
EmpathHeaderSpecWidget::focusInEvent(QFocusEvent *)
{
    headerBodyWidget_->setFocus();
}

    void
EmpathHeaderSpecWidget::keyPressEvent(QKeyEvent * e)
{
    if (e->state() & ControlButton) 
        switch (e->key()) {
        case Key_P:
            emit goUp();
            break;
        case Key_N:
            emit goDown();
            break;
        default:
            e->ignore();
        }
    else
        switch (e->key()) {
        case Key_Up:
            emit goUp();
            break;
        case Key_Down:
            emit goDown();
            break;
        case Key_Return:
            emit goDown();
            break;
        default:
            e->ignore();
        }
}

    void
EmpathHeaderSpecWidget::s_setFocus()
{
    headerBodyWidget_->setFocus();
}

// vim:ts=4:sw=4:tw=78
