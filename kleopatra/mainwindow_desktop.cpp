/* -*- mode: c++; c-basic-offset:4 -*-
    mainwindow_desktop.cpp

    This file is part of Kleopatra, the KDE keymanager
    Copyright (c) 2007 Klarälvdalens Datakonsult AB

    Kleopatra is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include <config-kleopatra.h>

#include "mainwindow_desktop.h"
#include "aboutdata.h"

#include "models/keylistmodel.h"
#include "models/keylistsortfilterproxymodel.h"

#include "view/searchbar.h"
#include "view/tabwidget.h"
#include "view/keylistcontroller.h"

#include "commands/selftestcommand.h"
#include "commands/importcrlcommand.h"
#include "commands/importcertificatefromfilecommand.h"
#include "commands/decryptverifyfilescommand.h"
#include "commands/signencryptfilescommand.h"

#include "utils/detail_p.h"
#include "utils/gnupg-helper.h"
#include "utils/action_data.h"
#include "utils/classify.h"
#include "utils/filedialog.h"

// from libkdepim
#include "statusbarprogresswidget.h"
#include "progressdialog.h"

#include <KActionCollection>
#include <KLocale>
#include <KTabWidget>
#include <KStatusBar>
#include <KStandardAction>
#include <KAction>
#include <KAboutData>
#include <KMessageBox>
#include <KStandardGuiItem>
#include <KStandardDirs>
#include <KShortcutsDialog>
#include <KXMLGUIFactory>
#include <KEditToolBar>
#include <KAboutApplicationDialog>
#include <kdebug.h>

#include <QAbstractItemView>
#include <QFile>
#include <QToolBar>
#include <QWidgetAction>
#include <QApplication>
#include <QCloseEvent>
#include <QMenu>
#include <QTimer>
#include <QProcess>
#include <QPointer>

#include <kleo/cryptobackendfactory.h>
#include <ui/cryptoconfigdialog.h>
#include <kleo/cryptoconfig.h>
#include <kleo/stl_util.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <vector>

#ifdef Q_OS_WIN32
static const bool OS_WIN = true;
#else
static const bool OS_WIN = false;
#endif

using namespace Kleo;
using namespace Kleo::Commands;
using namespace boost;
using namespace GpgME;

namespace {

    static const KAboutData * aboutGpg4WinData() {
        static const AboutGpg4WinData data;
        return &data;
    }

}

static KGuiItem KStandardGuiItem_quit() {
    static const QString app = KGlobal::mainComponent().aboutData()->programName();
    KGuiItem item = KStandardGuiItem::quit();
    item.setText( i18nc( "Quit [ApplicationName]", "&Quit %1", app ) );
    return item;
}

static KGuiItem KStandardGuiItem_close() {
    KGuiItem item = KStandardGuiItem::close();
    item.setText( i18n("Only &Close Window" ) );
    return item;
}

class MainWindow::Private {
    friend class ::MainWindow;
    MainWindow * const q;

public:
    explicit Private( MainWindow * qq );
    ~Private();

    template <typename T>
    void createAndStart() {
        ( new T( this->currentView(), &this->controller ) )->start();
    }
    template <typename T>
    void createAndStart( QAbstractItemView * view ) {
        ( new T( view, &this->controller ) )->start();
    }
    template <typename T>
    void createAndStart( const QStringList & a ) {
        ( new T( a, this->currentView(), &this->controller ) )->start();
    }
    template <typename T>
    void createAndStart( const QStringList & a, QAbstractItemView * view ) {
        ( new T( a, view, &this->controller ) )->start();
    }

    void closeAndQuit() {
        const QString app = KGlobal::mainComponent().aboutData()->programName();
        const int rc = KMessageBox::questionYesNoCancel( q,
                                                         i18n("%1 may be used by other applications as a service.\n"
                                                              "You may instead want to close this window without exiting %1.", app ),
                                                         i18n("Really Quit?"), KStandardGuiItem_close(), KStandardGuiItem_quit(), KStandardGuiItem::cancel(),
                                                         "really-quit-" + app.toLower() );
        if ( rc == KMessageBox::Cancel )
            return;
        if ( !q->close() )
            return;
        // WARNING: 'this' might be deleted at this point!
        if ( rc == KMessageBox::No )
            qApp->quit();
    }
    void configureToolbars() {
        KEditToolBar dlg( q->factory() );
        dlg.exec();
    }
    void editKeybindings() {
        KShortcutsDialog::configure( q->actionCollection(), KShortcutsEditor::LetterShortcutsAllowed );
    }

    void selfTest() {
        createAndStart<SelfTestCommand>();
    }
    void configureBackend();

    void showHandbook();

    void gnupgLogViewer() {
        if( !QProcess::startDetached("kwatchgnupg" ) )
            KMessageBox::error( q, i18n( "Could not start the GnuPG Log Viewer (kwatchgnupg). "
                                         "Please check your installation." ),
                                i18n( "Error Starting KWatchGnuPG" ) );
    }

    void gnupgAdministrativeConsole() {
        if( !QProcess::startDetached("kgpgconf" ) )
            KMessageBox::error( q, i18n( "Could not start the GnuPG Administrative Console (kgpgconf). "
                                         "Please check your installation." ),
                                i18n( "Error Starting KGpgConf" ) );
    }

    void slotConfigCommitted();
    void slotContextMenuRequested( QAbstractItemView *, const QPoint & p ) {
        if ( QMenu * const menu = qobject_cast<QMenu*>( q->factory()->container( "listview_popup", q ) ) )
            menu->exec( p );
        else
            kDebug() << "no \"listview_popup\" <Menu> in kleopatra's ui.rc file";
    }

    void aboutGpg4Win() {
        ( new KAboutApplicationDialog( aboutGpg4WinData(), KAboutApplicationDialog::HideKdeVersion|KAboutApplicationDialog::HideTranslators, q ) )->show();
    }

private:
    void setupActions();

    QAbstractItemView * currentView() const {
        return ui.tabWidget.currentView();
    }

private:
    Kleo::KeyListController controller;
    bool firstShow : 1;

    struct UI {

        TabWidget tabWidget;

	explicit UI( MainWindow * q )
	    : tabWidget( q )
	{
	    KDAB_SET_OBJECT_NAME( tabWidget );

	    q->setCentralWidget(&tabWidget);
	    KPIM::ProgressDialog * progressDialog = new KPIM::ProgressDialog( q->statusBar(), q );
	    KDAB_SET_OBJECT_NAME( progressDialog );
	    progressDialog->hide();
	    KPIM::StatusbarProgressWidget * statusBarProgressWidget 
		    = new KPIM::StatusbarProgressWidget( progressDialog, q->statusBar() );
	    KDAB_SET_OBJECT_NAME( statusBarProgressWidget );
	    q->statusBar()->addPermanentWidget( statusBarProgressWidget, 0 );
	    statusBarProgressWidget->show();
	}
    } ui;
};

MainWindow::Private::Private( MainWindow * qq )
    : q( qq ),
      controller( q ),
      firstShow( true ),
      ui( q )
{
    KDAB_SET_OBJECT_NAME( controller );
    
    AbstractKeyListModel * flatModel = AbstractKeyListModel::createFlatKeyListModel( q );
    AbstractKeyListModel * hierarchicalModel = AbstractKeyListModel::createHierarchicalKeyListModel( q );

    KDAB_SET_OBJECT_NAME( flatModel );
    KDAB_SET_OBJECT_NAME( hierarchicalModel );


    controller.setFlatModel( flatModel );
    controller.setHierarchicalModel( hierarchicalModel );
    controller.setTabWidget( &ui.tabWidget );

    ui.tabWidget.setFlatModel( flatModel );
    ui.tabWidget.setHierarchicalModel( hierarchicalModel );

    setupActions();

    connect( &controller, SIGNAL(message(QString,int)),  q->statusBar(), SLOT(showMessage(QString,int)) );
    connect( &controller, SIGNAL(contextMenuRequested(QAbstractItemView*,QPoint)),
             q, SLOT(slotContextMenuRequested(QAbstractItemView*,QPoint)) );

    q->createGUI( "kleopatra.rc" );

    q->setAcceptDrops( true );

    q->setAutoSaveSettings();
}

MainWindow::Private::~Private() {}

MainWindow::MainWindow( QWidget* parent, Qt::WindowFlags flags )
    : KXmlGuiWindow( parent, flags ), d( new Private( this ) )
{
}

MainWindow::~MainWindow() {}


void MainWindow::Private::setupActions() {

    KActionCollection * const coll = q->actionCollection();

    QWidgetAction * const searchBarAction = new QWidgetAction( q );
    SearchBar * const searchBar = new SearchBar( q );

    ui.tabWidget.connectSearchBar( searchBar );

    searchBarAction->setDefaultWidget( searchBar );
    coll->addAction( "key_search_bar", searchBarAction );

    const action_data action_data[] = {
        // most have been MOVED TO keylistcontroller.cpp
        // Tools menu
#ifndef Q_OS_WIN
        { "tools_start_kwatchgnupg", i18n("GnuPG Log Viewer"), QString(),
          "kwatchgnupg", q, SLOT(gnupgLogViewer()), QString(), false, true },
#endif
#if 0
        { "tools_start_kgpgconf", i18n("GnuPG Administrative Console"), QString(),
          "kgpgconf", q, SLOT(gnupgLogViewer()), QString(), false, true },
#endif
        // most have been MOVED TO keylistcontroller.cpp
#if 0
        { "configure_backend", i18n("Configure GnuPG Backend..."), QString(),
          0, q, SLOT(configureBackend()), QString(), false, true },
#endif
        // Settings menu
        { "settings_self_test", i18n("Perform Self-Test"), QString(),
          0, q, SLOT(selfTest()), QString(), false, true },
        // Help menu
#ifdef Q_WS_WIN
        { "help_about_gpg4win", i18n("About Gpg4win"), QString(),
          "gpg4win-compact", q, SLOT(aboutGpg4Win()), QString(), false, true },
#endif
        // most have been MOVED TO keylistcontroller.cpp
    };

    make_actions_from_data( action_data, /*sizeof action_data / sizeof *action_data,*/ coll );

    if ( QAction * action = coll->action( "configure_backend" ) )
        action->setMenuRole( QAction::NoRole ); //prevent Qt OS X heuristics for config* actions

    KStandardAction::close( q, SLOT(close()), coll );
    KStandardAction::quit( q, SLOT(closeAndQuit()), coll );
    KStandardAction::configureToolbars( q, SLOT(configureToolbars()), coll );
    KStandardAction::keyBindings( q, SLOT(editKeybindings()), coll );
    KStandardAction::preferences( qApp, SLOT(openOrRaiseConfigDialog()), coll );

    q->createStandardStatusBarAction();
    q->setStandardToolBarMenuEnabled( true );

    controller.createActions( coll );

    ui.tabWidget.createActions( coll );
}

