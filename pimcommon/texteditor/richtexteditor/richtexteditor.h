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

#ifndef RICHTEXTEDITOR_H
#define RICHTEXTEDITOR_H

#include "pimcommon_export.h"
#include <QTextEdit>

class QContextMenuEvent;
class QMenu;
class QWheelEvent;
namespace Sonnet {
class Highlighter;
}
namespace PimCommon {
class PIMCOMMON_EXPORT RichTextEditor : public QTextEdit
{
    Q_OBJECT
    Q_PROPERTY(bool searchSupport READ searchSupport WRITE setSearchSupport)
public:
    explicit RichTextEditor(QWidget *parent=0);
    ~RichTextEditor();

    void setSearchSupport(bool b);
    bool searchSupport() const;

    void setSpellCheckingConfigFileName(const QString &_fileName);

    bool checkSpellingEnabled() const;
    void setCheckSpellingEnabled( bool check );

    void setSpellCheckingLanguage(const QString &_language);
    const QString& spellCheckingLanguage() const;

    virtual void setReadOnly( bool readOnly );
    virtual void createHighlighter();


private Q_SLOTS:
    void slotSpeakText();
    void slotUndoableClear();
    void slotCheckSpelling();
    void slotSpellCheckerMisspelling( const QString &text, int pos );
    void slotSpellCheckerCorrected( const QString &, int,const QString &);
    void slotSpellCheckerAutoCorrect(const QString&,const QString&);
    void slotSpellCheckerCanceled();
    void slotSpellCheckerFinished();
    void slotToggleAutoSpellCheck();


protected:
    virtual void addExtraMenuEntry(QMenu *menu);
    void contextMenuEvent( QContextMenuEvent *event );
    void wheelEvent( QWheelEvent *event );
    void focusInEvent( QFocusEvent *event );

Q_SIGNALS:
    void findText();
    void replaceText();
    void spellCheckerAutoCorrect(const QString& currentWord, const QString& autoCorrectWord);
    void checkSpellingChanged(bool);
    void languageChanged(const QString &);

private:
    void setHighlighter(Sonnet::Highlighter *_highLighter);
    void highlightWord( int length, int pos );
    class RichTextEditorPrivate;
    RichTextEditorPrivate *const d;
};
}

#endif // RICHTEXTEDITOR_H
