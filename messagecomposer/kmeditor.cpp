/*
 * kmeditor.cpp
 *
 * Copyright 2007 Laurent Montel <montel@kde.org>
 * Copyright 2008 Thomas McGuire <mcguire@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "kmeditor.h"
#include "textpart.h"
#include "messageviewer/nodehelper.h"

#include <KEncodingFileDialog>
#include <KLocale>
#include <KMessageBox>
#include <KProcess>
#include <KPushButton>
#include <KTemporaryFile>

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QPointer>
#include <QProcess>
#include <QShortcut>
#include <QTextLayout>
#include <QTimer>

using namespace KPIMTextEdit;

namespace Message {

class KMeditorPrivate
{
  public:
    KMeditorPrivate( KMeditor *parent )
     : q( parent ),
       useExtEditor( false ),
       mExtEditorProcess( 0 ),
       mExtEditorTempFile( 0 )    {
    }

    ~KMeditorPrivate()
    {
    }

    //
    // Slots
    //

    // Just calls KTextEdit::ensureCursorVisible(), workaround for some bug.
    void ensureCursorVisibleDelayed();

    //
    // Normal functions
    //

    void init();
    QString addQuotesToText( const QString &inputText );

    void startExternalEditor();
    void slotEditorFinished( int, QProcess::ExitStatus exitStatus );

    /**
     * Replaces each text which matches the regular expression with another text.
     * Text inside quotes or the given signature will be ignored.
     */
    void cleanWhitespaceHelper( const QRegExp &regExp, const QString &newText,
                                const KPIMIdentities::Signature &sig );

    /**
     * Returns a list of all occurrences of the given signature.
     * The list contains pairs which consists of the starting position and
     * the end of the signature.
     *
     * @param sig this signature will be searched for
     * @return a list of pairs of start and end positions of the signature
     */
    QList< QPair<int,int> > signaturePositions( const KPIMIdentities::Signature &sig ) const;

    // Data members
    QString extEditorPath;
    KMeditor *q;
    bool useExtEditor;
    
    QString quotePrefix;

    KProcess *mExtEditorProcess;
    KTemporaryFile *mExtEditorTempFile;
};

}

using namespace Message;

void KMeditorPrivate::startExternalEditor()
{
  if ( extEditorPath.isEmpty() ) {
    q->setUseExternalEditor( false );
    //TODO: show messagebox
    return;
  }

  mExtEditorTempFile = new KTemporaryFile();
  if ( !mExtEditorTempFile->open() ) {
    delete mExtEditorTempFile;
    mExtEditorTempFile = 0;
    q->setUseExternalEditor( false );
    return;
  }

  mExtEditorTempFile->write( q->textOrHtml().toUtf8() );
  mExtEditorTempFile->close();

  mExtEditorProcess = new KProcess();
    // construct command line...
  QStringList command = extEditorPath.split( QLatin1Char( ' ' ), QString::SkipEmptyParts );
  bool filenameAdded = false;
  for ( QStringList::Iterator it = command.begin(); it != command.end(); ++it ) {
    if ( ( *it ).contains( QLatin1String( "%f" ) ) ) {
      ( *it ).replace( QRegExp( QLatin1String( "%f" ) ), mExtEditorTempFile->fileName() );
      filenameAdded = true;
    }
    else if ( ( *it ).contains( QLatin1String( "%l" ) ) ) {
      ( *it ).replace( QRegExp( QLatin1String( "%l" ) ), QString::number(q->textCursor().blockNumber() + 1) );  // line number
    }
    ( *mExtEditorProcess ) << ( *it );
  }
  if ( !filenameAdded ) { // no %f in the editor command
    ( *mExtEditorProcess ) << mExtEditorTempFile->fileName();
  }

  QObject::connect( mExtEditorProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
                    q, SLOT(slotEditorFinished(int,QProcess::ExitStatus)) );
  mExtEditorProcess->start();
  if ( !mExtEditorProcess->waitForStarted() ) {
    //TODO: messagebox
    mExtEditorProcess->deleteLater();
    mExtEditorProcess = 0;
    delete mExtEditorTempFile;
    mExtEditorTempFile = 0;
    q->setUseExternalEditor( false );
  }
}

