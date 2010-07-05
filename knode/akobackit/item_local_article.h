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

#ifndef KNODE_ITEMLOCALARTICLE_H
#define KNODE_ITEMLOCALARTICLE_H

#include "akobackit/item_remote_article.h"
#include "akobackit/constant.h"

#include <KMime/NewsArticle>

namespace Akonadi {
  class Item;
}

namespace KNode {

/**
 * Wrapper around an Akonadi::Item for message contains in local folder.
 */
class KNODE_EXPORT LocalArticle : public RemoteArticle
{
  public:
    /// Shared pointer to a LocalArticle.
    typedef boost::shared_ptr<LocalArticle> Ptr;
    /// List of articles.
    typedef QList<LocalArticle::Ptr> List;

    /**
     * Create a new LocalArticle.
     * @param item An item from the Akonadi backend or an invalid item for
     * LocalArticle which represents a new message.
     */
    explicit LocalArticle( const Akonadi::Item &item );
    /**
     * Destroys this local article.
     */
    virtual ~LocalArticle();

    /**
     * Indicate if this article is already present in the backend.
     *
     * This usually indicate that this is new article without content.
     */
    bool isValid() const;


  public:
#ifdef __GNUC__
  #warning AKONADI PORT: is this code still usefull after the port?
#endif
#define LOCAL_ITEM_FLAG_METHOD( getter, setter, flag ) \
    bool getter() \
    { \
      return mItem.hasFlag( flag ); \
    } \
    void setter( bool b ) \
    { \
      if ( b ) { \
        mItem.setFlag( flag ); \
      } else { \
        mItem.clearFlag( flag ); \
      } \
    }

    /** Does the article need to be posted? */
    LOCAL_ITEM_FLAG_METHOD( doPost, setDoPost, Akobackit::ARTICLE_FLAG_DO_POST )
    /** Already posted? */
    LOCAL_ITEM_FLAG_METHOD( posted, setPosted, Akobackit::ARTICLE_FLAG_POSTED )
    /** Does the article need be mailed? */
    LOCAL_ITEM_FLAG_METHOD( doMail, setDoMail, Akobackit::ARTICLE_FLAG_DO_MAIL )
    /** Already mailed? */
    LOCAL_ITEM_FLAG_METHOD( mailed, setMailed, Akobackit::ARTICLE_FLAG_MAILED )
    /** Can the article be edited? */
    LOCAL_ITEM_FLAG_METHOD( editDisabled, setEditDisabled, Akobackit::ARTICLE_FLAG_EDIT_DISABLED )
    /** The article was canceled? */
    LOCAL_ITEM_FLAG_METHOD( canceled, setCanceled, Akobackit::ARTICLE_FLAG_CANCELED )
#undef LOCAL_ITEM_FLAG_METHOD
};

}

#endif

