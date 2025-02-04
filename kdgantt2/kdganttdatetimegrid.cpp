/****************************************************************************
 ** Copyright (C) 2001-2006 Klarälvdalens Datakonsult AB.  All rights reserved.
 **
 ** This file is part of the KD Gantt library.
 **
 ** This file may be distributed and/or modified under the terms of the
 ** GNU General Public License version 2 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.GPL included in the
 ** packaging of this file.
 **
 ** Licensees holding valid commercial KD Gantt licenses may use this file in
 ** accordance with the KD Gantt Commercial License Agreement provided with
 ** the Software.
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** See http://www.kdab.net/kdgantt for
 **   information about KD Gantt Commercial License Agreements.
 **
 ** Contact info@kdab.net if any conditions of this
 ** licensing are not clear to you.
 **
 **********************************************************************/
#include "kdganttdatetimegrid.h"
#include "kdganttdatetimegrid_p.h"

#include "kdganttabstractrowcontroller.h"

#include <QApplication>
#include <QDateTime>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionHeader>
#include <QWidget>
#include <QDebug>

#include <cassert>

using namespace KDGantt;

QDebug operator<<( QDebug dbg, KDGantt::DateTimeScaleFormatter::Range range )
{
    switch( range ) {
    case KDGantt::DateTimeScaleFormatter::Second: dbg << "KDGantt::DateTimeScaleFormatter::Second"; break;
    case KDGantt::DateTimeScaleFormatter::Minute: dbg << "KDGantt::DateTimeScaleFormatter::Minute"; break;
    case KDGantt::DateTimeScaleFormatter::Hour:   dbg << "KDGantt::DateTimeScaleFormatter::Hour"; break;
    case KDGantt::DateTimeScaleFormatter::Day:    dbg << "KDGantt::DateTimeScaleFormatter::Day"; break;
    case KDGantt::DateTimeScaleFormatter::Week:   dbg << "KDGantt::DateTimeScaleFormatter::Week"; break;
    case KDGantt::DateTimeScaleFormatter::Month:  dbg << "KDGantt::DateTimeScaleFormatter::Month"; break;
    case KDGantt::DateTimeScaleFormatter::Year:   dbg << "KDGantt::DateTimeScaleFormatter::Year"; break;
    }
    return dbg;
}


/*!\class KDGantt::DateTimeGrid
 * \ingroup KDGantt
 *
 * This implementation of AbstractGrid works with QDateTime
 * and shows days and week numbers in the header
 */

// TODO: I think maybe this class should be responsible
// for unit-transformation of the scene...

qreal DateTimeGrid::Private::dateTimeToChartX( const QDateTime& dt ) const
{
    assert( startDateTime.isValid() );
    qreal result = startDateTime.date().daysTo(dt.date())*24.*60.*60.;
    result += startDateTime.time().msecsTo(dt.time())/1000.;
    result *= dayWidth/( 24.*60.*60. );

    return result;
}

QDateTime DateTimeGrid::Private::chartXtoDateTime( qreal x ) const
{
    assert( startDateTime.isValid() );
    int days = static_cast<int>( x/dayWidth );
    qreal secs = x*( 24.*60.*60. )/dayWidth;
    QDateTime dt = startDateTime;
    QDateTime result = dt.addDays( days )
                       .addSecs( static_cast<int>(secs-(days*24.*60.*60.) ) )
                       .addMSecs( qRound( ( secs-static_cast<int>( secs ) )*1000. ) );
    return result;
}

#define d d_func()

/*!\class KDGantt::DateTimeScaleFormatter
 * \ingroup KDGantt
 *
 * This class formats dates and times used in DateTimeGrid follawing a given format.
 *
 * The format follows the format of QDateTime::toString(), with one addition:
 * "w" is replaced with the week number of the date as number without a leading zero (1-53)
 * "ww" is replaced with the week number of the date as number with a leading zero (01-53)
 *
 * For example:
 *
 * \code
 *  // formatter to print the complete date over the current week
 *  // This leads to the first day of the week being printed
 *  DateTimeScaleFormatter formatter = DateTimeScaleFormatter( DateTimeScaleFormatter::Week, "yyyy-MM-dd" );
 * \endcode
 *
 * Optionally, you can set an user defined text alignment flag. The default value is Qt::AlignCenter.
 * \sa DateTimeScaleFormatter::DateTimeScaleFormatter
 *
 * This class even controls the range of the grid sections.
 * \sa KDGanttDateTimeScaleFormatter::Range
 */

/*! Creates a DateTimeScaleFormatter using \a range and \a format.
 *  The text on the header is aligned following \a alignment.
 */
DateTimeScaleFormatter::DateTimeScaleFormatter( Range range, const QString& format,
                                                const QString& templ, Qt::Alignment alignment )
    : _d( new Private( range, format, templ, alignment ) )
{
}

DateTimeScaleFormatter::DateTimeScaleFormatter( Range range, const QString& format, Qt::Alignment alignment )
    : _d( new Private( range, format, QString::fromLatin1( "%1" ), alignment ) )
{
}

DateTimeScaleFormatter::DateTimeScaleFormatter( const DateTimeScaleFormatter& other )
    : _d( new Private( other.range(), other.format(), other.d->templ, other.alignment() ) )
{
}

