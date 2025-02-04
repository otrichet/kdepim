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
#ifndef KNMAINWIDGET_H
#define KNMAINWIDGET_H

#include "knode_export.h"
#include "legacy_include.h"
#include "resource.h"

#include <Akonadi/Collection>
#include <kdialog.h>
#include <kvbox.h>
#include <QList>


namespace Akonadi {
  class Item;
}
class QSplitter;
class QTreeWidget;
class QTreeWidgetItem;
class KAction;
class KToggleAction;
class KSelectAction;
class KSqueezedTextLabel;
class KLineEdit;
class KXMLGUIClient;
class KNCollectionView;
class KNConfigManager;
class KNArticleManager;
class KNArticleFactory;
class KNFilterManager;
class KNScoringManager;
class KNFilterSelectAction;
namespace KNode {
  class ArticleWidget;
  namespace CollectionTree {
    class Widget;
  }
  namespace MessageListView {
    class Widget;
  }
}
class KActionCollection;


/** This is the central part of the KNode GUI. */
class KNODE_EXPORT KNMainWidget : public KVBox
{
  Q_OBJECT
public:
  KNMainWidget( KXMLGUIClient *client, QWidget* parent );
  ~KNMainWidget();

  /** exit */
  bool queryClose();
  void prepareShutdown();

  //GUI
  void setStatusMsg(const QString& = QString(), int id=SB_MAIN);
  void setStatusHelpMsg(const QString& text);
  /**
   * Emit signalCaptionChangeRequest() to update the main KNode window title.
   */
  void updateCaption();
  void disableAccels(bool b=true);

  /** useful default value */
  virtual QSize sizeHint() const;

  /** handle URL given as command-line argument */
  void openURL(const KUrl &url);
  void openURL(const QString &url);

  /** update fonts and colors */
  void configChanged();

  /** Returns the collection tree widget. */
  KNode::CollectionTree::Widget * collectionView() const { return mCollectionWidget; }
  /** Returns the article viewer. */
  KNode::ArticleWidget* articleViewer() const     { return mArticleViewer; }
  KSqueezedTextLabel*  statusBarLabelGroup() const { return s_tatusGroup; }
  KSqueezedTextLabel*  statusBarLabelFilter() const { return s_tatusFilter; }
  public slots: //The dcop interface
  // Implementation of KNodeIface
  /* Navigation */
  /// Move to the next article
  Q_SCRIPTABLE void nextArticle();
  /// Move to the previous article
  Q_SCRIPTABLE void previousArticle();
  /// Move to the next unread article
  Q_SCRIPTABLE void nextUnreadArticle();
  /// Move to the next unread thread
  Q_SCRIPTABLE void nextUnreadThread();
  /// Move to the next group
  Q_SCRIPTABLE void nextGroup();
  /// Move to the previous group
  Q_SCRIPTABLE void previousGroup();

  /* Group options */
  /// Open the editor to post a new article in the selected group
  Q_SCRIPTABLE void postArticle();
  /// Fetch the new headers in the selected groups
  Q_SCRIPTABLE void fetchHeadersInCurrentGroup();
  /// Expire the articles in the current group
  Q_SCRIPTABLE void expireArticlesInCurrentGroup();
  /// Mark all the articles in the current group as read
  Q_SCRIPTABLE void markAllAsRead();
  /// Mark all the articles in the current group as unread
  Q_SCRIPTABLE void markAllAsUnread();

  /* Header view */
  /// Mark the current article as read
  Q_SCRIPTABLE void markAsRead();
  /// Mark the current article as unread
  Q_SCRIPTABLE void markAsUnread();
  /// Mark the current thread as read
  Q_SCRIPTABLE void markThreadAsRead();
  /// Mark the current thread as unread
  Q_SCRIPTABLE void markThreadAsUnread();

  /* Articles */

  /// Send the pending articles
  Q_SCRIPTABLE void sendPendingMessages();
  /// Delete the current article
  Q_SCRIPTABLE void deleteArticle();
  /// Send the current article
  Q_SCRIPTABLE void sendNow();
  /// Edit the current article
  Q_SCRIPTABLE void editArticle();
  /// Fetch all the new article headers
  Q_SCRIPTABLE void fetchHeaders();
  /// Expire articles in all groups
  Q_SCRIPTABLE void expireArticles();

  /* Kontact integration */
  /// Process command-line options
  Q_SCRIPTABLE bool handleCommandLine();

  //end dcop interface
signals:
  void signalCaptionChangeRequest( const QString& );

protected:

  KActionCollection* actionCollection() const;
  void initActions();
  void initStatusBar();

  /** checks if run for the first time, sets some global defaults (email configuration) */
  bool firstStart();

  void readOptions();
  void saveOptions();

  bool requestShutdown();

  /** update appearance */
  virtual void fontChange( const QFont & );
  virtual void paletteChange ( const QPalette & );

  bool eventFilter(QObject *, QEvent *);

  //GUI
  KNode::ArticleWidget *mArticleViewer;
  KNode::MessageListView::Widget *mMessageList;

  //Core
  KNConfigManager   *c_fgManager;
  KNArticleManager  *a_rtManager;
  KNArticleFactory  *a_rtFactory;
  KNFilterManager   *f_ilManager;
  KNScoringManager  *s_coreManager;

protected slots:
  //listview slots
  void slotArticleSelected( const Akonadi::Item &item );
  void slotArticleSelectionChanged();

  /**
   * Called when the selected collection changed.
   * @param col the new selected collection.
   */
  void slotCollectionSelected( const Akonadi::Collection &col = Akonadi::Collection() );
  /** Called when a collection is renamed. */
  void slotCollectionRenamed( QTreeWidgetItem *i );
  /** Open selected article in own composer/reader window */
  void slotOpenArticle( const Akonadi::Item &item );
  void slotHdrViewSortingChanged(int i);

