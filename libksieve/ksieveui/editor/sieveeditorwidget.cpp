/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

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

#include "sieveeditorwidget.h"

#include "sieve-editor.h"
#include "sieveeditortextmodewidget.h"
#include "scriptsparsing/parsingutil.h"
#include "autocreatescripts/sieveeditorgraphicalmodewidget.h"
#include "autocreatescripts/sievescriptparsingerrordialog.h"
#include "sieveeditormenubar.h"

#include <knewstuff3/uploaddialog.h>
#include <klocale.h>
#include <KStandardGuiItem>
#include <ktempdir.h>
#include <kzip.h>
#include <ktemporaryfile.h>

#include <QPushButton>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QLineEdit>
#include <QToolBar>
#include <QDebug>
#include <QAction>
#include <QPointer>
#include <QDir>


using namespace KSieveUi;

SieveEditorWidget::SieveEditorWidget(bool useMenuBar, QWidget *parent)
    : QWidget(parent),
      mMode(TextMode),
      mModified(false)
{
    QVBoxLayout *lay = new QVBoxLayout;
#if !defined(NDEBUG)
    mGenerateXml = 0;
#endif

    QToolBar *toolbar = new QToolBar;
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);


    mCheckSyntax = new QAction(i18n("Check Syntax"), this);
    connect(mCheckSyntax, SIGNAL(triggered(bool)), SLOT(slotCheckSyntax()));
    toolbar->addAction(mCheckSyntax);
    mSaveAs = toolbar->addAction(KStandardGuiItem::saveAs().text(), this, SLOT(slotSaveAs()));
    toolbar->addAction(i18n("Import..."), this, SLOT(slotImport()));

    mAutoGenerateScript = new QAction(i18n("Autogenerate Script..."), this);
    connect(mAutoGenerateScript, SIGNAL(triggered(bool)), SLOT(slotAutoGenerateScripts()));
    toolbar->addAction(mAutoGenerateScript);
    //KF5 add i18n
    mSwitchMode = new QAction(this);
    toolbar->addAction(mSwitchMode);
    connect(mSwitchMode, SIGNAL(triggered(bool)), SLOT(slotSwitchMode()));
#if !defined(NDEBUG)
    if (!qgetenv("KDEPIM_SIEVEEDITOR_DEBUG").isEmpty()) {
        //Not necessary to translate it.
        mGenerateXml = new QAction(QLatin1String("Generate xml"), this);
        connect(mGenerateXml, SIGNAL(triggered(bool)), SLOT(slotGenerateXml()));
        toolbar->addAction(mGenerateXml);
    }
