/******************************************************************************
 *
 * KMail Folder Selection Tree Widget
 *
 * Copyright (c) 1997-1998 Stefan Taferner <taferner@kde.org>
 * Copyright (c) 2004-2005 Carsten Burghardt <burghardt@kde.org>
 * Copyright (c) 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *****************************************************************************/

#include "folderselectiontreewidget.h"
#include "kmfoldertree.h"
#include "kmfolder.h"

#include <kmenu.h>
#include <kiconloader.h>
#include <kconfiggroup.h>

namespace KMail {

class FolderSelectionTreeWidgetItem : public KPIM::FolderTreeWidgetItem
{
public:
  FolderSelectionTreeWidgetItem( KPIM::FolderTreeWidget * listView )
    : KPIM::FolderTreeWidgetItem( listView ), mFolder( 0 ) {};

  FolderSelectionTreeWidgetItem( KPIM::FolderTreeWidgetItem * listViewItem )
    : KPIM::FolderTreeWidgetItem( listViewItem ), mFolder( 0 ) {};

public:
  void setFolder( KMFolder * folder )
    { mFolder = folder; };

  KMFolder * folder()
    { return mFolder; };

private:
  KMFolder * mFolder;

};


FolderSelectionTreeWidget::FolderSelectionTreeWidget( QWidget * parent , KMFolderTree * folderTree )
  : KPIM::FolderTreeWidget( parent ), mFolderTree( folderTree )
{
  setSelectionMode( QTreeWidget::SingleSelection );

  mNameColumnIndex = addColumn( i18n( "Folder" ) );
  mPathColumnIndex = addColumn( i18n( "Path" ) );

  setContextMenuPolicy( Qt::CustomContextMenu );
  connect(
      this, SIGNAL( customContextMenuRequested( const QPoint & ) ),
      this, SLOT( slotContextMenuRequested( const QPoint & ) )
    );
}

void FolderSelectionTreeWidget::recursiveReload( KMFolderTreeItem *fti , FolderSelectionTreeWidgetItem *parent )
{
  // search folders are never shown
  if ( fti->protocol() == KFolderTreeItem::Search )
    return;

  // imap folders?
  if ( fti->protocol() == KFolderTreeItem::Imap && !mLastShowImapFolders )
    return;

  // the outbox?
  if ( fti->type() == KFolderTreeItem::Outbox && !mLastShowOutbox )
    return;

  // top level
  FolderSelectionTreeWidgetItem *item = parent ? new FolderSelectionTreeWidgetItem( parent ) : new FolderSelectionTreeWidgetItem( this );

  // Build the path (ParentItemPath/CurrentItemName)
  QString path;
  if( parent )
      path = parent->text( mPathColumnIndex ) + "/";
  path += fti->text( 0 );

  item->setText( mNameColumnIndex , fti->text( 0 ) );
  item->setText( mPathColumnIndex , path );
  item->setProtocol( (KPIM::FolderTreeWidgetItem::Protocol)( fti->protocol() ) );
  item->setFolderType( (KPIM::FolderTreeWidgetItem::FolderType)( fti->type() ) );
  QPixmap pix = fti->normalIcon(KIconLoader::SizeSmall);
  item->setIcon( mNameColumnIndex , pix.isNull() ? SmallIcon( "folder" ) : QIcon( pix ) );
  item->setExpanded( true );

  // Make items without folders and readonly items unselectable
  // if we're told so
  if ( mLastMustBeReadWrite && ( !fti->folder() || fti->folder()->isReadOnly() ) ) {
    item->setFlags( item->flags() & ~Qt::ItemIsSelectable );
  } else {
    if ( fti->folder() )
      item->setFolder( fti->folder() );
  }

  for (
       KMFolderTreeItem * child = static_cast<KMFolderTreeItem *>( fti->firstChild() );
       child;
       child = static_cast<KMFolderTreeItem *>( child->nextSibling() )
    )
      recursiveReload( child , item );
}

void FolderSelectionTreeWidget::reload( bool mustBeReadWrite, bool showOutbox,
                               bool showImapFolders, const QString& preSelection )
{
  mLastMustBeReadWrite = mustBeReadWrite;
  mLastShowOutbox = showOutbox;
  mLastShowImapFolders = showImapFolders;

  clear();

  QString selected = preSelection;
  if ( selected.isEmpty() && folder() )
    selected = folder()->idString();

  mFilter = "";

  for (
         KMFolderTreeItem * fti = static_cast<KMFolderTreeItem *>( mFolderTree->firstChild() ) ;
         fti;
         fti = static_cast<KMFolderTreeItem *>( fti->nextSibling() )
     )
     recursiveReload( fti , 0 );

  if ( preSelection.isEmpty() )
     return; // nothing more to do

  QTreeWidgetItemIterator it( this );
  while ( FolderSelectionTreeWidgetItem * fitem = static_cast<FolderSelectionTreeWidgetItem *>( *it ) )
  {
     if ( fitem->folder() ) {
       if ( fitem->folder()->idString() == preSelection ) {
          // found
          fitem->setSelected( true );
          scrollToItem( fitem );
          return;
       }
     }
     ++it;
  }

}

KMFolder * FolderSelectionTreeWidget::folder() const
{
  QTreeWidgetItem * item = currentItem();
  if ( item ) {
    if ( item->flags() & Qt::ItemIsSelectable )
      return static_cast<FolderSelectionTreeWidgetItem *>( item )->folder();
  }
  return 0;
}

void FolderSelectionTreeWidget::setFolder( KMFolder *folder )
{
  for ( QTreeWidgetItemIterator it( this ) ; *it ; ++it )
  {
    const KMFolder *fld = static_cast<FolderSelectionTreeWidgetItem *>( *it )->folder();
    if ( fld == folder )
    {
      ( *it )->setSelected( true );
      scrollToItem( *it );
      return;
    }
  }
}

void FolderSelectionTreeWidget::setFolder( const QString& idString )
{
  setFolder( kmkernel->findFolderById( idString ) );
}

void FolderSelectionTreeWidget::addChildFolder()
{
  const KMFolder *fld = folder();
  if ( fld ) {
    mFolderTree->addChildFolder( (KMFolder *) fld, parentWidget() );
    reload( mLastMustBeReadWrite, mLastShowOutbox, mLastShowImapFolders );
    setFolder( (KMFolder *) fld );
  }
}

void FolderSelectionTreeWidget::slotContextMenuRequested( const QPoint &p )
{
  QTreeWidgetItem * lvi = itemAt( p );

  if (!lvi)
    return;
  setCurrentItem( lvi );
  lvi->setSelected( true );

  const KMFolder * folder = static_cast<FolderSelectionTreeWidgetItem *>( lvi )->folder();
  if ( !folder || folder->noContent() || folder->noChildren() )
    return;

  KMenu *folderMenu = new KMenu;
  folderMenu->addTitle( folder->label() );
  folderMenu->addSeparator();
  folderMenu->addAction( KIcon("folder-new"),
                         i18n("&New Subfolder..."), this,
                         SLOT(addChildFolder()) );
  kmkernel->setContextMenuShown( true );
  folderMenu->exec ( viewport()->mapToGlobal( p ) , 0);
  kmkernel->setContextMenuShown( false );
  delete folderMenu;
  folderMenu = 0;
}

void FolderSelectionTreeWidget::applyFilter( const QString& filter )
{
  if ( filter.isEmpty() )
  {
    // Empty filter:
    // reset all items to enabled, visible, expanded and not selected
    QTreeWidgetItemIterator clean( this );
    while ( QTreeWidgetItem *item = *clean )
    {
      item->setDisabled( false );
      item->setHidden( false );
      item->setExpanded( true );
      item->setSelected( false );
      ++clean;
    }

    setColumnText( mPathColumnIndex , i18n("Path") );
    return;
  }

  // Not empty filter.
  // Reset all items to disabled, hidden, closed and not selected
  QTreeWidgetItemIterator clean( this );
  while ( QTreeWidgetItem *item = *clean )
  {
    item->setDisabled( true );
    item->setHidden( true );
    item->setExpanded( false );
    item->setSelected( false );
    ++clean;
  }

  // Now search...
  QList<QTreeWidgetItem *> lItems = findItems( mFilter , Qt::MatchContains | Qt::MatchRecursive , mPathColumnIndex );

  for( QList<QTreeWidgetItem *>::Iterator it = lItems.begin(); it != lItems.end(); ++it)
  {
    ( *it )->setDisabled( false );
    ( *it )->setHidden( false );
    // Open all the parents up to this item
    QTreeWidgetItem * p = ( *it )->parent();
    while( p )
    {
      p->setDisabled( false ); // we'd like to keep it disabled, but it disables the entire child tree :/
      p->setHidden( false );
      p->setExpanded( true );
      p = p->parent();
    }
  }


  // Iterate through the list to find the first selectable item
  QTreeWidgetItemIterator first ( this );
  while ( FolderSelectionTreeWidgetItem * item = static_cast< FolderSelectionTreeWidgetItem* >( *first ) )
  {
    if ( ( !item->isHidden() ) && ( !item->isDisabled() ) && ( item->flags() & Qt::ItemIsSelectable ) )
    {
      item->setSelected( true );
      scrollToItem( item );
      break;
    }
    ++first;
  }

  // Display and save the current filter
  if ( filter.length() > 0 )
    setColumnText( mPathColumnIndex , i18n("Path") + "  ( " + filter + " )" );
  else
    setColumnText( mPathColumnIndex , i18n("Path") );

}

void FolderSelectionTreeWidget::keyPressEvent( QKeyEvent *e )
{
  // Handle keyboard filtering.
  // Each key with text is appended to our search filter (which gets displayed
  // in the header for the Path column). Backpace removes text from the filter
  // while the del button clears the filter completly.

  QString s = e->text();

  switch(e->key())
  {
    case Qt::Key_Backspace:
      if ( mFilter.length() > 0 )
        mFilter.truncate( mFilter.length()-1 );
      applyFilter( mFilter );
      return;
    break;
    case Qt::Key_Delete:
      mFilter = "";
      applyFilter( mFilter);
      return;
    break;
    default:
     if ( !s.isEmpty() )
     {
       mFilter += s;
       applyFilter( mFilter );
       return;
     }
    break;
  }

  KPIM::FolderTreeWidget::keyPressEvent( e );
}

} // namespace KMail

#include "folderselectiontreewidget.moc"
