/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Tobias Koenig <tokoe@kdab.com>

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

#ifndef MAILCOMMON_SNIPPETVARIABLEDIALOG_P_H
#define MAILCOMMON_SNIPPETVARIABLEDIALOG_P_H

#include <kdialog.h>

class KTextEdit;
class QCheckBox;

namespace MailCommon {

class SnippetVariableDialog : public KDialog
{
  Q_OBJECT

  public:
    SnippetVariableDialog( const QString &variableName, QMap<QString, QString> *variables, QWidget *parent = 0 );

    QString variableValue() const;

  protected:
    virtual void slotButtonClicked( int button );

  private:
    QString mVariableName;
    QMap<QString, QString> *mVariables;
    KTextEdit *mVariableValueText;
    QCheckBox *mSaveVariable;
};

}

#endif
