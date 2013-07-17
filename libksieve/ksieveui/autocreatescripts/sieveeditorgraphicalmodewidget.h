/* Copyright (C) 2013 Laurent Montel <montel@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#ifndef SIEVEEDITORGRAPHICALMODEWIDGET_H
#define SIEVEEDITORGRAPHICALMODEWIDGET_H

#include <QWidget>

class QSplitter;
class QStackedWidget;

namespace KSieveUi {
class SieveScriptListBox;
class SieveEditorGraphicalModeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SieveEditorGraphicalModeWidget(QWidget *parent=0);
    ~SieveEditorGraphicalModeWidget();

    QString script(QString &requires) const;

    static void setSieveCapabilities( const QStringList &capabilities );
    static QStringList sieveCapabilities();

private:
    void readConfig();
    void writeConfig();

private Q_SLOTS:
    void slotAddScriptPage(QWidget *page);
    void slotRemoveScriptPage(QWidget *page);
    void slotActivateScriptPage(QWidget *page);

private:
    static QStringList sCapabilities;
    SieveScriptListBox *mSieveScript;
    QStackedWidget *mStackWidget;
    QSplitter *mSplitter;

};
}

#endif // SIEVEEDITORGRAPHICALMODEWIDGET_H