DateTimeScaleFormatter::~DateTimeScaleFormatter()
{
    delete _d;
}

DateTimeScaleFormatter& DateTimeScaleFormatter::operator=( const DateTimeScaleFormatter& other )
{
    delete _d;
    _d = new Private( other.range(), other.format(), other.d->templ, other.alignment() );
    return *this;
}

/*! \returns The format being used for formatting dates and times.
 */
QString DateTimeScaleFormatter::format() const
{
    return d->format;
}

/*! \returns The \a datetime as string respecting the format.
 */
QString DateTimeScaleFormatter::format( const QDateTime& datetime ) const
{
    QString result = d->format;
    // additional feature: Weeknumber
    const QString shortWeekNumber = QString::number( datetime.date().weekNumber() );
    const QString longWeekNumber = ( shortWeekNumber.length() == 1 ? QString::fromLatin1( "0" ) : QString() ) + shortWeekNumber;
    result.replace( QString::fromLatin1( "ww" ), longWeekNumber );
    result.replace( QString::fromLatin1( "w" ), shortWeekNumber );
    result = datetime.toLocalTime().toString( result );
    return result;
}

QString DateTimeScaleFormatter::text( const QDateTime& datetime ) const
{
    return d->templ.arg( format( datetime ) );
}

/*! \returns The range of each item on a DateTimeGrid header.
 * \sa DateTimeScaleFormatter::Range */
DateTimeScaleFormatter::Range DateTimeScaleFormatter::range() const
{
    return d->range;
}

Qt::Alignment DateTimeScaleFormatter::alignment() const
{
    return d->alignment;
}

/*! \returns the QDateTime being the begin of the range after the one containing \a datetime
 *  \sa currentRangeBegin
 */
QDateTime DateTimeScaleFormatter::nextRangeBegin( const QDateTime& datetime ) const
{
    QDateTime result = datetime;
    switch( d->range )
    {
    case Second:
        result = result.addSecs( 60 );
        break;
    case Minute:
        // set it to the begin of the next minute
        result.setTime( QTime( result.time().hour(), result.time().minute() ) );
        result = result.addSecs( 60 );
        break;
    case Hour:
        // set it to the begin of the next hour
        result.setTime( QTime( result.time().hour(), 0 ) );
        result = result.addSecs( 60 * 60 );
        break;
    case Day:
        // set it to midnight the next day
        result.setTime( QTime( 0, 0 ) );
        result = result.addDays( 1 );
        break;
    case Week:
        // set it to midnight
        result.setTime( QTime( 0, 0 ) );
        // iterate day-wise, until weekNumber changes
        {
            const int weekNumber = result.date().weekNumber();
            while( weekNumber == result.date().weekNumber() )
                result = result.addDays( 1 );
        }
        break;
    case Month:
        // set it to midnight
        result.setTime( QTime( 0, 0 ) );
        // set it to the first of the next month
        result.setDate( QDate( result.date().year(), result.date().month(), 1 ).addMonths( 1 ) );
        break;
    case Year:
        // set it to midnight
        result.setTime( QTime( 0, 0 ) );
        // set it to the first of the next year
        result.setDate( QDate( result.date().year(), 1, 1 ).addYears( 1 ) );
        break;
    }
    //result = result.toLocalTime();
    assert(  result != datetime );
    //qDebug() << "DateTimeScaleFormatter::nextRangeBegin("<<datetime<<")="<<d->range<<result;
    return result;
}

/*! \returns the QDateTime being the begin of the range containing \a datetime
 *  \sa nextRangeBegin
 */
QDateTime DateTimeScaleFormatter::currentRangeBegin( const QDateTime& datetime ) const
{
    QDateTime result = datetime;
    switch( d->range )
    {
    case Second:
        break; // nothing
    case Minute:
        // set it to the begin of the current minute
        result.setTime( QTime( result.time().hour(), result.time().minute() ) );
        break;
    case Hour:
        // set it to the begin of the current hour
        result.setTime( QTime( result.time().hour(), 0 ) );
        break;
    case Day:
        // set it to midnight the current day
        result.setTime( QTime( 0, 0 ) );
        break;
    case Week:
        // set it to midnight
        result.setTime( QTime( 0, 0 ) );
        // iterate day-wise, as long weekNumber is the same
        {
            const int weekNumber = result.date().weekNumber();
            while( weekNumber == result.date().addDays( -1 ).weekNumber() )
                result = result.addDays( -1 );
        }
        break;
    case Month:
        // set it to midnight
        result.setTime( QTime( 0, 0 ) );
        // set it to the first of the current month
        result.setDate( QDate( result.date().year(), result.date().month(), 1 ) );
        break;
    case Year:
        // set it to midnight
        result.setTime( QTime( 0, 0 ) );
        // set it to the first of the current year
        result.setDate( QDate( result.date().year(), 1, 1 ) );
        break;
    }
    return result;
}

DateTimeGrid::DateTimeGrid() : AbstractGrid( new Private )
{
}

DateTimeGrid::~DateTimeGrid()
{
}

