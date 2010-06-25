/*
  Copyright 2010 Olivier Trichet <nive@nivalis.org>

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include "akobackit/item_merge_job.h"

#include <Akonadi/ItemModifyJob>
#include <Akonadi/ItemCreateJob>
#include <QtCore/QTimer>

namespace KNode {
namespace Akobackit {

ItemsMergeJob::ItemsMergeJob( const LocalArticle::List &articles,
                              const Akonadi::Collection &destination, QObject *parent )
  : KJob( parent ),
    mArticles( articles ),
    mDestination( destination ),
    mCreationCount( 0 ),
    mModificationCount( 0 )
{
}

ItemsMergeJob::~ItemsMergeJob()
{
}


void ItemsMergeJob::start()
{
  QTimer::singleShot( 0, this, SLOT( doStart() ) );
}

void ItemsMergeJob::doStart()
{
  foreach ( LocalArticle::Ptr art, mArticles ) {
    const Akonadi::Item item = art->item();
    if ( item.isValid() ) {
      // TODO: Change parent collection to move the item
      Akonadi::ItemModifyJob *job = new Akonadi::ItemModifyJob( item, this );
      addSubjob( job, item );
    } else {
      Q_ASSERT( mDestination.isValid() );
      Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( item, mDestination, this );
      addSubjob( job, item );
    }
  }

  setTotalAmount( KJob::Files, mSubjobs.size() );
}

void ItemsMergeJob::addSubjob( KJob *job, const Akonadi::Item &i )
{
  connect( job, SIGNAL( result( KJob * ) ),
           this, SLOT( slotResult( KJob * ) ) );
  mSubjobs.insert( job, i );
}




void ItemsMergeJob::slotResult( KJob *job )
{
  Q_ASSERT( mSubjobs.contains( job ) );
  Akonadi::Item item = mSubjobs.take( job );

  if ( job->error() ) {
    mErrors.insert( item, job->error() );
    mErrorStrings.insert( item , job->errorString() );
  } else {
    if ( qobject_cast<Akonadi::ItemModifyJob*>( job ) ) {
      ++mModificationCount;
    } else if ( qobject_cast<Akonadi::ItemCreateJob*>( job ) ) {
      ++mCreationCount;
    }
  }

  setProcessedAmount( KJob::Files, ( totalAmount( KJob::Files ) - mSubjobs.size() ) );
  if ( mSubjobs.isEmpty() ) {
    emitResult();
  }
}


}
}

#include "akobackit/item_merge_job.moc"
