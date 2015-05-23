/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "networkutil.h"
#include <qglobal.h>

using namespace PimCommon;

Q_GLOBAL_STATIC(NetworkUtil, s_networkUtil)

NetworkUtil *NetworkUtil::self()
{
    return s_networkUtil;
}

NetworkUtil::NetworkUtil()
    : mLowBandwidh(false)
{

}

bool NetworkUtil::lowBandwidh() const
{
    return mLowBandwidh;
}

void NetworkUtil::setLowBandwidh(bool lowBandwidh)
{
    mLowBandwidh = lowBandwidh;
}

