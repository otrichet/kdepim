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

#include <Akonadi/ItemCreateJob>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/ItemMoveJob>
#include <KDebug>
#include <QtCore/QTimer>

namespace KNode {
namespace Akobackit {

ItemsMergeJob::ItemsMergeJob( const LocalArticle::List &articles,
                              const Akonadi::Collection &destination, QObject *parent )
  : Akonadi::TransactionSequence( parent ),
    mArticles( articles ),
    mDestination( destination )
{
}

ItemsMergeJob::~ItemsMergeJob()
{
}

void ItemsMergeJob::doStart()
{
  if ( mArticles.isEmpty() ) {
    kError() << "Empty article list: aborting job without error";
    emitResult();
  }

  Akonadi::Item::List itemsToMove;

  foreach ( LocalArticle::Ptr art, mArticles ) {
    Akonadi::Item item = art->item();
    if ( item.isValid() ) {
      Akonadi::ItemModifyJob *job = new Akonadi::ItemModifyJob( item, this );
      addSubjob( job );
      itemsToMove << item;
    } else {
      Q_ASSERT( mDestination.isValid() );
      Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( item, mDestination, this );
      addSubjob( job );
    }
  }

  if ( mDestination.isValid() && !itemsToMove.isEmpty() ) {
    Akonadi::ItemMoveJob *job = new Akonadi::ItemMoveJob( itemsToMove, mDestination );
    addSubjob( job );
  }

  TransactionSequence::doStart();
}


}
}

#include "akobackit/item_merge_job.moc"