void KMeditorPrivate::slotEditorFinished( int, QProcess::ExitStatus exitStatus )
{
  if ( exitStatus == QProcess::NormalExit ) {
    // the external editor could have renamed the original file and recreated a new file
    // with the given filename, so we need to reopen the file after the editor exited
    QFile localFile(mExtEditorTempFile->fileName());
    if ( localFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
      QByteArray f = localFile.readAll();
      q->setTextOrHtml( QString::fromUtf8( f.data(), f.size() ) );
      q->document()->setModified( true );
      localFile.close();
    }
  }

  q->killExternalEditor();   // cleanup...
}

void KMeditorPrivate::ensureCursorVisibleDelayed()
{
  static_cast<KPIMTextEdit::TextEdit*>( q )->ensureCursorVisible();
}

void KMeditor::keyPressEvent ( QKeyEvent *e )
{
  if ( d->useExtEditor &&
       ( e->key() != Qt::Key_Shift ) &&
       ( e->key() != Qt::Key_Control ) &&
       ( e->key() != Qt::Key_Meta ) &&
       ( e->key() != Qt::Key_CapsLock ) &&
       ( e->key() != Qt::Key_NumLock ) &&
       ( e->key() != Qt::Key_ScrollLock ) &&
       ( e->key() != Qt::Key_Alt ) &&
       ( e->key() != Qt::Key_AltGr ) ) {
    if ( !d->mExtEditorProcess ) {
      d->startExternalEditor();
    }
    return;
  }

  if ( e->key() == Qt::Key_Up && e->modifiers() != Qt::ShiftModifier &&
       textCursor().block().position() == 0 &&
       textCursor().block().layout()->lineForTextPosition( textCursor().position() ).lineNumber() == 0 ) {
    textCursor().clearSelection();
    emit focusUp();
  } else if ( e->key() == Qt::Key_Backtab && e->modifiers() == Qt::ShiftModifier ) {
    textCursor().clearSelection();
    emit focusUp();
  } else {
    TextEdit::keyPressEvent( e );
  }
}

KMeditor::KMeditor( const QString &text, QWidget *parent )
 : TextEdit( text, parent ), d( new KMeditorPrivate( this ) )
{
  d->init();
}

KMeditor::KMeditor( QWidget *parent )
 : TextEdit( parent ), d( new KMeditorPrivate( this ) )
{
  d->init();
}

KMeditor::KMeditor( QWidget *parent, const QString & configFile )
 : TextEdit( parent, configFile ), d( new KMeditorPrivate( this ) )
{
  d->init();
}

KMeditor::~KMeditor()
{
  delete d;
}

void KMeditorPrivate::init()
{
  QShortcut * insertMode = new QShortcut( QKeySequence( Qt::Key_Insert ), q );
  q->connect( insertMode, SIGNAL(activated()),
              q, SLOT(slotChangeInsertMode()) );
}

void KMeditor::slotChangeInsertMode()
{
  setOverwriteMode( !overwriteMode() );
  emit insertModeChanged();
}

void KMeditor::setUseExternalEditor( bool use )
{
  d->useExtEditor = use;
}

void KMeditor::setExternalEditorPath( const QString &path )
{
  d->extEditorPath = path;
}

void KMeditor::setFontForWholeText( const QFont &font )
{
  QTextCharFormat fmt;
  fmt.setFont( font );
  QTextCursor cursor( document() );
  cursor.movePosition( QTextCursor::End, QTextCursor::KeepAnchor );
  cursor.mergeCharFormat( fmt );
  document()->setDefaultFont( font );
}

KUrl KMeditor::insertFile()
{
  QPointer<KEncodingFileDialog> fdlg =
    new KEncodingFileDialog( QString(), QString(),  QString(), QString(),
                             KFileDialog::Opening, this );
  fdlg->okButton()->setText( i18nc( "@action:button", "&Insert" ) );
  fdlg->setCaption( i18nc( "@title:window", "Insert File" ) );

  KUrl url;
  if ( fdlg->exec() && fdlg ) {
    url = fdlg->selectedUrl();
    url.setFileEncoding( MessageViewer::NodeHelper::fixEncoding( fdlg->selectedEncoding() ) );
  }
  delete fdlg;
  return url;
}

