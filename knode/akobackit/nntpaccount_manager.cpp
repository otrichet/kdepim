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

#include "akobackit/nntpaccount_manager.h"

#include "akobackit/akonadi_manager.h"
#include "akobackit/constant.h"
#include "akobackit/nntpaccount.h"
#include "knconfigwidgets.h"

#include <Akonadi/AgentManager>
#include <Akonadi/AgentInstanceCreateJob>
#include <KLocalizedString>
#include <KMessageBox>
#include <QDBusInterface>
#include <QDBusReply>
#include <QWidget>



namespace KNode {
namespace Akobackit {

// Local method
inline bool isNntpResource( const Akonadi::AgentInstance &agent )
{
  return agent.identifier().startsWith( NNTP_RESOURCE_AGENTTYPE );
}



NntpAccountManager::NntpAccountManager( AkoManager *parent )
  : QObject( parent )
{
}

NntpAccountManager::~NntpAccountManager()
{
}


NntpAccount::List NntpAccountManager::accounts()
{
  NntpAccount::List res;
  foreach ( const Akonadi::AgentInstance &agent, Akonadi::AgentManager::self()->instances() ) {
    if ( isNntpResource( agent ) ) {
      res << account( agent );
    }
  }
  return res;
}

NntpAccount::Ptr NntpAccountManager::account( const Akonadi::AgentInstance &agent )
{
  return NntpAccount::Ptr( new NntpAccount( agent ) );
}




void NntpAccountManager::createAccount( QWidget *parentWidget )
{
  Akonadi::AgentType type = Akonadi::AgentManager::self()->type( NNTP_RESOURCE_AGENTTYPE );
  Akonadi::AgentInstanceCreateJob *job = new Akonadi::AgentInstanceCreateJob( type, this );
  connect( job, SIGNAL( result( KJob * ) ),
            this, SLOT( accountCreated( KJob * ) ) );

  // use this widget as parent for the config dialog
  job->setProperty( "parentWidget", static_cast<qulonglong>( parentWidget->winId() ) );

  job->start();
}

void NntpAccountManager::accountCreated( KJob *job )
{
  if ( job->error() ) {
    // TODO: shall we do something here
    kError() << "Unable to create an NNTP resource:" << job->errorString();
  } else {
    Akonadi::AgentInstanceCreateJob *createJob = static_cast<Akonadi::AgentInstanceCreateJob*>( job );
    NntpAccount::Ptr nntpAccount = account( createJob->instance() );

    QWidget *parent = 0;
    QVariant parentWidgetId = job->property( "parentWidget" );
    if ( parentWidgetId.isValid() ) {
      parent = QWidget::find( parentWidgetId.toULongLong() );
    }
    NntpAccountConfDialog *dialog = new NntpAccountConfDialog( nntpAccount, parent );
    dialog->setDeleteAccountOnCancel( true );
    dialog->open();
  }
}



void NntpAccountManager::editAccount( NntpAccount::Ptr account, QWidget *parentWidget )
{
  NntpAccountConfDialog *dialog = new NntpAccountConfDialog( account, parentWidget );
  dialog->open();
}



void NntpAccountManager::saveAccount( NntpAccount::Ptr account )
{
  Q_ASSERT( account->isValid() );
  account->save();
  account->agent().reconfigure(); // Reload the instance configuration.
}



void NntpAccountManager::removeAccount( NntpAccount::Ptr account, QWidget *parentWidget  )
{
  Q_ASSERT( account->agent().isValid() );
  if ( !account->agent().isValid() ) {
    return;
  }

  int res = KMessageBox::warningContinueCancel(
                            parentWidget,
                            i18n( "Do you really want to delete this account?" ),
                            QString(),
                            KGuiItem( i18nc( "@action:button delete a newsgroup account", "Delete" ), "edit-delete" )
                          );
  if ( res == KMessageBox::Continue ) {
    Akonadi::AgentManager::self()->removeInstance( account->agent() );
  }
}


}
}

#include "akobackit/nntpaccount_manager.moc"