#endif

    QStringList overlays;
    overlays <<QLatin1String("list-add");
    //KF5 add i18n + tooltip
    mUpload = new QAction(KIcon(QLatin1String("get-hot-new-stuff"), 0, overlays), QLatin1String("Share..."), this);
    connect(mUpload, SIGNAL(triggered(bool)), SLOT(slotUploadScripts()));

    //Add action to toolBar

    toolbar->addAction(mUpload);


    SieveEditorMenuBar *menuBar = 0;
    if (useMenuBar) {
        menuBar = new SieveEditorMenuBar;
        connect(menuBar, SIGNAL(copy()), SLOT(copy()));
        connect(menuBar, SIGNAL(find()), SLOT(find()));
        connect(menuBar, SIGNAL(replace()), SLOT(replace()));
        connect(menuBar, SIGNAL(undo()), SLOT(undo()));
        connect(menuBar, SIGNAL(redo()), SLOT(redo()));
        connect(menuBar, SIGNAL(paste()), SLOT(paste()));
        connect(menuBar, SIGNAL(cut()), SLOT(cut()));
        connect(menuBar, SIGNAL(selectAll()), SLOT(selectAll()));
        connect(menuBar, SIGNAL(gotoLine()), SLOT(goToLine()));
        connect(menuBar, SIGNAL(comment()), SLOT(comment()));
        connect(menuBar, SIGNAL(uncomment()), SLOT(uncomment()));

        connect(this, SIGNAL(copyAvailable(bool)), menuBar, SLOT(slotCopyAvailable(bool)));
        connect(this, SIGNAL(redoAvailable(bool)), menuBar, SLOT(slotRedoAvailable(bool)));
        connect(this, SIGNAL(undoAvailable(bool)), menuBar, SLOT(slotUndoAvailable(bool)));
        menuBar->fileMenu()->addAction(mSaveAs);
        menuBar->fileMenu()->addSeparator();
        menuBar->fileMenu()->addAction(mUpload);
        menuBar->toolsMenu()->addSeparator();
        menuBar->toolsMenu()->addAction(mAutoGenerateScript);
        menuBar->toolsMenu()->addAction(mCheckSyntax);
        lay->addWidget(menuBar);
    }

    lay->addWidget(toolbar);


    QHBoxLayout *nameLayout = new QHBoxLayout;
    QLabel *label = new QLabel( i18n( "Script name:" ) );
    nameLayout->addWidget( label );
    mScriptName = new QLineEdit;
    mScriptName->setReadOnly( true );
    nameLayout->addWidget( mScriptName );
    lay->addLayout( nameLayout );


    lay->setMargin(0);
    setLayout(lay);
    mStackedWidget = new QStackedWidget;

    mTextModeWidget = new SieveEditorTextModeWidget;
    connect(mTextModeWidget, SIGNAL(valueChanged()), this, SLOT(slotModified()));
    mStackedWidget->addWidget(mTextModeWidget);
    mGraphicalModeWidget = new SieveEditorGraphicalModeWidget;
    connect(mGraphicalModeWidget, SIGNAL(valueChanged()), this, SLOT(slotModified()));
    mStackedWidget->addWidget(mGraphicalModeWidget);

    lay->addWidget(mStackedWidget);
    connect(mTextModeWidget, SIGNAL(enableButtonOk(bool)), this, SLOT(slotEnableButtonOk(bool)));
    connect(mTextModeWidget, SIGNAL(undoAvailable(bool)), this, SIGNAL(undoAvailable(bool)));
    connect(mTextModeWidget, SIGNAL(redoAvailable(bool)), this, SIGNAL(redoAvailable(bool)));
    connect(mTextModeWidget, SIGNAL(copyAvailable(bool)), this, SIGNAL(copyAvailable(bool)));
    connect(mGraphicalModeWidget, SIGNAL(enableButtonOk(bool)), this, SLOT(slotEnableButtonOk(bool)));
    connect(mGraphicalModeWidget, SIGNAL(switchTextMode(QString)), this, SLOT(slotSwitchTextMode(QString)));
    connect(mTextModeWidget, SIGNAL(switchToGraphicalMode()), SLOT(slotSwitchToGraphicalMode()));
    if (KSieveUi::EditorSettings::useGraphicEditorByDefault()) {
        changeMode(GraphicMode);
    } else {
        changeSwitchButtonText();
    }
}

SieveEditorWidget::~SieveEditorWidget()
{
}

void SieveEditorWidget::setReadOnly(bool b)
{
    mTextModeWidget->setReadOnly(b);
    mGraphicalModeWidget->setDisabled(b);
}

void SieveEditorWidget::slotModified()
{
    mModified = true;
    Q_EMIT valueChanged(mModified);
}

bool SieveEditorWidget::isModified() const
{
    return mModified;
}

void SieveEditorWidget::undo()
{
    if (mMode == TextMode) {
        mTextModeWidget->undo();
    }
}

void SieveEditorWidget::redo()
{
    if (mMode == TextMode) {
        mTextModeWidget->redo();
    }
}

void SieveEditorWidget::goToLine()
{
    if (mMode == TextMode) {
        mTextModeWidget->goToLine();
    }
}

void SieveEditorWidget::cut()
{
    if (mMode == TextMode) {
        mTextModeWidget->cut();
    }
}

void SieveEditorWidget::paste()
{
    if (mMode == TextMode) {
        mTextModeWidget->paste();
    }
}

void SieveEditorWidget::copy()
{
    if (mMode == TextMode) {
        mTextModeWidget->copy();
    }
}

void SieveEditorWidget::selectAll()
{
    if (mMode == TextMode) {
        mTextModeWidget->selectAll();
    }
}

void SieveEditorWidget::find()
{
    if (mMode == TextMode) {
        mTextModeWidget->find();
    }
}

void SieveEditorWidget::replace()
{
    if (mMode == TextMode) {
        mTextModeWidget->replace();
    }
}

bool SieveEditorWidget::isUndoAvailable() const
{
    if (mMode == TextMode) {
        return mTextModeWidget->isUndoAvailable();
    }
    return false;
}

