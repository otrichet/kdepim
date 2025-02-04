/*
  Copyright (c) 2009 KDAB
  Author: Frank Osterfeld <osterfeld@kde.org>

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
#include "calendarsearch.h"
#include "calendarmodel.h"
#include "calendarsearchinterface.h"
#include "calfilterproxymodel.h"
#include "daterangefilterproxymodel.h"
#include "incidencefilterproxymodel.h"
#include "utils.h"

#include <Akonadi/ChangeRecorder>
#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/EntityMimeTypeFilterModel>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Session>

#include <KDateTime>
#include <KJob>
#include <KRandom>
#include <kselectionproxymodel.h>  //krazy:exclude=camelcase to build against kdelibs4.5

#include <QItemSelection>
#include <QItemSelectionModel>
#include <QTimer>
#include <QVariantMap>

using namespace CalendarSupport;

/*
static QString dateToString( const KDateTime &dt )
{
  return dt.isValid() ? dt.toString() : QString();
}
*/

class CalendarSearch::Private
{
  CalendarSearch *const q;

  public:
    explicit Private( CalendarSearch *qq );
    ~Private() {
      //TODO delete created search collection
    }

    void updateSearch();
    void triggerDelayedUpdate();
    void searchCreated( const QVariantMap &result );
    void collectionSelectionChanged( const QItemSelection &, const QItemSelection & );
    void collectionFetched( KJob *job );
    void rowsInserted( const QModelIndex &, int, int );

    Akonadi::Collection createdCollection;
    KDateTime startDate;
    KDateTime endDate;
    QTimer updateTimer;
    OrgFreedesktopAkonadiCalendarSearchAgentInterface *interface;
    QString errorString;
    CalendarSearch::Error error;
    CalendarModel *calendarModel;
    Akonadi::EntityMimeTypeFilterModel *filterProxy;
    KSelectionProxyModel *selectionProxyModel;
    DateRangeFilterProxyModel *dateRangeProxyModel;
    IncidenceFilterProxyModel *incidenceFilterProxyModel;
    CalFilterProxyModel *kcalFilterProxyModel; // support for libkcal filters
    QItemSelectionModel *selectionModel;
    Akonadi::ChangeRecorder *monitor;
    CalendarSearch::IncidenceTypes incidenceTypes;
    QList<Akonadi::Collection::Id> preselectedCollections;
};

CalendarSearch::Private::Private( CalendarSearch *qq )
  : q( qq ),
    interface( new OrgFreedesktopAkonadiCalendarSearchAgentInterface(
                 QLatin1String( "org.freedesktop.Akonadi.Agent.akonadi_calendarsearch_agent" ),
                 QLatin1String( "/CalendarSearchAgent" ),
                 QDBusConnection::sessionBus(),
                 q ) ),
    error( CalendarSearch::NoError ),
    selectionProxyModel( 0 ),
    selectionModel( 0 ),
    incidenceTypes( CalendarSearch::Events|CalendarSearch::Todos|CalendarSearch::Journals )
{
  updateTimer.setSingleShot( true );
  updateTimer.setInterval( 0 );
  q->connect( &updateTimer, SIGNAL(timeout()), q, SLOT(updateSearch()) );

  Akonadi::Session *session =
    new Akonadi::Session( "CalendarSearch-" + KRandom::randomString( 8 ).toLatin1(), q );
  monitor = new Akonadi::ChangeRecorder( q );
  monitor->setSession( session );

  Akonadi::ItemFetchScope scope;
  scope.fetchFullPayload( true );
  scope.fetchAttribute<Akonadi::EntityDisplayAttribute>();

  monitor->setCollectionMonitored( Akonadi::Collection::root() );
  monitor->fetchCollection( true );
  monitor->setItemFetchScope( scope );
  monitor->setMimeTypeMonitored( "text/calendar" );
  monitor->setMimeTypeMonitored( KCalCore::Event::eventMimeType(), true );
  monitor->setMimeTypeMonitored( KCalCore::Todo::todoMimeType(), true );
  monitor->setMimeTypeMonitored( KCalCore::Journal::journalMimeType(), true );

  calendarModel = new CalendarModel( monitor, q );
  connect( calendarModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
           q, SLOT(rowsInserted(QModelIndex,int,int)) );

  selectionModel = new QItemSelectionModel( calendarModel );
  selectionProxyModel = new KSelectionProxyModel( selectionModel, q );
  selectionProxyModel->setFilterBehavior( KSelectionProxyModel::ChildrenOfExactSelection );
  selectionProxyModel->setSourceModel( calendarModel );

  incidenceFilterProxyModel = new IncidenceFilterProxyModel( q );
  incidenceFilterProxyModel->setDynamicSortFilter( true );
  incidenceFilterProxyModel->setSourceModel( selectionProxyModel );
  incidenceFilterProxyModel->showAll();

  kcalFilterProxyModel = new CalFilterProxyModel( q );
  kcalFilterProxyModel->setSourceModel( incidenceFilterProxyModel );
  kcalFilterProxyModel->setDynamicSortFilter( true );

  dateRangeProxyModel = new DateRangeFilterProxyModel;
  dateRangeProxyModel->setDynamicSortFilter( true );
  dateRangeProxyModel->setSourceModel( kcalFilterProxyModel );
}

