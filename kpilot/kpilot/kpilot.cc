/* kpilot.cc			KPilot
**
** Copyright (C) 1998-2001 by Dan Pilone
**
** This is the main program in KPilot.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program in a file called COPYING; if not, write to
** the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, 
** MA 02139, USA.
*/

/*
** Bug reports and questions can be sent to kde-pim@kde.org
*/


static const char *kpilot_id =
	"$Id$";


#ifndef _KPILOT_OPTIONS_H
#include "options.h"
#endif

#ifndef QFILE_H
#include <qfile.h>
#endif

#include <qptrlist.h>

#ifndef QSTRING_H
#include <qstring.h>
#endif

#include <qvbox.h>
#include <qtimer.h>

#include <kjanuswidget.h>

#ifndef _KURL_H_
#include <kurl.h>
#endif
#ifndef _KMESSAGEBOX_H_
#include <kmessagebox.h>
#endif
#ifndef _KSTATUSBAR_H_
#include <kstatusbar.h>
#endif
#ifndef _KCONFIG_H_
#include <kconfig.h>
#endif
#ifndef _KWIN_H_
#include <kwin.h>
#endif
#ifndef _KCOMBOBOX_H_
#include <kcombobox.h>
#endif
#ifndef _KMENUBAR_H_
#include <kmenubar.h>
#endif
#ifndef _KSTDDIRS_H_
#include <kstddirs.h>
#endif
#ifndef _KABOUTDATA_H_
#include <kaboutdata.h>
#endif
#ifndef _KCMDLINEARGS_H_
#include <kcmdlineargs.h>
#endif
#ifndef _KICONLOADER_H_
#include <kiconloader.h>
#endif
#ifndef _KDEBUG_H_
#include <kdebug.h>
#endif
#ifndef _KACTION_H_
#include <kaction.h>
#endif
#ifndef _KSTDACTION_H_
#include <kstdaction.h>
#endif
#ifndef _KUNIQUEAPP_H_
#include <kuniqueapp.h>
#endif
#include <kkeydialog.h>
#include <kedittoolbar.h>

#include <kprogress.h>


#include "kpilotConfigDialog.h"
#include "kpilotConfig.h"

#include "pilotComponent.h"

#include "addressWidget.h"
#include "memoWidget.h"
#include "fileInstallWidget.h"
#include "logWidget.h"

#include "conduitConfigDialog.h"

#ifndef _KPILOT_PILOTDAEMON_H
#include "pilotDaemonDCOP.h"
#endif

#ifndef __PILOTDAEMONDCOP_STUB__
#include "pilotDaemonDCOP_stub.h"
#endif

#include "kpilot.moc"

class KPilotInstaller::KPilotPrivate
{
public:
	typedef QList<PilotComponent> ComponentList;

private:
	ComponentList  fPilotComponentList;

public:
	ComponentList &list() { return fPilotComponentList; } ;
} ;
	
KPilotInstaller::KPilotInstaller() :
	KMainWindow(0),
	fDaemonStub(new PilotDaemonDCOP_stub("kpilotDaemon", 
		"KPilotDaemonIface")),
	fP(new KPilotPrivate),
	fMenuBar(0L),
	fToolBar(0L),
	fQuitAfterCopyComplete(false),
	fManagingWidget(0L),
	fKillDaemonOnExit(false),
	fDaemonWasRunning(true),
	fStatus(Startup),
	fFileInstallWidget(0L), 
	fLogWidget(0L)
{
	FUNCTIONSETUP;

	readConfig();
	setupWidget();

	/* NOTREACHED */
	(void) kpilot_id;
}

KPilotInstaller::~KPilotInstaller()
{
	FUNCTIONSETUP;
	killDaemonIfNeeded();
	delete fDaemonStub;
}

void KPilotInstaller::killDaemonIfNeeded()
{
	FUNCTIONSETUP;
	if (fKillDaemonOnExit)
	{
#ifdef DEBUG
		DEBUGKPILOT << fname << ": Killing daemon." << endl;
#endif

		if (!fDaemonWasRunning)
		{
			getDaemon().quitNow();
		}
	}
}