bool SieveEditorWidget::isRedoAvailable() const
{
    if (mMode == TextMode) {
        return mTextModeWidget->isRedoAvailable();
    }
    return false;
}

bool SieveEditorWidget::hasSelection() const
{
    if (mMode == TextMode) {
        return mTextModeWidget->hasSelection();
    }
    return false;
}

void SieveEditorWidget::comment()
{
    if (mMode == TextMode) {
        mTextModeWidget->comment();
    }
}

void SieveEditorWidget::uncomment()
{
    if (mMode == TextMode) {
        mTextModeWidget->uncomment();
    }
}

SieveEditorWidget::EditorMode SieveEditorWidget::mode() const
{
    return mMode;
}

void SieveEditorWidget::setModified(bool b)
{
    if (mModified != b) {
        mModified = b;
        Q_EMIT valueChanged(mModified);
    }
}

void SieveEditorWidget::slotUploadScripts()
{
    KTempDir tmp;
    KTemporaryFile tmpFile;
    if (tmpFile.open()) {
        QTextStream out(&tmpFile);
        out.setCodec("UTF-8");
        out << script();
        tmpFile.close();
        const QString sourceName = mScriptName->text();
        const QString zipFileName = tmp.name() + QDir::separator() + sourceName + QLatin1String(".zip");
        KZip *zip = new KZip(zipFileName);
        if (zip->open(QIODevice::WriteOnly)) {
            if (zip->addLocalFile(tmpFile.fileName(), sourceName + QLatin1String(".siv"))) {
                zip->close();
                QPointer<KNS3::UploadDialog> dialog = new KNS3::UploadDialog(QLatin1String("ksieve_script.knsrc"), this);
                dialog->setUploadFile(zipFileName);
                dialog->setUploadName(sourceName);
                dialog->setDescription(i18nc("Default description for the source", "My Sieve Script"));
                dialog->exec();
                delete dialog;
            } else {
                zip->close();
            }
        }
        delete zip;
    }
}

void SieveEditorWidget::changeMode(EditorMode mode)
{
    if (mode != mMode) {
        mMode = mode;
        mStackedWidget->setCurrentIndex(static_cast<int>(mode));
        const bool isTextMode = (mMode == TextMode);
        mAutoGenerateScript->setEnabled(isTextMode);
#if !defined(NDEBUG)
        if (mGenerateXml)
            mGenerateXml->setEnabled((mMode == TextMode));
#endif
        if (mMode == GraphicMode)
            mCheckSyntax->setEnabled(false);
        else
            mCheckSyntax->setEnabled(!mTextModeWidget->currentscript().isEmpty());
        Q_EMIT modeEditorChanged(mode);
        changeSwitchButtonText();
    }
}

void SieveEditorWidget::changeSwitchButtonText()
{
    //KF5 add i18n
    mSwitchMode->setText((mMode == TextMode) ?  QLatin1String("Simple Mode") : QLatin1String("Advanced Mode"));
}

void SieveEditorWidget::slotEnableButtonOk(bool b)
{
    Q_EMIT enableButtonOk(b);
    mSaveAs->setEnabled(b);
    if (mMode == TextMode) {
        mCheckSyntax->setEnabled(b);
    } else {
        mCheckSyntax->setEnabled(false);
    }
}

QString SieveEditorWidget::script() const
{
    QString currentScript;
    switch (mMode) {
    case TextMode:
        currentScript = mTextModeWidget->script();
        break;
    case GraphicMode:
        currentScript = mGraphicalModeWidget->currentscript();
        break;
    case Unknown:
        qDebug()<<" Unknown Mode!";
        break;
    }
    return currentScript;
}

QString SieveEditorWidget::originalScript() const
{
    return mOriginalScript;
}

void SieveEditorWidget::setScript( const QString &script )
{
    mTextModeWidget->setScript( script );
    mOriginalScript = script;
}

void SieveEditorWidget::addFailedMessage(const QString &err)
{
    addMessageEntry( err, QColor( Qt::darkRed ) );
}

void SieveEditorWidget::addOkMessage(const QString &msg)
{
    addMessageEntry( msg, QColor( Qt::darkGreen ) );
}