  //network slots
  void slotNetworkActive(bool b);

  //---------------------------------- <Actions> ----------------------------------

protected:

  //navigation
  KAction   *a_ctNavNextArt,
    *a_ctNavPrevArt,
    *a_ctNavNextUnreadArt,
    *a_ctNavNextUnreadThread,
    *a_ctNavNextGroup,
    *a_ctNavPrevGroup,
    *a_ctNavReadThrough;

  //collection-view - accounts
  KAction   *a_ctAccProperties,
    *a_ctAccRename,
    *a_ctAccSubscribe,
    *a_ctAccExpireAll,
    *a_ctAccGetNewHdrs,
    *a_ctAccGetNewHdrsAll,
    *a_ctAccDelete,
    *a_ctAccPostNewArticle;

  //collection-view - groups
  KAction   *a_ctGrpProperties,
    *a_ctGrpRename,
    *a_ctGrpGetNewHdrs,
    *a_ctGrpExpire,
    *a_ctGrpUnsubscribe,
    *a_ctGrpSetAllRead,
    *a_ctGrpSetAllUnread,
    *a_ctGrpSetUnread;

  //collection-view - folder
  KAction   *a_ctFolNew,
    *a_ctFolNewChild,
    *a_ctFolDelete,
    *a_ctFolRename,
    *a_ctFolEmpty,
    *a_ctFolMboxImport,
    *a_ctFolMboxExport;

  //header-view - list-handling
  KSelectAction         *a_ctArtSortHeaders;
  KNFilterSelectAction  *a_ctArtFilter;
  KAction               *a_ctArtSortHeadersKeyb,
    *a_ctArtFilterKeyb,
    *a_ctArtSearch,
    *a_ctArtRefreshList,
    *a_ctArtCollapseAll,
    *a_ctArtExpandAll,
    *a_ctArtToggleThread;
  KToggleAction         *a_ctArtToggleShowThreads;

  //header-view - remote articles
  KAction *a_ctArtSetArtRead,
    *a_ctArtSetArtUnread,
    *a_ctArtSetThreadRead,
    *a_ctArtSetThreadUnread,
    *a_ctArtOpenNewWindow;

  // scoring
  KAction *a_ctScoresEdit,
    *a_ctReScore,
    *a_ctScoreLower,
    *a_ctScoreRaise,
    *a_ctArtToggleIgnored,
    *a_ctArtToggleWatched;

  //header-view local articles
  KAction *a_ctArtSendOutbox,
    *a_ctArtDelete,
    *a_ctArtSendNow,
    *a_ctArtEdit;

  //network
  KAction *a_ctNetCancel;

  KAction *a_ctFetchArticleWithID;

  // settings menu
  KToggleAction *a_ctToggleQuickSearch;

protected slots:
  void slotNavReadThrough();

  void slotAccProperties();
  void slotAccRename();
  void slotAccSubscribe();
  void slotAccExpireAll();
  void slotAccGetNewHdrs();
  void slotAccGetNewHdrsAll();
  void slotAccDelete();
  void slotAccPostNewArticle();

  void slotGrpProperties();
  void slotGrpRename();
  void slotGrpGetNewHdrs();
  void slotGrpExpire();
  void slotGrpUnsubscribe();
  void slotGrpSetAllRead();
  void slotGrpSetAllUnread();
  void slotGrpSetUnread();

  /**
   * Create a new folder under the root folder and request a name.
   */
  void slotFolNew();
  /**
   * Create a new folder under the folder f if is it valid
   * or under the currently selected folder.
   */
  void slotFolNewChild( const Akonadi::Collection &parent = Akonadi::Collection() );
  void slotFolDelete();
  /**
   * Request a new name of a folder to the user.
   * @param col The folder to rename. If the folder is valid, it is
   * selected and a new name is requested, otherwise if the currently
   * selected collection is a folder, it is renamed.
   */
  void slotFolRename( const Akonadi::Collection &col = Akonadi::Collection() );
  void slotFolEmpty();
  void slotFolMBoxImport();
  void slotFolMBoxExport();

  void slotArtSortHeaders(int i);
  void slotArtSortHeadersKeyb();
  void slotArtSearch();
  void slotArtToggleThread();
  void slotArtToggleShowThreads();

  void slotArtSetArtRead();
  void slotArtSetArtUnread();
  void slotArtSetThreadRead();
  void slotArtSetThreadUnread();

  void slotScoreEdit();
  void slotReScore();
  void slotScoreLower();
  void slotScoreRaise();
  void slotArtToggleIgnored();
  void slotArtToggleWatched();

  void slotArtOpenNewWindow();
  void slotArtSendOutbox();
  void slotArtDelete();
  void slotArtSendNow();
  void slotArtEdit();

  void slotNetCancel();

  void slotFetchArticleWithID();

  void slotSettings();

  //--------------------------- </Actions> -----------------------------

private:
  KSqueezedTextLabel *s_tatusGroup; // widget used in the statusBar() for the group status
  KSqueezedTextLabel *s_tatusFilter;
  KXMLGUIClient *m_GUIClient;
  QSplitter *mPrimarySplitter, *mSecondSplitter;
  KNode::CollectionTree::Widget *mCollectionWidget;
};


namespace KNode {

/** Dialog to request a message ID. */
class  FetchArticleIdDlg : public KDialog
{
    Q_OBJECT
public:
    FetchArticleIdDlg( QWidget *parent );
    QString messageId() const;

protected slots:
    void slotTextChanged(const QString & );
protected:
    KLineEdit *edit;
};

}

#endif