/*! \returns The QDateTime used as start date for the grid.
 *
 * The default is three days before the current date.
 */
QDateTime DateTimeGrid::startDateTime() const
{
    return d->startDateTime;
}

/*! \param dt The start date of the grid. It is used as the beginning of the
 * horizontal scrollbar in the view.
 *
 * Emits gridChanged() after the start date has changed.
 */
void DateTimeGrid::setStartDateTime( const QDateTime& dt )
{
    d->startDateTime = dt;
    emit gridChanged();
}

/*! \returns The width in pixels for each day in the grid.
 *
 * The default is 100 pixels.
 */
qreal DateTimeGrid::dayWidth() const
{
    return d->dayWidth;
}

/*! Maps a given point in time \a dt to an X value in the scene.
 */
qreal DateTimeGrid::mapFromDateTime( const QDateTime& dt) const
{
    return d->dateTimeToChartX( dt );
}

/*! Maps a given X value \a x in scene coordinates to a point in time.
 */
QDateTime DateTimeGrid::mapToDateTime( qreal x ) const
{
    return d->chartXtoDateTime( x );
}

/*! \param w The width in pixels for each day in the grid.
 *
 * The signal gridChanged() is emitted after the day width is changed.
 */
void DateTimeGrid::setDayWidth( qreal w )
{
    assert( w>0 );
    d->dayWidth = w;
    emit gridChanged();
}

/*! \param s The scale to be used to paint the grid.
 *
 * The signal gridChanged() is emitted after the scale has changed.
 * \sa Scale
 */
void DateTimeGrid::setScale( Scale s )
{
    d->scale = s;
    emit gridChanged();
}

/*! \returns The scale used to paint the grid.
 *
 * The default is ScaleAuto, which means the day scale will be used
 * as long as the day width is less or equal to 500.
 * \sa Scale
 */
DateTimeGrid::Scale DateTimeGrid::scale() const
{
    return d->scale;
}

/*! Sets the scale formatter for the lower part of the header to the
 *  user defined formatter to \a lower. The DateTimeGrid object takes
 *  ownership of the formatter, which has to be allocated with new.
 *
 * You have to set the scale to ScaleUserDefined for this setting to take effect.
 * \sa DateTimeScaleFormatter
 */
void DateTimeGrid::setUserDefinedLowerScale( DateTimeScaleFormatter* lower )
{
    delete d->lower;
    d->lower = lower;
    emit gridChanged();
}

/*! Sets the scale formatter for the upper part of the header to the
 *  user defined formatter to \a upper. The DateTimeGrid object takes
 *  ownership of the formatter, which has to be allocated with new.
 *
 * You have to set the scale to ScaleUserDefined for this setting to take effect.
 * \sa DateTimeScaleFormatter
 */
void DateTimeGrid::setUserDefinedUpperScale( DateTimeScaleFormatter* upper )
{
    delete d->upper;
    d->upper = upper;
    emit gridChanged();
}

/*! \return The DateTimeScaleFormatter being used to render the lower scale.
 */
DateTimeScaleFormatter* DateTimeGrid::userDefinedLowerScale() const
{
    return d->lower;
}

/*! \return The DateTimeScaleFormatter being used to render the upper scale.
 */
DateTimeScaleFormatter* DateTimeGrid::userDefinedUpperScale() const
{
    return d->upper;
}

/*! \param ws The start day of the week.
 *
 * A solid line is drawn on the grid to mark the beginning of a new week.
 * Emits gridChanged() after the start day has changed.
 */
void DateTimeGrid::setWeekStart( Qt::DayOfWeek ws )
{
    d->weekStart = ws;
    emit gridChanged();
}

/*! \returns The start day of the week */
Qt::DayOfWeek DateTimeGrid::weekStart() const
{
    return d->weekStart;
}

/*! \param fd A set of days to mark as free in the grid.
 *
 * Free days are filled with the alternate base brush of the
 * palette used by the view.
 * The signal gridChanged() is emitted after the free days are changed.
 */
void DateTimeGrid::setFreeDays( const QSet<Qt::DayOfWeek>& fd )
{
    d->freeDays = fd;
    emit gridChanged();
}

/*! \returns The days marked as free in the grid. */
QSet<Qt::DayOfWeek> DateTimeGrid::freeDays() const
{
    return d->freeDays;
}

/*! \returns true if row separators are used. */
bool DateTimeGrid::rowSeparators() const
{
    return d->rowSeparators;
}
/*! \param enable Whether to use row separators or not. */
void DateTimeGrid::setRowSeparators( bool enable )
{
    d->rowSeparators = enable;
}

/*! Sets the brush used to display rows where no data is found.
 * Default is a red pattern. If set to QBrush() rows with no
 * information will not be marked.
 */
void DateTimeGrid::setNoInformationBrush( const QBrush& brush )
{
    d->noInformationBrush = brush;
    emit gridChanged();
}

/*! \returns the brush used to mark rows with no information.
 */
QBrush DateTimeGrid::noInformationBrush() const
{
    return d->noInformationBrush;
}

/*! \param idx The index to get the Span for.
 * \returns The start and end pixels, in a Span, of the specified index.
 */
