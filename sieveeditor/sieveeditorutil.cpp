/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/


#include "sieveeditorutil.h"
#include <KConfig>
#include <KGlobal>
#include <KLocale>
#include <KConfigGroup>

#include <QRegExp>


QList<SieveEditorUtil::SieveServerConfig> SieveEditorUtil::readServerSieveConfig()
{
    QList<SieveServerConfig> lstConfig;
    KSharedConfigPtr cfg = KGlobal::config();
    QRegExp re( QLatin1String( "^ServerSieve (.+)$" ) );
    const QStringList groups = cfg->groupList().filter( re );

    Q_FOREACH (const QString &conf, groups) {
        SieveServerConfig sieve;
        KConfigGroup group = cfg->group(conf);
        sieve.port = group.readEntry(QLatin1String("Port"), -1);
        sieve.serverName = group.readEntry(QLatin1String("ServerName"));
        sieve.userName = group.readEntry(QLatin1String("UserName"));
        sieve.password = group.readEntry(QLatin1String("Password"));
        lstConfig.append(sieve);
    }
    return lstConfig;
}

void SieveEditorUtil::writeServerSieveConfig(const QList<SieveEditorUtil::SieveServerConfig> &lstConfig)
{
    KSharedConfigPtr cfg = KGlobal::config();
    QRegExp re( QLatin1String( "^ServerSieve (.+)$" ) );
    //Delete Old Group
    const QStringList groups = cfg->groupList().filter( re );
    Q_FOREACH (const QString &conf, groups) {
        KConfigGroup group = cfg->group(conf);
        group.deleteGroup();
    }

    int i = 0;
    Q_FOREACH (const SieveEditorUtil::SieveServerConfig &conf, lstConfig) {
        KConfigGroup group = cfg->group(QString::fromLatin1("ServerSieve %1").arg(i));
        group.writeEntry(QLatin1String("Port"), conf.port);
        group.writeEntry(QLatin1String("ServerName"), conf.serverName);
        group.writeEntry(QLatin1String("UserName"), conf.userName);
        group.writeEntry(QLatin1String("Password"), conf.password);

        ++i;
    }
    cfg->sync();
}
