/*
� � � � � � � �=.            This file is part of the OPIE Project
� � � � � � �.=l.            Copyright (c)  2002 Holger Freyther <zecke@handhelds.org>
� � � � � �.>+-=                            2002 Maximilian Rei� <harlekin@handhelds.org>
�_;:, � � .> � �:=|.         This library is free software; you can
.> <`_, � > �. � <=          redistribute it and/or  modify it under
:`=1 )Y*s>-.-- � :           the terms of the GNU Library General Public
.="- .-=="i, � � .._         License as published by the Free Software
�- . � .-<_> � � .<>         Foundation; either version 2 of the License,
� � �._= =} � � � :          or (at your option) any later version.
� � .%`+i> � � � _;_.
� � .i_,=:_. � � �-<s.       This library is distributed in the hope that
� � �+ �. �-:. � � � =       it will be useful,  but WITHOUT ANY WARRANTY;
� � : .. � �.:, � � . . .    without even the implied warranty of
� � =_ � � � �+ � � =;=|`    MERCHANTABILITY or FITNESS FOR A
� _.=:. � � � : � �:=>`:     PARTICULAR PURPOSE. See the GNU
..}^=.= � � � = � � � ;      Library General Public License for more
++= � -. � � .` � � .:       details.
�: � � = �...= . :.=-
�-. � .:....=;==+<;          You should have received a copy of the GNU
� -_. . . � )=. �=           Library General Public License along with
� � -- � � � �:-=`           this library; see the file COPYING.LIB.
                             If not, write to the Free Software Foundation,
                             Inc., 59 Temple Place - Suite 330,
                             Boston, MA 02111-1307, USA.

*/


#include <qvbox.h>
#include <qwidgetstack.h>
#include <qsize.h>

#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kdebug.h>
#include <ktrader.h>
#include <kstatusbar.h>

#include <kparts/componentfactory.h>
#include <kpopupmenu.h>

#include <kapabilities.h>
#include <kdevice.h>
#include <konnector.h>

#include "ksync_configpart.h"
#include "ksync_configuredialog.h"
#include "manipulatorpart.h"
#include "partbar.h"

#include "ksync_mainwindow.h"


using namespace KitchenSync;

KSyncMainWindow::KSyncMainWindow(QWidget *widget, const char *name, WFlags f)
  :
  KParts::MainWindow( widget, name, f ){

  m_konnector = 0;
  initActions();
  setXMLFile("ksyncgui.rc");
  setInstance( KGlobal::instance() );

  createGUI( 0l );
  // now add a layout or QWidget?
  m_lay = new QHBox(this,   "main widget" );
  setCentralWidget( m_lay );

  m_bar = new PartBar(m_lay , "partBar" );
  m_stack = new QWidgetStack( m_lay, "dummy" );
  QWidget *test = new QWidget(m_stack);
  test->setBackgroundColor(Qt::red);
  m_stack->addWidget(test, 0);
  m_stack->raiseWidget(0);
  m_bar->setMaximumWidth(100 );
  m_bar->setMinimumWidth(100 );

  connect( m_bar, SIGNAL(activated(ManipulatorPart*) ), this, SLOT(slotActivated(ManipulatorPart*) ));

  resize(600,400);
  m_parts.setAutoDelete( true );

  kdDebug() << "Init konnector" << endl;
  initKonnector();
  initPlugins();

  //statusBar()->insertItem(i18n("Not Connected"), 10, 0, true );
  statusBar()->message(i18n("Not connected") );
  statusBar()->show();

  // show systemtraypart
  initSystray();
  m_tray->show();

};

KSyncMainWindow::~KSyncMainWindow()
{
    createGUI(0l );
}
void KSyncMainWindow::initActions()
{
  (void)new KAction( i18n("Synchronize" ), "reload", 0, this, SLOT( slotSync() ),
		     actionCollection(), "sync" );
  (void)new KAction( i18n("Backup") ,  "mail_get", 0, this, SLOT( slotBackup() ),
		     actionCollection(), "backup" );
  (void)new KAction( i18n("Restore"),  "mail_send", 0, this, SLOT (slotRestore() ),
		     actionCollection(), "restore" );
  (void)new KAction( i18n("Quit"),  "exit", 0, this, SLOT(slotQuit()),
		     actionCollection(), "quit" );
  (void)new KAction( i18n("Configure Kitchensync"), "configure" , 0, this,
		     SLOT (slotConfigure() ), actionCollection(), "configure" );
}
void KSyncMainWindow::initPlugins()
{
  KTrader::OfferList offers = KTrader::self()->query(QString::fromLatin1("KitchenSync/Manipulator"),
						     QString::null);

  for (KTrader::OfferList::ConstIterator it = offers.begin(); it != offers.end(); ++it){
      ManipulatorPart *plugin = KParts::ComponentFactory
	::createInstanceFromService<ManipulatorPart>(*it, this);
      if (!plugin)
	continue;
      addModPart( plugin );
  }
  /*
  OrganizerPart *org = new OrganizerPart(this, "wallah" );

  OverviewPart *view = new OverviewPart(this, "hallaw" );
  addModPart(view);
  addModPart(org);
  */
}

