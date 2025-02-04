/*
    This file is part of KMail.

    Copyright (c) 2005 David Faure <faure@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2,
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/


#include "messagecomposersettings.h"
#include <QTimer>

using namespace MessageComposer;

MessageComposerSettings *MessageComposerSettings::mSelf = 0;

MessageComposerSettings *MessageComposerSettings::self()
{
  if ( !mSelf ) {
    mSelf = new MessageComposerSettings();
    mSelf->readConfig();
  }

  return mSelf;
}

MessageComposerSettings::MessageComposerSettings()
{
  mConfigSyncTimer = new QTimer( this );
  mConfigSyncTimer->setSingleShot( true );
  connect( mConfigSyncTimer, SIGNAL( timeout() ), this, SLOT( slotSyncNow() ) );
}

void MessageComposerSettings::requestSync()
{
 if ( !mConfigSyncTimer->isActive() )
   mConfigSyncTimer->start( 0 );
}

void MessageComposerSettings::slotSyncNow()
{
  config()->sync();
}

MessageComposerSettings::~MessageComposerSettings()
{
}

#include "messagecomposersettings.moc"