void KMeditor::enableWordWrap( int wrapColumn )
{
  setWordWrapMode( QTextOption::WordWrap );
  setLineWrapMode( QTextEdit::FixedColumnWidth );
  setLineWrapColumnOrWidth( wrapColumn );
}

void KMeditor::disableWordWrap()
{
  setLineWrapMode( QTextEdit::WidgetWidth );
}

void KMeditor::slotPasteAsQuotation()
{
#ifndef QT_NO_CLIPBOARD
  if ( hasFocus() ) {
    QString s = QApplication::clipboard()->text();
    if ( !s.isEmpty() ) {
      insertPlainText( d->addQuotesToText( s ) );
    }
  }
#endif
}

void KMeditor::slotRemoveQuotes()
{
  QTextCursor cursor = textCursor();
  cursor.beginEditBlock();
  if ( !cursor.hasSelection() ) {
    cursor.select( QTextCursor::Document );
  }

  QTextBlock block = document()->findBlock( cursor.selectionStart() );
  int selectionEnd = cursor.selectionEnd();
  while ( block.isValid() && block.position() <= selectionEnd ) {
    cursor.setPosition( block.position() );
    if ( isLineQuoted( block.text() ) ) {
      int length = quoteLength( block.text() );
      cursor.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor, length );
      cursor.removeSelectedText();
      selectionEnd -= length;
    }
    block = block.next();
  }
  cursor.clearSelection();
  cursor.endEditBlock();
}

void KMeditor::slotAddQuotes()
{
  QTextCursor cursor = textCursor();
  cursor.beginEditBlock();
  if ( !cursor.hasSelection() ) {
    cursor.select( QTextCursor::Document );
  }

  QTextBlock block = document()->findBlock( cursor.selectionStart() );
  int selectionEnd = cursor.selectionEnd();
  while ( block.isValid() && block.position() <= selectionEnd ) {
    cursor.setPosition( block.position() );
    cursor.insertText( defaultQuoteSign() );
    selectionEnd += defaultQuoteSign().length();
    block = block.next();
  }
  cursor.clearSelection();
  cursor.endEditBlock();
}

QString KMeditorPrivate::addQuotesToText( const QString &inputText )
{
  QString answer = QString( inputText );
  QString indentStr = q->defaultQuoteSign();
  answer.replace( QLatin1Char( '\n' ), QLatin1Char( '\n' ) + indentStr );
  //cursor.selectText() as QChar::ParagraphSeparator as paragraph separator.
  answer.replace( QChar::ParagraphSeparator, QLatin1Char( '\n' ) + indentStr );
  answer.prepend( indentStr );
  answer += QLatin1Char( '\n' );
  return q->smartQuote( answer );
}


const QString KMeditor::defaultQuoteSign() const
{
  if ( !d->quotePrefix.simplified().isEmpty() )
    return d->quotePrefix;
  else
    return KPIMTextEdit::TextEdit::defaultQuoteSign();
}

int KMeditor::quoteLength( const QString& line ) const
{
  if ( !d->quotePrefix.simplified().isEmpty() ) {
    if ( line.startsWith( d->quotePrefix ) )
      return d->quotePrefix.length();
    else
      return 0;
  }
  else
    return KPIMTextEdit::TextEdit::quoteLength( line );
}

void KMeditor::setQuotePrefixName( const QString &quotePrefix )
{
  d->quotePrefix = quotePrefix;
}

QString KMeditor::quotePrefixName() const
{
  if ( !d->quotePrefix.simplified().isEmpty() )
    return d->quotePrefix;
  else
    return QLatin1String( ">" );
}

QString KMeditor::smartQuote( const QString &msg )
{
  return msg;
}

bool KMeditor::checkExternalEditorFinished()
{
  if ( !d->mExtEditorProcess ) {
    return true;
  }

  int ret = KMessageBox::warningYesNoCancel(
    topLevelWidget(),
    i18nc( "@info",
           "The external editor is still running.<nl/>"
           "Do you want to stop the editor or keep it running?<nl/>"
           "<warning>Stopping the editor will cause all your "
           "unsaved changes to be lost.</warning>" ),
    i18nc( "@title:window", "External Editor Running" ),
    KGuiItem( i18nc( "@action:button", "Stop Editor" ) ),
    KGuiItem( i18nc( "@action:button", "Keep Editor Running" ) ) );

  switch( ret ) {
  case KMessageBox::Yes:
    killExternalEditor();
    return true;
  case KMessageBox::No:
    return true;
  default:
    return false;
  }
}