void MainWindow::Private::configureBackend() {
    Kleo::CryptoConfig * const config = Kleo::CryptoBackendFactory::instance()->config();
    if ( !config ) {
    	KMessageBox::error( q, i18n( "Could not configure the cryptography backend (gpgconf tool not found)" ), i18n( "Configuration Error" ) );
    	return;
    }

    Kleo::CryptoConfigDialog dlg( config );

    const int result = dlg.exec();

    // Forget all data parsed from gpgconf, so that we show updated information
    // when reopening the configuration dialog.
    config->clear();

    if ( result == QDialog::Accepted ) {
#if 0
        // Tell other apps (e.g. kmail) that the gpgconf data might have changed
        QDBusMessage message =
            QDBusMessage::createSignal(QString(), "org.kde.kleo.CryptoConfig", "changed");
        QDBusConnection::sessionBus().send(message);
#endif
    }
}

void MainWindow::Private::slotConfigCommitted() {
    controller.updateConfig();
}

void MainWindow::closeEvent( QCloseEvent * e ) {
    // KMainWindow::closeEvent() insists on quitting the application,
    // so do not let it touch the event...
    kDebug();
    if ( d->controller.hasRunningCommands() ) {
        if ( d->controller.shutdownWarningRequired() ) {
            const int ret = KMessageBox::warningContinueCancel( this, i18n("There are still some background operations ongoing. "
                                                                           "These will be terminated when closing the window. "
                                                                           "Proceed?"),
                                                                i18n("Ongoing Background Tasks") );
            if ( ret != KMessageBox::Continue ) {
                e->ignore();
                return;
            }
        }
        d->controller.cancelCommands();
        if ( d->controller.hasRunningCommands() ) {
            // wait for them to be finished:
            setEnabled( false );
            QEventLoop ev;
            QTimer::singleShot( 100, &ev, SLOT(quit()) );
            connect( &d->controller, SIGNAL(commandsExecuting(bool)), &ev, SLOT(quit()) );
            ev.exec();
            kWarning( d->controller.hasRunningCommands() )
                << "controller still has commands running, this may crash now...";
            setEnabled( true );
        }
    }
    d->ui.tabWidget.saveViews( KGlobal::config().data() );
    saveMainWindowSettings( KConfigGroup( KGlobal::config(), autoSaveGroup() ) );
    e->accept();
}