void KPilotInstaller::startDaemonIfNeeded()
{
	FUNCTIONSETUP;

	QString daemonError;
	QCString daemonDCOP;
	int daemonPID;

	QString s = getDaemon().statusString();

#ifdef DEBUG
	DEBUGKPILOT << fname << ": Daemon status is " << s << endl;
#endif

	if ((s.isNull()) || (!getDaemon().ok()))
	{
#ifdef DEBUG
		DEBUGKPILOT << fname 
			<< ": Daemon not responding, trying to start it." 
			<< endl;
#endif
		fDaemonWasRunning = false;
	}
	else
	{
		fDaemonWasRunning = true;
	}

	if (!fDaemonWasRunning && KApplication::startServiceByDesktopName(
		"kpilotdaemon",
		QString::null, &daemonError, &daemonDCOP, &daemonPID
#if (KDE_VERSION >= 220)
			// Startup notification was added in 2.2
			, "0"
#endif
		))
	{
		kdError() << k_funcinfo << ": Can't start daemon : " << daemonError << endl;
	}
	else
	{
#ifdef DEBUG
		s = getDaemon().statusString();
		DEBUGKPILOT << fname << ": Daemon status is " << s << endl;
#endif
	}
}

void KPilotInstaller::readConfig()
{
	FUNCTIONSETUP;

	KPilotConfigSettings & c = KPilotConfig::getConfig();
	fKillDaemonOnExit = c.getKillDaemonOnExit();
}


void KPilotInstaller::setupWidget()
{
	FUNCTIONSETUP;

#ifdef DEBUG
	DEBUGKPILOT << fname << ": Creating central widget." << endl;
#endif

	setCaption("KPilot");
	setMinimumSize(500, 405);


	fManagingWidget = new KJanusWidget(this,"mainWidget",
		KJanusWidget::IconList);
	fManagingWidget->setMinimumSize(500, 330);
	fManagingWidget->show();
	setCentralWidget(fManagingWidget);

	initIcons();
	initMenu();
	initComponents();

	createGUI("kpilotui.rc", false);
#ifdef DEBUG
	DEBUGKPILOT << fname
		<< ": Got XML from "
		<< xmlFile() << " and " << localXMLFile() << endl;
#endif
}


void KPilotInstaller::initComponents()
{
	FUNCTIONSETUP;

	QString defaultDBPath = KPilotConfig::getDefaultDBPath();

#ifdef DEBUG
	DEBUGKPILOT << fname << ": Creating component pages." << endl;
#endif

	QString pixfile;
	QWidget *w;

#define VIEWICON(a) KGlobal::dirs()->findResource("data",(a))

#define ADDICONPAGE(a,b) pixfile = VIEWICON(b); \
	w = getManagingWidget()->addVBoxPage(a,QString::null, \
		(pixfile.isEmpty() ? QPixmap() : QPixmap(pixfile))) ;

	ADDICONPAGE(i18n("HotSync"),"kpilot/kpilot-hotsync.png");
	fLogWidget = new LogWidget(w);
	addComponentPage(fLogWidget, i18n("HotSync"));
	fLogWidget->setShowTime(true);

	
	ADDICONPAGE(i18n("Memo Viewer"),"kpilot/kpilot-knotes.png");
	addComponentPage(new MemoWidget(w, defaultDBPath),
		i18n("Memo Viewer"));

	ADDICONPAGE(i18n("Address Viewer"),"kpilot/kpilot-address.png");
	addComponentPage(new AddressWidget(w,defaultDBPath),
		i18n("Address Viewer"));

	ADDICONPAGE(i18n("File Installer"),"kpilot/kpilot-fileinstaller.png");
	fFileInstallWidget = new FileInstallWidget(
		w,defaultDBPath);
	addComponentPage(fFileInstallWidget, i18n("File Installer"));

#undef ADDICONPAGE
#undef VIEWICON

	QTimer::singleShot(500,this,SLOT(initializeComponents()));
}



void KPilotInstaller::initIcons()
{
	FUNCTIONSETUP;

}



void KPilotInstaller::slotSelectComponent(PilotComponent * c)
{
	FUNCTIONSETUP;

	QWidget *p = static_cast <QWidget *>(c);
	if (!p) { kdWarning() << k_funcinfo << ": Not a widget." << endl; return;}

	QObject *o = p->parent();
	if (!o) { kdWarning() << k_funcinfo << ": No parent." << endl; return; }

	QWidget *parent = dynamic_cast<QWidget *>(o);
	if (!parent) { kdWarning() << k_funcinfo << ": No widget parent." << endl; return; }

	int index = fManagingWidget->pageIndex(parent);

	if (index < 0)
	{
		kdWarning() << k_funcinfo << ": Index " << index << endl;
		return;
	}

	fManagingWidget->showPage(index);
}




