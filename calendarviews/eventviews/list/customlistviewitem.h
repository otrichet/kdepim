/*
  Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>

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
#ifndef CALENDARVIEWS_CUSTOMLISTVIEWITEM_H
#define CALENDARVIEWS_CUSTOMLISTVIEWITEM_H

#include <QMap>
#include <QString>
#include <QTreeWidget>
#include <QKeyEvent>

namespace EventViews {
  class ListView;
}

template<class T>
// TODO, rename to CustomTreeWidgetItem
class CustomListViewItem : public QTreeWidgetItem
{
  public:
  CustomListViewItem( T data, QTreeWidget *parent, EventViews::ListView *listView ) :
                          QTreeWidgetItem( parent ),
                          mData( data ),
                          mListView( listView )
      {
        updateItem();
      }

    ~CustomListViewItem() {}

    void updateItem() {}

    T data() const { return mData; }

    QVariant data( int column, int role ) const
    {
      return QTreeWidgetItem::data( column, role );
    }

    QString key( int column, bool ) const
    {
      QMap<int,QString>::ConstIterator it = mKeyMap.find( column );
      if ( it == mKeyMap.end() ) {
        return text( column );
      } else {
        return *it;
      }
    }

    void setSortKey( int column, const QString &key )
    {
      mKeyMap.insert( column, key );
    }

  private:
    T mData;
    EventViews::ListView *mListView;

    QMap<int,QString> mKeyMap;
};

#endif
