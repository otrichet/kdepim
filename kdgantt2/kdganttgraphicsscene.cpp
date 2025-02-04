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
#include "kdganttgraphicsscene.h"
#include "kdganttgraphicsscene_p.h"
#include "kdganttgraphicsitem.h"
#include "kdganttconstraint.h"
#include "kdganttconstraintgraphicsitem.h"
#include "kdganttitemdelegate.h"
#include "kdganttabstractrowcontroller.h"
#include "kdganttdatetimegrid.h"
#include "kdganttsummaryhandlingproxymodel.h"

#include <QApplication>
#include <QGraphicsSceneHelpEvent>
#include <QPainter>
#include <QPrinter>
#include <QTextDocument>
#include <QToolTip>
#include <QSet>

#include <QDebug>

#include <functional>
#include <algorithm>
#include <cassert>

/* Older Qt dont have this macro, so define it... */
#ifndef QT_VERSION_CHECK
#  define QT_VERSION_CHECK(major, minor, patch) ((major<<16)|(minor<<8)|(patch))
#endif

/*!\class KDGantt::GraphicsScene
 * \internal
 */

using namespace KDGantt;

GraphicsScene::Private::Private( GraphicsScene* _q )
    : q( _q ),
      dragSource( 0 ),
      itemDelegate( new ItemDelegate( _q ) ),
      rowController( 0 ),
      grid( &default_grid ),
      readOnly( false ),
      isPrinting( false ),
      summaryHandlingModel( new SummaryHandlingProxyModel( _q ) ),
      selectionModel( 0 )
{
    default_grid.setStartDateTime( QDateTime::currentDateTime().addDays( -1 ) );
}

void GraphicsScene::Private::resetConstraintItems()
{
    q->clearConstraintItems();
    if ( constraintModel.isNull() ) return;
    QList<Constraint> clst = constraintModel->constraints();
    Q_FOREACH( Constraint c, clst ) {
        createConstraintItem( c );
    }
    q->updateItems();
}

void GraphicsScene::Private::createConstraintItem( const Constraint& c )
{
    GraphicsItem* sitem = q->findItem( summaryHandlingModel->mapFromSource( c.startIndex() ) );
    GraphicsItem* eitem = q->findItem( summaryHandlingModel->mapFromSource( c.endIndex() ) );

    if ( sitem && eitem ) {
        ConstraintGraphicsItem* citem = new ConstraintGraphicsItem( c );
        sitem->addStartConstraint( citem );
        eitem->addEndConstraint( citem );
        q->addItem( citem );
    }



    //q->insertConstraintItem( c, citem );
}

// Delete the constraint item, and clean up pointers in the start- and end item
void GraphicsScene::Private::deleteConstraintItem( ConstraintGraphicsItem *citem )
{
    //qDebug()<<"GraphicsScene::Private::deleteConstraintItem citem="<<(void*)citem;
    if ( citem == 0 ) {
        return;
    }
    Constraint c = citem->constraint();
    GraphicsItem* item = items.value( summaryHandlingModel->mapFromSource( c.startIndex() ), 0 );
    if ( item ) {
        //qDebug()<<"GraphicsScene::Private::deleteConstraintItem startConstraints"<<item<<(void*)citem;
        item->removeStartConstraint( citem );
    } //else qDebug()<<"GraphicsScene::Private::deleteConstraintItem"<<c.startIndex()<<"start item not found";
    item = items.value( summaryHandlingModel->mapFromSource( c.endIndex() ), 0 );
    if ( item ) {
        //qDebug()<<"GraphicsScene::Private::deleteConstraintItem endConstraints"<<item<<(void*)citem;
        item->removeEndConstraint( citem );
    } //else qDebug()<<"GraphicsScene::Private::deleteConstraintItem"<<c.endIndex()<<"end item not found";
    //qDebug()<<"GraphicsScene::Private::deleteConstraintItem"<<citem<<"deleted";
    delete citem;
}

