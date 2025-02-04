/*
    KNode, the KDE newsreader
    Copyright (c) 2003 Zack Rusin <zack@kde.org>
    Copyright (c) 2004-2006 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#include "knmainwidget.h"

#include "akobackit/akonadi_manager.h"
#include "akobackit/folder_manager.h"
#include "akobackit/group_manager.h"
#include "akobackit/nntpaccount_manager.h"
#include "collectiontree/widget.h"
#include "messagelistview/widget.h"

#include <Akonadi/AgentManager>
#include <Akonadi/Collection>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/ItemFetchJob>
#include <akonadi/kmime/messagestatus.h>
#include <KLineEdit>
#include <Q3Accel>
#include <QEvent>
#include <QLabel>
#include <QVBoxLayout>
#include <messagelist/storagemodel.h>
#include <QSplitter>
#include <kicon.h>
#include <kactioncollection.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kstandardaction.h>
#include <kdebug.h>
#include <kmenubar.h>
#include <kiconloader.h>
#include <kstatusbar.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <khbox.h>
#include <kselectaction.h>
#include <kstandardshortcut.h>
#include <ktoggleaction.h>
#include <kxmlguiclient.h>
#include <ksqueezedtextlabel.h>
#include <libkdepim/uistatesaver.h>
#include <broadcaststatus.h>
#include <recentaddresses.h>
using KPIM::BroadcastStatus;
using KPIM::RecentAddresses;
#include <mailtransport/transportmanager.h>
using MailTransport::TransportManager;

//GUI
#include "knarticlewindow.h"

//Core
#include "articlewidget.h"
#include "knglobals.h"
#include "knconfigmanager.h"
#include "knarticlemanager.h"
#include "knarticlefactory.h"
#include "knfiltermanager.h"
#include "kncleanup.h"
#include "utilities.h"
#include "knscoring.h"
#include "settings.h"

#include "knodeadaptor.h"

using namespace KNode;

KNMainWidget::KNMainWidget( KXMLGUIClient* client, QWidget* parent ) :
  KVBox( parent ),
  m_GUIClient( client ),
  mCollectionWidget( 0 )
{
  (void) new KnodeAdaptor( this );
  QDBusConnection::sessionBus().registerObject("/KNode", this);
  knGlobals.top=this;
  knGlobals.topWidget=this;

  //------------------------------- <CONFIG> ----------------------------------
  c_fgManager = knGlobals.configManager();
  //------------------------------- </CONFIG> ----------------------------------

  //-------------------------------- <GUI> ------------------------------------
  // this will enable keyboard-only actions like that don't appear in any menu
  //actionCollection()->setDefaultShortcutContext( Qt::WindowShortcut );

  Q3Accel *accel = new Q3Accel( this );
  initStatusBar();
  setSpacing( 0 );
  setMargin( 0 );
  setLineWidth( 0 );

  // splitters
  mPrimarySplitter = new QSplitter( Qt::Horizontal, this );
  mPrimarySplitter->setObjectName( "mPrimarySplitter" );
  mSecondSplitter = new QSplitter( Qt::Vertical, mPrimarySplitter );
  mSecondSplitter->setObjectName( "mSecondSplitter" );

  //article view
  mArticleViewer = new ArticleWidget( mSecondSplitter, client, actionCollection(), true/*main viewer*/ );

  //collection view
  mCollectionWidget = new CollectionTree::Widget( client, mPrimarySplitter );

  connect( mCollectionWidget, SIGNAL( selectedCollectionChanged( const Akonadi::Collection & ) ),
           this, SLOT( slotCollectionSelected( const Akonadi::Collection & ) ) );
  connect( mCollectionWidget, SIGNAL( renamed( QTreeWidgetItem * ) ),
           this, SLOT(slotCollectionRenamed(QTreeWidgetItem*)) );

  accel->connectItem( accel->insertItem(Qt::Key_Up), mArticleViewer, SLOT(scrollUp()) );
  accel->connectItem( accel->insertItem(Qt::Key_Down), mArticleViewer, SLOT(scrollDown()) );
  accel->connectItem( accel->insertItem(Qt::Key_PageUp), mArticleViewer, SLOT(scrollPrior()) );
  accel->connectItem( accel->insertItem(Qt::Key_PageDown), mArticleViewer, SLOT(scrollNext()) );

  //header view
  mMessageList = new MessageListView::Widget( mSecondSplitter, client );
  MessageList::StorageModel *sm = new MessageList::StorageModel( Akobackit::manager()->entityModel(),
                                                                 mCollectionWidget->selectionModel(), mMessageList );
  mMessageList->setStorageModel( sm );

  connect( mMessageList, SIGNAL( messageSelected( const Akonadi::Item & ) ),
           this, SLOT( slotArticleSelected( const Akonadi::Item & ) ) );
  connect( mMessageList, SIGNAL(selectionChanged()),
          SLOT(slotArticleSelectionChanged()));
  connect( mMessageList, SIGNAL( messageActivated( const Akonadi::Item & ) ),
           this, SLOT( slotOpenArticle( const Akonadi::Item & ) ) );
  connect( mMessageList, SIGNAL(sortingChanged(int)),
          SLOT(slotHdrViewSortingChanged(int)));
  connect( mMessageList, SIGNAL( messagelistEndReached() ),
           this, SLOT( nextGroup() ) );

  //actions
  initActions();

  // splitter setup
  mPrimarySplitter->addWidget( mCollectionWidget );
  mPrimarySplitter->addWidget( mSecondSplitter );
  mSecondSplitter->addWidget( mMessageList );
  mSecondSplitter->addWidget( mArticleViewer );

  //-------------------------------- </GUI> ------------------------------------

  //-------------------------------- <CORE> ------------------------------------

  //Network
#if 0
  connect( knGlobals.scheduler(), SIGNAL(netActive(bool)), this, SLOT(slotNetworkActive(bool)) );
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif

  //Filter Manager
  f_ilManager = knGlobals.filterManager();
  f_ilManager->setMenuAction(a_ctArtFilter, a_ctArtFilterKeyb);

  //Article Manager
  a_rtManager = knGlobals.articleManager();
  a_rtManager->setView( 0 );

  //Article Factory
  a_rtFactory = KNGlobals::self()->articleFactory();

  // Score Manager
  s_coreManager = knGlobals.scoringManager();
  //connect(s_coreManager, SIGNAL(changedRules()), SLOT(slotReScore()));
  connect(s_coreManager, SIGNAL(finishedEditing()), SLOT(slotReScore()));

  QDBusConnection::sessionBus().registerObject( "/", this, QDBusConnection::ExportScriptableSlots );
  //-------------------------------- </CORE> -----------------------------------

  //apply saved options
  readOptions();

  //apply configuration
  configChanged();

  mCollectionWidget->setFocus();

  setStatusMsg();

  if( firstStart() ) {  // open the config dialog on the first start
    show();              // the settings dialog must appear in front of the main window!
    slotSettings();
  }

  actionCollection()->addAssociatedWidget( this );
  foreach (QAction* action, actionCollection()->actions())
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
}

KNMainWidget::~KNMainWidget()
{
#if 0
  // Avoid that removals of items from c_olView call this object back and lead to a crash.
  disconnect( c_olView, SIGNAL(itemSelectionChanged()),
              this, SLOT(slotCollectionSelected()) );
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif

  delete a_rtManager;
  kDebug(5003) <<"KNMainWidget::~KNMainWidget() : Article Manager deleted";

  delete a_rtFactory;
  kDebug(5003) <<"KNMainWidget::~KNMainWidget() : Article Factory deleted";

  delete f_ilManager;
  kDebug(5003) <<"KNMainWidget::~KNMainWidget() : Filter Manager deleted";

  delete c_fgManager;
  kDebug(5003) <<"KNMainWidget::~KNMainWidget() : Config deleted";
}

void KNMainWidget::initStatusBar()
{
  //statusbar
  KMainWindow *mainWin = dynamic_cast<KMainWindow*>(topLevelWidget());
  KStatusBar *sb =  mainWin ? mainWin->statusBar() : 0;
  s_tatusFilter = new KSqueezedTextLabel( QString(), sb );
  s_tatusFilter->setTextElideMode( Qt::ElideRight );
  s_tatusFilter->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
  s_tatusGroup = new KSqueezedTextLabel( QString(), sb );
  s_tatusGroup->setTextElideMode( Qt::ElideRight );
  s_tatusGroup->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
}

//================================== GUI =================================

void KNMainWidget::setStatusMsg(const QString& text, int id)
{
  KMainWindow *mainWin = dynamic_cast<KMainWindow*>(topLevelWidget());
  KStatusBar *bar =  mainWin ? mainWin->statusBar() : 0;
  if ( !bar )
    return;
  bar->clearMessage();
  if (text.isEmpty() && (id==SB_MAIN)) {
    BroadcastStatus::instance()->setStatusMsg(i18n(" Ready"));
  } else {
    switch(id) {
      case SB_MAIN:
        BroadcastStatus::instance()->setStatusMsg(text); break;
      case SB_GROUP:
        s_tatusGroup->setText(text); break;
      case SB_FILTER:
        s_tatusFilter->setText(text); break;
    }
  }
}


