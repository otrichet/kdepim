/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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

#include "autocorrectiontest.h"
#include "../autocorrection.h"
#include "pimcommon/settings/pimcommonsettings.h"
#include <qtest.h>
#include <QDebug>
#include <QTextDocument>

AutoCorrectionTest::AutoCorrectionTest()
{
    mConfig = KSharedConfig::openConfig( QLatin1String("autocorrectiontestrc") );
    PimCommon::PimCommonSettings::self()->setSharedConfig( mConfig );
    PimCommon::PimCommonSettings::self()->load();
}

AutoCorrectionTest::~AutoCorrectionTest()
{
}

void AutoCorrectionTest::shouldHaveDefaultValue()
{
    PimCommon::AutoCorrection autocorrection;
    QVERIFY(!autocorrection.isEnabledAutoCorrection());
    QVERIFY(!autocorrection.isUppercaseFirstCharOfSentence());
    QVERIFY(!autocorrection.isFixTwoUppercaseChars());
    QVERIFY(!autocorrection.isSingleSpaces());
    QVERIFY(!autocorrection.isAutoFractions());
    QVERIFY(!autocorrection.isCapitalizeWeekDays());
    QVERIFY(!autocorrection.isReplaceDoubleQuotes());
    QVERIFY(!autocorrection.isReplaceSingleQuotes());
    QVERIFY(!autocorrection.isAdvancedAutocorrect());
    QVERIFY(!autocorrection.isAutoFormatUrl());
    QVERIFY(!autocorrection.isAutoBoldUnderline());
    QVERIFY(!autocorrection.isSuperScript());
    QVERIFY(!autocorrection.isAddNonBreakingSpace());
}

void AutoCorrectionTest::shouldRestoreValue()
{
    PimCommon::AutoCorrection autocorrection;
    //TODO
}

void AutoCorrectionTest::shouldUpperCaseFirstCharOfSentence()
{
    PimCommon::AutoCorrection autocorrection;
    autocorrection.setEnabledAutoCorrection(true);
    autocorrection.setUppercaseFirstCharOfSentence(true);

    //Uppercase here.
    QTextDocument doc;
    QString text = QLatin1String("foo");
    doc.setPlainText(text);
    int position = text.length();
    autocorrection.autocorrect(false, doc, position);
    QCOMPARE(doc.toPlainText(), QLatin1String("Foo"));

    //IT's not first char -> not uppercase
    text = QLatin1String(" foo");
    doc.setPlainText(text);
    position = text.length();
    autocorrection.autocorrect(false, doc, position);
    QCOMPARE(doc.toPlainText(), text);

    //It's already uppercase
    text = QLatin1String("Foo");
    doc.setPlainText(text);
    position = text.length();
    autocorrection.autocorrect(false, doc, position);
    QCOMPARE(doc.toPlainText(), text);

    //Word is after a ". "
    text = QLatin1String("Foo. foo");
    doc.setPlainText(text);
    position = text.length();
    autocorrection.autocorrect(false, doc, position);
    QCOMPARE(doc.toPlainText(), QLatin1String("Foo. Foo"));


}

void AutoCorrectionTest::shouldFixTwoUpperCaseChars()
{
    PimCommon::AutoCorrection autocorrection;
    autocorrection.setEnabledAutoCorrection(true);
    autocorrection.setFixTwoUppercaseChars(true);

    //Remove two uppercases
    QTextDocument doc;
    QString text = QLatin1String("FOo");
    doc.setPlainText(text);
    int position = text.length();
    autocorrection.autocorrect(false, doc, position);
    QCOMPARE(doc.toPlainText(), QLatin1String("Foo"));

    //There is not two uppercase
    text = QLatin1String("foo");
    doc.setPlainText(text);
    position = text.length();
    autocorrection.autocorrect(false, doc, position);
    QCOMPARE(doc.toPlainText(), text);

    text = QLatin1String("Foo");
    doc.setPlainText(text);
    position = text.length();
    autocorrection.autocorrect(false, doc, position);
    QCOMPARE(doc.toPlainText(), text);

    //There is a uppercase word
    text = QLatin1String("FOO");
    doc.setPlainText(text);
    position = text.length();
    autocorrection.autocorrect(false, doc, position);
    QCOMPARE(doc.toPlainText(), text);



    //Exclude 2 upper letter
    text = QLatin1String("ABc");
    doc.setPlainText(text);
    position = text.length();
    autocorrection.autocorrect(false, doc, position);
    QCOMPARE(doc.toPlainText(), QLatin1String("Abc"));


    QSet<QString> exception;
    exception.insert(QLatin1String("ABc"));
    autocorrection.setTwoUpperLetterExceptions(exception);
    text = QLatin1String("ABc");
    doc.setPlainText(text);
    position = text.length();
    autocorrection.autocorrect(false, doc, position);
    QCOMPARE(doc.toPlainText(), text);
}

void AutoCorrectionTest::shouldRemoveDoubleSpace()
{
    PimCommon::AutoCorrection autocorrection;
    autocorrection.setEnabledAutoCorrection(true);
    autocorrection.setSingleSpaces(true);

    //Remove double space here.
    QTextDocument doc;
    QString text = QLatin1String("  ");
    doc.setPlainText(text);
    int position = text.length();
    autocorrection.autocorrect(false, doc, position);
    //TODO FIXME
    QEXPECT_FAIL( "", "Double space support is broken", Continue );

    QCOMPARE(doc.toPlainText(), QLatin1String(" "));
}


QTEST_MAIN(AutoCorrectionTest)
