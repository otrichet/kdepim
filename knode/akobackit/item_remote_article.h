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

#ifndef KNODE_ITEMREMOTEARTICLE_H
#define KNODE_ITEMREMOTEARTICLE_H

#include "knode_export.h"

#include <Akonadi/Item>
#include <KMime/NewsArticle>


namespace KNode {

/**
 * Wrapper around an Akonadi::Item for message contains in newsgroups folder.
 */
class KNODE_EXPORT RemoteArticle : public KMime::NewsArticle
{
  public:
    /// Shared pointer to a RemoteArticle.
    typedef boost::shared_ptr<RemoteArticle> Ptr;
    /// List of articles.
    typedef QList<RemoteArticle::Ptr> List;

    /**
     * Create a new RemoteArticle.
     * @param item An item from the Akonadi backend (must be valid).
     */
    explicit RemoteArticle( const Akonadi::Item &item );
    /**
     * Destroys this local article.
     */
    virtual ~RemoteArticle();

    /**
     * Returns the wrapped item.
     */
    Akonadi::Item item();

  protected:
    Akonadi::Item mItem;
};

}

#endif

