/*
  This file is part of KOrganizer.

  Copyright (c) 2000,2001,2003 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>
  Copyright (c) 2008 Thomas Thrainer <tom_t@gmx.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/
#ifndef KOTODOVIEW_H
#define KOTODOVIEW_H

#include "korganizer/baseview.h"

#include <Akonadi/Item>

#include <KCalCore/Todo>

namespace CalendarSupport {
  class Calendar;
}

namespace KPIM {
  class KDatePickerPopup;
}

using namespace KOrg;

class QMenu;
class QAction;
class QCheckBox;
class QDate;
class QItemSelection;
class QModelIndex;

class KOTodoModel;
class KOTodoViewView;
class KOTodoCategoriesDelegate;
class KOTodoViewSortFilterProxyModel;
class KOTodoViewQuickSearch;
class KOTodoViewQuickAddLine;

class KOTodoView : public BaseView
{
  Q_OBJECT

  public:
    KOTodoView( QWidget *parent );
    ~KOTodoView();

    virtual void setCalendar( CalendarSupport::Calendar *cal );

    virtual Akonadi::Item::List selectedIncidences();
    virtual KCalCore::DateList selectedIncidenceDates();
    virtual int currentDateCount() const { return 0; }

    void setDocumentId( const QString & ) {}

    void saveLayout( KConfig *config, const QString &group ) const;

    void restoreLayout( KConfig *config, const QString &group, bool minimalDefaults );

    /** documentation in baseview.h */
    void getHighlightMode( bool &highlightEvents,
                           bool &highlightTodos,
                           bool &highlightJournals );

    bool usesFullWindow();

    bool supportsDateRangeSelection() { return false; }
    virtual KOrg::CalPrinterBase::PrintType printType() const;

  public Q_SLOTS:
    virtual void setIncidenceChanger( CalendarSupport::IncidenceChanger *changer );
    virtual void showDates( const QDate &start, const QDate &end );
    virtual void showIncidences( const Akonadi::Item::List &incidenceList, const QDate &date );
    virtual void updateView();
    void updateCategories();
    virtual void changeIncidenceDisplay( const Akonadi::Item &incidence, int action );
    virtual void updateConfig();
    virtual void clearSelection();
    void expandIndex( const QModelIndex &index );

  protected Q_SLOTS:
    void addQuickTodo( Qt::KeyboardModifiers modifier );

    void contextMenu( const QPoint &pos );

    void selectionChanged( const QItemSelection &selected,
                           const QItemSelection &deselected );

    // slots used by popup-menus
    void showTodo();
    void editTodo();
    void printTodo();
    void printPreviewTodo();
    void deleteTodo();
    void newTodo();
    void newSubTodo();
    void copyTodoToDate( const QDate &date );

  private Q_SLOTS:
    void resizeColumnsToContent();
    void itemDoubleClicked( const QModelIndex &index );
    void setNewDate( const QDate &date );
    void setNewPercentage( QAction *action );
    void setNewPriority( QAction *action );
    void changedCategories( QAction *action );
    void setFlatView( bool flatView );

  Q_SIGNALS:
    void purgeCompletedSignal();
    void unSubTodoSignal();
    void unAllSubTodoSignal();
    void configChanged();

  private:
    QMenu *createCategoryPopupMenu();
    void printTodo( bool preview );

    /** Creates a new todo with the given text as summary under the given parent */
    void addTodo( const QString &summary,
                  const KCalCore::Todo::Ptr &parent = KCalCore::Todo::Ptr(),
                  const QStringList &categories = QStringList() );

    KOTodoViewView *mView;
    KOTodoViewSortFilterProxyModel *mProxyModel;
    KOTodoCategoriesDelegate *mCategoriesDelegate;

    KOTodoViewQuickSearch *mQuickSearch;
    KOTodoViewQuickAddLine *mQuickAdd;
    QCheckBox *mFlatView;

    QMenu *mItemPopupMenu;
    KPIM::KDatePickerPopup *mCopyPopupMenu;
    KPIM::KDatePickerPopup *mMovePopupMenu;
    QMenu *mPriorityPopupMenu;
    QMenu *mPercentageCompletedPopupMenu;
    QList<QAction*> mItemPopupMenuItemOnlyEntries;
    QList<QAction*> mItemPopupMenuReadWriteEntries;

    QAction *mMakeTodoIndependent;
    QAction *mMakeSubtodosIndependent;

    QMap<QAction *,int> mPercentage;
    QMap<QAction *,int> mPriority;
    QMap<QAction *,QString> mCategory;

  enum {
      eSummaryColumn = 0,
      eRecurColumn = 1,
      ePriorityColumn = 2,
      ePercentColumn = 3,
      eDueDateColumn = 4,
      eCategoriesColumn = 5,
      eDescriptionColumn = 6
    };
};

#endif /*KOTODOVIEW_H*/