void MainWindow::showEvent( QShowEvent * e ) {
    KXmlGuiWindow::showEvent( e );
    if ( d->firstShow ) {
        d->ui.tabWidget.loadViews( KGlobal::config().data() );
        d->firstShow = false;
    }
}

void MainWindow::importCertificatesFromFile( const QStringList & files ) {
    if ( !files.empty() )
        d->createAndStart<ImportCertificateFromFileCommand>( files );
}


static QStringList extract_local_files( const QMimeData * data ) {
    const QList<QUrl> urls = data->urls();
    // begin workaround KDE/Qt misinterpretation of text/uri-list
    QList<QUrl>::const_iterator end = urls.end();
    if ( urls.size() > 1 && !urls.back().isValid() )
        --end;
    // end workaround
    QStringList result;
    std::transform( urls.begin(), end,
                    std::back_inserter( result ),
                    bind( &QUrl::toLocalFile, _1 ) );
    result.erase( std::remove_if( result.begin(), result.end(),
                                  bind( &QString::isEmpty, _1 ) ), result.end() );
    return result;
}

static bool can_decode_local_files( const QMimeData * data ) {
    if ( !data )
        return false;
    return !extract_local_files( data ).empty();
}

void MainWindow::dragEnterEvent( QDragEnterEvent * e ) {
    kDebug();

    if ( can_decode_local_files( e->mimeData() ) )
        e->acceptProposedAction();
}

