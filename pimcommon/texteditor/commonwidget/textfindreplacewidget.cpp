/*
  Copyright (c) 2013-2015 Montel Laurent <montel@kde.org>

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

#include "textfindreplacewidget.h"

#include <KLocalizedString>
#include <qpushbutton.h>
#include <klineedit.h>
#include <KColorScheme>

#include <QLabel>
#include <QMenu>
#include <QHBoxLayout>

using namespace PimCommon;

TextReplaceWidget::TextReplaceWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *lay = new QHBoxLayout;
    QLabel *label = new QLabel(i18nc("Replace text", "Replace:"), this);
    lay->addWidget(label);
    const int marg = lay->margin();
    lay->setContentsMargins(marg, 0, marg, 0);

    mReplace = new QLineEdit;
    mReplace->setClearButtonEnabled(true);
    lay->addWidget(mReplace);

    mReplaceBtn = new QPushButton(i18n("Replace"), this);
    connect(mReplaceBtn, &QPushButton::clicked, this, &TextReplaceWidget::replaceText);
    lay->addWidget(mReplaceBtn);

    mReplaceAllBtn = new QPushButton(i18n("Replace All"), this);
    connect(mReplaceAllBtn, &QPushButton::clicked, this, &TextReplaceWidget::replaceAllText);
    lay->addWidget(mReplaceAllBtn);

    setLayout(lay);
}

TextReplaceWidget::~TextReplaceWidget()
{

}

QLineEdit *TextReplaceWidget::replace() const
{
    return mReplace;
}

void TextReplaceWidget::slotSearchStringEmpty(bool isEmpty)
{
    mReplaceBtn->setDisabled(isEmpty);
    mReplaceAllBtn->setDisabled(isEmpty);
}

TextFindWidget::TextFindWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *lay = new QHBoxLayout;
    const int marg = lay->margin();
    lay->setContentsMargins(marg, 0, marg, 0);
    QLabel *label = new QLabel(i18nc("Find text", "F&ind:"), this);
    lay->addWidget(label);

    mSearch = new QLineEdit(this);
    mSearch->setToolTip(i18n("Text to search for"));
    mSearch->setClearButtonEnabled(true);
    label->setBuddy(mSearch);
    lay->addWidget(mSearch);

    mFindNextBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("go-down-search")), i18nc("Find and go to the next search match", "Next"), this);
    mFindNextBtn->setToolTip(i18n("Jump to next match"));
    lay->addWidget(mFindNextBtn);
    mFindNextBtn->setEnabled(false);

    mFindPrevBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("go-up-search")), i18nc("Find and go to the previous search match", "Previous"), this);
    mFindPrevBtn->setToolTip(i18n("Jump to previous match"));
    lay->addWidget(mFindPrevBtn);
    mFindPrevBtn->setEnabled(false);

    QPushButton *optionsBtn = new QPushButton(this);
    optionsBtn->setText(i18n("Options"));
    optionsBtn->setToolTip(i18n("Modify search behavior"));
    QMenu *optionsMenu = new QMenu(optionsBtn);
    mCaseSensitiveAct = optionsMenu->addAction(i18n("Case sensitive"));
    mCaseSensitiveAct->setCheckable(true);

    mWholeWordAct = optionsMenu->addAction(i18n("Whole word"));
    mWholeWordAct->setCheckable(true);

    optionsBtn->setMenu(optionsMenu);
    lay->addWidget(optionsBtn);

    connect(mFindNextBtn, &QPushButton::clicked, this, &TextFindWidget::findNext);
    connect(mFindPrevBtn, &QPushButton::clicked, this, &TextFindWidget::findPrev);
    connect(mCaseSensitiveAct, &QAction::toggled, this, &TextFindWidget::updateSearchOptions);
    connect(mWholeWordAct, &QAction::toggled, this, &TextFindWidget::updateSearchOptions);
    connect(mSearch, &QLineEdit::textChanged, this, &TextFindWidget::slotAutoSearch);
    setLayout(lay);
}

TextFindWidget::~TextFindWidget()
{

}

QRegExp TextFindWidget::findRegExp() const
{
    QString str = mSearch->text();
    if (mWholeWordAct->isChecked()) {
        str = QLatin1String("\\b") + str + QLatin1String("\\b");
    }
    if (mCaseSensitiveAct->isChecked()) {
        return QRegExp(str, Qt::CaseSensitive);
    } else {
        return QRegExp(str, Qt::CaseInsensitive);
    }
}

void TextFindWidget::setFoundMatch(bool match)
{
#ifndef QT_NO_STYLE_STYLESHEET
    QString styleSheet;

    if (! mSearch->text().isEmpty()) {
        KColorScheme::BackgroundRole bgColorScheme;

        if (match) {
            bgColorScheme = KColorScheme::PositiveBackground;
        } else {
            bgColorScheme = KColorScheme::NegativeBackground;
        }

        KStatefulBrush bgBrush(KColorScheme::View, bgColorScheme);

        styleSheet = QStringLiteral("QLineEdit{ background-color:%1 }")
                     .arg(bgBrush.brush(mSearch).color().name());
    }

    mSearch->setStyleSheet(styleSheet);
#endif
}

void TextFindWidget::slotAutoSearch(const QString &str)
{
    const bool isNotEmpty = (!str.isEmpty());
    mFindPrevBtn->setEnabled(isNotEmpty);
    mFindNextBtn->setEnabled(isNotEmpty);
    Q_EMIT searchStringEmpty(!isNotEmpty);
    Q_EMIT autoSearch(str);
    if (str.isEmpty()) {
        Q_EMIT clearSearch();
    }
}

QLineEdit *TextFindWidget::search() const
{
    return mSearch;
}

QTextDocument::FindFlags TextFindWidget::searchOptions() const
{
    QTextDocument::FindFlags opt = Q_NULLPTR;
    if (mCaseSensitiveAct->isChecked()) {
        opt |= QTextDocument::FindCaseSensitively;
    }
    if (mWholeWordAct->isChecked()) {
        opt |= QTextDocument::FindWholeWords;
    }
    return opt;
}