void KSyncMainWindow::addModPart(ManipulatorPart *part)
{
  static int id=1;
  //m_parts.clear();
  // diable it for testing
  if( part->partIsVisible() )
  {
    int pos = -1;
    kdDebug() << "before part insert \n"  ;
    m_stack->addWidget( part->widget(), id );
    if( part->type() == QString::fromLatin1("Overview") ){ // Overview is special for us ;)
      m_stack->raiseWidget(id );
    }

    m_bar->insertItem( part, pos );
  }
  m_parts.append( part );

  id++;
}

void KSyncMainWindow::initSystray( void ) {

    m_tray = new KSyncSystemTray( this, "KSyncSystemTray");
    KPopupMenu *popMenu = m_tray->getContextMenu();
    popMenu->insertSeparator();

}

void KSyncMainWindow::slotSync() {
}

void KSyncMainWindow::slotBackup() {
}

void KSyncMainWindow::slotRestore() {
}

void KSyncMainWindow::slotConfigure() {
  ConfigureDialog dlg(this);
  ManipulatorPart *part;
  Kapabilities cap;
  cap.setExtraOption("test",  "test123");
  ConfigPart par( cap, &dlg );
  for (part = m_parts.first(); part != 0; part = m_parts.next() ) {
    if( part->configIsVisible() )
      dlg.addWidget(part->configWidget(), part->name(), part->pixmap() );
  }
  dlg.addWidget( &par, "Test", &QPixmap() );
  if (dlg.exec()) {
     for (part = m_parts.first(); part != 0; part = m_parts.next() ) {
       part->slotConfigOk();
     }
  }
}

void KSyncMainWindow::slotActivated(ManipulatorPart *part) {
  m_stack->raiseWidget(part->widget() );
  createGUI( part );
}

void KSyncMainWindow::slotQuit() {
  close();
}
KSyncSystemTray* KSyncMainWindow::tray()
{
    return m_tray;
}
Konnector* KSyncMainWindow::konnector()
{
    return m_konnector;
}
QString KSyncMainWindow::currentId() const
{
    return m_currentId;
}
QMap<QString,QString> KSyncMainWindow::ids() const
{
    return m_ids;
}
void KSyncMainWindow::initKonnector()
{
    kdDebug() << "init konnector" << endl;
    m_konnector = new Konnector(this,  "Konnector");
    connect(m_konnector,SIGNAL(wantsToSync(const QString&, QPtrList<KSyncEntry> ) ),
            this, SLOT(slotSync( const QString&,  QPtrList<KSyncEntry>) ) );

    connect(m_konnector, SIGNAL(stateChanged(const QString&,  bool) ),
            this,  SLOT(slotStateChanged(const QString&,  bool) ) );

    connect(m_konnector, SIGNAL(konnectorError(const QString&,  int,  const QString& ) ),
            this,  SLOT(slotKonnectorError( const QString&,  int, const QString&) ) );
    // ok now just load the Opie Konnector // FIXME Don't hard code
    QValueList<KDevice> device;
    device = m_konnector->query();
    for(QValueList<KDevice>::Iterator it = device.begin(); it != device.end(); ++it ){
        kdDebug() << "Identify "  << (*it).identify() << endl;
        kdDebug() << "Group " << (*it).group() << endl;
        kdDebug() << "Vendor " << (*it).vendor() << endl;
        kdDebug() << "UNIX " << (*it).id() << endl;
        if ( (*it).id() == QString::fromLatin1("Opie-1") ) {
            QString tmp = m_konnector->registerKonnector( (*it) );
            if ( !tmp.isEmpty() ) {

            }
        }
    }
}
void KSyncMainWindow::slotSync( const QString &udi,
                                QPtrList<KSyncEntry> )
{

}
void KSyncMainWindow::slotStateChanged( const QString &udi,
                                        bool connected )
{

}
void KSyncMainWindow::slotKonnectorError( const QString& udi,
                                          int error,
                                          const QString& id )
{

}
//#include "ksync_mainwindow.moc"