void GraphicsScene::Private::deleteConstraintItem( const Constraint& c )
{
    //qDebug()<<"GraphicsScene::Private::deleteConstraintItem c="<<c;
    deleteConstraintItem( findConstraintItem( c ) );
}

ConstraintGraphicsItem* GraphicsScene::Private::findConstraintItem( const Constraint& c ) const
{
    GraphicsItem* item = items.value( summaryHandlingModel->mapFromSource( c.startIndex() ), 0 );
    if ( item ) {
        QList<ConstraintGraphicsItem*> clst = item->startConstraints();
        QList<ConstraintGraphicsItem*>::iterator it = clst.begin();
        //qDebug()<<"GraphicsScene::Private::findConstraintItem start:"<<c<<item<<clst;
        for( ; it != clst.end() ; ++it )
            if ((*it)->constraint() == c )
                break;
        if (  it != clst.end() ) {
            return *it;
        }
    }
    item = items.value( summaryHandlingModel->mapFromSource( c.endIndex() ), 0 );
    if ( item ) {
        QList<ConstraintGraphicsItem*> clst = item->endConstraints();
        QList<ConstraintGraphicsItem*>::iterator it = clst.begin();
        //qDebug()<<"GraphicsScene::Private::findConstraintItem end:"<<c<<item<<clst;
        for( ; it != clst.end() ; ++it )
            if ((*it)->constraint() == c )
                break;
        if (  it != clst.end() ) {
            return *it;
        }
    }
    //qDebug()<<"GraphicsScene::Private::findConstraintItem No item or constraintitem"<<c;
    return 0;
}

GraphicsScene::GraphicsScene( QObject* parent )
    : QGraphicsScene( parent ), _d( new Private( this ) )
{
    init();
}

GraphicsScene::~GraphicsScene()
{
    clearConstraintItems();
    clearItems();
}

#define d d_func()

void GraphicsScene::init()
{
    setItemIndexMethod( QGraphicsScene::NoIndex );
    setConstraintModel( new ConstraintModel( this ) );
    connect( d->grid, SIGNAL( gridChanged() ), this, SLOT( slotGridChanged() ) );
}

/* NOTE: The delegate should really be a property
 * of the view, but that doesn't really fit at
 * this time
 */
void GraphicsScene::setItemDelegate( ItemDelegate* delegate )
{
    if ( !d->itemDelegate.isNull() && d->itemDelegate->parent()==this ) delete d->itemDelegate;
    d->itemDelegate = delegate;
    update();
}

ItemDelegate* GraphicsScene::itemDelegate() const
{
    return d->itemDelegate;
}

QAbstractItemModel* GraphicsScene::model() const
{
    assert(!d->summaryHandlingModel.isNull());
    return d->summaryHandlingModel->sourceModel();
}

void GraphicsScene::setModel( QAbstractItemModel* model )
{
    assert(!d->summaryHandlingModel.isNull());
    d->summaryHandlingModel->setSourceModel(model);
    d->grid->setModel( d->summaryHandlingModel );
    setSelectionModel( new QItemSelectionModel( model, this ) );
}

QAbstractProxyModel* GraphicsScene::summaryHandlingModel() const
{
    return d->summaryHandlingModel;
}

void GraphicsScene::setSummaryHandlingModel( QAbstractProxyModel* proxyModel )
{
    proxyModel->setSourceModel( model() );
    d->summaryHandlingModel = proxyModel;
}

void GraphicsScene::setRootIndex( const QModelIndex& idx )
{
    d->grid->setRootIndex( idx );
}

QModelIndex GraphicsScene::rootIndex() const
{
    return d->grid->rootIndex();
}

ConstraintModel* GraphicsScene::constraintModel() const
{
    return d->constraintModel;
}

