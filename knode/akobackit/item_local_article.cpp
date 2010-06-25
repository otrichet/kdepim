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

#include "akobackit/item_local_article.h"

namespace KNode {

LocalArticle::LocalArticle( const Akonadi::Item &item )
  : NewsArticle(),
    mItem( item )
{
  // Sync KMime::Content
  if ( mItem.isValid() ) {
    setContent( mItem.payloadData() );
  }
}

LocalArticle::~LocalArticle()
{
}

Akonadi::Item LocalArticle::item()
{
  mItem.setMimeType( "message/news" );
  assemble(); // mandated by encodedContent()
  mItem.setPayloadFromData( encodedContent() );
  return mItem;
}

bool LocalArticle::isValid() const
{
  return mItem.isValid();
}


}