void KNMainWidget::setStatusHelpMsg(const QString& text)
{
  KMainWindow *mainWin = dynamic_cast<KMainWindow*>(topLevelWidget());
  KStatusBar *bar =  mainWin ? mainWin->statusBar() : 0;
  if ( bar )
    bar->showMessage(text, 2000);
}


void KNMainWidget::updateCaption()
{
  // FIXME: the new caption is not in sync with the current collection :-(
  QString newCaption=i18n("KDE News Reader");

  const Akonadi::Collection currentCollection = mCollectionWidget->selectedCollection();
  if ( currentCollection.isValid() ) {
    Akonadi::EntityDisplayAttribute *attribute = currentCollection.attribute<Akonadi::EntityDisplayAttribute>();
    if ( attribute ) {
      newCaption = attribute->displayName();
    } else {
      newCaption = currentCollection.name();
    }

    Akobackit::GroupManager *gm = Akobackit::manager()->groupManager();
    if( gm->isGroup( currentCollection ) ) {
      Group::Ptr group( new Group( currentCollection ) );
      if ( group->postingStatus() == Group::Moderated ) {
        newCaption = i18nc( "@title:window %1:newsgroup name", "%1 (moderated)", newCaption );
      }
    }
  }
  emit signalCaptionChangeRequest(newCaption);
}

void KNMainWidget::disableAccels(bool b)
{
#ifdef __GNUC__
#warning Port me!
#endif
  /*
  KMainWindow *mainWin = dynamic_cast<KMainWindow*>(topLevelWidget());
  KAccel *naccel = mainWin ? mainWin->accel() : 0;
  if ( naccel )
    naccel->setEnabled(!b);*/
  if (b)
    installEventFilter(this);
  else
    removeEventFilter(this);
}


QSize KNMainWidget::sizeHint() const
{
  return QSize(759,478);    // default optimized for 800x600
}


void KNMainWidget::openURL(const QString &url)
{
  openURL(KUrl(url));
}

