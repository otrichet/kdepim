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


#ifndef KNODE_AKOBACKIT_NNTPACCOUNTMANAGER_H
#define KNODE_AKOBACKIT_NNTPACCOUNTMANAGER_H

#include "akobackit/nntpaccount.h"

#include <QObject>

class KJob;

namespace KNode {
namespace Akobackit {

class AkoManager;

/**
 * Manager of NNTP accounts.
 */
class NntpAccountManager : public QObject
{
  Q_OBJECT

  public:
    NntpAccountManager( AkoManager *parent );
    virtual ~NntpAccountManager();

    /**
     * Returns the list of NNTP resource in the Akonadi
     * backend.
     */
    NntpAccount::List accounts();

    /**
     * Returns the account for @p agent.
     */
    NntpAccount::Ptr account( const Akonadi::AgentInstance &agent );


    /**
     * Creates and then configures an account.
     * @param parentWidget the parent widget of the configuration dialog.
     */
    void createAccount( QWidget *parentWidget );

    /**
     * Edits an account. This opens the edition dialog of @p account.
     * @param account The resource that have to be configured (must be valid).
     * @param parentWidget the parent widget of the edition dialog.
     */
    void editAccount( NntpAccount::Ptr account, QWidget *parentWidget );

    /**
     * Save modification made to @p account. The @p account must wrap
     * a valid AgentInstance.
     */
    void saveAccount( NntpAccount::Ptr account );

    /**
     * Remove an account.
     * @param account The account to remove.
     * @param parentWidget The parent widget of the confirmatio dialog.
     */
    void removeAccount( NntpAccount::Ptr account, QWidget *parentWidget );

  private slots:
    /**
     * Result slot of an AgentInstanceCreateJob in createAccount().
     */
    void accountCreated( KJob *job );
};

}
}

#endif