void KMeditor::killExternalEditor()
{
  if ( d->mExtEditorProcess ) {
    d->mExtEditorProcess->deleteLater();
  }
  d->mExtEditorProcess = 0;
  delete d->mExtEditorTempFile;
  d->mExtEditorTempFile = 0;
}

void KMeditor::setCursorPositionFromStart( unsigned int pos )
{
  if ( pos > 0 ) {
    QTextCursor cursor = textCursor();
    cursor.setPosition( pos );
    setTextCursor( cursor );
    ensureCursorVisible();
  }
}

int KMeditor::linePosition()
{
  const QTextCursor cursor = textCursor();
  const QTextDocument *doc = document();
  QTextBlock block = doc->begin();
  int lineCount = 0;

  // Simply using cursor.block.blockNumber() would not work since that does not
  // take word-wrapping into account, i.e. it is possible to have more than one
  // line in a block.
  //
  // What we have to do therefore is to iterate over the blocks and count the
  // lines in them. Once we have reached the block where the cursor is, we have
  // to iterate over each line in it, to find the exact line in the block where
  // the cursor is.
  while ( block.isValid() ) {
    const QTextLayout *layout = block.layout();

    // If the current block has the cursor in it, iterate over all its lines
    if ( block == cursor.block() ) {

      // Special case: Cursor at end of single non-wrapped line, exit early
      // in this case as the logic below can't handle it
      if ( block.lineCount() == layout->lineCount() ) {
        return lineCount;
      }

      const int cursorBasePosition = cursor.position() - block.position();
      for ( int i = 0; i < layout->lineCount(); i++ ) {
        QTextLine line = layout->lineAt( i );
        if ( cursorBasePosition >= line.textStart() &&
             cursorBasePosition < line.textStart() + line.textLength() ) {
          break;
        }
        lineCount++;
      }
      return lineCount;
    } else {
      // No, cursor is not in the current block
      lineCount += layout->lineCount();
    }

    block = block.next();
  }

  // Only gets here if the cursor block can't be found, shouldn't happen except
  // for an empty document maybe
  return lineCount;
}

int KMeditor::columnNumber()
{
  QTextCursor cursor = textCursor();
  return cursor.columnNumber();
}

void KMeditor::ensureCursorVisible()
{
  QCoreApplication::processEvents();

  // Hack: In KMail, the layout of the composer changes again after
  //       creating the editor (the toolbar/menubar creation is delayed), so
  //       the size of the editor changes as well, possibly hiding the cursor
  //       even though we called ensureCursorVisible() before the layout phase.
  //
  //       Delay the actual call to ensureCursorVisible() a bit to work around
  //       the problem.
  QTimer::singleShot( 500, this, SLOT( ensureCursorVisibleDelayed() ) );
}

void KMeditorPrivate::cleanWhitespaceHelper( const QRegExp &regExp,
                                             const QString &newText,
                                             const KPIMIdentities::Signature &sig )
{
  int currentSearchPosition = 0;

  forever {

    // Find the text
    QString text = q->document()->toPlainText();
    int currentMatch = regExp.indexIn( text, currentSearchPosition );
    currentSearchPosition = currentMatch;
    if ( currentMatch == -1 ) {
      break;
    }

    // Select the text
    QTextCursor cursor( q->document() );
    cursor.setPosition( currentMatch );
    cursor.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                         regExp.matchedLength() );

    // Skip quoted text
    if ( q->isLineQuoted( cursor.block().text() ) ) {
      currentSearchPosition += regExp.matchedLength();
      continue;
    }

    // Skip text inside signatures
    bool insideSignature = false;
    QList< QPair<int,int> > sigPositions = signaturePositions( sig );
    QPair<int,int> position;
    foreach ( position, sigPositions ) { //krazy:exclude=foreach
      if ( cursor.position() >= position.first &&
           cursor.position() <= position.second ) {
        insideSignature = true;
      }
    }
    if ( insideSignature ) {
      currentSearchPosition += regExp.matchedLength();
      continue;
    }

    // Replace the text
    cursor.removeSelectedText();
    cursor.insertText( newText );
    currentSearchPosition += newText.length();
  }
}