Span DateTimeGrid::mapToChart( const QModelIndex& idx ) const
{
    assert( model() );
    if ( !idx.isValid() ) return Span();
    assert( idx.model()==model() );
    const QVariant sv = model()->data( idx, StartTimeRole );
    const QVariant ev = model()->data( idx, EndTimeRole );
    if( qVariantCanConvert<QDateTime>(sv) &&
    qVariantCanConvert<QDateTime>(ev) &&
    !(sv.type() == QVariant::String && qVariantValue<QString>(sv).isEmpty()) &&
    !(ev.type() == QVariant::String && qVariantValue<QString>(ev).isEmpty())
    ) {
      QDateTime st = sv.toDateTime();
      QDateTime et = ev.toDateTime();
      if ( et.isValid() && st.isValid() ) {
        qreal sx = d->dateTimeToChartX( st );
        qreal ex = d->dateTimeToChartX( et )-sx;
        //qDebug() << "DateTimeGrid::mapToChart("<<st<<et<<") => "<< Span( sx, ex );
        return Span( sx, ex);
      }
    }
    // Special case for Events with only a start date
    if( qVariantCanConvert<QDateTime>(sv) && !(sv.type() == QVariant::String && qVariantValue<QString>(sv).isEmpty()) ) {
      QDateTime st = sv.toDateTime();
      if ( st.isValid() ) {
        qreal sx = d->dateTimeToChartX( st );
        return Span( sx, 0 );
      }
    }
    return Span();
}

#if 0
static void debug_print_idx( const QModelIndex& idx )
{
    if ( !idx.isValid() ) {
        qDebug() << "[Invalid]";
        return;
    }
    QDateTime st = idx.data( StartTimeRole ).toDateTime();
    QDateTime et = idx.data( StartTimeRole ).toDateTime();
    qDebug() << idx << "["<<st<<et<<"]";
}
#endif

/*! Maps the supplied Span to QDateTimes, and puts them as start time and
 * end time for the supplied index.
 *
 * \param span The span used to map from.
 * \param idx The index used for setting the start time and end time in the model.
 * \param constraints A list of hard constraints to match against the start time and
 * end time mapped from the span.
 *
 * \returns true if the start time and time was successfully added to the model, or false
 * if unsucessful.
 * Also returns false if any of the constraints isn't satisfied. That is, if the start time of
 * the constrained index is before the end time of the dependency index, or the end time of the
 * constrained index is before the start time of the dependency index.
 */
bool DateTimeGrid::mapFromChart( const Span& span, const QModelIndex& idx,
    const QList<Constraint>& constraints ) const
{
    assert( model() );
    if ( !idx.isValid() ) return false;
    assert( idx.model()==model() );

    QDateTime st = d->chartXtoDateTime(span.start());
    QDateTime et = d->chartXtoDateTime(span.start()+span.length());
    //qDebug() << "DateTimeGrid::mapFromChart("<<span<<") => "<< st << et;
    Q_FOREACH( const Constraint& c, constraints ) {
        if ( c.type() != Constraint::TypeHard || !isSatisfiedConstraint( c )) continue;
        if ( c.startIndex() == idx ) {
            QDateTime tmpst = model()->data( c.endIndex(), StartTimeRole ).toDateTime();
            //qDebug() << tmpst << "<" << et <<"?";
            if ( tmpst<et ) return false;
        } else if ( c.endIndex() == idx ) {
            QDateTime tmpet = model()->data( c.startIndex(), EndTimeRole ).toDateTime();
            //qDebug() << tmpet << ">" << st <<"?";
            if ( tmpet>st ) return false;
        }
    }
    return model()->setData( idx, qVariantFromValue(st), StartTimeRole )
        && model()->setData( idx, qVariantFromValue(et), EndTimeRole );
}

void DateTimeGrid::Private::paintVerticalDayLines( QPainter* painter,
                                                   const QRectF& sceneRect,
                                                   const QRectF& exposedRect,
                                                   QWidget* widget )
{
        QDateTime dt = chartXtoDateTime( exposedRect.left() );
        dt.setTime( QTime( 0, 0, 0, 0 ) );
        for ( qreal x = dateTimeToChartX( dt ); x < exposedRect.right();
              dt = dt.addDays( 1 ),x=dateTimeToChartX( dt ) ) {
            if ( x >= exposedRect.left() ) {
                QPen pen = painter->pen();
                pen.setBrush( QApplication::palette().dark() );
                if ( dt.date().dayOfWeek() == weekStart ) {
                    pen.setStyle( Qt::SolidLine );
                } else {
                    pen.setStyle( Qt::DashLine );
                }
                painter->setPen( pen );
                if ( freeDays.contains( static_cast<Qt::DayOfWeek>( dt.date().dayOfWeek() ) ) ) {
                    painter->setBrush( widget?widget->palette().midlight()
                                       :QApplication::palette().midlight() );
                    painter->fillRect( QRectF( x, exposedRect.top(), dayWidth, exposedRect.height() ), painter->brush() );
                }
                painter->drawLine( QPointF( x, sceneRect.top() ), QPointF( x, sceneRect.bottom() ) );
            }
        }
}

