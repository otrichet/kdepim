/*
  Copyright (c) 2007 Volker Krause <vkrause@kde.org>
  Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Copyright (c) 2010 Andras Mantia <andras@kdab.com>


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
*/

#include "timelineitem.h"

#include <kdgantt2/kdganttglobal.h>

#include <calendarsupport/kcalprefs.h>
#include <calendarsupport/calendar.h>
#include <calendarsupport/utils.h>

#include <KCalCore/Incidence>
#include <KCalUtils/IncidenceFormatter>

using namespace KCalCore;
using namespace KCalUtils;
using namespace EventViews;

TimelineItem::TimelineItem( CalendarSupport::Calendar *calendar, uint index,
                            QStandardItemModel* model, QObject *parent )
  : QObject( parent ), mCalendar( calendar ), mModel( model ), mIndex( index )
{
 mModel->removeRow( mIndex );
 QStandardItem * dummyItem = new QStandardItem;
 dummyItem->setData( KDGantt::TypeTask, KDGantt::ItemTypeRole );

 mModel->insertRow( mIndex, dummyItem );
}

void TimelineItem::insertIncidence( const Akonadi::Item &aitem,
                                    const KDateTime & _start, const KDateTime & _end )
{
  const Incidence::Ptr incidence = CalendarSupport::incidence( aitem );
  KDateTime start = incidence->dtStart().toTimeSpec( CalendarSupport::KCalPrefs::instance()->timeSpec() );
  KDateTime end = incidence->dateTime( Incidence::RoleEnd ).toTimeSpec( CalendarSupport::KCalPrefs::instance()->timeSpec() );

  if ( _start.isValid() ) {
    start = _start;
  }
  if ( _end.isValid() ) {
    end = _end;
  }
  if ( incidence->allDay() ) {
    end = end.addDays( 1 );
  }

  typedef QList<QStandardItem*> ItemList;
  ItemList list = mItemMap.value( aitem.id() );
  for ( ItemList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it ) {
    if ( KDateTime( static_cast<TimelineSubItem* >(*it)->startTime() ) == start &&
         KDateTime( static_cast<TimelineSubItem* >(*it)->endTime() ) == end ) {
      return;
    }
  }

  TimelineSubItem * item = new TimelineSubItem( aitem, this );

  item->setStartTime( start.dateTime() );
  item->setOriginalStart( start );
  item->setEndTime( end.dateTime() );
  item->setData( mColor, Qt::DecorationRole );

  list = mModel->takeRow( mIndex );

  mItemMap[aitem.id()].append( item );

  list.append( mItemMap[aitem.id()] );

  mModel->insertRow( mIndex, list );
}

void TimelineItem::removeIncidence( const Akonadi::Item &incidence )
{
  qDeleteAll( mItemMap.value( incidence.id() ) );
  mItemMap.remove( incidence.id() );
}

void TimelineItem::moveItems( const Akonadi::Item &incidence, int delta, int duration )
{
  typedef QList<QStandardItem*> ItemList;
  ItemList list = mItemMap.value( incidence.id() );
  for ( ItemList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it ) {
    QDateTime start = static_cast<TimelineSubItem* >(*it)->originalStart().dateTime();
    start = start.addSecs( delta );
    static_cast<TimelineSubItem* >(*it)->setStartTime( start );
    static_cast<TimelineSubItem* >(*it)->setOriginalStart( KDateTime(start) );
    static_cast<TimelineSubItem* >(*it)->setEndTime( start.addSecs( duration ) );
  }
}

void TimelineItem::setColor(const QColor& color)
{
  mColor = color;
}


TimelineSubItem::TimelineSubItem( const Akonadi::Item &incidence,
                                  TimelineItem *parent
                                )
  : QStandardItem(), mIncidence( incidence ),
    mParent( parent ), mToolTipNeedsUpdate( true )
{
  setData( KDGantt::TypeTask, KDGantt::ItemTypeRole );
  if ( !CalendarSupport::incidence( incidence )->isReadOnly() ) {
    setFlags( Qt::ItemIsSelectable );
  }
}

TimelineSubItem::~TimelineSubItem()
{
}

void TimelineSubItem::setStartTime(const QDateTime& dt)
{
  setData( dt, KDGantt::StartTimeRole );
}

QDateTime TimelineSubItem::startTime() const
{
  return data( KDGantt::StartTimeRole ).toDateTime();
}

void TimelineSubItem::setEndTime(const QDateTime& dt)
{
  setData( dt, KDGantt::EndTimeRole );
}

QDateTime TimelineSubItem::endTime() const
{
  return data( KDGantt::EndTimeRole ).toDateTime();
}

void TimelineSubItem::updateToolTip()
{
  if ( !mToolTipNeedsUpdate )
    return;

  mToolTipNeedsUpdate = false;

  setData( IncidenceFormatter::toolTipStr(
                  CalendarSupport::displayName( mIncidence.parentCollection() ),
                  CalendarSupport::incidence( mIncidence ), originalStart().date(),
                  true, CalendarSupport::KCalPrefs::instance()->timeSpec() ), Qt::ToolTipRole );
}
