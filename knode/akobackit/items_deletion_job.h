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


#ifndef KNODE_AKOBACKIT_ITEMS_DELETE_JOB_H
#define KNODE_AKOBACKIT_ITEMS_DELETE_JOB_H

#include <Akonadi/TransactionSequence>

namespace Akonadi {
  class Item;
}
class QWidget;


namespace KNode {
namespace Akobackit {

/**
 * Delete articles in the Akonadi backend.
 */
class ItemsDeletionJob : public Akonadi::TransactionSequence
{
  Q_OBJECT

  public:
    /**
     * Create a new job to delete items.
     *
     * A confirmation is asked when the @p parentWidget is not null.
     * @param items The list of items to delete in the backend.
     * @param parentWidget The parent widget used for the confirmation dialog.
     */
    ItemsDeletionJob( const QList<Akonadi::Item> items, QWidget *parentWidget = 0 );
    /**
     * Destructor.
     */
    virtual ~ItemsDeletionJob();


  protected:
    /**
     * @reimp
     */
    void doStart();

  private:
    QList<Akonadi::Item> mItems;
    QWidget *mWidget;
};


}
}

#endif