void DateTimeGrid::Private::paintVerticalHourLines( QPainter* painter,
                                                   const QRectF& sceneRect,
                                                   const QRectF& exposedRect,
                                                   QWidget* widget )
{
        QDateTime dt = chartXtoDateTime( exposedRect.left() );
        dt.setTime( QTime( 0, 0, 0, 0 ) );
        for ( qreal x = dateTimeToChartX( dt ); x < exposedRect.right();
              dt = dt.addSecs( 60*60 ),x=dateTimeToChartX( dt ) ) {
            if ( x >= exposedRect.left() ) {
                QPen pen = painter->pen();
                pen.setBrush( QApplication::palette().dark() );
                if ( dt.time().hour() == 0 ) {
                    pen.setStyle( Qt::SolidLine );
                } else {
                    pen.setStyle( Qt::DashLine );
                }
                painter->setPen( pen );
                if ( freeDays.contains( static_cast<Qt::DayOfWeek>( dt.date().dayOfWeek() ) ) ) {
                    painter->setBrush( widget?widget->palette().midlight()
                                       :QApplication::palette().midlight() );
                    painter->fillRect( QRectF( x, exposedRect.top(), dayWidth, exposedRect.height() ), painter->brush() );
                }
                painter->drawLine( QPointF( x, sceneRect.top() ), QPointF( x, sceneRect.bottom() ) );
            }
        }
}

void DateTimeGrid::Private::paintVerticalUserDefinedLines( QPainter* painter,
                                                           const QRectF& sceneRect,
                                                           const QRectF& exposedRect,
                                                           const DateTimeScaleFormatter* formatter,
                                                           QWidget* widget )
{
    Q_UNUSED( widget );
    QDateTime dt = chartXtoDateTime( exposedRect.left() );
    dt = formatter->currentRangeBegin( dt );
    QPen pen = painter->pen();
    pen.setBrush( QApplication::palette().dark() );
    pen.setStyle( Qt::DashLine );
    painter->setPen( pen );
    for ( qreal x = dateTimeToChartX( dt ); x < exposedRect.right();
          dt = formatter->nextRangeBegin( dt ),x=dateTimeToChartX( dt ) ) {
        if ( x >= exposedRect.left() ) {
            painter->drawLine( QPointF( x, sceneRect.top() ), QPointF( x, sceneRect.bottom() ) );
        }
    }
}

void DateTimeGrid::paintGrid( QPainter* painter,
                              const QRectF& sceneRect,
                              const QRectF& exposedRect,
                              AbstractRowController* rowController,
                              QWidget* widget )
{
    // TODO: Support hours
    switch( scale() ) {
    case ScaleDay:
        d->paintVerticalDayLines( painter, sceneRect, exposedRect, widget );
        break;
    case ScaleHour:
        d->paintVerticalHourLines( painter, sceneRect, exposedRect, widget );
        break;
    case ScaleWeek:
        d->paintVerticalUserDefinedLines( painter, sceneRect, exposedRect, &d->week_lower, widget );
        break;
    case ScaleMonth:
        d->paintVerticalUserDefinedLines( painter, sceneRect, exposedRect, &d->month_lower, widget );
        break;
    case ScaleAuto: {
        const qreal tabw = QApplication::fontMetrics().width( QLatin1String( "XXXXX" ) );
        const qreal dayw = dayWidth();
        if ( dayw > 24*60*60*tabw ) {
            d->paintVerticalUserDefinedLines( painter, sceneRect, exposedRect, &d->minute_lower, widget );
        } else if ( dayw > 24*60*tabw ) {
            d->paintVerticalHourLines( painter, sceneRect, exposedRect, widget );
        } else if ( dayw > 24*tabw ) {
        d->paintVerticalDayLines( painter, sceneRect, exposedRect, widget );
        } else if ( dayw > tabw ) {
            d->paintVerticalUserDefinedLines( painter, sceneRect, exposedRect, &d->week_lower, widget );
        } else if ( 4*dayw > tabw ) {
            d->paintVerticalUserDefinedLines( painter, sceneRect, exposedRect, &d->month_lower, widget );
        } else {
            d->paintVerticalUserDefinedLines( painter, sceneRect, exposedRect, &d->year_lower, widget );
        }
        break;
    }
    case ScaleUserDefined:
        d->paintVerticalUserDefinedLines( painter, sceneRect, exposedRect, d->lower, widget );
        break;
    }
    if ( rowController ) {
        // First draw the rows
        QPen pen = painter->pen();
        pen.setBrush( QApplication::palette().dark() );
        pen.setStyle( Qt::DashLine );
        painter->setPen( pen );
        QModelIndex idx = rowController->indexAt( qRound( exposedRect.top() ) );
        if ( rowController->indexAbove( idx ).isValid() ) idx = rowController->indexAbove( idx );
        qreal y = 0;
        while ( y < exposedRect.bottom() && idx.isValid() ) {
            const Span s = rowController->rowGeometry( idx );
            y = s.start()+s.length();
            if ( d->rowSeparators ) {
                painter->drawLine( QPointF( sceneRect.left(), y ),
                                   QPointF( sceneRect.right(), y ) );
            }
            if ( !idx.data( ItemTypeRole ).isValid() && d->noInformationBrush.style() != Qt::NoBrush ) {
                painter->fillRect( QRectF( exposedRect.left(), s.start(), exposedRect.width(), s.length() ), d->noInformationBrush );
            }
            // Is alternating background better?
            //if ( idx.row()%2 ) painter->fillRect( QRectF( exposedRect.x(), s.start(), exposedRect.width(), s.length() ), QApplication::palette().alternateBase() );
            idx =  rowController->indexBelow( idx );
        }
    }
}