void SieveEditorWidget::addMessageEntry( const QString &errorMsg, const QColor &color )
{
    QString msg = errorMsg;
    msg.replace(QLatin1Char('\n'), QLatin1String("<br>"));
    const QString logText = QString::fromLatin1( "<font color=%1>%2</font>" )
           .arg( color.name() ).arg(msg);

    setDebugScript( logText );
}

void SieveEditorWidget::setDebugScript( const QString &debug )
{
    mTextModeWidget->setDebugScript( debug );
}

void SieveEditorWidget::setScriptName( const QString &name )
{
    mScriptName->setText( name );
}

void SieveEditorWidget::resultDone()
{
    mCheckSyntax->setEnabled(true);
}

void SieveEditorWidget::setSieveCapabilities( const QStringList &capabilities )
{
    mTextModeWidget->setSieveCapabilities(capabilities);
    mGraphicalModeWidget->setSieveCapabilities(capabilities);
}

void SieveEditorWidget::slotAutoGenerateScripts()
{
    switch (mMode) {
    case TextMode:
        mTextModeWidget->autoGenerateScripts();
        break;
    case GraphicMode:
    case Unknown:
        break;
    }
}

void SieveEditorWidget::slotCheckSyntax()
{
    switch (mMode) {
    case TextMode:
        mCheckSyntax->setEnabled(false);
        Q_EMIT checkSyntax();
        break;
    case GraphicMode:
    case Unknown:
        break;
    }
}

void SieveEditorWidget::slotGenerateXml()
{
#if !defined(NDEBUG)
    switch (mMode) {
    case TextMode:
        mTextModeWidget->generateXml();
        break;
    case GraphicMode:
    case Unknown:
        break;
    }
#endif
}

void SieveEditorWidget::checkSpelling()
{
    switch (mMode) {
    case TextMode:
        mTextModeWidget->checkSpelling();
        break;
    case GraphicMode:
    case Unknown:
        break;
    }
}

void SieveEditorWidget::slotSaveAs()
{
    switch (mMode) {
    case TextMode:
        mTextModeWidget->saveAs(mScriptName->text());
        break;
    case GraphicMode:
        mGraphicalModeWidget->saveAs(mScriptName->text());
        break;
    case Unknown:
        qDebug()<<" Unknown mode";
        break;
    }
}

void SieveEditorWidget::slotImport()
{
    switch (mMode) {
    case TextMode:
        mTextModeWidget->slotImport();
        break;
    case GraphicMode:
        mGraphicalModeWidget->slotImport();
        break;
    case Unknown:
        qDebug()<<" Unknown mode";
        break;
    }
}

void SieveEditorWidget::slotSwitchToGraphicalMode()
{
    mTextModeWidget->hideEditorWarning();
    changeMode(GraphicMode);
}

void SieveEditorWidget::slotSwitchMode()
{
    switch (mMode) {
    case TextMode: {
        bool result = false;
        const QDomDocument doc = ParsingUtil::parseScript(mTextModeWidget->currentscript(), result);
        if (result) {
            QString error;
            mGraphicalModeWidget->loadScript(doc, error);
            mTextModeWidget->hideEditorWarning();
            if (!error.isEmpty()) {
                mTextModeWidget->setParsingEditorWarningError(mTextModeWidget->currentscript(), error);
                mTextModeWidget->showParsingEditorWarning();
            } else {
                changeMode(GraphicMode);
            }
        } else {
            mTextModeWidget->showEditorWarning();
            qDebug() << "cannot parse file";
        }
        break;
    }
    case GraphicMode: {
        const QString script = mGraphicalModeWidget->currentscript();
        changeMode(TextMode);
        mTextModeWidget->setScript(script);
        break;
    }
    case Unknown:
        qDebug()<<" Unknown mode";
        break;
    }
}

void SieveEditorWidget::slotSwitchTextMode(const QString &script)
{
    changeMode(TextMode);
    mTextModeWidget->setScript(script);
}

void SieveEditorWidget::reverseCase()
{
    if (mMode == TextMode) {
        mTextModeWidget->reverseCase();
    }
}

void SieveEditorWidget::lowerCase()
{
    if (mMode == TextMode) {
        mTextModeWidget->lowerCase();
    }
}

void SieveEditorWidget::upperCase()
{
    if (mMode == TextMode) {
        mTextModeWidget->upperCase();
    }
}

void SieveEditorWidget::sentenceCase()
{
    if (mMode == TextMode) {
        mTextModeWidget->sentenceCase();
    }
}
