/*
 * Copyright (c) 2015 Olivier Trichet <olivier@trichet.fr>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "group_model.h"

#include <QtCore/QModelIndex>
#include <QtCore/QTimer>
#include <KDE/KLocalizedString>

#include "groupselection/enums.h"
#include "kngroupmanager.h"

namespace KNode {
namespace GroupSelection {


GroupModel::GroupModel(QObject* parent)
    : QAbstractItemModel(parent),
      mGroups(0)
{
}

GroupModel::~GroupModel()
{
    delete mGroups;
}


void GroupModel::newList(QList<KNGroupInfo>* groups)
{
    if(groups) {
        qSort(*groups);
    }

    beginResetModel();
    delete mGroups;
    mGroups = groups;
    endResetModel();
}


QVariant GroupModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section) {
        case GroupModelColumn_Name:
            return i18n("Name");
        case GroupModelColumn_Description:
            return i18n("Description");
        }
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}


QVariant GroupModel::data(const QModelIndex& index, int role) const
{
    switch(role) {
        case Qt::DisplayRole: {
            const KNGroupInfo& gi = mGroups->at(index.internalId());
            switch(index.column()) {
                case GroupModelColumn_Name:
                    if(gi.status == KNGroup::moderated) {
                        const QString ret = gi.name + QLatin1String(" (m)");
                        return ret;
                    } else {
                        return gi.name;
                    }
                    break;
                case GroupModelColumn_Description:
                    return gi.description;
                    break;
            }
        }
            break;
        case GroupInfoRole:
            return QVariant::fromValue(mGroups->at(index.internalId()));
            break;
    }

    return QVariant();
}

int GroupModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return GroupModelColumn_Count;
}

int GroupModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid()) {
        return 0;
    }
    return mGroups ? mGroups->size() : 0;
}

QModelIndex GroupModel::parent(const QModelIndex& child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

QModelIndex GroupModel::index(int row, int column, const QModelIndex& parent) const
{
    if(parent.isValid()) {
        return QModelIndex();
    }
    return createIndex(row, column, row);
}


}
}
