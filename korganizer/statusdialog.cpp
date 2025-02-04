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
#include "statusdialog.h"

#include <kcalutils/stringify.h>

#include <kdebug.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kstandardguiitem.h>
#include <KComboBox>

#include <QLabel>
#include <QStringList>
#include <QLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QBoxLayout>

#include "statusdialog.moc"

using namespace KCalUtils;

StatusDialog::StatusDialog( QWidget *parent )
  : KDialog( parent )
{
  setModal( true );
  setCaption( i18n( "Set Your Status" ) );

  QBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setSpacing( spacingHint() );
  topLayout->setMargin( marginHint() );

  QBoxLayout *statusLayout = new QHBoxLayout();
  topLayout->addItem( statusLayout );

  QLabel *text = new QLabel( i18n( "Set your status" ), this );
  statusLayout->addWidget( text );

  mStatus = new KComboBox( this );
  mStatus->setEditable( false );
  mStatus->addItems( Stringify::attendeeStatusList() );
  statusLayout->addWidget( mStatus );

  QBoxLayout *buttonLayout = new QHBoxLayout();
  topLayout->addItem( buttonLayout );

  QPushButton *ok = new KPushButton( KStandardGuiItem::ok(), this );
  connect ( ok, SIGNAL(clicked()), this, SLOT(accept()) );
  buttonLayout->addWidget( ok );

  QPushButton *cancel = new KPushButton( KStandardGuiItem::cancel(), this );
  connect ( cancel, SIGNAL(clicked()), this, SLOT(reject()) );
  buttonLayout->addWidget( cancel );
}

StatusDialog::~StatusDialog()
{
}

Attendee::PartStat StatusDialog::status()
{
  return Attendee::PartStat( mStatus->currentIndex() ) ;
}
