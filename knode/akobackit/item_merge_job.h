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

#ifndef KNODE_AKOBACKIT_ITEMSMERGEJOB_H
#define KNODE_AKOBACKIT_ITEMSMERGEJOB_H

#include "akobackit/item_local_article.h"
#include "knode_export.h"

#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <Akonadi/TransactionSequence>


namespace KNode {
namespace Akobackit {

/**
 * @brief This job allow to create/update many items at once.
 *
 * This job wraps around Akonadi::ItemCreateJob and
 * Akonadi::ItemModifyJob to do its works.
 * Any invalid item is treated as a new item and created and
 * any valid item is treated as an existing item and just modified.
 *
 * @see Akonadi::TransactionSequence
 */
class KNODE_EXPORT ItemsMergeJob : public Akonadi::TransactionSequence
{
  Q_OBJECT

  public:
    /**
     * Create a new items merging job.
     *
     * When the @p destination is a valid collection, all items are moved into it; otherwise
     * existing items are not moved.
     *
     * @note When @p destination is not valid, all items must already exist in the backend.
     *
     * @param articles The list of article to merge into the akonadi backend.
     * @param destination The optional destination collection.
     */
    explicit ItemsMergeJob( const LocalArticle::List &articles,
                            const Akonadi::Collection &destination = Akonadi::Collection(),
                            QObject *parent = 0 );
    /**
     * Destroys this items merging job.
     */
    virtual ~ItemsMergeJob();


  protected:
    /**
     * @reimp
     */
    void doStart();

  private:
    LocalArticle::List mArticles;
    Akonadi::Collection mDestination;

};

}
}

#endif