int DateTimeGrid::Private::tabHeight( const QString& txt, QWidget* widget ) const
{
    QStyleOptionHeader opt;
    if ( widget ) opt.initFrom( widget );
    opt.text = txt;
    QStyle* style;
    if ( widget ) style = widget->style();
    else style = QApplication::style();
    QSize s = style->sizeFromContents(QStyle::CT_HeaderSection, &opt, QSize(), widget);
    return s.height();
}

void DateTimeGrid::Private::getAutomaticFormatters( DateTimeScaleFormatter** lower, DateTimeScaleFormatter** upper)
{
    const qreal tabw = QApplication::fontMetrics().width( QLatin1String( "XXXXX" ) );
    const qreal dayw = dayWidth;
    if ( dayw > 24*60*60*tabw ) {
        *lower = &minute_lower;
        *upper = &minute_upper;
    } else if ( dayw > 24*60*tabw ) {
        *lower = &hour_lower;
        *upper = &hour_upper;
    } else if ( dayw > 24*tabw ) {
        *lower = &day_lower;
        *upper = &day_upper;
    } else if ( dayw > tabw ) {
        *lower = &week_lower;
        *upper = &week_upper;
    } else if ( 4*dayw > tabw ) {
        *lower = &month_lower;
        *upper = &month_upper;
    } else {
        *lower = &year_lower;
        *upper = &year_upper;
    }
}


void DateTimeGrid::paintHeader( QPainter* painter,  const QRectF& headerRect, const QRectF& exposedRect,
                                qreal offset, QWidget* widget )
{
    painter->save();
    QPainterPath clipPath;
    clipPath.addRect( headerRect );
    painter->setClipPath( clipPath, Qt::IntersectClip );
    switch( scale() )
    {
    case ScaleHour:
        paintHourScaleHeader( painter, headerRect, exposedRect, offset, widget );
        break;
    case ScaleDay:
        paintDayScaleHeader( painter, headerRect, exposedRect, offset, widget );
        break;
    case ScaleWeek:
        {
            DateTimeScaleFormatter *lower = &d->week_lower;
            DateTimeScaleFormatter *upper = &d->week_upper;
            const qreal lowerHeight = d->tabHeight( lower->text( startDateTime() ) );
            const qreal upperHeight = d->tabHeight( upper->text( startDateTime() ) );
            const qreal upperRatio = upperHeight/( lowerHeight+upperHeight );

            const QRectF upperHeaderRect( headerRect.x(), headerRect.top(), headerRect.width()-1, headerRect.height() * upperRatio );
            const QRectF lowerHeaderRect( headerRect.x(), upperHeaderRect.bottom()+1, headerRect.width()-1,  headerRect.height()-upperHeaderRect.height()-1 );

            paintUserDefinedHeader( painter, lowerHeaderRect, exposedRect, offset, lower, widget );
            paintUserDefinedHeader( painter, upperHeaderRect, exposedRect, offset, upper, widget );
            break;
        }
    case ScaleMonth:
        {
            DateTimeScaleFormatter *lower = &d->month_lower;
            DateTimeScaleFormatter *upper = &d->month_upper;
            const qreal lowerHeight = d->tabHeight( lower->text( startDateTime() ) );
            const qreal upperHeight = d->tabHeight( upper->text( startDateTime() ) );
            const qreal upperRatio = upperHeight/( lowerHeight+upperHeight );

            const QRectF upperHeaderRect( headerRect.x(), headerRect.top(), headerRect.width()-1, headerRect.height() * upperRatio );
            const QRectF lowerHeaderRect( headerRect.x(), upperHeaderRect.bottom()+1, headerRect.width()-1,  headerRect.height()-upperHeaderRect.height()-1 );

            paintUserDefinedHeader( painter, lowerHeaderRect, exposedRect, offset, lower, widget );
            paintUserDefinedHeader( painter, upperHeaderRect, exposedRect, offset, upper, widget );
            break;
        }
    case ScaleAuto:
        {
            DateTimeScaleFormatter *lower, *upper;
            d->getAutomaticFormatters( &lower, &upper );
            const qreal lowerHeight = d->tabHeight( lower->text( startDateTime() ) );
            const qreal upperHeight = d->tabHeight( upper->text( startDateTime() ) );
            const qreal upperRatio = upperHeight/( lowerHeight+upperHeight );

            const QRectF upperHeaderRect( headerRect.x(), headerRect.top(), headerRect.width()-1, headerRect.height() * upperRatio );
            const QRectF lowerHeaderRect( headerRect.x(), upperHeaderRect.bottom()+1, headerRect.width()-1,  headerRect.height()-upperHeaderRect.height()-1 );

            paintUserDefinedHeader( painter, lowerHeaderRect, exposedRect, offset, lower, widget );
            paintUserDefinedHeader( painter, upperHeaderRect, exposedRect, offset, upper, widget );
            break;
        }
    case ScaleUserDefined:
        {
            const qreal lowerHeight = d->tabHeight( d->lower->text( startDateTime() ) );
            const qreal upperHeight = d->tabHeight( d->upper->text( startDateTime() ) );
            const qreal upperRatio = upperHeight/( lowerHeight+upperHeight );

            const QRectF upperHeaderRect( headerRect.x(), headerRect.top(), headerRect.width()-1, headerRect.height() * upperRatio );
            const QRectF lowerHeaderRect( headerRect.x(), upperHeaderRect.bottom()+1, headerRect.width()-1,  headerRect.height()-upperHeaderRect.height()-1 );

            paintUserDefinedHeader( painter, lowerHeaderRect, exposedRect, offset, d->lower, widget );
            paintUserDefinedHeader( painter, upperHeaderRect, exposedRect, offset, d->upper, widget );
        }
        break;
    }
    painter->restore();
}

