/*
  Copyright (c) 2000,2001,2003 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

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
#ifndef EVENTVIEWS_AGENDAITEM_H
#define EVENTVIEWS_AGENDAITEM_H

#include "eventviews_export.h"
#include "cellitem.h"

#include <Akonadi/Item>

#include <QDate>
#include <QWidget>
#include <QPointer>

namespace CalendarSupport {
  class Calendar;
}

namespace EventViews {

class AgendaItem;
class EventView;

struct MultiItemInfo
{
  int mStartCellXLeft, mStartCellXRight;
  int mStartCellYTop, mStartCellYBottom;
  QPointer<AgendaItem> mFirstMultiItem;
  QPointer<AgendaItem> mPrevMultiItem;
  QPointer<AgendaItem> mNextMultiItem;
  QPointer<AgendaItem> mLastMultiItem;
};

/**
  @class AgendaItem

  @brief This class describes the widgets that represent the various calendar
         items in the agenda view

  The AgendaItem has to make sure that it receives all mouse events, which
  are to be used for dragging and resizing. That means it has to be installed
  as event filter for its children, if it has children, and it has to pass
  mouse events from the children to itself. See eventFilter().

  Some comments on the movement of multi-day items:
  Basically, the agenda items are arranged in two implicit double-linked lists.
  The mMultiItemInfo works like before to describe the currently viewed
  multi-item.
  When moving, new events might need to be added to the beginning or the end of
  the multi-item sequence, or events might need to be hidden. I cannot just
  delete this items, since I have to restore/show them if the move is reset
  (i.e. if a drag started). So internally, I keep another doubly-linked list
  which is longer than the one defined by mMultiItemInfo, but includes the
  multi-item sequence, too.

  The mStartMoveInfo stores the first and last item of the multi-item sequence
  when the move started. The prev and next members of mStartMoveInfo are used
  for that longer sequence including all (shown and hidden) items.
*/

class EVENTVIEWS_EXPORT AgendaItem : public QWidget, public CellItem
{
  Q_OBJECT
  public:
    typedef QPointer<AgendaItem> QPtr;

    AgendaItem( EventView *eventView,
                CalendarSupport::Calendar *calendar,
                const Akonadi::Item &incidence,
                int itemPos,
                int itemCount,
                const QDate &qd,
                bool isSelected,
                QWidget *parent );
    ~AgendaItem();

    int cellXLeft() const { return mCellXLeft; }
    int cellXRight() const { return mCellXRight; }
    int cellYTop() const { return mCellYTop; }
    int cellYBottom() const { return mCellYBottom; }
    int cellHeight() const;
    int cellWidth() const;

    int itemPos() const { return mItemPos; }
    int itemCount() const { return mItemCount; }

    void setCellXY( int X, int YTop, int YBottom );
    void setCellY( int YTop, int YBottom );
    void setCellX( int XLeft, int XRight );
    void setCellXRight( int XRight );

    /** Start movement */
    void startMove();

    /** Reset to original values */
    void resetMove();

    /** End the movement (i.e. clean up) */
    void endMove();

    void moveRelative( int dx, int dy );

    /**
     * Expands the item's top.
     *
     * @param dy             delta y, number of units to be added to mCellYTop
     * @param allowOverLimit If false, the new mCellYTop can't be bigger than
     *                       mCellYBottom, instead, it gets mCellYBottom's value.
     *                       If true, @p dy is always added, regardless if mCellYTop
     *                       becomes bigger than mCellYBottom, this is useful when
     *                       moving items because it guarantees expandTop and the
     *                       following expandBottom call add the same value.
     */
    void expandTop( int dy, const bool allowOverLimit = false );
    void expandBottom( int dy );
    void expandLeft( int dx );
    void expandRight( int dx );

    bool isMultiItem() const;
    AgendaItem::QPtr prevMoveItem() const
    { return (mStartMoveInfo) ? (mStartMoveInfo->mPrevMultiItem) : 0; }

    AgendaItem::QPtr nextMoveItem() const
    { return (mStartMoveInfo) ? (mStartMoveInfo->mNextMultiItem) : 0; }

    MultiItemInfo *moveInfo() const { return mStartMoveInfo; }

    void setMultiItem( AgendaItem::QPtr first,AgendaItem::QPtr prev,
                       AgendaItem::QPtr next, AgendaItem::QPtr last );