void KNMainWidget::openURL(const KUrl &url)
{
#if 0
  kDebug(5003) << url;
  QString host = url.host();
  short int port = url.port();
  KNNntpAccount::Ptr acc;

  if (url.url().left(7) == "news://") {

    // lets see if we already have an account for this host...
    KNNntpAccount::List list = a_ccManager->accounts();
    for ( KNNntpAccount::List::Iterator it = list.begin(); it != list.end(); ++it ) {
      if ( (*it)->server().toLower() == host.toLower() && ( port==-1 || (*it)->port() == port ) ) {
        acc = *it;
        break;
      }
    }

    if(!acc) {
      acc = KNNntpAccount::Ptr( new KNNntpAccount() );
      acc->setName(host);
      acc->setServer(host);

      if(port!=-1)
        acc->setPort(port);

      if(url.hasUser() && url.hasPass()) {
        acc->setNeedsLogon(true);
        acc->setUser(url.user());
        acc->setPass(url.pass());
      }

      if(!a_ccManager->newAccount(acc))
        return;
    }
  } else {
    if (url.url().left(5) == "news:") {
      // TODO: make the default server configurable
      acc = a_ccManager->currentAccount();
      if ( acc == 0 )
        acc = a_ccManager->first();
    } else {
      kDebug(5003) <<"KNMainWidget::openURL() URL is not a valid news URL";
    }
  }

  if (acc) {
    QString decodedUrl = KUrl::fromPercentEncoding( url.url().toLatin1() );
    bool isMID=decodedUrl.contains('@' );

    if (!isMID) {
      QString groupname=url.path( KUrl::RemoveTrailingSlash );
      while(groupname.startsWith('/'))
        groupname.remove(0,1);
      QTreeWidgetItem *item=0;
      if ( groupname.isEmpty() ) {
        item=acc->listItem();
      } else {
        KNGroup::Ptr grp = g_rpManager->group( groupname, acc );

        if(!grp) {
          KNGroupInfo inf(groupname, "");
          g_rpManager->subscribeGroup(&inf, acc);
          grp=g_rpManager->group(groupname, acc);
          if(grp)
            item=grp->listItem();
        }
        else
          item=grp->listItem();
      }

      if (item) {
        c_olView->setActive( item );
      }
    } else {
      QString groupname = decodedUrl.mid( url.protocol().length()+1 );
      KNGroup::Ptr g = g_rpManager->currentGroup();
      if (g == 0)
        g = g_rpManager->firstGroupOfAccount(acc);

      if (g) {
        if ( !ArticleWindow::raiseWindowForArticle( groupname.toLatin1() ) ) { //article not yet opened
          KNRemoteArticle::Ptr a( new KNRemoteArticle(g) );
          QString messageID = '<' + groupname + '>';
          a->messageID()->from7BitString(messageID.toLatin1());
          ArticleWindow *awin = new ArticleWindow( a );
          awin->show();
        }
      } else {
        // TODO: fetch without group
        kDebug(5003) <<"KNMainWidget::openURL() account has no groups";
      }
    }
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


// update fonts and colors
void KNMainWidget::configChanged()
{
#if 0
  h_drView->readConfig();
  c_olView->readConfig();
  a_rtManager->updateListViewItems();
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::initActions()
{
  //navigation
  a_ctNavNextArt = actionCollection()->addAction("go_nextArticle" );
  a_ctNavNextArt->setText(i18n("&Next Article"));
  a_ctNavNextArt->setToolTip(i18n("Go to next article"));
  a_ctNavNextArt->setShortcuts(KShortcut("N; Right"));
  connect( a_ctNavNextArt, SIGNAL( triggered( bool ) ), mMessageList, SLOT( nextArticle() ) );

  a_ctNavPrevArt = actionCollection()->addAction("go_prevArticle" );
  a_ctNavPrevArt->setText(i18n("&Previous Article"));
  a_ctNavPrevArt->setShortcuts(KShortcut("P; Left"));
  a_ctNavPrevArt->setToolTip(i18n("Go to previous article"));
  connect( a_ctNavPrevArt, SIGNAL( triggered( bool ) ), mMessageList, SLOT( previousArticle() ) );

  a_ctNavNextUnreadArt = actionCollection()->addAction("go_nextUnreadArticle");
  a_ctNavNextUnreadArt->setIcon(KIcon("go-next"));
  a_ctNavNextUnreadArt->setText(i18n("Next Unread &Article"));
  connect( a_ctNavNextUnreadArt, SIGNAL( triggered( bool ) ), mMessageList, SLOT( nextUnreadArticle() ) );
  a_ctNavNextUnreadArt->setShortcut(QKeySequence(Qt::ALT+Qt::SHIFT+Qt::Key_Space));

  a_ctNavNextUnreadThread = actionCollection()->addAction("go_nextUnreadThread");
  a_ctNavNextUnreadThread->setIcon(KIcon("go-last"));
  a_ctNavNextUnreadThread->setText(i18n("Next Unread &Thread"));
  connect( a_ctNavNextUnreadThread, SIGNAL( triggered( bool ) ), mMessageList, SLOT( nextUnreadThread() ) );
  a_ctNavNextUnreadThread->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_Space));

  a_ctNavNextGroup = actionCollection()->addAction("go_nextGroup");
  a_ctNavNextGroup->setIcon(KIcon("go-down"));
  a_ctNavNextGroup->setText(i18n("Ne&xt Group"));
  connect(a_ctNavNextGroup, SIGNAL(triggered(bool)), mCollectionWidget, SLOT(nextGroup()));
  a_ctNavNextGroup->setShortcut(QKeySequence(Qt::Key_Plus));

  a_ctNavPrevGroup = actionCollection()->addAction("go_prevGroup");
  a_ctNavPrevGroup->setIcon(KIcon("go-up"));
  a_ctNavPrevGroup->setText(i18n("Pre&vious Group"));
  connect( a_ctNavPrevGroup, SIGNAL( triggered( bool ) ), mCollectionWidget, SLOT( previousGroup() ) );
  a_ctNavPrevGroup->setShortcut(QKeySequence(Qt::Key_Minus));

  a_ctNavReadThrough = actionCollection()->addAction("go_readThrough");
  a_ctNavReadThrough->setText(i18n("Read &Through Articles"));
  connect(a_ctNavReadThrough, SIGNAL(triggered(bool) ), SLOT(slotNavReadThrough()));
  a_ctNavReadThrough->setShortcut(QKeySequence(Qt::Key_Space));

  KAction *action = actionCollection()->addAction("inc_current_folder");
  action->setText(i18n("Focus on Next Folder"));
  connect( action, SIGNAL( triggered( bool ) ), mCollectionWidget, SLOT( incCurrentFolder() ) );
  action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Right));

  action = actionCollection()->addAction("dec_current_folder");
  action->setText(i18n("Focus on Previous Folder"));
  connect( action, SIGNAL( triggered( bool ) ), mCollectionWidget, SLOT( decCurrentFolder() ) );
  action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Left));

  action = actionCollection()->addAction("select_current_folder");
  action->setText(i18n("Select Folder with Focus"));
  connect( action, SIGNAL( triggered( bool ) ), mCollectionWidget, SLOT( selectCurrentFolder() ) );
  action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Space));

  action = actionCollection()->addAction("inc_current_article");
  action->setText(i18n("Focus on Next Article"));
  connect(action, SIGNAL(triggered(bool) ), mMessageList, SLOT(incCurrentArticle()));
  action->setShortcut(QKeySequence(Qt::ALT+Qt::Key_Right));

  action = actionCollection()->addAction("dec_current_article");
  action->setText(i18n("Focus on Previous Article"));
  connect(action, SIGNAL(triggered(bool) ), mMessageList, SLOT(decCurrentArticle()));
  action->setShortcut(QKeySequence(Qt::ALT+Qt::Key_Left));

  action = actionCollection()->addAction("select_current_article");
  action->setText(i18n("Select Article with Focus"));
  connect(action, SIGNAL(triggered(bool) ), mMessageList, SLOT(selectCurrentArticle()));
  action->setShortcut(QKeySequence(Qt::ALT+Qt::Key_Space));

  //collection-view - accounts
  a_ctAccProperties = actionCollection()->addAction("account_properties");
  a_ctAccProperties->setIcon(KIcon("document-properties"));
  a_ctAccProperties->setText(i18n("Account &Properties"));
  connect(a_ctAccProperties, SIGNAL(triggered(bool)), SLOT(slotAccProperties()));

  a_ctAccRename = actionCollection()->addAction("account_rename");
  a_ctAccRename->setIcon(KIcon("edit-rename"));
  a_ctAccRename->setText(i18n("&Rename Account"));
  connect(a_ctAccRename, SIGNAL(triggered(bool)), SLOT(slotAccRename()));

  a_ctAccSubscribe = actionCollection()->addAction("account_subscribe");
  a_ctAccSubscribe->setIcon(KIcon("news-subscribe"));
  a_ctAccSubscribe->setText(i18n("&Subscribe to Newsgroups..."));
  connect(a_ctAccSubscribe, SIGNAL(triggered(bool)), SLOT(slotAccSubscribe()));

  a_ctAccExpireAll = actionCollection()->addAction("account_expire_all");
  a_ctAccExpireAll->setText(i18n("&Expire All Groups"));
  connect(a_ctAccExpireAll, SIGNAL(triggered(bool) ), SLOT(slotAccExpireAll()));

  a_ctAccGetNewHdrs = actionCollection()->addAction("account_dnlHeaders");
  a_ctAccGetNewHdrs->setIcon(KIcon("mail-receive"));
  a_ctAccGetNewHdrs->setText(i18n("&Get New Articles in All Groups"));
  connect(a_ctAccGetNewHdrs, SIGNAL(triggered(bool)), SLOT(slotAccGetNewHdrs()));

  a_ctAccGetNewHdrsAll = actionCollection()->addAction("account_dnlAllHeaders");
  a_ctAccGetNewHdrsAll->setIcon(KIcon("mail-receive-all"));
  a_ctAccGetNewHdrsAll->setText(i18n("&Get New Articles in All Accounts"));
  connect(a_ctAccGetNewHdrsAll, SIGNAL(triggered(bool)), SLOT(slotAccGetNewHdrsAll()));

  a_ctAccDelete = actionCollection()->addAction("account_delete");
  a_ctAccDelete->setIcon(KIcon("edit-delete"));
  a_ctAccDelete->setText(i18n("&Delete Account"));
  connect(a_ctAccDelete, SIGNAL(triggered(bool)), SLOT(slotAccDelete()));

  a_ctAccPostNewArticle = actionCollection()->addAction("article_postNew");
  a_ctAccPostNewArticle->setIcon(KIcon("mail-message-new"));
  a_ctAccPostNewArticle->setText(i18n("&Post to Newsgroup..."));
  connect(a_ctAccPostNewArticle, SIGNAL(triggered(bool)), SLOT(slotAccPostNewArticle()));
  a_ctAccPostNewArticle->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_N));

  //collection-view - groups
  a_ctGrpProperties = actionCollection()->addAction("group_properties");
  a_ctGrpProperties->setIcon(KIcon("document-properties"));
  a_ctGrpProperties->setText(i18n("Group &Properties"));
  connect(a_ctGrpProperties, SIGNAL(triggered(bool)), SLOT(slotGrpProperties()));

  a_ctGrpRename = actionCollection()->addAction("group_rename");
  a_ctGrpRename->setIcon(KIcon("edit-rename"));
  a_ctGrpRename->setText(i18n("Rename &Group"));
  connect(a_ctGrpRename, SIGNAL(triggered(bool)), SLOT(slotGrpRename()));

  a_ctGrpGetNewHdrs = actionCollection()->addAction("group_dnlHeaders");
  a_ctGrpGetNewHdrs->setIcon(KIcon("mail-receive"));
  a_ctGrpGetNewHdrs->setText(i18n("&Get New Articles"));
  connect(a_ctGrpGetNewHdrs, SIGNAL(triggered(bool)), SLOT(slotGrpGetNewHdrs()));

  a_ctGrpExpire = actionCollection()->addAction("group_expire");
  a_ctGrpExpire->setText(i18n("E&xpire Group"));
  connect(a_ctGrpExpire, SIGNAL(triggered(bool)), SLOT(slotGrpExpire()));

  a_ctGrpUnsubscribe = actionCollection()->addAction("group_unsubscribe");
  a_ctGrpUnsubscribe->setIcon(KIcon("news-unsubscribe"));
  a_ctGrpUnsubscribe->setText(i18n("&Unsubscribe From Group"));
  connect(a_ctGrpUnsubscribe, SIGNAL(triggered(bool)), SLOT(slotGrpUnsubscribe()));

  a_ctGrpSetAllRead = actionCollection()->addAction("group_allRead");
  a_ctGrpSetAllRead->setIcon(KIcon("mail-mark-read"));
  a_ctGrpSetAllRead->setText(i18n("Mark All as &Read"));
  connect(a_ctGrpSetAllRead, SIGNAL(triggered(bool)), SLOT(slotGrpSetAllRead()));

  a_ctGrpSetAllUnread = actionCollection()->addAction("group_allUnread");
  a_ctGrpSetAllUnread->setText(i18n("Mark All as U&nread"));
  connect(a_ctGrpSetAllUnread, SIGNAL(triggered(bool) ), SLOT(slotGrpSetAllUnread()));

  a_ctGrpSetUnread = actionCollection()->addAction("group_unread");
  a_ctGrpSetUnread->setText(i18n("Mark Last as Unr&ead..."));
  connect(a_ctGrpSetUnread, SIGNAL(triggered(bool) ), SLOT(slotGrpSetUnread()));

  action = actionCollection()->addAction("knode_configure_knode");
  action->setIcon(KIcon("configure"));
  action->setText(i18n("&Configure KNode..."));
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotSettings()));

  //collection-view - folder
  a_ctFolNew = actionCollection()->addAction("folder_new");
  a_ctFolNew->setIcon(KIcon("folder-new"));
  a_ctFolNew->setText(i18n("&New Folder"));
  connect(a_ctFolNew, SIGNAL(triggered(bool)), SLOT(slotFolNew()));

  a_ctFolNewChild = actionCollection()->addAction("folder_newChild");
  a_ctFolNewChild->setIcon(KIcon("folder-new"));
  a_ctFolNewChild->setText(i18n("New &Subfolder"));
  connect(a_ctFolNewChild, SIGNAL(triggered(bool)), SLOT(slotFolNewChild()));

  a_ctFolDelete = actionCollection()->addAction("folder_delete");
  a_ctFolDelete->setIcon(KIcon("edit-delete"));
  a_ctFolDelete->setText(i18n("&Delete Folder"));
  connect(a_ctFolDelete, SIGNAL(triggered(bool)), SLOT(slotFolDelete()));

  a_ctFolRename = actionCollection()->addAction("folder_rename");
  a_ctFolRename->setIcon(KIcon("edit-rename"));
  a_ctFolRename->setText(i18n("&Rename Folder"));
  connect(a_ctFolRename, SIGNAL(triggered(bool)), SLOT(slotFolRename()));

  a_ctFolEmpty = actionCollection()->addAction("folder_empty");
  a_ctFolEmpty->setText(i18n("&Empty Folder"));
  connect(a_ctFolEmpty, SIGNAL(triggered(bool) ), SLOT(slotFolEmpty()));

  a_ctFolMboxImport = actionCollection()->addAction("folder_MboxImport");
  a_ctFolMboxImport->setText(i18n("&Import MBox Folder..."));
  connect(a_ctFolMboxImport, SIGNAL(triggered(bool) ), SLOT(slotFolMBoxImport()));

  a_ctFolMboxExport = actionCollection()->addAction("folder_MboxExport");
  a_ctFolMboxExport->setText(i18n("E&xport as MBox Folder..."));
  connect(a_ctFolMboxExport, SIGNAL(triggered(bool) ), SLOT(slotFolMBoxExport()));

  //header-view - list-handling
  a_ctArtSortHeaders = actionCollection()->add<KSelectAction>("view_Sort");
  a_ctArtSortHeaders->setText(i18n("S&ort"));
  QStringList items;
  items += i18n("By &Subject");
  items += i18n("By S&ender");
  items += i18n("By S&core");
  items += i18n("By &Lines");
  items += i18n("By &Date");
  a_ctArtSortHeaders->setItems(items);
  a_ctArtSortHeaders->setShortcutConfigurable(false);
  connect(a_ctArtSortHeaders, SIGNAL(activated(int)), this, SLOT(slotArtSortHeaders(int)));

  a_ctArtSortHeadersKeyb = actionCollection()->addAction("view_Sort_Keyb");
  a_ctArtSortHeadersKeyb->setText(i18n("Sort"));
  connect(a_ctArtSortHeadersKeyb, SIGNAL(triggered(bool)), SLOT(slotArtSortHeadersKeyb()));
  a_ctArtSortHeadersKeyb->setShortcut(QKeySequence(Qt::Key_F7));

  a_ctArtFilter = new KNFilterSelectAction(i18n("&Filter"), "view-filter",
                                           actionCollection(), "view_Filter");
  a_ctArtFilter->setShortcutConfigurable(false);

  a_ctArtFilterKeyb = actionCollection()->addAction("view_Filter_Keyb");
  a_ctArtFilterKeyb->setText(i18n("Filter"));
  a_ctArtFilterKeyb->setShortcut(QKeySequence(Qt::Key_F6));

  a_ctArtSearch = actionCollection()->addAction("article_search");
  a_ctArtSearch->setIcon(KIcon("edit-find-mail"));
  a_ctArtSearch->setText(i18n("&Search Articles..."));
  connect(a_ctArtSearch, SIGNAL(triggered(bool)), SLOT(slotArtSearch()));
  a_ctArtSearch->setShortcut(QKeySequence(Qt::Key_F4));

  a_ctArtRefreshList = actionCollection()->addAction("view_Refresh");
  a_ctArtRefreshList->setIcon(KIcon("view-refresh"));
  a_ctArtRefreshList->setText(i18n("&Refresh List"));
  connect(a_ctArtRefreshList, SIGNAL(triggered(bool)), SLOT(slotArtRefreshList()));
  a_ctArtRefreshList->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::Reload));

  a_ctArtCollapseAll = actionCollection()->addAction("view_CollapseAll");
  a_ctArtCollapseAll->setText(i18n("&Collapse All Threads"));
  connect( a_ctArtCollapseAll, SIGNAL( triggered( bool ) ), mMessageList, SLOT( collapseAll() ) );

  a_ctArtExpandAll = actionCollection()->addAction("view_ExpandAll");
  a_ctArtExpandAll->setText(i18n("E&xpand All Threads"));
  connect( a_ctArtExpandAll, SIGNAL( triggered( bool ) ), mMessageList, SLOT( expandAll() ) );

  a_ctArtToggleThread = actionCollection()->addAction("thread_toggle");
  a_ctArtToggleThread->setText(i18n("&Toggle Subthread"));
  connect(a_ctArtToggleThread, SIGNAL(triggered(bool) ), SLOT(slotArtToggleThread()));
  a_ctArtToggleThread->setShortcut(QKeySequence(Qt::Key_T));

  a_ctArtToggleShowThreads = actionCollection()->add<KToggleAction>("view_showThreads");
  a_ctArtToggleShowThreads->setText(i18n("Show T&hreads"));
  connect(a_ctArtToggleShowThreads, SIGNAL(triggered(bool) ), SLOT(slotArtToggleShowThreads()));

  a_ctArtToggleShowThreads->setChecked( knGlobals.settings()->showThreads() );

  //header-view - remote articles
  a_ctArtSetArtRead = actionCollection()->addAction("article_read");
  a_ctArtSetArtRead->setIcon(KIcon("mail-mark-read"));
  a_ctArtSetArtRead->setText(i18n("Mark as &Read"));
  connect(a_ctArtSetArtRead, SIGNAL(triggered(bool) ), SLOT(slotArtSetArtRead()));
  a_ctArtSetArtRead->setShortcut(QKeySequence(Qt::Key_D));

  a_ctArtSetArtUnread = actionCollection()->addAction("article_unread");
  a_ctArtSetArtUnread->setIcon(KIcon("mail-mark-unread"));
  a_ctArtSetArtUnread->setText(i18n("Mar&k as Unread"));
  connect(a_ctArtSetArtUnread, SIGNAL(triggered(bool) ), SLOT(slotArtSetArtUnread()));
  a_ctArtSetArtUnread->setShortcut(QKeySequence(Qt::Key_U));

  a_ctArtSetThreadRead = actionCollection()->addAction("thread_read");
  a_ctArtSetThreadRead->setText(i18n("Mark &Thread as Read"));
  connect(a_ctArtSetThreadRead, SIGNAL(triggered(bool) ), SLOT(slotArtSetThreadRead()));
  a_ctArtSetThreadRead->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_D));

  a_ctArtSetThreadUnread = actionCollection()->addAction("thread_unread");
  a_ctArtSetThreadUnread->setText(i18n("Mark T&hread as Unread"));
  connect(a_ctArtSetThreadUnread, SIGNAL(triggered(bool) ), SLOT(slotArtSetThreadUnread()));
  a_ctArtSetThreadUnread->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_U));

  a_ctArtOpenNewWindow = actionCollection()->addAction("article_ownWindow");
  a_ctArtOpenNewWindow->setIcon(KIcon("window-new"));
  a_ctArtOpenNewWindow->setText(i18n("Open in Own &Window"));
  connect(a_ctArtOpenNewWindow, SIGNAL(triggered(bool)), SLOT(slotArtOpenNewWindow()));
  a_ctArtOpenNewWindow->setShortcut(QKeySequence(Qt::Key_O));

  // scoring
  a_ctScoresEdit = actionCollection()->addAction("scoreedit");
  a_ctScoresEdit->setIcon(KIcon("document-properties"));
  a_ctScoresEdit->setText(i18n("&Edit Scoring Rules..."));
  connect(a_ctScoresEdit, SIGNAL(triggered(bool)), SLOT(slotScoreEdit()));
  a_ctScoresEdit->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_E));

  a_ctReScore = actionCollection()->addAction("rescore");
  a_ctReScore->setText(i18n("Recalculate &Scores"));
  connect(a_ctReScore, SIGNAL(triggered(bool) ), SLOT(slotReScore()));

  a_ctScoreLower = actionCollection()->addAction("scorelower");
  a_ctScoreLower->setText(i18n("&Lower Score for Author..."));
  connect(a_ctScoreLower, SIGNAL(triggered(bool) ), SLOT(slotScoreLower()));
  a_ctScoreLower->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_L));

  a_ctScoreRaise = actionCollection()->addAction("scoreraise");
  a_ctScoreRaise->setText(i18n("&Raise Score for Author..."));
  connect(a_ctScoreRaise, SIGNAL(triggered(bool) ), SLOT(slotScoreRaise()));
  a_ctScoreRaise->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_I));

  a_ctArtToggleIgnored = actionCollection()->addAction("thread_ignore");
  a_ctArtToggleIgnored->setIcon(KIcon("go-bottom"));
  a_ctArtToggleIgnored->setText(i18n("&Ignore Thread"));
  connect(a_ctArtToggleIgnored, SIGNAL(triggered(bool)), SLOT(slotArtToggleIgnored()));
  a_ctArtToggleIgnored->setShortcut(QKeySequence(Qt::Key_I));

  a_ctArtToggleWatched = actionCollection()->addAction("thread_watch");
  a_ctArtToggleWatched->setIcon(KIcon("go-top"));
  a_ctArtToggleWatched->setText(i18n("&Watch Thread"));
  connect(a_ctArtToggleWatched, SIGNAL(triggered(bool)), SLOT(slotArtToggleWatched()));
  a_ctArtToggleWatched->setShortcut(QKeySequence(Qt::Key_W));

  //header-view local articles
  a_ctArtSendOutbox = actionCollection()->addAction("net_sendPending");
  a_ctArtSendOutbox->setIcon(KIcon("mail-send"));
  a_ctArtSendOutbox->setText(i18n("Sen&d Pending Messages"));
  connect(a_ctArtSendOutbox, SIGNAL(triggered(bool)), SLOT(slotArtSendOutbox()));

  a_ctArtDelete = actionCollection()->addAction("article_delete");
  a_ctArtDelete->setIcon(KIcon("edit-delete"));
  a_ctArtDelete->setText(i18n("&Delete Article"));
  connect(a_ctArtDelete, SIGNAL(triggered(bool)), SLOT(slotArtDelete()));
  a_ctArtDelete->setShortcut(QKeySequence(Qt::Key_Delete));

  a_ctArtSendNow = actionCollection()->addAction("article_sendNow");
  a_ctArtSendNow->setIcon(KIcon("mail-send"));
  a_ctArtSendNow->setText(i18n("Send &Now"));
  connect(a_ctArtSendNow, SIGNAL(triggered(bool)), SLOT(slotArtSendNow()));

  a_ctArtEdit = actionCollection()->addAction("article_edit");
  a_ctArtEdit->setIcon(KIcon("document-properties"));
  a_ctArtEdit->setText(i18nc("edit article","&Edit Article..."));
  connect(a_ctArtEdit, SIGNAL(triggered(bool)), SLOT(slotArtEdit()));
  a_ctArtEdit->setShortcut(QKeySequence(Qt::Key_E));

  //network
  a_ctNetCancel = actionCollection()->addAction("net_stop");
  a_ctNetCancel->setIcon(KIcon("process-stop"));
  a_ctNetCancel->setText(i18n("Stop &Network"));
  connect(a_ctNetCancel, SIGNAL(triggered(bool)), SLOT(slotNetCancel()));
  a_ctNetCancel->setEnabled(false);

  a_ctFetchArticleWithID = actionCollection()->addAction("fetch_article_with_id");
  a_ctFetchArticleWithID->setText(i18n("&Fetch Article with ID..."));
  connect(a_ctFetchArticleWithID, SIGNAL(triggered(bool) ), SLOT(slotFetchArticleWithID()));
  a_ctFetchArticleWithID->setEnabled(false);

  a_ctToggleQuickSearch = actionCollection()->add<KToggleAction>("settings_show_quickSearch");
  a_ctToggleQuickSearch->setText(i18n("Show Quick Search"));
  connect( a_ctToggleQuickSearch, SIGNAL( triggered( bool ) ), mMessageList, SLOT( changeQuicksearchVisibility() ) );
  a_ctToggleQuickSearch->setChecked( !mMessageList->quickSearch()->isVisible() );
}

