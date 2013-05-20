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

#include "themeeditormainwindow.h"
#include "themeeditorpage.h"
#include "newthemedialog.h"
#include "themeconfiguredialog.h"
#include "managethemes.h"

#include <KTemporaryFile>
#include <KTempDir>
#include <KStandardAction>
#include <KApplication>
#include <KAction>
#include <KToggleAction>
#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageBox>
#include <KFileDialog>
#include <KDebug>
#include <KStandardDirs>
#include <KNS3/KNewStuffAction>

#include <QPointer>
#include <QCloseEvent>
#include <QActionGroup>

ThemeEditorMainWindow::ThemeEditorMainWindow()
    : KXmlGuiWindow(),
      mThemeEditor(0)
{
    setupActions();
    setupGUI();
    updateActions();
    updateActions();
}

ThemeEditorMainWindow::~ThemeEditorMainWindow()
{
}

void ThemeEditorMainWindow::updateActions()
{
    const bool projectDirectoryIsEmpty = (mThemeEditor!=0);
    mAddExtraPage->setEnabled(projectDirectoryIsEmpty);
    mCloseAction->setEnabled(projectDirectoryIsEmpty);
    mUploadTheme->setEnabled(projectDirectoryIsEmpty);
    mSaveAction->setEnabled(projectDirectoryIsEmpty);
    mInstallTheme->setEnabled(projectDirectoryIsEmpty);
    mInsertFile->setEnabled(projectDirectoryIsEmpty);
    mPrintingMode->setEnabled(projectDirectoryIsEmpty);
    mNormalMode->setEnabled(projectDirectoryIsEmpty);
}

void ThemeEditorMainWindow::setupActions()
{
    mAddExtraPage = new KAction(i18n("Add Extra Page"), this);
    connect(mAddExtraPage, SIGNAL(triggered(bool)),SLOT(slotAddExtraPage()));
    actionCollection()->addAction( QLatin1String( "add_extra_page" ), mAddExtraPage );

    mUploadTheme = KNS3::standardAction(i18n("Upload theme..."), this, SLOT(slotUploadTheme()), actionCollection(), "upload_theme");

    mNewThemeAction = KStandardAction::openNew(this, SLOT(slotNewTheme()), actionCollection());
    mNewThemeAction->setText(i18n("New theme..."));

    mOpenAction = KStandardAction::open(this, SLOT(slotOpenTheme()), actionCollection());
    mOpenAction->setText(i18n("Open theme..."));
    mSaveAction = KStandardAction::save(this, SLOT(slotSaveTheme()), actionCollection());
    mCloseAction = KStandardAction::close( this, SLOT(slotCloseTheme()), actionCollection());
    KStandardAction::quit(this, SLOT(slotQuitApp()), actionCollection() );
    KStandardAction::preferences( this, SLOT(slotConfigure()), actionCollection() );

    mInstallTheme = new KAction(i18n("Install theme"), this);
    actionCollection()->addAction( QLatin1String( "install_theme" ), mInstallTheme );
    connect(mInstallTheme, SIGNAL(triggered(bool)), SLOT(slotInstallTheme()));

    mInsertFile = new KAction(i18n("Insert File..."), this);
    actionCollection()->addAction( QLatin1String( "insert_file" ), mInsertFile );
    connect(mInsertFile, SIGNAL(triggered(bool)), SLOT(slotInsertFile()));

    QActionGroup *group = new QActionGroup( this );

    mPrintingMode  = new KToggleAction(i18n("Printing mode"), this);
    actionCollection()->addAction(QLatin1String("printing_mode"), mPrintingMode );
    connect(mPrintingMode, SIGNAL(triggered(bool)), SLOT(slotPrintingMode()));
    group->addAction( mPrintingMode );

    mNormalMode  = new KToggleAction(i18n("Normal mode"), this);
    mNormalMode->setChecked(true);
    actionCollection()->addAction(QLatin1String("normal_mode"), mNormalMode );
    connect(mNormalMode, SIGNAL(triggered(bool)), SLOT(slotNormalMode()));
    group->addAction( mNormalMode );

    mManageTheme = new KAction(i18n("Manage themes..."), this);
    connect(mManageTheme, SIGNAL(triggered(bool)),SLOT(slotManageTheme()));
    actionCollection()->addAction( QLatin1String( "manage_themes" ), mManageTheme );
}