void GraphicsScene::setConstraintModel( ConstraintModel* cm )
{
    if ( !d->constraintModel.isNull() ) {
        disconnect( d->constraintModel );
    }
    d->constraintModel = cm;

    connect( cm, SIGNAL( constraintAdded( const Constraint& ) ), this, SLOT( slotConstraintAdded( const Constraint& ) ) );
    connect( cm, SIGNAL( constraintRemoved( const Constraint& ) ), this, SLOT( slotConstraintRemoved( const Constraint& ) ) );
    d->resetConstraintItems();
}

void GraphicsScene::setSelectionModel( QItemSelectionModel* smodel )
{
    d->selectionModel = smodel;
    // TODO: update selection from model and connect signals
}

QItemSelectionModel* GraphicsScene::selectionModel() const
{
    return d->selectionModel;
}

void GraphicsScene::setRowController( AbstractRowController* rc )
{
    d->rowController = rc;
}

AbstractRowController* GraphicsScene::rowController() const
{
    return d->rowController;
}

void GraphicsScene::setGrid( AbstractGrid* grid )
{
    QAbstractItemModel* model = d->grid->model();
    if ( grid == 0 ) grid = &d->default_grid;
    if ( d->grid ) disconnect( d->grid );
    d->grid = grid;
    connect( d->grid, SIGNAL( gridChanged() ), this, SLOT( slotGridChanged() ) );
    d->grid->setModel( model );
    slotGridChanged();
}

AbstractGrid* GraphicsScene::grid() const
{
    return d->grid;
}

void GraphicsScene::setReadOnly( bool ro )
{
    d->readOnly = ro;
}

bool GraphicsScene::isReadOnly() const
{
    return d->readOnly;
}

/* Returns the index with column=0 fromt the
 * same row as idx and with the same parent.
 * This is used to traverse the tree-structure
 * of the model
 */
QModelIndex GraphicsScene::mainIndex( const QModelIndex& idx )
{
#if 0
    if ( idx.isValid() ) {
        return idx.model()->index( idx.row(), 0,idx.parent() );
    } else {
        return QModelIndex();
    }
#else
    return idx;
#endif
}

/*! Returns the index pointing to the last column
 * in the same row as idx. This can be thought of
 * as in "inverse" of mainIndex()
 */
QModelIndex GraphicsScene::dataIndex( const QModelIndex& idx )
{
#if 0
    if ( idx.isValid() ) {
        const QAbstractItemModel* model = idx.model();
        return model->index( idx.row(), model->columnCount( idx.parent() )-1,idx.parent() );
    } else {
        return QModelIndex();
    }
#else
    return idx;
#endif
}

/*! Creates a new item of type type.
 * TODO: If the user should be allowed to override
 * this in any way, it needs to be in View!
 */
GraphicsItem* GraphicsScene::createItem( ItemType type ) const
{
#if 0
    switch( type ) {
    case TypeEvent:   return 0;
    case TypeTask:    return new TaskItem;
    case TypeSummary: return new SummaryItem;
    default:          return 0;
    }
#endif
    //qDebug() << "GraphicsScene::createItem("<<type<<")";
    Q_UNUSED( type );
    return new GraphicsItem;
}

void GraphicsScene::Private::recursiveUpdateMultiItem( const Span& span, const QModelIndex& idx )
{
    //qDebug() << "recursiveUpdateMultiItem("<<span<<idx<<")";
    GraphicsItem* item = q->findItem( idx );
    const int itemtype = summaryHandlingModel->data( idx, ItemTypeRole ).toInt();
    if (!item) {
        item = q->createItem( static_cast<ItemType>( itemtype ) );
        item->setIndex( idx );
        q->insertItem( idx, item);
    }
    item->updateItem( span, idx );
    QModelIndex child;
    int cr = 0;
    while ( ( child = idx.child( cr, 0 ) ).isValid() ) {
        recursiveUpdateMultiItem( span, child );
        ++cr;
    }
}