void KPilotInstaller::slotBackupRequested()
{
	FUNCTIONSETUP;
	setupSync(PilotDaemonDCOP::Backup,
		i18n("Backing up Pilot. ") +
		i18n("Please press the HotSync button."));
}

void KPilotInstaller::slotRestoreRequested()
{
	FUNCTIONSETUP;
	setupSync(PilotDaemonDCOP::Restore,
		i18n("Restoring Pilot. ") +
		i18n("Please press the HotSync button."));
}

void KPilotInstaller::slotHotSyncRequested()
{
	FUNCTIONSETUP;
	setupSync(PilotDaemonDCOP::HotSync,
		i18n("HotSyncing. ") +
		i18n("Please press the HotSync button."));
}

void KPilotInstaller::slotFastSyncRequested()
{
	FUNCTIONSETUP;
	setupSync(PilotDaemonDCOP::FastSync,
		i18n("FastSyncing. ") +
		i18n("Please press the HotSync button."));
}

bool KPilotInstaller::componentPreSync()
{
	FUNCTIONSETUP;

	QString reason;
	QString rprefix(i18n("Can't start a Sync now. %1"));

	for (fP->list().first();
		fP->list().current(); fP->list().next())
	{
#ifdef DEBUG
		DEBUGKPILOT << fname
			<< ": Pre-sync for builtin "
			<< fP->list().current()->name() << endl;
#endif
		if (!fP->list().current()->preHotSync(reason))
			break;
	}

	if (!reason.isNull())
	{
		KMessageBox::sorry(this,
			rprefix.arg(reason),
			i18n("Can't start Sync"));
		return false;
	}
	return true;
}

void KPilotInstaller::setupSync(int kind, const QString & message)
{
	FUNCTIONSETUP;

	if (!componentPreSync())
	{
		return;
	}
	if (!message.isEmpty())
	{
		QString m(message);
		if (fLogWidget)
		{
			fLogWidget->logMessage(m);
		}
	}
	getDaemon().requestSync(kind);
}


void KPilotInstaller::closeEvent(QCloseEvent * e)
{
	FUNCTIONSETUP;

	quit();
	e->accept();
}

void KPilotInstaller::initMenu()
{
	FUNCTIONSETUP;

	KAction *p;

	// File actions
	p = new KAction(i18n("&HotSync"), "hotsync", 0,
		this, SLOT(slotHotSyncRequested()),
		actionCollection(), "file_hotsync");
	p = new KAction(i18n("&FastSync"), "fastsync", 0,
		this, SLOT(slotHotSyncRequested()),
		actionCollection(), "file_fastsync");
	p = new KAction(i18n("&Backup"), "backup", 0,
		this, SLOT(slotBackupRequested()),
		actionCollection(), "file_backup");
	p = new KAction(i18n("&Restore"), "restore", 0,
		this, SLOT(slotRestoreRequested()),
		actionCollection(), "file_restore");
	p = KStdAction::quit(this, SLOT(quit()), actionCollection());

	// View actions

	// Options actions
	m_toolbarAction =
		KStdAction::showToolbar(this, SLOT(optionsShowToolbar()),
		actionCollection());
	p = KStdAction::keyBindings(this, SLOT(optionsConfigureKeys()),
		actionCollection());
	p = KStdAction::configureToolbars(this, SLOT(optionsConfigureKeys()),
		actionCollection());
	p = KStdAction::preferences(this, SLOT(slotConfigureKPilot()),
		actionCollection());
	p = new KAction(i18n("C&onfigure Conduits..."), "configure", 0, this,
		SLOT(slotConfigureConduits()), actionCollection(),
		"options_configure_conduits");
}

void KPilotInstaller::fileInstalled(int)
{
	FUNCTIONSETUP;
}

void KPilotInstaller::quit()
{
	FUNCTIONSETUP;

	killDaemonIfNeeded();
	kapp->quit();
}

