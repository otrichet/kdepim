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

#include "quicksearchwarningtest.h"
#include "../core/widgets/quicksearchwarning.h"
#include <qtest_kde.h>
QuickSearchWarningTest::QuickSearchWarningTest(QObject *parent)
    : QObject(parent)
{

}

QuickSearchWarningTest::~QuickSearchWarningTest()
{

}

void QuickSearchWarningTest::shouldHaveDefaultValue()
{
    MessageList::Core::QuickSearchWarning w;
    QVERIFY(!w.isVisible());
}

void QuickSearchWarningTest::shouldSetVisible()
{
    MessageList::Core::QuickSearchWarning w;
    w.setSearchText(QLatin1String("1"));
    QVERIFY(w.isVisible());
}

void QuickSearchWarningTest::shouldSetSearchText()
{
    QFETCH( QString, input );
    QFETCH( bool, visible );
    MessageList::Core::QuickSearchWarning w;
    w.setSearchText(input);
    QCOMPARE(w.isVisible(), visible);
}

void QuickSearchWarningTest::shouldSetSearchText_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("visible");
    QTest::newRow("bigword") <<  QString(QLatin1String("foofoofoo")) << false;
    QTest::newRow("1character") <<  QString(QLatin1String("f")) << true;
    QTest::newRow("multibigword") <<  QString(QLatin1String("foo foo foo")) << false;
    QTest::newRow("multibigwordwithasmallone") <<  QString(QLatin1String("foo foo foo 1")) << true;
    QTest::newRow("aspace") <<  QString(QLatin1String(" ")) << false;
}

QTEST_KDEMAIN(QuickSearchWarningTest, GUI)