void GraphicsScene::updateRow( const QModelIndex& rowidx )
{
    //qDebug() << "GraphicsScene::updateRow("<<rowidx<<")" << rowidx.data( Qt::DisplayRole );
    if ( !rowidx.isValid() ) return;
    const QAbstractItemModel* model = rowidx.model(); // why const?
    assert( model );
    assert( rowController() );
    assert( model == summaryHandlingModel() );

    const QModelIndex sidx = summaryHandlingModel()->mapToSource( rowidx );
    Span rg = rowController()->rowGeometry( sidx );
    for ( QModelIndex treewalkidx = sidx; treewalkidx.isValid(); treewalkidx = treewalkidx.parent() ) {
        if ( treewalkidx.data( ItemTypeRole ).toInt() == TypeMulti
             && !rowController()->isRowExpanded( treewalkidx )) {
            rg = rowController()->rowGeometry( treewalkidx );
        }
    }

    bool blocked = blockSignals( true );
    for ( int col = 0; col < summaryHandlingModel()->columnCount( rowidx.parent() ); ++col ) {
        const QModelIndex idx = summaryHandlingModel()->index( rowidx.row(), col, rowidx.parent() );
        const QModelIndex sidx = summaryHandlingModel()->mapToSource( idx );
        const int itemtype = summaryHandlingModel()->data( idx, ItemTypeRole ).toInt();
        const bool isExpanded = rowController()->isRowExpanded( sidx );
        if ( itemtype == TypeNone ) {
            removeItem( idx );
            continue;
        }
        if ( itemtype == TypeMulti && !isExpanded ) {
            d->recursiveUpdateMultiItem( rg, idx );
        } else {
            if ( summaryHandlingModel()->data( rowidx.parent(), ItemTypeRole ).toInt() == TypeMulti && !isExpanded ) {
                //continue;
            }

            GraphicsItem* item = findItem( idx );
            if (!item) {
                item = createItem( static_cast<ItemType>( itemtype ) );
                item->setIndex( idx );
                insertItem(idx, item);
            }
            const Span span = rowController()->rowGeometry( sidx );
            item->updateItem( span, idx );
        }
    }
    blockSignals( blocked );
}

void GraphicsScene::insertItem( const QPersistentModelIndex& idx, GraphicsItem* item )
{
    if ( !d->constraintModel.isNull() ) {
        // Create items for constraints
        const QModelIndex sidx = summaryHandlingModel()->mapToSource( idx );
        const QList<Constraint> clst = d->constraintModel->constraintsForIndex( sidx );
        Q_FOREACH( Constraint c,  clst ) {
            QModelIndex other_idx;
            if ( c.startIndex() == sidx ) {
                other_idx = c.endIndex();
                GraphicsItem* other_item = d->items.value(summaryHandlingModel()->mapFromSource( other_idx ),0);
                if ( !other_item ) continue;
                ConstraintGraphicsItem* citem = new ConstraintGraphicsItem( c );
                item->addStartConstraint( citem );
                other_item->addEndConstraint( citem );
                addItem( citem );
            } else if ( c.endIndex() == sidx ) {
                other_idx = c.startIndex();
                GraphicsItem* other_item = d->items.value(summaryHandlingModel()->mapFromSource( other_idx ),0);
                if ( !other_item ) continue;
                ConstraintGraphicsItem* citem = new ConstraintGraphicsItem( c );
                other_item->addStartConstraint( citem );
                item->addEndConstraint( citem );
                addItem( citem );
            } else {
                assert( 0 ); // Impossible
            }
        }
    }
    d->items.insert( idx, item );
    addItem( item );
}

void GraphicsScene::removeItem( const QModelIndex& idx )
{
    //qDebug() << "GraphicsScene::removeItem("<<idx<<")";
    QHash<QPersistentModelIndex,GraphicsItem*>::iterator it = d->items.find( idx );
    if ( it != d->items.end() ) {
        GraphicsItem* item = *it;
        assert( item );
        // We have to remove the item from the list first because
        // there is a good chance there will be reentrant calls
        d->items.erase( it );
        {
            // Remove any constraintitems attached
            const QSet<ConstraintGraphicsItem*> clst = QSet<ConstraintGraphicsItem*>::fromList( item->startConstraints() ) +
                                                       QSet<ConstraintGraphicsItem*>::fromList( item->endConstraints() );
            Q_FOREACH( ConstraintGraphicsItem* citem, clst ) {
                d->deleteConstraintItem( citem );
            }
        }
        // Get rid of the item
        delete item;
    }
}