void KMeditor::cleanWhitespace( const KPIMIdentities::Signature &sig )
{
  QTextCursor cursor( document() );
  cursor.beginEditBlock();

  // Squeeze tabs and spaces
  d->cleanWhitespaceHelper( QRegExp( QLatin1String( "[\t ]+" ) ),
                            QString( QLatin1Char( ' ' ) ), sig );

  // Remove trailing whitespace
  d->cleanWhitespaceHelper( QRegExp( QLatin1String( "[\t ][\n]" ) ),
                            QString( QLatin1Char( '\n' ) ), sig );

  // Single space lines
  d->cleanWhitespaceHelper( QRegExp( QLatin1String( "[\n]{3,}" ) ),
                            QLatin1String( "\n\n" ), sig );

  if ( !textCursor().hasSelection() ) {
    textCursor().clearSelection();
  }

  cursor.endEditBlock();
}

QList< QPair<int,int> >
KMeditorPrivate::signaturePositions( const KPIMIdentities::Signature &sig ) const
{
  QList< QPair<int,int> > signaturePositions;
  if ( !sig.rawText().isEmpty() ) {

    QString sigText = sig.toPlainText();

    int currentSearchPosition = 0;
    forever {

      // Find the next occurrence of the signature text
      QString text = q->document()->toPlainText();
      int currentMatch = text.indexOf( sigText, currentSearchPosition );
      currentSearchPosition = currentMatch + sigText.length();
      if ( currentMatch == -1 ) {
        break;
      }

      signaturePositions.append( QPair<int,int>( currentMatch,
                                 currentMatch + sigText.length() ) );
    }
  }
  return signaturePositions;
}

bool KMeditor::replaceSignature( const KPIMIdentities::Signature &oldSig,
                                 const KPIMIdentities::Signature &newSig )
{
  bool found = false;
  QString oldSigText = oldSig.toPlainText();
  if ( oldSigText.isEmpty() )
    return false;

  QTextCursor cursor( document() );
  cursor.beginEditBlock();
  int currentSearchPosition = 0;
  forever {

    // Find the next occurrence of the signature text
    const QString text = document()->toPlainText();
    int currentMatch = text.indexOf( oldSigText, currentSearchPosition );
    currentSearchPosition = currentMatch;
    if ( currentMatch == -1 ) {
      break;
    }

    // Select the signature
    QTextCursor cursor( document() );
    cursor.setPosition( currentMatch );

    // If the new signature is completely empty, we also want to remove the
    // signature separator, so include it in the selection
    int additionalMove = 0;
    if ( newSig.rawText().isEmpty() &&
         text.mid( currentMatch - 4, 4 ) == QLatin1String( "-- \n" ) ) {
      cursor.movePosition( QTextCursor::PreviousCharacter,
                           QTextCursor::MoveAnchor, 5 );
      additionalMove = 5;
    }
    cursor.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                         oldSigText.length() + additionalMove );

    // Skip quoted signatures
    if ( isLineQuoted( cursor.block().text() ) ) {
      currentSearchPosition += oldSig.toPlainText().length();
      continue;
    }

    // Remove the old and insert the new signature
    cursor.removeSelectedText();
    setTextCursor( cursor );
    newSig.insertIntoTextEdit( KPIMIdentities::Signature::AtCursor,
                               KPIMIdentities::Signature::AddNothing, this );
    found = true;

    currentSearchPosition += newSig.toPlainText().length();
  }

  cursor.endEditBlock();
  return found;
}

void KMeditor::fillComposerTextPart ( TextPart* textPart ) const
{
  textPart->setCleanPlainText( toCleanPlainText() );
  textPart->setWrappedPlainText( toWrappedPlainText() );
  if( isFormattingUsed() ) {
    textPart->setCleanHtml( toCleanHtml() );
    textPart->setEmbeddedImages( embeddedImages() );
  }
}

#include "kmeditor.moc"