void CalendarSearch::Private::updateSearch()
{
#if 0
  const QString startStr = dateToString( startDate );
  const QString endStr = dateToString( endDate );
  if ( !interface->callWithCallback( QLatin1String( "createSearch" ),
                                     QList<QVariant>() << startStr << endStr, q,
                                     SLOT(searchCreated(QVariantMap)) ) ) {
    error = CalendarSearch::SomeError;
    errorString = i18n( "Could not create search." );
    emit q->errorOccurredceived();
  }
#endif
}

void CalendarSearch::Private::triggerDelayedUpdate()
{
  if ( !updateTimer.isActive() ) {
    updateTimer.start();
  }
}

#if 0
void CalendarSearch::Private::searchCreated( const QVariantMap &result )
{
  kDebug() << "search created";
  bool ok = false;
  const int errorCode = result.value( QLatin1String( "error" ) ).toInt();
  if ( errorCode ) {
    error = CalendarSearch::SomeError;
    errorString = result.value( QLatin1String( "errorString" ) ).toString();
    emit q->errorOccurredceived();
    return;
  }
  const int id = result.value( QLatin1String( "Collection" ) ).toInt( &ok );
  if ( !ok || id < 0 ) {
    error = CalendarSearch::SomeError;
    errorString = i18n( "Could not parse the collection ID" );
    emit q->errorOccurredceived();
    return;
  }

  Collection col;
  col.setId( id );
  Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob( col );
  connect( job, SIGNAL(finished(KJob *)), q, SLOT(collectionFetched(KJob *)) );
}

void CalendarSearch::Private::collectionFetched( KJob *j )
{
  const CollectionFetchJob *const job = qobject_cast<const CollectionFetchJob * const>( j );
  Q_ASSERT( job );
  if ( job->error() ) {
    error = CalendarSearch::SomeError;
    errorString = job->errorString();
    emit q->errorOccurredceived();
    return;
  }
  if ( !job->collections().isEmpty() ) {
    createdCollection = job->collections().first();
  }
}

#endif

CalendarSearch::CalendarSearch( QObject *parent )
  : QObject( parent ), d( new Private( this ) )
{
}

QAbstractItemModel *CalendarSearch::model() const
{
  return d->dateRangeProxyModel;
}

CalendarSearch::~CalendarSearch()
{
  delete d;
}

bool CalendarSearch::hasError() const
{
  return d->error != NoError;
}

CalendarSearch::Error CalendarSearch::error() const
{
  return d->error;
}

QString CalendarSearch::errorString() const
{
  return d->errorString;
}

KDateTime CalendarSearch::startDate() const
{
  return d->startDate;
}

void CalendarSearch::setStartDate( const KDateTime &startDate )
{
  if ( d->startDate != startDate ) {
    d->startDate = startDate;
    d->dateRangeProxyModel->setStartDate( startDate );
    // d->triggerDelayedUpdate();
  }
}

KDateTime CalendarSearch::endDate() const
{
  return d->endDate;
}

void CalendarSearch::setEndDate( const KDateTime &endDate )
{
  if ( d->endDate != endDate ) {
    d->endDate = endDate;
    d->dateRangeProxyModel->setEndDate( endDate );
   // d->triggerDelayedUpdate();
  }
}

