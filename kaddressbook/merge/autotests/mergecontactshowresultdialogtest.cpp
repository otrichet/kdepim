/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

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

#include "mergecontactshowresultdialogtest.h"
#include "mergecontactshowresultdialog.h"
#include <qtest.h>
#include <AkonadiCore/Item>
#include <KContacts/Addressee>
#include "../widgets/mergecontactshowresulttabwidget.h"

using namespace KABMergeContacts;

MergeContactShowResultDialogTest::MergeContactShowResultDialogTest()
{
}

void MergeContactShowResultDialogTest::shouldHaveDefaultValueOnCreation()
{
    MergeContactShowResultDialog dlg;
    dlg.show();
    KABMergeContacts::MergeContactShowResultTabWidget *tabWidget = dlg.findChild<KABMergeContacts::MergeContactShowResultTabWidget *>(QStringLiteral("tabwidget"));
    QVERIFY(tabWidget);
    QCOMPARE(tabWidget->count(), 0);
    QCOMPARE(tabWidget->tabBarVisible(), false);
}

void MergeContactShowResultDialogTest::shouldDontShowTabBarWhenWeHaveJustOneContact()
{
    MergeContactShowResultDialog dlg;
    Akonadi::Item::List lst;
    Akonadi::Item item;
    KContacts::Addressee address;
    address.setName(QStringLiteral("foo1"));
    item.setPayload<KContacts::Addressee>(address);

    lst.append(item);
    dlg.setContacts(lst);
    dlg.show();
    KABMergeContacts::MergeContactShowResultTabWidget *tabWidget = dlg.findChild<KABMergeContacts::MergeContactShowResultTabWidget *>(QStringLiteral("tabwidget"));
    QCOMPARE(tabWidget->tabBarVisible(), false);
    QCOMPARE(tabWidget->count(), 1);
}

void MergeContactShowResultDialogTest::shouldShowTabBarWhenWeHaveMoreThanOneContact()
{
    MergeContactShowResultDialog dlg;
    Akonadi::Item item;
    KContacts::Addressee address;
    address.setName(QStringLiteral("foo1"));
    item.setPayload<KContacts::Addressee>(address);
    Akonadi::Item::List lst;
    lst << item << item;
    dlg.setContacts(lst);
    dlg.show();
    KABMergeContacts::MergeContactShowResultTabWidget *tabWidget = dlg.findChild<KABMergeContacts::MergeContactShowResultTabWidget *>(QStringLiteral("tabwidget"));
    QCOMPARE(tabWidget->tabBarVisible(), true);
    QCOMPARE(tabWidget->count(), 2);
}

QTEST_MAIN(MergeContactShowResultDialogTest)
