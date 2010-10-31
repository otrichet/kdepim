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

#include "akobackit/items_deletion_job.h"

#include <Akonadi/ItemDeleteJob>
#include <KDebug>
#include <KLocalizedString>
#include <KMessageBox>

namespace KNode {
namespace Akobackit {

ItemsDeletionJob::ItemsDeletionJob( const QList<Akonadi::Item> items, QWidget *parentWidget )
  : Akonadi::TransactionSequence(),
    mItems( items ),
    mWidget( parentWidget )
{
}

ItemsDeletionJob::~ItemsDeletionJob()
{
}

void ItemsDeletionJob::doStart()
{
  if ( mItems.isEmpty() ) {
    kError() << "Empty article list: aborting job without error";
    emitResult();
    return;
  }

  if ( mWidget ) {
    int answer = KMessageBox::warningContinueCancel(
                                 mWidget,
                                 i18n( "Do you really want to delete these articles?" ),
                                 i18nc( "@title:window", "Delete Articles" ),
                                 KGuiItem( i18nc( "@action:button Confirm deletion of articles", "&Delete" ), "edit-delete" )
                               );
    if ( KMessageBox::Continue != answer ) {
      rollback();
      return;
    }
  }

  Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob( mItems );
  addSubjob( job );

  Akonadi::TransactionSequence::doStart();
}


}
}

#include "akobackit/items_deletion_job.moc"
