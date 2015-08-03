/*
  Copyright (c) 2008 Thomas Thrainer <tom_t@gmx.at>
  Copyright (c) 2012 Sérgio Martins <iamsergio@gmail.com>

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

#ifndef CALENDARVIEWS_TODOMODEL_H
#define CALENDARVIEWS_TODOMODEL_H

#include "prefs.h"

#include <Akonadi/Calendar/IncidenceChanger>
#include <Akonadi/Calendar/ETMCalendar>
#include <Item>

#include <KCalCore/Todo>
#include <QAbstractItemModel>
#include <EntityTreeModel>
#include <QAbstractProxyModel>

class QMimeData;

class TodoModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    /** This enum defines all columns this model provides */
    enum {
        SummaryColumn = 0,
        RecurColumn,
        PriorityColumn,
        PercentColumn,
        StartDateColumn,
        DueDateColumn,
        CategoriesColumn,
        DescriptionColumn,
        CalendarColumn,
        ColumnCount // Just for iteration/column count purposes. Always keep at the end of enum.
    };

    /** This enum defines the user defined roles of the items in this model */
    enum {
        TodoRole = Akonadi::EntityTreeModel::UserRole + 1,
        IsRichTextRole
    };

    explicit TodoModel(const EventViews::PrefsPtr &preferences, QObject *parent = Q_NULLPTR);

    ~TodoModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    void setSourceModel(QAbstractItemModel *sourceModel) Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;

    bool setData(const QModelIndex &index, const QVariant &value, int role) Q_DECL_OVERRIDE;

    QVariant headerData(int section, Qt::Orientation, int role) const Q_DECL_OVERRIDE;

    void setCalendar(const Akonadi::ETMCalendar::Ptr &calendar);

    void setIncidenceChanger(Akonadi::IncidenceChanger *changer);

    QMimeData *mimeData(const QModelIndexList &indexes) const Q_DECL_OVERRIDE;

    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) Q_DECL_OVERRIDE;

    QStringList mimeTypes() const Q_DECL_OVERRIDE;

    Qt::DropActions supportedDropActions() const Q_DECL_OVERRIDE;

    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

    QModelIndex parent(const QModelIndex &child) const Q_DECL_OVERRIDE;

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const Q_DECL_OVERRIDE;

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const Q_DECL_OVERRIDE;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    QModelIndex buddy(const QModelIndex &index) const Q_DECL_OVERRIDE;

private:
    class Private;
    Private *const d;
};

#endif
