/* Copyright (C) 2011, 2012, 2013 Laurent Montel <montel@kde.org>
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

#ifndef KSIEVE_KSIEVEUI_SIEVETEXTEDIT_H
#define KSIEVE_KSIEVEUI_SIEVETEXTEDIT_H

#include "ksieveui_export.h"

#include <QPlainTextEdit>

class QCompleter;

namespace KSieveUi {

class SieveLineNumberArea;

class KSIEVEUI_EXPORT SieveTextEdit : public QPlainTextEdit
{
  Q_OBJECT

  public:
    explicit SieveTextEdit( QWidget *parent );
    ~SieveTextEdit();

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

  private Q_SLOTS:
    void slotInsertCompletion( const QString& );
    void slotUpdateLineNumberAreaWidth(int newBlockCount);
    void slotUpdateLineNumberArea(const QRect &, int);
    void slotUndoableClear();
    void slotSpeakText();

  protected:
    QString wordUnderCursor() const;
    void initCompleter();
    void keyPressEvent(QKeyEvent* e);
    void resizeEvent(QResizeEvent *event);
    void contextMenuEvent( QContextMenuEvent *event );

  signals:
    void findText();

  private:
    QCompleter *m_completer;
    SieveLineNumberArea *m_sieveLineNumberArea;
};

}
#endif