void MainWindow::dropEvent( QDropEvent * e ) {
    kDebug();

    if ( !can_decode_local_files( e->mimeData() ) )
        return;

    e->setDropAction( Qt::CopyAction );

    const QStringList files = extract_local_files( e->mimeData() );

    const unsigned int classification = classify( files );

    QMenu menu;

    QAction * const signEncrypt = menu.addAction( i18n("Sign/Encrypt...") );
    QAction * const decryptVerify = mayBeAnyMessageType( classification ) ? menu.addAction( i18n("Decrypt/Verify...") ) : 0 ;
    if ( signEncrypt || decryptVerify )
        menu.addSeparator();

    QAction * const importCerts = mayBeAnyCertStoreType( classification ) ? menu.addAction( i18n("Import Certificates") ) : 0 ;
    QAction * const importCRLs  = mayBeCertificateRevocationList( classification ) ? menu.addAction( i18n("Import CRLs") ) : 0 ;
    if ( importCerts || importCRLs )
        menu.addSeparator();

    if ( !signEncrypt && !decryptVerify && !importCerts && !importCRLs )
        return;

    menu.addAction( i18n("Cancel") );

    const QAction * const chosen = menu.exec( mapToGlobal( e->pos() ) );

    if ( !chosen )
        return;

    if ( chosen == signEncrypt )
        d->createAndStart<SignEncryptFilesCommand>( files );
    else if ( chosen == decryptVerify )
        d->createAndStart<DecryptVerifyFilesCommand>( files );
    else if ( chosen == importCerts )
        d->createAndStart<ImportCertificateFromFileCommand>( files );
    else if ( chosen == importCRLs )
        d->createAndStart<ImportCrlCommand>( files );

    e->accept();
}

#include "moc_mainwindow_desktop.cpp"
