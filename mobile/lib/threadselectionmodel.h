/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef THREADSELECTIONMODEL_H
#define THREADSELECTIONMODEL_H

#include <QtGui/QItemSelectionModel>

#include "mobileui_export.h"

class ThreadSelectionModelPrivate;

class MOBILEUI_EXPORT ThreadSelectionModel : public QItemSelectionModel
{
  Q_OBJECT
public:
  explicit ThreadSelectionModel(QAbstractItemModel* model, QItemSelectionModel *contentSelectionModel, QItemSelectionModel *navigationModel, QObject *parent = 0);
  virtual ~ThreadSelectionModel();
  virtual void select(const QModelIndex& index, SelectionFlags command);
  virtual void select(const QItemSelection& selection, SelectionFlags command);
private:
  Q_DECLARE_PRIVATE(ThreadSelectionModel)
  ThreadSelectionModelPrivate * const d_ptr;
  Q_PRIVATE_SLOT(d_func(), void contentSelectionChanged(QItemSelection,QItemSelection))
};

#endif