void ThemeEditorMainWindow::slotManageTheme()
{
    QPointer<ManageThemes> dialog = new ManageThemes(this);
    dialog->exec();
    delete dialog;
}

void ThemeEditorMainWindow::slotNormalMode()
{
    mThemeEditor->setPrinting(false);
}

void ThemeEditorMainWindow::slotPrintingMode()
{
    mThemeEditor->setPrinting(true);
}

void ThemeEditorMainWindow::slotInsertFile()
{
    mThemeEditor->insertFile();
}

void ThemeEditorMainWindow::slotConfigure()
{
    QPointer<ThemeConfigureDialog> dialog = new ThemeConfigureDialog(this);
    if (dialog->exec()) {
        if (mThemeEditor) {
            mThemeEditor->reloadConfig();
        }
    }

    delete dialog;
}

void ThemeEditorMainWindow::slotInstallTheme()
{
    //Save before installing :)
    if (slotSaveTheme()) {
        const QString localThemePath = KStandardDirs::locateLocal("data",QLatin1String("messageviewer/themes/"));
        mThemeEditor->installTheme(localThemePath);
    }
}

void ThemeEditorMainWindow::slotUploadTheme()
{
    //Save before upload :)
    if (slotSaveTheme())
        mThemeEditor->uploadTheme();
}

bool ThemeEditorMainWindow::slotSaveTheme()
{
    bool result = false;
    if (mThemeEditor)
        result = mThemeEditor->saveTheme(false);
    return result;
}

void ThemeEditorMainWindow::slotCloseTheme()
{
    saveCurrentProject(false);
}

void ThemeEditorMainWindow::slotOpenTheme()
{
    if (!saveCurrentProject(false))
        return;

    const QString directory = KFileDialog::getExistingDirectory(KUrl( "kfiledialog:///OpenTheme" ), this, i18n("Select theme directory"));
    if (!directory.isEmpty()) {
        const QString filename = directory + QDir::separator() + QLatin1String("theme.themerc");
        QFile file(filename);
        if (!file.exists()) {
            KMessageBox::error(this, i18n("Directory does not contain a theme file. We can not load theme."));
            return;
        }

        mThemeEditor = new ThemeEditorPage(QString(), QString());
        connect(mThemeEditor, SIGNAL(changed(bool)), mSaveAction, SLOT(setEnabled(bool)));
        mThemeEditor->loadTheme(filename);
        setCentralWidget(mThemeEditor);
        updateActions();
    }
}

void ThemeEditorMainWindow::slotAddExtraPage()
{
    if (mThemeEditor)
        mThemeEditor->addExtraPage();
}

bool ThemeEditorMainWindow::saveCurrentProject(bool createNewTheme)
{
    if (mThemeEditor) {
        if (!mThemeEditor->saveTheme())
            return false;
    }
    if (createNewTheme) {
        delete mThemeEditor;
        mThemeEditor = 0;
        QPointer<NewThemeDialog> dialog = new NewThemeDialog(this);
        QString newTheme;
        QString projectDirectory;
        if (dialog->exec()) {
            newTheme = dialog->themeName();
            projectDirectory = dialog->directory();
        }
        if (!projectDirectory.isEmpty()) {
            mThemeEditor = new ThemeEditorPage(projectDirectory, newTheme);
            connect(mThemeEditor, SIGNAL(changed(bool)), mSaveAction, SLOT(setEnabled(bool)));
            setCentralWidget(mThemeEditor);
        } else {
            setCentralWidget(0);
        }
        delete dialog;
        updateActions();
    } else {
        delete mThemeEditor;
        mThemeEditor = 0;
        setCentralWidget(0);
        updateActions();
    }
    return true;
}

void ThemeEditorMainWindow::slotNewTheme()
{
    saveCurrentProject(true);
}

void ThemeEditorMainWindow::closeEvent(QCloseEvent *e)
{
    if (!saveCurrentProject(false))
        e->ignore();
    else
        e->accept();
}

void ThemeEditorMainWindow::slotQuitApp()
{
    if (saveCurrentProject(false))
        kapp->quit();
}

#include "themeeditormainwindow.moc"
