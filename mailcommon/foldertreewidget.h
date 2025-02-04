/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009, 2010 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef MAILCOMMON_FOLDERTREEWIDGET_H
#define MAILCOMMON_FOLDERTREEWIDGET_H

#include "mailcommon_export.h"
#include "readablecollectionproxymodel.h"

#include <QWidget>

#include <QAbstractItemView>
#include <akonadi/collection.h>

class KXMLGUIClient;
class QItemSelectionModel;
class KLineEdit;

namespace Akonadi {
  class StatisticsProxyModel;
}

namespace MailCommon {

class EntityCollectionOrderProxyModel;
class FolderTreeView;
class Kernel;

/**
 * This is the widget that shows the main folder tree in KMail.
 *
 * It consists of the view (FolderTreeView) and a search line.
 * Internally, several proxy models are used on top of a entity tree model.
 */
class MAILCOMMON_EXPORT FolderTreeWidget : public QWidget
{
  Q_OBJECT
public:
  enum TreeViewOption
  {
    None = 0,
    ShowUnreadCount = 1,
    UseLineEditForFiltering = 2,
    UseDistinctSelectionModel = 4,
    ShowCollectionStatisticAnimation = 8,
    DontKeyFilter = 16
  };
  Q_DECLARE_FLAGS( TreeViewOptions, TreeViewOption )

  FolderTreeWidget( QWidget *parent = 0, KXMLGUIClient *xmlGuiClient = 0,
                    TreeViewOptions options = (TreeViewOptions) (ShowUnreadCount|ShowCollectionStatisticAnimation),
                    ReadableCollectionProxyModel::ReadableCollectionOptions optReadableProxy = ReadableCollectionProxyModel::None );
  ~FolderTreeWidget();

  /**
   * The possible tooltip display policies.
   */
  enum ToolTipDisplayPolicy
  {
    DisplayAlways,           ///< Always display a tooltip when hovering over an item
    DisplayWhenTextElided,   ///< Display the tooltip if the item text is actually elided
    DisplayNever             ///< Nevery display tooltips
  };

  /**
   * The available sorting policies.
   */
  enum SortingPolicy
  {
    SortByCurrentColumn,      ///< Columns are clickable, sorting is by the current column
    SortByDragAndDropKey      ///< Columns are NOT clickable, sorting is done by drag and drop
  };



  void selectCollectionFolder( const Akonadi::Collection & col );

  void setSelectionMode( QAbstractItemView::SelectionMode mode );

  QAbstractItemView::SelectionMode selectionMode() const;

  QItemSelectionModel * selectionModel () const;

  QModelIndex currentIndex() const;

  Akonadi::Collection selectedCollection() const;

  Akonadi::Collection::List selectedCollections() const;

  FolderTreeView *folderTreeView() const;

  Akonadi::StatisticsProxyModel * statisticsProxyModel() const;

  ReadableCollectionProxyModel *readableCollectionProxyModel() const;

  EntityCollectionOrderProxyModel *entityOrderProxy() const;

  void quotaWarningParameters( const QColor &color, qreal threshold );
  void readQuotaConfig();

  KLineEdit *filterFolderLineEdit() const;
  void applyFilter( const QString& );
  void clearFilter();

  void disableContextMenuAndExtraColumn();

  void readConfig();

  void restoreHeaderState( const QByteArray& data );

protected:
  void changeToolTipsPolicyConfig( ToolTipDisplayPolicy );

protected slots:
  void slotChangeTooltipsPolicy( FolderTreeWidget::ToolTipDisplayPolicy );
  void slotManualSortingChanged( bool );

private:
  virtual bool eventFilter( QObject*o, QEvent *e );
  class FolderTreeWidgetPrivate;
  FolderTreeWidgetPrivate * const d;

};

}

#endif
