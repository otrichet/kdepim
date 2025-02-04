/*
  Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company

  Author: Tobias Koenig <tokoe@kde.org>

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

#include "incidenceviewer.h"
#include "utils.h"

#include "libkdepimdbusinterfaces/urihandler.h"
#include "akonadi_next/incidenceattachmentmodel.h"

#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ItemFetchScope>

#include <KCalUtils/IncidenceFormatter>

#include <KJob>
#include <KSystemTimeZone>
#include <KTextBrowser>

#include <QVBoxLayout>

using namespace CalendarSupport;

class TextBrowser : public KTextBrowser
{
  public:
    TextBrowser( QWidget *parent = 0 )
      : KTextBrowser( parent )
    {
#ifdef KDEPIM_MOBILE_UI
      setFrameStyle( QFrame::NoFrame );
#endif
    }

    void setSource( const QUrl &name )
    {
      QString uri = name.toString();
      // QTextBrowser for some reason insists on putting // or / in links,
      // this is a crude workaround
      if ( uri.startsWith( QLatin1String( "uid:" ) ) ||
           uri.startsWith( QLatin1String( "kmail:" ) ) ||
           uri.startsWith( QString( "urn:x-ical" ).section( ':', 0, 0 ) ) ||
           uri.startsWith( QLatin1String( "news:" ) ) ||
           uri.startsWith( QLatin1String( "mailto:" ) ) ) {
        uri.replace( QRegExp( "^([^:]+:)/+" ), "\\1" );
      }

      UriHandler::process( uri );
    }
};

class IncidenceViewer::Private
{
  public:
    Private( IncidenceViewer *parent )
      : mParent( parent ), mDelayedClear( false ), mParentCollectionFetchJob( 0 ),
        mAttachmentModel( 0 )
    {
      mBrowser = new TextBrowser;
    }

    void updateView()
    {
      QString text;

      if ( mCurrentItem.isValid() ) {
        text = KCalUtils::IncidenceFormatter::extensiveDisplayStr(
          CalendarSupport::displayName( mParentCollection ),
          CalendarSupport::incidence( mCurrentItem ),
          mDate, KSystemTimeZones::local() );
        text.prepend( mHeaderText );
        mBrowser->setHtml( text );
      } else {
        text = mDefaultText;
        if ( !mDelayedClear ) {
          mBrowser->setHtml( text );
        }
      }

    }

    void slotParentCollectionFetched( KJob *job )
    {
      mParentCollectionFetchJob = 0;
      mParentCollection = Akonadi::Collection();

      if ( !job->error() ) {
        Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob*>( job );
        if ( !fetchJob->collections().isEmpty() ) {
          mParentCollection = fetchJob->collections().first();
        }
      }

      updateView();
    }

    IncidenceViewer *mParent;
    TextBrowser *mBrowser;
    Akonadi::Item mCurrentItem;
    QDate mDate;
    QString mHeaderText;
    QString mDefaultText;
    bool mDelayedClear;
    Akonadi::Collection mParentCollection;
    Akonadi::CollectionFetchJob *mParentCollectionFetchJob;
    Akonadi::IncidenceAttachmentModel *mAttachmentModel;
};

IncidenceViewer::IncidenceViewer( QWidget *parent )
  : QWidget( parent ), d( new Private( this ) )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setMargin( 0 );

  d->mBrowser->setNotifyClick( true );
  d->mBrowser->setMinimumHeight( 1 );

  layout->addWidget( d->mBrowser );

  // always fetch full payload for incidences
  fetchScope().fetchFullPayload();
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );

  d->updateView();
}

IncidenceViewer::~IncidenceViewer()
{
  delete d;
}

Akonadi::Item IncidenceViewer::incidence() const
{
  return ItemMonitor::item();
}

QDate IncidenceViewer::activeDate() const
{
  return d->mDate;
}

QAbstractItemModel *IncidenceViewer::attachmentModel() const
{
  if ( !d->mAttachmentModel ) {
    d->mAttachmentModel =
      new Akonadi::IncidenceAttachmentModel( const_cast<IncidenceViewer*>( this ) );
  }
  return d->mAttachmentModel;
}

void IncidenceViewer::setDelayedClear( bool delayed )
{
  d->mDelayedClear = delayed;
}

void IncidenceViewer::setDefaultMessage( const QString &message )
{
  d->mDefaultText = message;
}

void IncidenceViewer::setHeaderText( const QString &text )
{
  d->mHeaderText = text;
}

void IncidenceViewer::setIncidence( const Akonadi::Item &incidence, const QDate &date )
{
  d->mDate = date;
  ItemMonitor::setItem( incidence );

  d->updateView();
}

void IncidenceViewer::itemChanged( const Akonadi::Item &item )
{
  if ( !item.hasPayload<KCalCore::Incidence::Ptr>() ) {
    return;
  }

  d->mCurrentItem = item;

  if ( d->mAttachmentModel ) {
    d->mAttachmentModel->setItem( d->mCurrentItem );
  }

  if ( d->mParentCollectionFetchJob ) {
    disconnect( d->mParentCollectionFetchJob, SIGNAL(result(KJob *)),
                this, SLOT(slotParentCollectionFetched(KJob *)) );
    delete d->mParentCollectionFetchJob;
  }

  d->mParentCollectionFetchJob =
    new Akonadi::CollectionFetchJob( d->mCurrentItem.parentCollection(),
                                     Akonadi::CollectionFetchJob::Base, this );

  connect( d->mParentCollectionFetchJob, SIGNAL(result(KJob *)),
           this, SLOT(slotParentCollectionFetched(KJob *)) );
}

void IncidenceViewer::itemRemoved()
{
  d->mBrowser->clear();
}

#include "incidenceviewer.moc"