void KPilotInstaller::addComponentPage(PilotComponent * p,
	const QString & name)
{
	FUNCTIONSETUP;

	if (!p)
	{
		kdWarning() << k_funcinfo
			<< ": Adding NULL component?" << endl;
		return;
	}

#ifdef DEBUG
	DEBUGKPILOT << fname
		<< ": Adding component @"
		<< (int) p << " called " << p->name("(none)") << endl;
#endif

	fP->list().append(p);

	// The first component added gets id 1, while the title
	// screen -- added elsewhere -- has id 0.
	//
	// fManagingWidget->addWidget(p, fP->list().count());


	const char *componentname = p->name("(none)");
	char *actionname = 0L;

	if (strncmp(componentname, "component_", 10) == 0)
	{
		actionname = new char[strlen(componentname) - 10 + 8];

		strcpy(actionname, "view_");
		strcat(actionname, componentname + 10);
	}
	else
	{
		actionname = new char[8 + strlen(componentname)];

		strcpy(actionname, "view_");
		strcat(actionname, componentname);
	}

#ifdef DEBUG
	DEBUGKPILOT << fname
		<< ": Using component action name "
		<< name << " for " << actionname << endl;
#endif

	KToggleAction *pt =
		new KToggleAction(name, /* "kpilot" -- component icon, */ 0,
		p, SLOT(slotShowComponent()),
		actionCollection(), actionname);

	pt->setExclusiveGroup("view_menu");

	connect(p, SIGNAL(showComponent(PilotComponent *)),
		this, SLOT(slotSelectComponent(PilotComponent *)));
}

/* slot */ void KPilotInstaller::initializeComponents()
{
	FUNCTIONSETUP;
	
	for (PilotComponent *p = fP->list().first();
		p ; p = fP->list().next())
	{
		p->initialize();
	}
}


void KPilotInstaller::optionsShowToolbar()
{
	FUNCTIONSETUP;
	if (m_toolbarAction->isChecked())
	{
		toolBar()->show();
	}
	else
	{
		toolBar()->hide();
	}

	kapp->processEvents();
	resizeEvent(0);
}


void KPilotInstaller::optionsConfigureKeys()
{
	FUNCTIONSETUP;
	KKeyDialog::configureKeys(actionCollection(), xmlFile());
}

void KPilotInstaller::optionsConfigureToolbars()
{
	FUNCTIONSETUP;
	// use the standard toolbar editor
	KEditToolbar dlg(actionCollection());

	if (dlg.exec())
	{
		// recreate our GUI
		createGUI();
	}
}


void KPilotInstaller::slotConfigureKPilot()
{
	FUNCTIONSETUP;

	// Display the (modal) options page.
	//
	//
	int rememberedSync = getDaemon().nextSyncType();
	getDaemon().requestSync(0);

	KPilotConfig::getConfig().reparseConfiguration();
	KPilotConfigDialog *options = new KPilotConfigDialog(this,
		"configDialog", true);

	if (!options)
	{
		kdError() << k_funcinfo
			<< ": Can't allocate KPilotOptions object" << endl;
		getDaemon().requestSync(rememberedSync);
		return;
	}

	options->exec();

	if (options->result())
	{
#ifdef DEBUG
		DEBUGKPILOT << fname << ": Updating link." << endl;
#endif

		KPilotConfig::getConfig().sync();
		readConfig();

		// Update the daemon to reflect new settings.
		//
		//
		getDaemon().reloadSettings();

		// Update each installed component.
		//
		//
		for (fP->list().first();
			fP->list().current();
			fP->list().next())
		{
			fP->list().current()->initialize();
		}
	}

	KPILOT_DELETE(options);
	getDaemon().requestSync(rememberedSync);

#ifdef DEBUG
	DEBUGKPILOT << fname << ": Done with options." << endl;
#endif
}

void KPilotInstaller::slotConfigureConduits()
{
	FUNCTIONSETUP;

	ConduitConfigDialog *conSetup = 0L;

	conSetup = new ConduitConfigDialog(this,0L,true);
	int r = conSetup->exec();
	if (r)
	{
		KPilotConfig::getConfig().sync();
		getDaemon().reloadSettings();
	}
	delete conSetup;
}


/* static */ const char *KPilotInstaller::version(int kind)
{
	FUNCTIONSETUP;
	// I don't think the program title needs to be translated. (ADE)
	//
	//
	if (kind)
	{
		return ::kpilot_id;
	}
	else
	{
		return "KPilot v" KPILOT_VERSION;
	}
}

// Command line options descriptions.
//
//
//
//
static KCmdLineOptions kpilotoptions[] = {
	{"s", 0, 0},
	{"setup",
		I18N_NOOP("Setup the Pilot device and other parameters"),
		0L},
	{"c", 0, 0},
	{"conduit-setup", I18N_NOOP("Run conduit setup"), 0L},
	{0, 0, 0}
};




