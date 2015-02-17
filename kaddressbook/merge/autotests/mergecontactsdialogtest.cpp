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

#include "mergecontactsdialogtest.h"
#include "merge/manualmerge/mergecontactsdialog.h"

#include <QStackedWidget>
#include <qtest.h>

MergeContactsDialogTest::MergeContactsDialogTest(QObject *parent)
    : QObject(parent)
{

}

MergeContactsDialogTest::~MergeContactsDialogTest()
{

}

void MergeContactsDialogTest::shouldHaveDefaultValue()
{
    KABMergeContacts::MergeContactsDialog dlg;
    dlg.show();
    QStackedWidget *stackedWidget = qFindChild<QStackedWidget *>(&dlg, QLatin1String("stackedwidget"));
    QVERIFY(stackedWidget);
    QCOMPARE(stackedWidget->currentWidget()->objectName(), QLatin1String("nocontactselected"));

    for (int i = 0; i < stackedWidget->count(); ++i) {
        QWidget *w = stackedWidget->widget(i);
        const QString objName = w->objectName();
        const bool hasGoodNamePage = (objName == QLatin1String("notenoughcontactselected") ||
                                      objName == QLatin1String("nocontactselected") ||
                                      objName == QLatin1String("manualmergeresultwidget") ||
                                      objName == QLatin1String("selectioninformation") ||
                                      objName == QLatin1String("mergecontactinfowidget"));
        QVERIFY(hasGoodNamePage);
    }

}

void MergeContactsDialogTest::shouldSwithStackedWidget()
{
    KABMergeContacts::MergeContactsDialog dlg;
    dlg.show();
    QStackedWidget *stackedWidget = qFindChild<QStackedWidget *>(&dlg, QLatin1String("stackedwidget"));
    Akonadi::Item::List lst;
    //Empty
    dlg.setContacts(lst);
    QCOMPARE(stackedWidget->currentWidget()->objectName(), QLatin1String("nocontactselected"));
    lst << Akonadi::Item(42);
    //1 element
    dlg.setContacts(lst);
    QCOMPARE(stackedWidget->currentWidget()->objectName(), QLatin1String("notenoughcontactselected"));
    lst.clear();
    //1 element
    lst << Akonadi::Item(42);
    dlg.setContacts(lst);
    QCOMPARE(stackedWidget->currentWidget()->objectName(), QLatin1String("notenoughcontactselected"));
    // 2 elements
    lst.clear();
    lst << Akonadi::Item(42) << Akonadi::Item(42);
    dlg.setContacts(lst);
    QCOMPARE(stackedWidget->currentWidget()->objectName(), QLatin1String("manualmergeresultwidget"));
}

QTEST_MAIN(MergeContactsDialogTest)
