/*
  This file is part of KOrganizer.
  Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/
#ifndef PUBLISHDIALOG_H
#define PUBLISHDIALOG_H

#include <kcalcore/attendee.h>

#include <kdialog.h>
#include "ui_publishdialog_base.h"

using namespace KCalCore;

class PublishDialog_base;

class PublishDialog : public KDialog
{
  Q_OBJECT
  public:
    explicit PublishDialog( QWidget *parent=0 );
    ~PublishDialog();

    void addAttendee( const Attendee::Ptr &attendee );
    QString addresses();

  signals:
    void numMessagesChanged( int );

  protected slots:
    void addItem();
    void removeItem();
    void openAddressbook();
    void updateItem();
    void updateInput();

  protected:
    Ui::PublishDialog_base mUI;
};

#endif
