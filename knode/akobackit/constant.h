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

#ifndef KNODE_AKOBACKIT_CONSTANT_H
#define KNODE_AKOBACKIT_CONSTANT_H

#include <Akonadi/Item>
#include <QtCore/QLatin1String>

namespace KNode {
namespace Akobackit {

  /**
    * Type of Akonadi collections
    */
  enum CollectionType {
    InvalidCollection, ///< Invalid collection.

    RootFolder, ///< Root of the local folders.
    OutboxFolder, ///< Outbox folder.
    SentmailFolder, ///< Sent-mail folder.
    DraftFolder, ///< Draft folder.
    UserFolder, ///< Folder created by the user.

    NntpServer, ///< The main (root) collection of an NNTP resource.
    NewsGroup, ///< A newsgroup.
  };

  /**
   * Prefix of the identifier of an Akonadi maildir resource.
   */
  static const QLatin1String MAILDIR_RESOURCE_AGENTTYPE( "akonadi_maildir_resource" );
  /**
   * Prefix of the identifier of an Akonadi NNTP resource.
   */
  static const QLatin1String NNTP_RESOURCE_AGENTTYPE( "akonadi_nntp_resource" );


  static const Akonadi::Item::Flag ARTICLE_FLAG_DO_POST( "knode-doPost" );
  static const Akonadi::Item::Flag ARTICLE_FLAG_POSTED( "knode-posted" );
  static const Akonadi::Item::Flag ARTICLE_FLAG_DO_MAIL( "knode-doMail" );
  static const Akonadi::Item::Flag ARTICLE_FLAG_MAILED( "knode-mailed" );
  static const Akonadi::Item::Flag ARTICLE_FLAG_EDIT_DISABLED( "knode-editDisabled" );
  static const Akonadi::Item::Flag ARTICLE_FLAG_CANCELED( "knode-canceled" );

}
}


#endif
