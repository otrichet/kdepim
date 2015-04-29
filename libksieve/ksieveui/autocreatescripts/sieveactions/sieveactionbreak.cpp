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

#include "sieveactionbreak.h"
#include "autocreatescripts/autocreatescriptutil_p.h"
#include "editor/sieveeditorutil.h"

#include <KLocalizedString>
#include <QLineEdit>

#include <QHBoxLayout>
#include <QLabel>
#include <QDomNode>
#include "libksieve_debug.h"

using namespace KSieveUi;
SieveActionBreak::SieveActionBreak(QObject *parent)
    : SieveAction(QStringLiteral("break"), i18n("Break"), parent)
{
}

SieveAction *SieveActionBreak::newAction()
{
    return new SieveActionBreak;
}

QWidget *SieveActionBreak::createParamWidget(QWidget *parent) const
{
    QWidget *w = new QWidget(parent);
    QHBoxLayout *lay = new QHBoxLayout;
    lay->setMargin(0);
    w->setLayout(lay);

    QLabel *lab = new QLabel(i18n("Name (optional):"));
    lay->addWidget(lab);

    QLineEdit *subject = new QLineEdit;
    subject->setObjectName(QStringLiteral("name"));
    connect(subject, &QLineEdit::textChanged, this, &SieveActionBreak::valueChanged);
    lay->addWidget(subject);
    return w;
}

bool SieveActionBreak::setParamWidgetValue(const QDomElement &element, QWidget *w , QString &error)
{
    QDomNode node = element.firstChild();
    while (!node.isNull()) {
        QDomElement e = node.toElement();
        if (!e.isNull()) {
            const QString tagName = e.tagName();
            if (tagName == QStringLiteral("tag")) {
                const QString tagValue = e.text();
                if (tagValue == QStringLiteral("name")) {
                    QLineEdit *name = w->findChild<QLineEdit *>(QStringLiteral("name"));
                    name->setText(AutoCreateScriptUtil::strValue(e));
                } else {
                    unknowTagValue(tagValue, error);
                    qCDebug(LIBKSIEVE_LOG) << " SieveActionBreak::setParamWidgetValue unknown tagValue " << tagValue;
                }
            } else if (tagName == QStringLiteral("str")) {
                //Nothing
            } else if (tagName == QStringLiteral("crlf")) {
                //nothing
            } else if (tagName == QStringLiteral("comment")) {
                //implement in the future ?
            } else {
                unknownTag(tagName, error);
                qCDebug(LIBKSIEVE_LOG) << "SieveActionBreak::setParamWidgetValue unknown tag " << tagName;
            }
        }
        node = node.nextSibling();
    }
    return true;
}

QString SieveActionBreak::href() const
{
    return SieveEditorUtil::helpUrl(SieveEditorUtil::strToVariableName(name()));
}

QString SieveActionBreak::code(QWidget *w) const
{
    const QLineEdit *name = w->findChild<QLineEdit *>(QStringLiteral("name"));
    const QString nameStr = name->text();
    if (!nameStr.isEmpty()) {
        return QStringLiteral("break :name \"%1\";").arg(nameStr);
    }
    return QStringLiteral("break;");
}

QString SieveActionBreak::help() const
{
    return i18n("The break command terminates the closest enclosing loop.");
}

QStringList SieveActionBreak::needRequires(QWidget */*parent*/) const
{
    return QStringList() << QStringLiteral("foreverypart");
}

bool SieveActionBreak::needCheckIfServerHasCapability() const
{
    return true;
}

QString SieveActionBreak::serverNeedsCapability() const
{
    return QStringLiteral("foreverypart");
}