GraphicsItem* GraphicsScene::findItem( const QModelIndex& idx ) const
{
    if ( !idx.isValid() ) return 0;
    assert( idx.model() == summaryHandlingModel() );
    QHash<QPersistentModelIndex,GraphicsItem*>::const_iterator it = d->items.find( idx );
    return ( it != d->items.end() )?*it:0;
}

GraphicsItem* GraphicsScene::findItem( const QPersistentModelIndex& idx ) const
{
    if ( !idx.isValid() ) return 0;
    assert( idx.model() == summaryHandlingModel() );
    QHash<QPersistentModelIndex,GraphicsItem*>::const_iterator it = d->items.find( idx );
    return ( it != d->items.end() )?*it:0;
}

void GraphicsScene::clearItems()
{
    // TODO constraints
    qDeleteAll( items() );
    d->items.clear();
}

void GraphicsScene::updateItems()
{
    for ( QHash<QPersistentModelIndex,GraphicsItem*>::iterator it = d->items.begin();
          it != d->items.end(); ++it ) {
        GraphicsItem* const item = it.value();
        const QPersistentModelIndex& idx = it.key();
        item->updateItem( Span( item->pos().y(), item->rect().height() ), idx );
    }
    invalidate( QRectF(), QGraphicsScene::BackgroundLayer );
}

void GraphicsScene::deleteSubtree( const QModelIndex& _idx )
{
    QModelIndex idx = dataIndex( _idx );
    if ( !idx.model() ) return;
    const QModelIndex parent( idx.parent() );
    const int colcount = idx.model()->columnCount( parent );
    {for ( int i = 0; i < colcount; ++i ) {
        removeItem( parent.child( idx.row(), i ) );
    }}
    const int rowcount = summaryHandlingModel()->rowCount( _idx );
    {for ( int i = 0; i < rowcount; ++i ) {
        deleteSubtree( summaryHandlingModel()->index( i, summaryHandlingModel()->columnCount(_idx)-1, _idx ) );
    }}
}


ConstraintGraphicsItem* GraphicsScene::findConstraintItem( const Constraint& c ) const
{
    return d->findConstraintItem( c );
}

void GraphicsScene::clearConstraintItems()
{
    // TODO
    // d->constraintItems.clearConstraintItems();
}

void GraphicsScene::slotConstraintAdded( const Constraint& c )
{
    d->createConstraintItem( c );
}

void GraphicsScene::slotConstraintRemoved( const Constraint& c )
{
    d->deleteConstraintItem( c );
}

void GraphicsScene::slotGridChanged()
{
    updateItems();
    update();
    emit gridChanged();
}

void GraphicsScene::helpEvent( QGraphicsSceneHelpEvent *helpEvent )
{
#ifndef QT_NO_TOOLTIP
    QGraphicsItem *item = itemAt( helpEvent->scenePos() );
    if ( GraphicsItem* gitem = qgraphicsitem_cast<GraphicsItem*>( item ) ) {
        QToolTip::showText(helpEvent->screenPos(), gitem->ganttToolTip());
    } else if ( ConstraintGraphicsItem* citem = qgraphicsitem_cast<ConstraintGraphicsItem*>( item ) ) {
        QToolTip::showText(helpEvent->screenPos(), citem->ganttToolTip());
    } else {
        QGraphicsScene::helpEvent( helpEvent );
    }
#endif /* QT_NO_TOOLTIP */
}

