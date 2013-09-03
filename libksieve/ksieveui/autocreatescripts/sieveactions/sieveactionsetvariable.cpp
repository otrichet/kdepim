/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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

#include "sieveactionsetvariable.h"
#include "widgets/selectvariablemodifiercombobox.h"
#include "autocreatescripts/autocreatescriptutil_p.h"

#include <KLocale>
#include <KLineEdit>

#include <QHBoxLayout>
#include <QLabel>
#include <QDomNode>
#include <QDebug>

using namespace KSieveUi;
SieveActionSetVariable::SieveActionSetVariable(QObject *parent)
    : SieveAction(QLatin1String("set"), i18n("Variable"), parent)
{
}

SieveAction* SieveActionSetVariable::newAction()
{
    return new SieveActionSetVariable;
}

QWidget *SieveActionSetVariable::createParamWidget( QWidget *parent ) const
{
    QWidget *w = new QWidget(parent);
    QGridLayout *grid = new QGridLayout;
    grid->setMargin(0);
    w->setLayout(grid);

    SelectVariableModifierComboBox *modifier = new SelectVariableModifierComboBox;
    modifier->setObjectName(QLatin1String("modifier"));
    grid->addWidget(modifier, 0, 0);

    QLabel *lab = new QLabel(i18n("Value:"));
    grid->addWidget(lab, 0, 1);

    KLineEdit *value = new KLineEdit;
    value->setObjectName(QLatin1String("value"));
    grid->addWidget(value, 0, 2);

    lab = new QLabel(i18n("In variable:"));
    grid->addWidget(lab, 1, 1);

    KLineEdit *variable = new KLineEdit;
    variable->setObjectName(QLatin1String("variable"));
    grid->addWidget(variable, 1, 2);

    return w;
}

bool SieveActionSetVariable::setParamWidgetValue(const QDomElement &element, QWidget *w , QString &error)
{
    QDomNode node = element.firstChild();
    while (!node.isNull()) {
        QDomElement e = node.toElement();
        if (!e.isNull()) {
            const QString tagName = e.tagName();
            if (tagName == QLatin1String("str")) {
                const QString tagValue = e.text();
                KLineEdit *value = w->findChild<KLineEdit*>(QLatin1String("value"));
                value->setText(tagValue);
                node = node.nextSibling();
                QDomElement variableElement = node.toElement();
                if (!variableElement.isNull()) {
                    const QString variableTagName = variableElement.tagName();
                    if (variableTagName == QLatin1String("str")) {
                        KLineEdit *variable = w->findChild<KLineEdit*>(QLatin1String("variable"));
                        variable->setText(variableElement.text());
                    }
                } else {
                    return false;
                }
            } else if (tagName == QLatin1String("tag")) {
                SelectVariableModifierComboBox *modifier = w->findChild<SelectVariableModifierComboBox*>(QLatin1String("modifier"));
                modifier->setCode(AutoCreateScriptUtil::tagValue(e.text()), name(), error);
            } else {
                unknownTag(tagName, error);
                qDebug()<<" SieveActionSetVariable::setParamWidgetValue unknown tagName "<<tagName;
            }
        }
        node = node.nextSibling();
    }
    return true;
}

QString SieveActionSetVariable::code(QWidget *w) const
{
    QString result = QLatin1String("set ");
    const SelectVariableModifierComboBox *modifier = w->findChild<SelectVariableModifierComboBox*>(QLatin1String("modifier"));
    const QString modifierStr = modifier->code();
    if (!modifierStr.isEmpty()) {
        result += modifierStr + QLatin1Char(' ');
    }
    const KLineEdit *value = w->findChild<KLineEdit*>(QLatin1String("value"));
    const QString valueStr = value->text();
    result += QString::fromLatin1("\"%1\" ").arg(valueStr);

    const KLineEdit *variable = w->findChild<KLineEdit*>(QLatin1String("variable"));
    const QString variableStr = variable->text();
    result += QString::fromLatin1("\"%1\";").arg(variableStr);


    return result;
}

QStringList SieveActionSetVariable::needRequires(QWidget *) const
{
    return QStringList() << QLatin1String("variables");
}

bool SieveActionSetVariable::needCheckIfServerHasCapability() const
{
    return true;
}

QString SieveActionSetVariable::serverNeedsCapability() const
{
    return QLatin1String("variables");
}

QString SieveActionSetVariable::help() const
{
    return i18n("The \"set\" action stores the specified value in the variable identified by name.");
}

#include "sieveactionsetvariable.moc"