bool KNMainWidget::firstStart()
{
  KConfigGroup conf(knGlobals.config(), "GENERAL");
  QString ver = conf.readEntry("Version");
  if(!ver.isEmpty())
    return false;

  if ( TransportManager::self()->isEmpty() )
    TransportManager::self()->createDefaultTransport();

  conf.writeEntry("Version", KNODE_VERSION);

  return true;
}


void KNMainWidget::readOptions()
{
#if 0
  mCollectionWidget->readConfig();
  mMessageList->readConfig();
  a_ctArtSortHeaders->setCurrentItem( h_drView->sortColumn() );

#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
  resize(787,478);  // default optimized for 800x600

  KPIM::UiStateSaver::restoreState( this, KConfigGroup( knGlobals.config(), "UI State" ) );
}


void KNMainWidget::saveOptions()
{
#if 0
  mCollectionWidget->writeConfig();
  mMessageList->writeConfig();
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
  mArticleViewer->writeConfig();

  KConfigGroup cfg( knGlobals.config(), "UI State" );
  KPIM::UiStateSaver::saveState( this, cfg );
}


bool KNMainWidget::requestShutdown()
{
  kDebug(5003) <<"KNMainWidget::requestShutdown()";

#if 0
  if( a_rtFactory->jobsPending() &&
      KMessageBox::No==KMessageBox::warningYesNo(this, i18n(
"KNode is currently sending articles. If you quit now you might lose these \
articles.\nDo you want to quit anyway?"), QString(), KStandardGuiItem::quit(), KStandardGuiItem::cancel())
    )
    return false;

  if(!a_rtFactory->closeComposeWindows())
    return false;

#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
  return true;
}