    AgendaItem::QPtr prependMoveItem( AgendaItem::QPtr  );

    AgendaItem::QPtr appendMoveItem( AgendaItem::QPtr  );

    AgendaItem::QPtr removeMoveItem( AgendaItem::QPtr  );

    AgendaItem::QPtr firstMultiItem() const
    { return (mMultiItemInfo) ? (mMultiItemInfo->mFirstMultiItem) : 0; }

    AgendaItem::QPtr prevMultiItem() const
    { return (mMultiItemInfo) ? (mMultiItemInfo->mPrevMultiItem) : 0; }

    AgendaItem::QPtr nextMultiItem() const
    { return (mMultiItemInfo) ? (mMultiItemInfo->mNextMultiItem) : 0; }

    AgendaItem::QPtr lastMultiItem() const
    { return (mMultiItemInfo) ? (mMultiItemInfo->mLastMultiItem) : 0; }

    bool dissociateFromMultiItem();

    void setIncidence( const Akonadi::Item &incidence );
    const Akonadi::Item & incidence() const { return mIncidence; }
    QDate itemDate() { return mDate; }

    /** Update the date of this item's occurrence (not in the event) */
    void setItemDate( const QDate &qd );

    void setText ( const QString &text ) { mLabelText = text; }
    QString text () { return mLabelText; }

    QList<AgendaItem::QPtr> &conflictItems();
    void setConflictItems( QList<AgendaItem::QPtr> );
    void addConflictItem( AgendaItem::QPtr ci );

    QString label() const;

    /** Tells whether this item overlaps item @p o */
    bool overlaps( CellItem *o ) const;

    void setResourceColor( const QColor &color ) { mResourceColor = color; }
    QColor resourceColor() { return mResourceColor; }

  signals:
    void removeAgendaItem( AgendaItem::QPtr  );
    void showAgendaItem( AgendaItem::QPtr  );

  public slots:
    void updateIcons();
    void select( bool selected = true );
    void addAttendee( const QString & );

  protected:
    bool eventFilter( QObject *obj, QEvent *event );
    bool event( QEvent *event );
    void dragEnterEvent( QDragEnterEvent *e );
    void dropEvent( QDropEvent *e );
    /**reimp*/void paintEvent( QPaintEvent *e );

    /** private movement functions. startMove needs to be called of only one of
     *  the multitems. it will then loop through the whole series using
     *  startMovePrivate. Same for resetMove and endMove */
    void startMovePrivate();
    void resetMovePrivate();
    void endMovePrivate();

    // Variables to remember start position
    MultiItemInfo *mStartMoveInfo;
    //Color of the resource
    QColor mResourceColor;

  private:
    void paintIcon( QPainter *p, int &x, int y, int ft );

    // paint all visible icons
    void paintIcons( QPainter *p, int &x, int y, int ft );

    void drawRoundedRect( QPainter *p, const QRect &rect,
                          bool selected, const QColor &bgcolor,
                          bool frame, int ft, bool roundTop, bool roundBottom );

    int mCellXLeft, mCellXRight;
    int mCellYTop, mCellYBottom;

    EventView *mEventView;
    CalendarSupport::Calendar *mCalendar;
    Akonadi::Item mIncidence;
    QDate mDate;
    bool mValid;
    bool mCloned;
    QString mLabelText;
    bool mSelected;
    bool mIconAlarm, mIconRecur, mIconReadonly;
    bool mIconReply, mIconGroup, mIconGroupTent;
    bool mIconOrganizer, mSpecialEvent;

    // For incidences that expand through more than 1 day
    // Will be 1 for single day incidences
    int mItemPos;
    int mItemCount;

    // Multi item pointers
    MultiItemInfo *mMultiItemInfo;

    QList<AgendaItem::QPtr> mConflictItems;

    static QPixmap *alarmPxmp;
    static QPixmap *recurPxmp;
    static QPixmap *readonlyPxmp;
    static QPixmap *replyPxmp;
    static QPixmap *groupPxmp;
    static QPixmap *groupPxmpTent;
    static QPixmap *organizerPxmp;
    static QPixmap *eventPxmp;
    static QPixmap *todoPxmp;
    static QPixmap *completedPxmp;
};

}

#endif