void DateTimeGrid::paintUserDefinedHeader( QPainter* painter,
                                           const QRectF& headerRect, const QRectF& exposedRect,
                                           qreal offset, const DateTimeScaleFormatter* formatter,
                                           QWidget* widget )
{
    const QStyle* const style = widget ? widget->style() : QApplication::style();

    QDateTime dt = formatter->currentRangeBegin( d->chartXtoDateTime( offset + exposedRect.left() ) ).toUTC();
    qreal x = d->dateTimeToChartX( dt );

    while( x < exposedRect.right() + offset )
    {
        const QDateTime next = formatter->nextRangeBegin( dt );
        const qreal nextx = d->dateTimeToChartX( next );

        QStyleOptionHeader opt;
        if ( widget ) opt.init( widget );
        opt.rect = QRectF( x - offset+1, headerRect.top(), qMax<qreal>( 1., nextx-x-1 ), headerRect.height() ).toAlignedRect();
        opt.textAlignment = formatter->alignment();
        opt.text = formatter->text( dt );
        style->drawControl( QStyle::CE_Header, &opt, painter, widget );

        dt = next;
        x = nextx;
    }
}

/*! Paints the hour scale header.
 * \sa paintHeader()
 */
void DateTimeGrid::paintHourScaleHeader( QPainter* painter,
                                         const QRectF& headerRect, const QRectF& exposedRect,
                                         qreal offset, QWidget* widget )
{
    QStyle* style = widget?widget->style():QApplication::style();

    // Paint a section for each hour
    QDateTime dt = d->chartXtoDateTime( offset+exposedRect.left() );
    dt.setTime( QTime( dt.time().hour(), 0, 0, 0 ) );
    for ( qreal x = d->dateTimeToChartX( dt ); x < exposedRect.right()+offset;
          dt = dt.addSecs( 60*60 /*1 hour*/ ),x=d->dateTimeToChartX( dt ) ) {
        QStyleOptionHeader opt;
        if ( widget ) opt.init( widget );
        opt.rect = QRectF( x-offset+1, headerRect.top()+headerRect.height()/2., dayWidth()/24., headerRect.height()/2. ).toAlignedRect();
        opt.text = dt.time().toString( QString::fromAscii( "hh" ) );
        opt.textAlignment = Qt::AlignCenter;
        style->drawControl(QStyle::CE_Header, &opt, painter, widget);
    }

    dt = d->chartXtoDateTime( offset+exposedRect.left() );
    dt.setTime( QTime( 0, 0, 0, 0 ) );
    // Paint a section for each day
    for ( qreal x2 = d->dateTimeToChartX( dt ); x2 < exposedRect.right()+offset;
          dt = dt.addDays( 1 ),x2=d->dateTimeToChartX( dt ) ) {
        QStyleOptionHeader opt;
        opt.init( widget );
        opt.rect = QRectF( x2-offset, headerRect.top(), dayWidth(), headerRect.height()/2. ).toRect();
        opt.text = dt.date().toString();
        opt.textAlignment = Qt::AlignCenter;
        style->drawControl(QStyle::CE_Header, &opt, painter, widget);
    }
}

/*! Paints the day scale header.
 * \sa paintHeader()
 */