void KNMainWidget::prepareShutdown()
{
  kDebug(5003) <<"KNMainWidget::prepareShutdown()";

#if 0
  //cleanup article-views
  ArticleWidget::cleanup();

  // expire groups (if necessary)
  KNCleanUp *cup = new KNCleanUp();
  g_rpManager->expireAll(cup);
  cup->start();

  // compact folders
  KNode::Cleanup *conf=c_fgManager->cleanup();
  if (conf->compactToday()) {
    cup->reset();
    f_olManager->compactAll(cup);
    cup->start();
    conf->setLastCompactDate();
  }

  delete cup;

#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
  saveOptions();
  RecentAddresses::self(knGlobals.config())->save( knGlobals.config() );
#if 0
  c_fgManager->syncConfig();
  a_rtManager->deleteTempFiles();
  g_rpManager->syncGroups();
  f_olManager->syncFolders();
  f_ilManager->prepareShutdown();
  a_ccManager->prepareShutdown();
  s_coreManager->save();
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


bool KNMainWidget::queryClose()
{
  if(!requestShutdown())
    return false;

  prepareShutdown();

  return true;
}


void KNMainWidget::fontChange( const QFont & )
{
  a_rtFactory->configChanged();
  ArticleWidget::configChanged();
  configChanged();
}


void KNMainWidget::paletteChange( const QPalette & )
{
  ArticleWidget::configChanged();
  configChanged();
}


bool KNMainWidget::eventFilter(QObject *o, QEvent *e)
{
  if ((e->type() == QEvent::KeyPress) ||
       (e->type() == QEvent::KeyRelease) ||
       (e->type() == QEvent::Shortcut) ||
       (e->type() == QEvent::ShortcutOverride))
    return true;
  return QWidget::eventFilter(o, e);
}



void KNMainWidget::slotArticleSelected( const Akonadi::Item &item )
{
  kDebug() << "valid item:" << item.isValid();

  RemoteArticle::Ptr selectedArticle;
  if ( item.isValid() ) {
    bool isInFolder = Akobackit::manager()->folderManager()->isFolder( mCollectionWidget->selectedCollection() );
    if ( isInFolder ) {
      selectedArticle = LocalArticle::Ptr( new LocalArticle( item ) );
    } else {
      selectedArticle = RemoteArticle::Ptr( new RemoteArticle( item ) );
    }
  }

  mArticleViewer->setArticle( selectedArticle );

  Akobackit::CollectionType type = Akobackit::manager()->type( mCollectionWidget->selectedCollection() );

  //actions
  bool enabled;

  enabled = ( selectedArticle && selectedArticle->type() == RemoteArticle::ATremote );
  if(a_ctArtSetArtRead->isEnabled() != enabled) {
    a_ctArtSetArtRead->setEnabled(enabled);
    a_ctArtSetArtUnread->setEnabled(enabled);
    a_ctArtSetThreadRead->setEnabled(enabled);
    a_ctArtSetThreadUnread->setEnabled(enabled);
    a_ctArtToggleIgnored->setEnabled(enabled);
    a_ctArtToggleWatched->setEnabled(enabled);
    a_ctScoreLower->setEnabled(enabled);
    a_ctScoreRaise->setEnabled(enabled);
  }

  a_ctArtOpenNewWindow->setEnabled( selectedArticle && ( type != Akobackit::OutboxFolder )
                                                    && ( type != Akobackit::DraftFolder ) );

  enabled = ( selectedArticle && selectedArticle->type() == RemoteArticle::ATlocal );
  a_ctArtDelete->setEnabled(enabled);
  a_ctArtSendNow->setEnabled( enabled && ( type == Akobackit::OutboxFolder ) );
  a_ctArtEdit->setEnabled( enabled && ( ( type == Akobackit::OutboxFolder )
                                        || ( type == Akobackit::DraftFolder ) ) );
}


void KNMainWidget::slotArticleSelectionChanged()
{
  // enable all actions that work with multiple selection

  Akobackit::CollectionType type = Akobackit::manager()->type( mCollectionWidget->selectedCollection() );

  //actions
  bool enabled = ( type == Akobackit::NewsGroup );
  if(a_ctArtSetArtRead->isEnabled() != enabled) {
    a_ctArtSetArtRead->setEnabled(enabled);
    a_ctArtSetArtUnread->setEnabled(enabled);
    a_ctArtSetThreadRead->setEnabled(enabled);
    a_ctArtSetThreadUnread->setEnabled(enabled);
    a_ctArtToggleIgnored->setEnabled(enabled);
    a_ctArtToggleWatched->setEnabled(enabled);
    a_ctScoreLower->setEnabled(enabled);
    a_ctScoreRaise->setEnabled(enabled);
  }

  enabled = Akobackit::manager()->folderManager()->isFolder( mCollectionWidget->selectedCollection() );
  a_ctArtDelete->setEnabled(enabled);
  a_ctArtSendNow->setEnabled( type == Akobackit::OutboxFolder );
}


void KNMainWidget::slotCollectionSelected( const Akonadi::Collection &col )
{
  kDebug() << "Enter";

  slotArticleSelected( Akonadi::Item() );

  // mark all articles in current group as not new/read
  if ( knGlobals.settings()->leaveGroupMarkAsRead() ) {
    mMessageList->markAll( Akonadi::MessageStatus::statusRead() );
  }

  Akobackit::CollectionType collectionType = Akobackit::manager()->type( col );

  if ( collectionType != Akobackit::NntpServer ) {
    if ( !mMessageList->hasFocus() && !mArticleViewer->hasFocus() ) {
      mMessageList->setFocus();
    }
  }

  a_rtManager->updateStatusString();
  updateCaption();

  //actions
  bool enabled;

  enabled = ( collectionType == Akobackit::NewsGroup ) || ( collectionType != Akobackit::RootFolder );
  if(a_ctNavNextArt->isEnabled() != enabled) {
    a_ctNavNextArt->setEnabled(enabled);
    a_ctNavPrevArt->setEnabled(enabled);
  }

  enabled = ( collectionType == Akobackit::NewsGroup );
  if(a_ctNavNextUnreadArt->isEnabled() != enabled) {
    a_ctNavNextUnreadArt->setEnabled(enabled);
    a_ctNavNextUnreadThread->setEnabled(enabled);
    a_ctNavReadThrough->setEnabled(enabled);
    a_ctFetchArticleWithID->setEnabled(enabled);
  }

  enabled = ( collectionType == Akobackit::NntpServer );
  if(a_ctAccProperties->isEnabled() != enabled) {
    a_ctAccProperties->setEnabled(enabled);
    a_ctAccRename->setEnabled(enabled);
    a_ctAccSubscribe->setEnabled(enabled);
    a_ctAccExpireAll->setEnabled(enabled);
    a_ctAccGetNewHdrs->setEnabled(enabled);
    a_ctAccGetNewHdrsAll->setEnabled(enabled);
    a_ctAccDelete->setEnabled(enabled);
    //Laurent fix me
    //a_ctAccPostNewArticle->setEnabled(enabled);
  }

  enabled = ( collectionType == Akobackit::NewsGroup );
  if(a_ctGrpProperties->isEnabled() != enabled) {
    a_ctGrpProperties->setEnabled(enabled);
    a_ctGrpRename->setEnabled(enabled);
    a_ctGrpGetNewHdrs->setEnabled(enabled);
    a_ctGrpExpire->setEnabled(enabled);
    a_ctGrpUnsubscribe->setEnabled(enabled);
    a_ctGrpSetAllRead->setEnabled(enabled);
    a_ctGrpSetAllUnread->setEnabled(enabled);
    a_ctGrpSetUnread->setEnabled(enabled);
    a_ctArtFilter->setEnabled(enabled);
    a_ctArtFilterKeyb->setEnabled(enabled);
    a_ctArtRefreshList->setEnabled(enabled);
    a_ctArtCollapseAll->setEnabled(enabled);
    a_ctArtExpandAll->setEnabled(enabled);
    a_ctArtToggleShowThreads->setEnabled(enabled);
    a_ctReScore->setEnabled(enabled);
  }

  bool isFolder = Akobackit::manager()->folderManager()->isFolder( col );
  a_ctFolNewChild->setEnabled( isFolder );

  enabled = ( collectionType == Akobackit::UserFolder );
  if(a_ctFolDelete->isEnabled() != enabled) {
    a_ctFolDelete->setEnabled(enabled);
    a_ctFolRename->setEnabled(enabled);
  }

  enabled = isFolder && ( collectionType != Akobackit::RootFolder );
  if ( a_ctFolEmpty->isEnabled() != enabled ) {
    a_ctFolEmpty->setEnabled(enabled);
    a_ctFolMboxImport->setEnabled(enabled);
    a_ctFolMboxExport->setEnabled(enabled);
  }
}


void KNMainWidget::slotCollectionRenamed(QTreeWidgetItem *i)
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotCollectionRenamed(QListViewItem *i)";

  if (i) {
    static_cast<KNCollectionViewItem*>( i )->collection()->setName( i->text( 0 ) );
    updateCaption();
    a_rtManager->updateStatusString();
    if ( static_cast<KNCollectionViewItem*>( i )->collection()->type() == KNCollection::CTnntpAccount ) {
      a_ccManager->accountRenamed( boost::static_pointer_cast<KNNntpAccount>( static_cast<KNCollectionViewItem*>( i )->collection() ) );
    }
    disableAccels(false);
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}

void KNMainWidget::slotOpenArticle( const Akonadi::Item &item )
{
#if 0
  if (item) {
    KNArticle::Ptr art = (static_cast<KNHdrViewItem*>(item))->art;

    if ((art->type()==KNArticle::ATlocal) && ((f_olManager->currentFolder()==f_olManager->outbox())||
                                               (f_olManager->currentFolder()==f_olManager->drafts()))) {
      a_rtFactory->edit( boost::static_pointer_cast<KNLocalArticle>( art ) );
    } else {
      if ( !ArticleWindow::raiseWindowForArticle( art ) ) {
        ArticleWindow *w = new ArticleWindow( art );
        w->show();
      }
    }
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotHdrViewSortingChanged(int i)
{
#if 0
  a_ctArtSortHeaders->setCurrentItem(i);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotNetworkActive(bool b)
{
#if 0
  a_ctNetCancel->setEnabled(b);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


//------------------------------ <Actions> --------------------------------


void KNMainWidget::slotNavReadThrough()
{
  kDebug();
  if ( !mArticleViewer->atBottom() ) {
    mArticleViewer->scrollNext();
  } else if( Akobackit::manager()->groupManager()->isGroup( mMessageList->currentCollection() ) ) {
    nextUnreadArticle();
  }
}


void KNMainWidget::slotAccProperties()
{
  kDebug();
  const Akonadi::Collection collection = mCollectionWidget->selectedCollection();
  const Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance(  collection.resource() );
  if ( !agent.isValid() ) {
    return;
  }

  Akobackit::NntpAccountManager *am = Akobackit::manager()->accountManager();
  am->editAccount( am->account( agent ), this );
#if 0
  // FIXME editAccount() is async now!
  updateCaption();
  a_rtManager->updateStatusString();
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotAccRename()
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotAccRename()";
  if(a_ccManager->currentAccount()) {
//     disableAccels(true);   // hack: global accels break the inplace renaming
    c_olView->editItem( a_ccManager->currentAccount()->listItem(), c_olView->labelColumnIndex() );
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotAccSubscribe()
{
  kDebug();
  const Akonadi::Collection collection = mCollectionWidget->selectedCollection();
  const Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance( collection.resource() );
  if ( !agent.isValid() ) {
    return;
  }

  Akobackit::NntpAccountManager *am = Akobackit::manager()->accountManager();
  Akobackit::GroupManager *gm = Akobackit::manager()->groupManager();
  gm->showSubscriptionDialog( am->account( agent ), this );
}


void KNMainWidget::slotAccExpireAll()
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotAccExpireAll()";
  if(a_ccManager->currentAccount())
    g_rpManager->expireAll(a_ccManager->currentAccount());
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotAccGetNewHdrs()
{
  kDebug();
  const Akonadi::Collection collection = mCollectionWidget->selectedCollection();
  const Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance( collection.resource() );
  if ( !agent.isValid() ) {
    return;
  }

  Akobackit::NntpAccountManager *am = Akobackit::manager()->accountManager();
  Akobackit::GroupManager *gm = Akobackit::manager()->groupManager();
  gm->fetchNewHeaders( am->account( agent ) );
}



void KNMainWidget::slotAccDelete()
{
  kDebug();
  const Akonadi::Collection collection = mCollectionWidget->selectedCollection();
  const Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance(  collection.resource() );
  if ( !agent.isValid() ) {
    return;
  }
  Akobackit::NntpAccountManager *am = Akobackit::manager()->accountManager();
  am->editAccount( am->account( agent ), this );
}

void KNMainWidget::slotAccGetNewHdrsAll()
{
  Akobackit::manager()->groupManager()->fetchNewHeaders();
}

void KNMainWidget::slotAccPostNewArticle()
{
  kDebug();
  a_rtFactory->createPosting( mCollectionWidget->selectedCollection() );
}


void KNMainWidget::slotGrpProperties()
{
  kDebug();
  const Akonadi::Collection collection = mCollectionWidget->selectedCollection();
  if ( collection.isValid() ) {
    Akobackit::GroupManager *gm = Akobackit::manager()->groupManager();
    if ( gm->isGroup( collection ) ) {
      gm->editGroup( gm->group( collection ), this );
    }
  }

  kDebug() << "AKONADI PORT: does the following works (I guess no: edition is not blocking)?";
  updateCaption();
  a_rtManager->updateStatusString();
}


void KNMainWidget::slotGrpRename()
{
#if 0
  kDebug(5003) <<"slotGrpRename()";
  if(g_rpManager->currentGroup()) {
//     disableAccels(true);   // hack: global accels break the inplace renaming
    c_olView->editItem( g_rpManager->currentGroup()->listItem(),  c_olView->labelColumnIndex() );
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotGrpGetNewHdrs()
{
  kDebug();
  Akobackit::GroupManager *grpManager = Akobackit::manager()->groupManager();

  const Akonadi::Collection col = collectionView()->selectedCollection();
  if ( grpManager->isGroup( col ) ) {
    const Group::Ptr group = grpManager->group( col );
    grpManager->fetchNewHeaders( group );
  }
}


void KNMainWidget::slotGrpExpire()
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotGrpExpire()";
  if(g_rpManager->currentGroup())
    g_rpManager->expireGroupNow(g_rpManager->currentGroup());
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotGrpUnsubscribe()
{
  kDebug();
  const Akonadi::Collection collection = mCollectionWidget->selectedCollection();
  Akobackit::GroupManager *gm = Akobackit::manager()->groupManager();
  if ( gm->isGroup( collection ) ) {
    Group::Ptr group = gm->group( collection );
    int response = KMessageBox::questionYesNo( this,
                                               i18n( "Do you really want to unsubscribe from %1?", group->name() ),
                                               QString(),
                                               KGuiItem( i18nc( "@action:button", "Unsubscribe" ) ),
                                               KStandardGuiItem::cancel() );
    if ( KMessageBox::Yes == response ) {
      gm->unsubscribeGroup( group );
    }
  }
}


void KNMainWidget::slotGrpSetAllRead()
{
  mMessageList->markAll( Akonadi::MessageStatus::statusRead() );
  if ( knGlobals.settings()->markAllReadGoNext() ) {
    mCollectionWidget->nextGroup();
  }
}


void KNMainWidget::slotGrpSetAllUnread()
{
  mMessageList->markAll( Akonadi::MessageStatus::statusUnread() );
}

void KNMainWidget::slotGrpSetUnread()
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotGrpSetUnread()";
  int groupLength = g_rpManager->currentGroup()->length();

  bool ok = false;
  int res = KInputDialog::getInteger(
                i18n( "Mark Last as Unread" ),
                i18n( "Enter how many articles should be marked unread:" ), groupLength, 1, groupLength, 1, &ok, this );
  if ( ok )
    a_rtManager->setAllRead( false, res );
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}

void KNMainWidget::slotFolNew()
{
  kDebug();

  Akobackit::FolderManager *folderManager = Akobackit::manager()->folderManager();
  const Akonadi::Collection rootFolder = folderManager->rootFolder();
  slotFolNewChild( rootFolder );
}


void KNMainWidget::slotFolNewChild( const Akonadi::Collection &parent )
{
  kDebug();
  Akobackit::FolderManager *folderManager = Akobackit::manager()->folderManager();
  Akonadi::Collection parentFolder;
  if ( folderManager->isFolder( parent ) ) {
    parentFolder = parent;
  } else {
    parentFolder = mCollectionWidget->selectedCollection();
    if ( !folderManager->isFolder( parentFolder) ) {
      return;
    }
  }

  QString name = i18nc( "Default name of a newly created folder", "New folder" );
  if ( folderManager->hasChild( parentFolder, name ) ) {
    name = i18nc( "Default name of a newly created folder; %1=number", "New folder %1" );
    int i = 1;
    while ( folderManager->hasChild( parentFolder, name.arg( i ) ) ) {
      ++i;
    }
    name = name.arg( i );
  }
  Akobackit::manager()->folderManager()->createNewFolder( parentFolder, name );
  connect( Akobackit::manager()->folderManager(), SIGNAL( folderCreated( const Akonadi::Collection & ) ),
            this, SLOT( slotFolRename( const Akonadi::Collection & ) ),
            Qt::UniqueConnection );
}


void KNMainWidget::slotFolDelete()
{
  kDebug();

  const Akonadi::Collection f = mCollectionWidget->selectedCollection();
  if ( !f.isValid() ) {
    return;
  }
  Akobackit::CollectionType type = Akobackit::manager()->type( f );
  if ( type != Akobackit::UserFolder ) {
    return;
  }

  int res = KMessageBox::warningContinueCancel( this, i18n( "Do you really want to delete this folder and all its children?" ),
                                                "", KGuiItem( i18nc( "@action:button", "&Delete" ), "edit-delete" ) );
  if( KMessageBox::Continue == res ) {
    Akobackit::manager()->folderManager()->removeFolder( f );
  }
}


void KNMainWidget::slotFolRename( const Akonadi::Collection &col )
{
  kDebug() << col.id() << col.name();

  Akobackit::FolderManager *folderManager = Akobackit::manager()->folderManager();
  Akonadi::Collection folder;
  if ( folderManager->isFolder( col ) ) {
    folder = col;
  } else {
    const Akonadi::Collection selection = mCollectionWidget->selectedCollection();
    if ( folderManager->isFolder( selection ) ) {
      folder = selection;
    }
  }

  if ( !folder.isValid() ) {
    return;
  }

  mCollectionWidget->renameCollection( folder );
}


void KNMainWidget::slotFolEmpty()
{
  kDebug();
  const Akonadi::Collection col = mCollectionWidget->selectedCollection();
  Akobackit::FolderManager *folderManager = Akobackit::manager()->folderManager();
  if ( !folderManager->isFolder( col ) ) {
    return;
  }

  int res = KMessageBox::warningContinueCancel( this, i18n( "Do you really want to delete all articles in %1?", col.name() ),
                                                "", KGuiItem( i18nc( "@action:button", "&Delete" ), "edit-delete" ) );
  if( KMessageBox::Continue == res ) {
    folderManager->emptyFolder( col );
  }
}


void KNMainWidget::slotFolMBoxImport()
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotFolMBoxImport()";
  if(f_olManager->currentFolder() && !f_olManager->currentFolder()->isRootFolder()) {
     f_olManager->importFromMBox(f_olManager->currentFolder());
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotFolMBoxExport()
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotFolMBoxExport()";
  if(f_olManager->currentFolder() && !f_olManager->currentFolder()->isRootFolder()) {
    f_olManager->exportToMBox(f_olManager->currentFolder());
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotArtSortHeaders(int i)
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotArtSortHeaders(int i)";
  h_drView->setSorting( i );
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotArtSortHeadersKeyb()
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotArtSortHeadersKeyb()";

  int newCol = KNHelper::selectDialog(this, i18n("Select Sort Column"), a_ctArtSortHeaders->items(), a_ctArtSortHeaders->currentItem());
  if (newCol != -1)
    h_drView->setSorting( newCol );
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotArtSearch()
{
  kDebug(5003) <<"KNMainWidget::slotArtSearch()";
  a_rtManager->search();
}


void KNMainWidget::slotArtToggleThread()
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotArtToggleThread()";
  if( mArticleViewer->article() && mArticleViewer->article()->listItem()->isExpandable() ) {
    bool o = !(mArticleViewer->article()->listItem()->isOpen());
    mArticleViewer->article()->listItem()->setOpen( o );
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotArtToggleShowThreads()
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotArtToggleShowThreads()";
  if(g_rpManager->currentGroup()) {
    knGlobals.settings()->setShowThreads( !knGlobals.settings()->showThreads() );
    a_rtManager->showHdrs(true);
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotArtSetArtRead()
{
  kDebug();
  if ( !Akobackit::manager()->groupManager()->isGroup( mMessageList->currentCollection() ) ) {
    return;
  }

  mMessageList->markSelection( Akonadi::MessageStatus::statusRead() );
}


void KNMainWidget::slotArtSetArtUnread()
{
  kDebug();
  if ( !Akobackit::manager()->groupManager()->isGroup( mMessageList->currentCollection() ) ) {
    return;
  }

  mMessageList->markSelection( Akonadi::MessageStatus::statusUnread() );
}


void KNMainWidget::slotArtSetThreadRead()
{
  kDebug();
  if ( !Akobackit::manager()->groupManager()->isGroup( mMessageList->currentCollection() ) ) {
    return;
  }

  mMessageList->markThread( Akonadi::MessageStatus::statusRead() );

  if ( mMessageList->currentMessageItem() ) {
    if ( knGlobals.settings()->markThreadReadCloseThread() )
      mMessageList->closeCurrentThread();
    if ( knGlobals.settings()->markThreadReadGoNext() )
      nextUnreadThread();
  }
}


void KNMainWidget::slotArtSetThreadUnread()
{
  kDebug();
  if ( !Akobackit::manager()->groupManager()->isGroup( mMessageList->currentCollection() ) ) {
    return;
  }

  mMessageList->markThread( Akonadi::MessageStatus::statusUnread() );
}


void KNMainWidget::slotScoreEdit()
{
  kDebug(5003) <<"KNMainWidget::slotScoreEdit()";
  s_coreManager->configure();
}


void KNMainWidget::slotReScore()
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotReScore()";
  if( !g_rpManager->currentGroup() )
    return;

  g_rpManager->currentGroup()->scoreArticles(false);
  a_rtManager->showHdrs(true);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotScoreLower()
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotScoreLower() start";
  if( !g_rpManager->currentGroup() )
    return;

  if ( mArticleViewer->article() && mArticleViewer->article()->type() == KNArticle::ATremote ) {
    KNRemoteArticle::Ptr ra = boost::static_pointer_cast<KNRemoteArticle>( mArticleViewer->article() );
    s_coreManager->addRule(KNScorableArticle(ra), g_rpManager->currentGroup()->groupname(), -10);
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotScoreRaise()
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotScoreRaise() start";
  if( !g_rpManager->currentGroup() )
    return;

  if ( mArticleViewer->article() && mArticleViewer->article()->type() == KNArticle::ATremote ) {
    KNRemoteArticle::Ptr ra = boost::static_pointer_cast<KNRemoteArticle>( mArticleViewer->article() );
    s_coreManager->addRule(KNScorableArticle(ra), g_rpManager->currentGroup()->groupname(), +10);
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotArtToggleIgnored()
{
  kDebug();
  if ( !Akobackit::manager()->groupManager()->isGroup( mMessageList->currentCollection() ) ) {
    return;
  }

  bool revert = mMessageList->toggleThread( Akonadi::MessageStatus::statusIgnored() );
#if 0
  a_rtManager->rescoreArticles(l);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif

  if ( mMessageList->currentMessageItem() && !revert ) {
    if ( knGlobals.settings()->ignoreThreadCloseThread() )
      mMessageList->closeCurrentThread();
    if ( knGlobals.settings()->ignoreThreadGoNext() )
      mMessageList->nextUnreadThread();
  }
}


void KNMainWidget::slotArtToggleWatched()
{
  kDebug();
  if ( !Akobackit::manager()->groupManager()->isGroup( mMessageList->currentCollection() ) ) {
    return;
  }

  mMessageList->toggleThread( Akonadi::MessageStatus::statusWatched() );
#if 0
  a_rtManager->rescoreArticles(l);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotArtOpenNewWindow()
{
  kDebug(5003) <<"KNMainWidget::slotArtOpenNewWindow()";

  if( mArticleViewer->article() ) {
    if ( !ArticleWindow::raiseWindowForArticle( mArticleViewer->article() ) ) {
      ArticleWindow *win = new ArticleWindow( mArticleViewer->article() );
      win->show();
    }
  }
}


void KNMainWidget::slotArtSendOutbox()
{
  kDebug(5003) <<"KNMainWidget::slotArtSendOutbox()";
  a_rtFactory->sendOutbox();
}


void KNMainWidget::slotArtDelete()
{
  kDebug();
  Akobackit::AkoManager *manager = Akobackit::manager();
  const bool isFolder = manager->folderManager()->isFolder( mCollectionWidget->selectedCollection() );
  if ( !isFolder ) {
    return;
  }

  mMessageList->deleteSelection();
}


void KNMainWidget::slotArtSendNow()
{
  kDebug();
  const bool isFolder = Akobackit::manager()->folderManager()->isFolder( mCollectionWidget->selectedCollection() );
  if ( !isFolder ) {
    return;
  }

  const LocalArticle::List selection = mMessageList->selectionAsArticleList();
  if ( !selection.isEmpty() ) {
    a_rtFactory->sendArticles( selection, true );
  }
}


void KNMainWidget::slotArtEdit()
{
  kDebug();
  if ( mArticleViewer->article() && mArticleViewer->article()->type() == RemoteArticle::ATlocal ) {
    a_rtFactory->edit( boost::static_pointer_cast<LocalArticle>( mArticleViewer->article() ) );
  }
}


void KNMainWidget::slotNetCancel()
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotNetCancel()";
  knGlobals.scheduler()->cancelJobs();
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNMainWidget::slotFetchArticleWithID()
{
#if 0
  kDebug(5003) <<"KNMainWidget::slotFetchArticleWithID()";
  if( !g_rpManager->currentGroup() )
    return;

  FetchArticleIdDlg *dlg = new FetchArticleIdDlg( this );
  dlg->setObjectName( "messageid" );

  if (dlg->exec()) {
    QString id = dlg->messageId().simplified();
    if ( id.indexOf( QRegExp("*@*", Qt::CaseInsensitive, QRegExp::Wildcard) ) != -1 ) {
      if ( id.indexOf( QRegExp("<*>", Qt::CaseInsensitive, QRegExp::Wildcard) ) == -1 )   // add "<>" when necessary
        id = QString("<%1>").arg(id);

      if ( !ArticleWindow::raiseWindowForArticle( id.toLatin1() ) ) { //article not yet opened
        KNRemoteArticle::Ptr a( new KNRemoteArticle( g_rpManager->currentGroup() ) );
        a->messageID()->from7BitString(id.toLatin1());
        ArticleWindow *awin = new ArticleWindow( a );
        awin->show();
      }
    }
  }

  KNHelper::saveWindowSize("fetchArticleWithID",dlg->size());
  delete dlg;
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}

void KNMainWidget::slotSettings()
{
  c_fgManager->configure();
}

KActionCollection* KNMainWidget::actionCollection() const
{
  return m_GUIClient->actionCollection();
}


//--------------------------------


KNode::FetchArticleIdDlg::FetchArticleIdDlg( QWidget *parent ) :
    KDialog( parent )
{
  setCaption( i18n("Fetch Article with ID") );
  setButtons( KDialog::Ok | KDialog::Cancel );
  setModal( true );
  KHBox *page = new KHBox( this );
  setMainWidget( page );

  QLabel *label = new QLabel(i18n("&Message-ID:"),page);
  edit = new KLineEdit(page);
  label->setBuddy(edit);
  edit->setFocus();
  enableButtonOk( false );
  setButtonText( KDialog::Ok, i18n("&Fetch") );
  connect( edit, SIGNAL(textChanged( const QString & )), this, SLOT(slotTextChanged(const QString & )));
  KNHelper::restoreWindowSize("fetchArticleWithID", this, QSize(325,66));
}

QString KNode::FetchArticleIdDlg::messageId() const
{
    return edit->text();
}

void KNode::FetchArticleIdDlg::slotTextChanged(const QString &_text )
{
    enableButtonOk( !_text.isEmpty() );
}


////////////////////////////////////////////////////////////////////////
//////////////////////// D-Bus implementation
// Move to the next article
void KNMainWidget::nextArticle()
{
  mMessageList->nextArticle();
}

// Move to the previous article
void KNMainWidget::previousArticle()
{
  mMessageList->previousArticle();
}

// Move to the next unread article
void KNMainWidget::nextUnreadArticle()
{
  mMessageList->nextUnreadArticle();
}

// Move to the next unread thread
void KNMainWidget::nextUnreadThread()
{
  mMessageList->nextUnreadThread();
}

// Move to the next group
void KNMainWidget::nextGroup()
{
  mCollectionWidget->nextGroup();
}

// Move to the previous group
void KNMainWidget::previousGroup()
{
  mCollectionWidget->previousGroup();
}

void KNMainWidget::fetchHeaders()
{
  // Simply call the slot
  slotAccGetNewHdrs();
}

void KNMainWidget::expireArticles()
{
  slotAccExpireAll();
}

// Open the editor to post a new article in the selected group
void KNMainWidget::postArticle()
{
  slotAccPostNewArticle();
}

// Fetch the new headers in the selected groups
void KNMainWidget::fetchHeadersInCurrentGroup()
{
  slotGrpGetNewHdrs();
}

// Expire the articles in the current group
void KNMainWidget::expireArticlesInCurrentGroup()
{
  slotGrpExpire();
}

// Mark all the articles in the current group as read
void KNMainWidget::markAllAsRead()
{
  slotGrpSetAllRead();
}

// Mark all the articles in the current group as unread
void KNMainWidget::markAllAsUnread()
{
  slotGrpSetAllUnread();
}

// Mark the current article as read
void KNMainWidget::markAsRead()
{
  slotArtSetArtRead();
}

// Mark the current article as unread
void KNMainWidget::markAsUnread()
{
  slotArtSetArtUnread();
}

// Mark the current thread as read
void KNMainWidget::markThreadAsRead()
{
  slotArtSetThreadRead();
}

// Mark the current thread as unread
void KNMainWidget::markThreadAsUnread()
{
  slotArtSetThreadUnread();
}

// Send the pending articles
void KNMainWidget::sendPendingMessages()
{
  slotArtSendOutbox();
}

// Delete the current article
void KNMainWidget::deleteArticle()
{
  slotArtDelete();
}

// Send the current article
void KNMainWidget::sendNow()
{
  slotArtSendNow();
}

// Edit the current article
void KNMainWidget::editArticle()
{
  slotArtEdit();
}

bool KNMainWidget::handleCommandLine()
{
  bool doneSomething = false;
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  if (args->count()>0) {
    KUrl url=args->url(0);    // we take only one URL
    openURL(url);
    doneSomething = true;
  }
  args->clear();
  return doneSomething;
}

//////////////////////// end D-Bus implementation
////////////////////////////////////////////////////////////////////////

#include "knmainwidget.moc"