void GraphicsScene::drawBackground( QPainter* painter, const QRectF& _rect )
{
    QRectF scn( sceneRect() );
    QRectF rect( _rect );
    if ( d->isPrinting ) {
        QRectF headerRect( scn.topLeft()+QPointF( d->labelsWidth, 0 ),
                           QSizeF( scn.width()-d->labelsWidth, d->rowController->headerHeight() ));
        d->grid->paintHeader( painter, headerRect, rect, 0, 0 );

        /* We have to blank out the part of the header that is invisible during
     * normal rendering when we are printing.
     */
        QRectF labelsTabRect( scn.topLeft(), QSizeF( d->labelsWidth, headerRect.height() ) );

        QStyleOptionHeader opt;
        opt.rect = labelsTabRect.toRect();
        opt.text = "";
        opt.textAlignment = Qt::AlignCenter;
#if QT_VERSION >= QT_VERSION_CHECK(4, 4, 0)
        style()->drawControl(QStyle::CE_Header, &opt, painter, 0);
#else
        QApplication::style()->drawControl(QStyle::CE_Header, &opt, painter, 0);
#endif
        scn.setTop( headerRect.bottom() );
        scn.setLeft( headerRect.left() );
        rect = rect.intersected( scn );
    }
    d->grid->paintGrid( painter, scn, rect, d->rowController );
}

void GraphicsScene::itemEntered( const QModelIndex& idx )
{
    emit entered( idx );
}

void GraphicsScene::itemPressed( const QModelIndex& idx )
{
    emit pressed( idx );
}

void GraphicsScene::itemClicked( const QModelIndex& idx )
{
    emit clicked( idx );
}

void GraphicsScene::itemDoubleClicked( const QModelIndex& idx )
{
    emit doubleClicked( idx );
}

void GraphicsScene::setDragSource( GraphicsItem* item )
{
    d->dragSource = item;
}

GraphicsItem* GraphicsScene::dragSource() const
{
    return d->dragSource;
}

/*! Print the Gantt chart using \a printer. If \a drawRowLabels
 * is true (the default), each row will have it's label printed
 * on the left side.
 *
 * This version of print() will print multiple pages.
 */
void GraphicsScene::print( QPrinter* printer, bool drawRowLabels )
{
//There is no printer support under wince
#ifndef _WIN32_WCE
    QPainter painter( printer );
    doPrint( &painter, printer->pageRect(), sceneRect().left(), sceneRect().right(), printer, drawRowLabels );
#endif
}

/*! Print part of the Gantt chart from \a start to \a end using \a printer.
 * If \a drawRowLabels is true (the default), each row will have it's
 * label printed on the left side.
 *
 * This version of print() will print multiple pages.
 *
 * To print a certain range of a chart with a DateTimeGrid, use
 * qreal DateTimeGrid::mapFromDateTime( const QDateTime& dt) const
 * to figure out the values for \a start and \a end.
 */
void GraphicsScene::print( QPrinter* printer, qreal start, qreal end, bool drawRowLabels )
{
//There is no printer support under wince
#ifndef _WIN32_WCE
    QPainter painter( printer );
    doPrint( &painter, printer->pageRect(), start, end, printer, drawRowLabels );
#endif
}

/*! Render the GanttView inside the rectangle \a target using the painter \a painter.
 * If \a drawRowLabels is true (the default), each row will have it's
 * label printed on the left side.
 */
void GraphicsScene::print( QPainter* painter, const QRectF& _targetRect, bool drawRowLabels )
{
//There is no printer support under wince
#ifndef _WIN32_WCE
    QRectF targetRect( _targetRect );
    if ( targetRect.isNull() ) {
        targetRect = sceneRect();
    }

    doPrint( painter, targetRect, sceneRect().left(), sceneRect().right(), 0, drawRowLabels );
#endif
}

/*! Render the GanttView inside the rectangle \a target using the painter \a painter.
 * If \a drawRowLabels is true (the default), each row will have it's
 * label printed on the left side.
 *
 * To print a certain range of a chart with a DateTimeGrid, use
 * qreal DateTimeGrid::mapFromDateTime( const QDateTime& dt) const
 * to figure out the values for \a start and \a end.
 */