void DateTimeGrid::paintDayScaleHeader( QPainter* painter,  const QRectF& headerRect, const QRectF& exposedRect,
                                qreal offset, QWidget* widget )
{
    // For starters, support only the regular tab-per-day look
    QStyle* style = widget?widget->style():QApplication::style();

    // Paint a section for each day
    QDateTime dt = d->chartXtoDateTime( offset+exposedRect.left() );
    dt.setTime( QTime( 0, 0, 0, 0 ) );
    for ( qreal x = d->dateTimeToChartX( dt ); x < exposedRect.right()+offset;
          dt = dt.addDays( 1 ),x=d->dateTimeToChartX( dt ) ) {
        QStyleOptionHeader opt;
        opt.init( widget );
        opt.rect = QRectF( x-offset+1, headerRect.top()+headerRect.height()/2., dayWidth(), headerRect.height()/2. ).toAlignedRect();
        opt.text = dt.toString( QString::fromAscii( "ddd" ) ).left( 1 );
        opt.textAlignment = Qt::AlignCenter;
        style->drawControl(QStyle::CE_Header, &opt, painter, widget);
    }

    dt = d->chartXtoDateTime( offset+exposedRect.left() );
    dt.setTime( QTime( 0, 0, 0, 0 ) );
    // Go backwards until start of week
    while ( dt.date().dayOfWeek() != d->weekStart ) dt = dt.addDays( -1 );
    // Paint a section for each week
    for ( qreal x2 = d->dateTimeToChartX( dt ); x2 < exposedRect.right()+offset;
          dt = dt.addDays( 7 ),x2=d->dateTimeToChartX( dt ) ) {
        QStyleOptionHeader opt;
        opt.init( widget );
        opt.rect = QRectF( x2-offset, headerRect.top(), dayWidth()*7., headerRect.height()/2. ).toRect();
        opt.text = QString::number( dt.date().weekNumber() );
        opt.textAlignment = Qt::AlignCenter;
        style->drawControl(QStyle::CE_Header, &opt, painter, widget);
    }
}

#undef d

#ifndef KDAB_NO_UNIT_TESTS

#include <QStandardItemModel>
#include "unittest/test.h"

namespace {
    std::ostream& operator<<( std::ostream& os, const QDateTime& dt )
    {
#ifdef QT_NO_STL
        os << dt.toString().toLatin1().constData();
#else
        os << dt.toString().toStdString();
#endif
        return os;
    }
}

KDAB_SCOPED_UNITTEST_SIMPLE( KDGantt, DateTimeGrid, "test" ) {
    QStandardItemModel model( 3, 2 );
    DateTimeGrid grid;
    QDateTime dt = QDateTime::currentDateTime();
    grid.setModel( &model );
    QDateTime startdt = dt.addDays( -10 );
    grid.setStartDateTime( startdt );

    model.setData( model.index( 0, 0 ), dt,               StartTimeRole );
    model.setData( model.index( 0, 0 ), dt.addDays( 17 ), EndTimeRole );

    model.setData( model.index( 2, 0 ), dt.addDays( 18 ), StartTimeRole );
    model.setData( model.index( 2, 0 ), dt.addDays( 19 ), EndTimeRole );

    Span s = grid.mapToChart( model.index( 0, 0 ) );
    //qDebug() << "span="<<s;

    assertTrue( s.start()>0 );
    assertTrue( s.length()>0 );

    assertTrue( startdt == grid.mapToDateTime( grid.mapFromDateTime( startdt ) ) );

    grid.mapFromChart( s, model.index( 1, 0 ) );

    QDateTime s1 = model.data( model.index( 0, 0 ), StartTimeRole ).toDateTime();
    QDateTime e1 = model.data( model.index( 0, 0 ), EndTimeRole ).toDateTime();
    QDateTime s2 = model.data( model.index( 1, 0 ), StartTimeRole ).toDateTime();
    QDateTime e2 = model.data( model.index( 1, 0 ), EndTimeRole ).toDateTime();

    assertTrue( s1.isValid() );
    assertTrue( e1.isValid() );
    assertTrue( s2.isValid() );
    assertTrue( e2.isValid() );

    assertEqual( s1, s2 );
    assertEqual( e1, e2 );

    assertTrue( grid.isSatisfiedConstraint( Constraint( model.index( 0, 0 ), model.index( 2, 0 ) ) ) );
    assertFalse( grid.isSatisfiedConstraint( Constraint( model.index( 2, 0 ), model.index( 0, 0 ) ) ) );

    s = grid.mapToChart( model.index( 0, 0 ) );
    s.setEnd( s.end()+100000. );
    bool rc = grid.mapFromChart( s, model.index( 0, 0 ) );
    assertTrue( rc );
    assertEqual( s1, model.data( model.index( 0, 0 ), StartTimeRole ).toDateTime() );
    Span newspan = grid.mapToChart( model.index( 0, 0 ) );
    assertEqual( newspan.start(), s.start() );
    assertEqual( newspan.length(), s.length() );

    {
        QDateTime startDateTime = QDateTime::currentDateTime();
        qreal dayWidth = 100;
        QDate currentDate = QDate::currentDate();
        QDateTime dt( QDate(currentDate.year(), 1, 1),  QTime( 0, 0, 0, 0 ) );
        assert( dt.isValid() );
        qreal result = startDateTime.date().daysTo(dt.date())*24.*60.*60.;
        result += startDateTime.time().msecsTo(dt.time())/1000.;
        result *= dayWidth/( 24.*60.*60. );

        int days = static_cast<int>( result/dayWidth );
        qreal secs = result*( 24.*60.*60. )/dayWidth;
        QDateTime dt2 = startDateTime;
        QDateTime result2 = dt2.addDays( days ).addSecs( static_cast<int>(secs-(days*24.*60.*60.) ) ).addMSecs( qRound( ( secs-static_cast<int>( secs ) )*1000. ) );

        assertEqual( dt, result2 );
    }
}

#endif /* KDAB_NO_UNIT_TESTS */

#include "moc_kdganttdatetimegrid.cpp"