// "Regular" mode == 0
// setup mode == 's'
// setup forced by config change == 'S'
// conduit setup == 'c'
//
// This is only changed by the --setup flag --
// kpilot still does a setup the first time it is run.
//
//
int run_mode = 0;


int main(int argc, char **argv)
{
	FUNCTIONSETUP;

	KAboutData about("kpilot", I18N_NOOP("KPilot"),
		KPILOT_VERSION,
		"KPilot - Hot-sync software for unix\n\n",
		KAboutData::License_GPL, "(c) 1998-2000,2001, Dan Pilone");
	about.addAuthor("Dan Pilone",
		I18N_NOOP("Project Leader"),
		"pilone@slac.com", "http://www.slac.com/pilone/kpilot_home/");
	about.addAuthor("Adriaan de Groot",
		I18N_NOOP("Maintainer"),
		"groot@kde.org", "http://www.cs.kun.nl/~adridg/kpilot/");
	about.addCredit("Preston Brown", I18N_NOOP("VCal conduit"));
	about.addCredit("Greg Stern", I18N_NOOP("Abbrowser conduit"));
	about.addCredit("Chris Molnar", I18N_NOOP("Expenses conduit"));
	about.addCredit("Heiko Purnhagen", I18N_NOOP("Bugfixer"));
	about.addCredit("Joerg Habenicht", I18N_NOOP("Bugfixer"));
	about.addCredit("Martin Junius",
		I18N_NOOP("XML GUI"),
		"mj@m-j-s.net", "http://www.m-j-s.net/kde/");
	about.addCredit("David Bishop",
		I18N_NOOP(".ui files"));
	about.addCredit("Aaron J. Seigo",
		I18N_NOOP("Bugfixer, coolness"));
	about.addCredit("Reinhold Kainhofer",
		I18N_NOOP("vCal and Addressbook conduit ports, time conduit, AvantGo conduit"), "reinhold@kainhofer.com", "http://reinhold.kainhofer.com/");


	KCmdLineArgs::init(argc, argv, &about);
#ifdef DEBUG
	KCmdLineArgs::addCmdLineOptions(debug_options, "debug", "debug");
#endif
	KCmdLineArgs::addCmdLineOptions(kpilotoptions, "kpilot", 0L, "debug");
	KUniqueApplication::addCmdLineOptions();
	KCmdLineArgs *p = KCmdLineArgs::parsedArgs();

	if (p->isSet("setup"))
	{
		run_mode = 's';
	}
	if (p->isSet("conduit-setup"))
	{
		run_mode = 'c';
	}

	if (!KUniqueApplication::start())
	{
		return 0;
	}
	KUniqueApplication a(true, true);

	KPilotConfig::getDebugLevel();

	KPilotConfigSettings & c = KPilotConfig::getConfig();
	if (c.getVersion() < KPilotConfig::ConfigurationVersion)
	{
		run_mode = 'S';
	}

	if (run_mode == 'c')
	{
		ConduitConfigDialog *cs = new ConduitConfigDialog(0L,0L,true);
		int r = cs->exec();

		if (r)
		{
			return 1;	// Dialog cancelled
		}
		else
		{
			return 0;
		}
	}

	if ((run_mode == 's') || (run_mode == 'S'))
	{
#ifdef DEBUG
		DEBUGKPILOT << fname
			<< ": Running setup first."
			<< " (mode " << run_mode << ")" << endl;
#endif

		KPilotConfigDialog *options = new KPilotConfigDialog(0L,
			"configDialog", true);
		int r = options->exec();

		if (run_mode == 's')
		{
			if (!r)
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}

		if (!r)
			return 1;


		// The options dialog may have changed the group
		// while reading or writing settings (still a
		// bad idea, actually).
		//
		c.resetGroup();
	}

	if (c.getVersion() < KPilotConfig::ConfigurationVersion)
	{
		kdWarning() << k_funcinfo <<
			": Is still not configured for use." << endl;
		return 1;
	}


	KPilotInstaller *tp = new KPilotInstaller();

	if (tp->status() == KPilotInstaller::Error)
	{
		delete tp;

		tp = 0;
		return 1;
	}

	tp->startDaemonIfNeeded();

	KGlobal::dirs()->addResourceType("pilotdbs",
		"share/apps/kpilot/DBBackup");
	tp->show();
	a.setMainWidget(tp);
	return a.exec();
}


//