static QModelIndex findIndex( QAbstractItemModel *m, const QModelIndex &parent,
                              const Akonadi::Collection::Id &cid )
{
  const int rows = m->rowCount( parent );
  for ( int i=0; i < rows; ++i ) {
    const QModelIndex idx = m->index( i, 0, parent );
    const Akonadi::Collection::Id found = collectionIdFromIndex( idx );

    if ( found < 0 ) {
      return QModelIndex(); // we only care about collections
    }

    if ( found == cid ) {
      return idx;
    }

    const QModelIndex inChildren = findIndex( m, idx, cid );
    if ( inChildren.isValid() ) {
      return inChildren;
    }
  }

  return QModelIndex();
}

void CalendarSearch::Private::collectionSelectionChanged( const QItemSelection &newSelection,
                                                          const QItemSelection &oldSelection )
{
  QSet<QModelIndex> oldIndexes = oldSelection.indexes().toSet();
  QSet<QModelIndex> newIndexes = newSelection.indexes().toSet();
  Q_FOREACH( const QModelIndex &i, oldIndexes - newIndexes ) {
    const QModelIndex idx = findIndex( calendarModel, QModelIndex(), collectionIdFromIndex( i ) );
    if ( idx.isValid() ) {
      selectionModel->select( idx, QItemSelectionModel::Deselect );
    }
  }

  Q_FOREACH( const QModelIndex &i, newIndexes ) {
    const Akonadi::Collection::Id id = collectionIdFromIndex( i );
    const QModelIndex idx = findIndex( calendarModel, QModelIndex(), id );
    if ( idx.isValid() ) {
      selectionModel->select( idx, QItemSelectionModel::Select );
    } else {
      preselectedCollections.append( id );
    }
  }
}

QItemSelectionModel *CalendarSearch::selectionModel() const
{
  return d->selectionModel;
}

CalendarSearch::IncidenceTypes CalendarSearch::incidenceTypes() const
{
  return d->incidenceTypes;
}

void CalendarSearch::setIncidenceTypes( IncidenceTypes types )
{
  if ( static_cast<int>( types ) != static_cast<int>( d->incidenceTypes ) ) {
    const bool showEvents = types.testFlag( Events );
    const bool showTodos = types.testFlag( Todos );
    const bool showJournals = types.testFlag( Journals );

    d->incidenceTypes = types;
    d->incidenceFilterProxyModel->setShowEvents( showEvents );
    d->incidenceFilterProxyModel->setShowTodos( showTodos );
    d->incidenceFilterProxyModel->setShowJournals( showJournals );

  }
}

void CalendarSearch::Private::rowsInserted( const QModelIndex &parent, int start, int end )
{
  for ( int i = start; i <= end; ++i ) {
    const QModelIndex idx = calendarModel->index( i, 0, parent );
    const Akonadi::Collection::Id id = collectionIdFromIndex( idx );

    const Akonadi::Item item = itemFromIndex( idx );

    if ( item.isValid() ) {
      const Akonadi::Collection::Rights rights = item.parentCollection().rights();
      KCalCore::Incidence::Ptr incidence = CalendarSupport::incidence( item );
      if ( incidence &&
           !( rights & Akonadi::Collection::CanDeleteItem ) &&
           !( rights & Akonadi::Collection::CanChangeItem ) &&
           !incidence->isReadOnly() ) {
        incidence->setReadOnly( true );
      }
      continue;
    }

    for ( int j = 0; j < preselectedCollections.size(); ++j ) {
      if ( preselectedCollections[j] == id ) {
        selectionModel->select( idx, QItemSelectionModel::Select );
      }
      const QModelIndex cidx = findIndex( calendarModel, idx, preselectedCollections[j] );
      if ( cidx.isValid() ) {
        selectionModel->select( cidx, QItemSelectionModel::Select );
      }
    }
  }
}

void CalendarSearch::setSelectionModel( QItemSelectionModel *selectionModel )
{
  connect( selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
           this, SLOT(collectionSelectionChanged(QItemSelection,QItemSelection)) );

  Q_FOREACH( const QModelIndex &i, selectionModel->selectedIndexes() ) {
    const Akonadi::Collection::Id cid = collectionIdFromIndex( i );
    kDebug() << cid;
    const QModelIndex idx = findIndex( d->calendarModel, QModelIndex(), cid );
    if ( idx.isValid() ) {
      d->selectionModel->select( idx, QItemSelectionModel::Select );
    } else {
      d->preselectedCollections.append( cid );
    }
  }
}

void CalendarSearch::setFilter( KCalCore::CalFilter *filter )
{
  d->kcalFilterProxyModel->setFilter( filter );
}

#include "calendarsearch.moc"