void GraphicsScene::print( QPainter* painter, qreal start, qreal end,
                           const QRectF& _targetRect, bool drawRowLabels )
{
//There is no printer support under wince
#ifndef _WIN32_WCE
    QRectF targetRect( _targetRect );
    if ( targetRect.isNull() ) {
        targetRect = sceneRect();
    }

    doPrint( painter, targetRect, start, end, 0, drawRowLabels );
#endif
}

/*!\internal
 */
void GraphicsScene::doPrint( QPainter* painter, const QRectF& targetRect,
                             qreal start, qreal end,
                             QPrinter* printer, bool drawRowLabels )
{
//There is no printer support under wince
#ifndef _WIN32_WCE
    assert( painter );
    d->isPrinting = true;
#if QT_VERSION >= QT_VERSION_CHECK(4, 4, 0)
    QFont sceneFont( font() );
    if ( printer ) {
        sceneFont = QFont( font(), printer );
        sceneFont.setPixelSize( font().pointSize() );
    }
#else
    QFont sceneFont( painter->font() );
    if ( printer ) {
        sceneFont = QFont( painter->font(), printer );
        sceneFont.setPixelSize( painter->font().pointSize() );
    }
#endif

    const QRectF oldScnRect( sceneRect() );
    QRectF scnRect( oldScnRect );
    scnRect.setLeft( start );
    scnRect.setRight( end );
    scnRect.setTop( -d->rowController->headerHeight() );
    bool b = blockSignals( true );

    /* row labels */
    QVector<QGraphicsTextItem*> textLabels;
    if ( drawRowLabels ) {
        qreal textWidth = 0.;
        QModelIndex sidx = summaryHandlingModel()->mapToSource( summaryHandlingModel()->index( 0, 0, rootIndex()) );
        do {
            QModelIndex idx = summaryHandlingModel()->mapFromSource( sidx );
            const Span rg=rowController()->rowGeometry( sidx );
            const QString txt = idx.data( Qt::DisplayRole ).toString();
            QGraphicsTextItem* item = new QGraphicsTextItem( txt );
            addItem( item );
            textLabels << item;
            item->adjustSize();
            textWidth = qMax( item->textWidth(), textWidth );
            item->setPos( 0, rg.start() );
        } while ( ( sidx = rowController()->indexBelow( sidx ) ).isValid() );
        // Add a little margin to textWidth
        textWidth += QFontMetricsF(sceneFont).width( QString::fromLatin1( "X" ) );
        Q_FOREACH( QGraphicsTextItem* item, textLabels ) {
            item->setPos( scnRect.left()-textWidth, item->y() );
            item->show();
        }
        scnRect.setLeft( scnRect.left()-textWidth );
        d->labelsWidth = textWidth;
    }

    setSceneRect( scnRect );

    painter->save();
    painter->setClipRect( targetRect );

    qreal yratio = targetRect.height()/scnRect.height();
    /* If we're not printing multiple pages,
     * check if the span fits and adjust:
     */
    if ( !printer && targetRect.width()/scnRect.width() < yratio ) {
        yratio = targetRect.width()/scnRect.width();
    }

    qreal offset = scnRect.left();
    int pagecount = 0;
    while ( offset < scnRect.width() ) {
        painter->setFont( sceneFont );
        render( painter, targetRect, QRectF( QPointF( offset, scnRect.top()),
                                             QSizeF( targetRect.width()/yratio, scnRect.height() ) ) );
        offset += targetRect.width()/yratio;
        ++pagecount;
        if ( printer && offset < scnRect.width() ) {
            printer->newPage();
        } else {
            break;
        }
    }

    d->isPrinting = false;
    qDeleteAll( textLabels );
    blockSignals( b );
    setSceneRect( oldScnRect );
    painter->restore();
#endif
}

#include "moc_kdganttgraphicsscene.cpp"

