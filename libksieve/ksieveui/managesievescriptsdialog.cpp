
#include "managesievescriptsdialog.h"
#include "managesievescriptsdialog_p.h"

#include <klocale.h>
#include <kiconloader.h>
#include <kwindowsystem.h>
#include <kinputdialog.h>
#include <kglobalsettings.h>
#include <kmessagebox.h>


#include <akonadi/agentinstance.h>
#include <kmanagesieve/sievejob.h>
#include <ksieveui/util.h>

#include <QApplication>
#include <QButtonGroup>
#include <QMenu>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <cassert>

using namespace KSieveUi;

bool ItemRadioButton::mTreeWidgetIsBeingCleared = false;

ManageSieveScriptsDialog::ManageSieveScriptsDialog( QWidget * parent, const char * name )
  : KDialog( parent ),
    mContextMenuItem( 0 ),
    mSieveEditor( 0 ),
    mWasActive( false )
{
  setCaption( i18n( "Manage Sieve Scripts" ) );
  setButtons( Close );
  setObjectName( name );
  setDefaultButton(  Close );
  setModal( false );
  setAttribute( Qt::WA_GroupLeader );
  setAttribute( Qt::WA_DeleteOnClose );
  KWindowSystem::setIcons( winId(), qApp->windowIcon().pixmap(IconSize(KIconLoader::Desktop),IconSize(KIconLoader::Desktop)), qApp->windowIcon().pixmap(IconSize(KIconLoader::Small),IconSize(KIconLoader::Small)) );
  QFrame *frame =new QFrame( this );
  setMainWidget( frame );
  QVBoxLayout * vlay = new QVBoxLayout( frame );
  vlay->setSpacing( 0 );
  vlay->setMargin( 0 );

  mListView = new TreeWidgetWithContextMenu( frame);
  mListView->setHeaderLabel( i18n( "Available Scripts" ) );
  mListView->setRootIsDecorated( true );
  mListView->setAlternatingRowColors( true );
  mListView->setSelectionMode( QAbstractItemView::SingleSelection );
#ifndef QT_NO_CONTEXTMENU
  connect( mListView, SIGNAL( contextMenuRequested( QTreeWidgetItem*, QPoint ) ),
           this, SLOT( slotContextMenuRequested( QTreeWidgetItem*, QPoint ) ) );
#endif
  connect( mListView, SIGNAL( itemDoubleClicked ( QTreeWidgetItem *, int ) ),
           this, SLOT( slotDoubleClicked( QTreeWidgetItem* ) ) );
  vlay->addWidget( mListView );

  resize( 2 * sizeHint().width(), sizeHint().height() );

  slotRefresh();
}

ManageSieveScriptsDialog::~ManageSieveScriptsDialog()
{
  clear();
}

void ManageSieveScriptsDialog::killAllJobs()
{
  for ( QMap<KManageSieve::SieveJob*,QTreeWidgetItem*>::const_iterator it = mJobs.constBegin(),
        end = mJobs.constEnd() ; it != end ; ++it )
    it.key()->kill();
  mJobs.clear();
}


void ManageSieveScriptsDialog::slotRefresh()
{
  clear();
  QTreeWidgetItem *last = 0;
  Akonadi::AgentInstance::List lst = KSieveUi::Util::imapAgentInstances();
  foreach ( const Akonadi::AgentInstance& type, lst )
  {
    if ( type.status() == Akonadi::AgentInstance::Broken )
      continue;

    last = new QTreeWidgetItem( mListView, last );
    last->setText( 0, type.name() );
    last->setIcon( 0, SmallIcon( "network-server" ) );

    const KUrl u = KSieveUi::Util::findSieveUrlForAccount( type.identifier() );
    if ( u.isEmpty() ) {
      QTreeWidgetItem *item = new QTreeWidgetItem( last );
      item->setText( 0, i18n( "No Sieve URL configured" ) );
      item->setFlags( item->flags() & ~Qt::ItemIsEnabled );
      mListView->expandItem( last );
    } else {
      KManageSieve::SieveJob * job = KManageSieve::SieveJob::list( u );
      connect( job, SIGNAL(item(KManageSieve::SieveJob*,const QString&,bool)),
               this, SLOT(slotItem(KManageSieve::SieveJob*,const QString&,bool)) );
      connect( job, SIGNAL(result(KManageSieve::SieveJob*,bool,const QString&,bool)),
               this, SLOT(slotResult(KManageSieve::SieveJob*,bool,const QString&,bool)) );
      mJobs.insert( job, last );
      mUrls.insert( last, u );
    }
  }
}

void ManageSieveScriptsDialog::slotResult( KManageSieve::SieveJob * job, bool success, const QString &, bool )
{
  QTreeWidgetItem * parent = mJobs[job];
  if ( !parent )
    return;

  mJobs.remove( job );

  mListView->expandItem( parent );

  if ( success )
    return;

  QTreeWidgetItem * item =
      new QTreeWidgetItem( parent );
  item->setText( 0, i18n( "Failed to fetch the list of scripts" ) );
  item->setFlags( item->flags() & ~Qt::ItemIsEnabled );
}

void ManageSieveScriptsDialog::slotItem( KManageSieve::SieveJob * job, const QString & filename, bool isActive )
{
  QTreeWidgetItem * parent = mJobs[job];
  if ( !parent )
    return;
  QTreeWidgetItem* item = new QTreeWidgetItem( parent );
  addRadioButton( item, filename );
  if ( isActive ) {
    setRadioButtonState( item, true );
    mSelectedItems[parent] = item;
  }
}

void ManageSieveScriptsDialog::slotContextMenuRequested( QTreeWidgetItem *item, QPoint p )
{
  if ( !item )
    return;
  if ( !item->parent() && !mUrls.count( item ))
    return;
  QMenu menu;
  mContextMenuItem = item;
  if ( isFileNameItem( item ) ) {
    // script items:
    menu.addAction( i18n( "Delete Script" ), this, SLOT(slotDeleteScript()) );
    menu.addAction( i18n( "Edit Script..." ), this, SLOT(slotEditScript()) );
    if ( isRadioButtonChecked( item ) )
      menu.addAction( i18n( "Deactivate Script" ), this, SLOT(slotDeactivateScript()) );
  } else if ( !item->parent() ) {
    // top-levels:
    menu.addAction( i18n( "New Script..." ), this, SLOT(slotNewScript()) );
  }
  if ( !menu.actions().isEmpty() )
    menu.exec( p );
  mContextMenuItem = 0;
}

void ManageSieveScriptsDialog::slotDeactivateScript()
{
  QTreeWidgetItem * item = mListView->currentItem();
  if ( !isFileNameItem( item ) )
    return;
  QTreeWidgetItem *parent = item->parent();
  if ( isRadioButtonChecked( item ) ) {
    mSelectedItems[parent] = item;
    changeActiveScript( parent, false );
  }
}

void ManageSieveScriptsDialog::slotSelectionChanged()
{
  QTreeWidgetItem * item = mListView->currentItem();
  if ( !isFileNameItem( item ) )
    return;
  QTreeWidgetItem *parent = item->parent();
  if ( isRadioButtonChecked( item ) && mSelectedItems[parent] != item ) {
    mSelectedItems[parent] = item;
    changeActiveScript( parent, true );
  }
}

void ManageSieveScriptsDialog::changeActiveScript( QTreeWidgetItem * item, bool activate )
{
  if ( !item )
    return;
  if ( !mUrls.count( item ) )
    return;
  if ( !mSelectedItems.count( item ) )
    return;
  KUrl u = mUrls[item];
  if ( u.isEmpty() )
    return;
  QTreeWidgetItem* selected = mSelectedItems[item];
  if ( !selected )
    return;
  u.setFileName( itemText( selected ) );

  KManageSieve::SieveJob * job;
  if ( activate )
    job = KManageSieve::SieveJob::activate( u );
  else
    job = KManageSieve::SieveJob::deactivate( u );
  connect( job, SIGNAL(result(KManageSieve::SieveJob*,bool,const QString&,bool)),
           this, SLOT(slotRefresh()) );
}

void ManageSieveScriptsDialog::addRadioButton( QTreeWidgetItem *item, const QString &text )
{
  Q_ASSERT( item && item->parent() );
  Q_ASSERT( !mListView->itemWidget( item, 0 ) );

  // Create the radio button and set it as item widget
  ItemRadioButton *radioButton = new ItemRadioButton( item );
  radioButton->setAutoExclusive( false );
  radioButton->setText( text );
  mListView->setItemWidget( item, 0, radioButton );
  connect( radioButton, SIGNAL( toggled ( bool ) ),
           this, SLOT( slotSelectionChanged() ) );

  // Add the radio button to the button group
  QTreeWidgetItem *parent = item->parent();
  QButtonGroup *buttonGroup = mButtonGroups.value( parent );
  if ( !buttonGroup ) {
    buttonGroup = new QButtonGroup();
    mButtonGroups.insert( parent, buttonGroup );
  }
  buttonGroup->addButton( radioButton );
}

void ManageSieveScriptsDialog::setRadioButtonState( QTreeWidgetItem *item, bool checked )
{
  Q_ASSERT( item && item->parent() );

  ItemRadioButton *radioButton = dynamic_cast<ItemRadioButton*>( mListView->itemWidget( item, 0 ) );
  Q_ASSERT( radioButton );
  radioButton->setChecked( checked );
}


bool ManageSieveScriptsDialog::isRadioButtonChecked( QTreeWidgetItem *item ) const
{
  Q_ASSERT( item && item->parent() );

  ItemRadioButton *radioButton = dynamic_cast<ItemRadioButton*>( mListView->itemWidget( item, 0 ) );
  Q_ASSERT( radioButton );
  return radioButton->isChecked();
}

QString ManageSieveScriptsDialog::itemText( QTreeWidgetItem *item ) const
{
  Q_ASSERT( item && item->parent() );

  ItemRadioButton *radioButton = dynamic_cast<ItemRadioButton*>( mListView->itemWidget( item, 0 ) );
  Q_ASSERT( radioButton );
  return radioButton->text().remove( '&' );
}

bool ManageSieveScriptsDialog::isFileNameItem( QTreeWidgetItem *item ) const
{
   if ( !item || !item->parent() )
     return false;

  ItemRadioButton *radioButton = dynamic_cast<ItemRadioButton*>( mListView->itemWidget( item, 0 ) );
  return ( radioButton != 0 );
}

void ManageSieveScriptsDialog::clear()
{
  killAllJobs();
  mSelectedItems.clear();
  qDeleteAll( mButtonGroups );
  mButtonGroups.clear();
  mUrls.clear();
  ItemRadioButton::setTreeWidgetIsBeingCleared( true );
  mListView->clear();
  ItemRadioButton::setTreeWidgetIsBeingCleared( false );
}

void ManageSieveScriptsDialog::slotDoubleClicked( QTreeWidgetItem * item )
{
  if ( !isFileNameItem( item ) )
    return;

  mContextMenuItem = item;
  slotEditScript();
  mContextMenuItem = 0;
}

void ManageSieveScriptsDialog::slotDeleteScript()
{
  if ( !isFileNameItem( mContextMenuItem ) )
    return;

  QTreeWidgetItem *parent = mContextMenuItem->parent();
  if ( !parent )
    return;

  if ( !mUrls.count( parent ) )
    return;

  KUrl u = mUrls[parent];
  if ( u.isEmpty() )
    return;

  u.setFileName( itemText( mContextMenuItem ) );

  if ( KMessageBox::warningContinueCancel( this, i18n( "Really delete script \"%1\" from the server?", u.fileName() ),
                                   i18n( "Delete Sieve Script Confirmation" ),
                                   KStandardGuiItem::del() )
       != KMessageBox::Continue )
    return;
  KManageSieve::SieveJob * job = KManageSieve::SieveJob::del( u );
  connect( job, SIGNAL(result(KManageSieve::SieveJob*,bool,const QString&,bool)),
           this, SLOT(slotRefresh()) );
}

void ManageSieveScriptsDialog::slotEditScript()
{
  if ( !isFileNameItem( mContextMenuItem ) )
    return;
  QTreeWidgetItem* parent = mContextMenuItem->parent();
  if ( !mUrls.count( parent ) )
    return;
  KUrl url = mUrls[parent];
  if ( url.isEmpty() )
    return;
  url.setFileName( itemText( mContextMenuItem ) );
  mCurrentURL = url;
  KManageSieve::SieveJob * job = KManageSieve::SieveJob::get( url );
  connect( job, SIGNAL(result(KManageSieve::SieveJob*,bool,const QString&,bool)),
           this, SLOT(slotGetResult(KManageSieve::SieveJob*,bool,const QString&,bool)) );
}

void ManageSieveScriptsDialog::slotNewScript()
{
  if ( !mContextMenuItem )
    return;
  if ( mContextMenuItem->parent() )
    mContextMenuItem = mContextMenuItem->parent();
  if ( !mContextMenuItem )
    return;

  if ( !mUrls.count( mContextMenuItem ) )
    return;

  KUrl u = mUrls[mContextMenuItem];
  if ( u.isEmpty() )
    return;

  bool ok = false;
  const QString name = KInputDialog::getText( i18n( "New Sieve Script" ),
                                              i18n( "Please enter a name for the new Sieve script:" ),
                                              i18n( "unnamed" ), &ok, this );
  if ( !ok || name.isEmpty() )
    return;

  u.setFileName( name );

  QTreeWidgetItem *newItem =
      new QTreeWidgetItem( mContextMenuItem );
  addRadioButton( newItem, name );
  mCurrentURL = u;
  slotGetResult( 0, true, QString(), false );
}

SieveEditor::SieveEditor( QWidget * parent, const char * name )
  : KDialog( parent )
{
  setCaption( i18n( "Edit Sieve Script" ) );
  setButtons( Ok|Cancel );
  setObjectName( name );
  setDefaultButton( Ok );
  setModal( true );
  QFrame *frame = new QFrame( this );
  setMainWidget( frame );
  QVBoxLayout * vlay = new QVBoxLayout( frame );
  vlay->setSpacing( spacingHint() );
  vlay->setMargin( 0 );
  mTextEdit = new KTextEdit( frame);
  mTextEdit->setFocus();
  vlay->addWidget( mTextEdit );
  mTextEdit->setAcceptRichText( false );
  mTextEdit->setWordWrapMode ( QTextOption::NoWrap );
  mTextEdit->setFont( KGlobalSettings::fixedFont() );
  connect( mTextEdit, SIGNAL( textChanged () ), SLOT( slotTextChanged() ) );
  resize( 3 * sizeHint() );
}

SieveEditor::~SieveEditor()
{
}

void SieveEditor::slotTextChanged()
{
  enableButtonOk( !script().isEmpty() );
}

void ManageSieveScriptsDialog::slotGetResult( KManageSieve::SieveJob *, bool success, const QString & script, bool isActive )
{
  if ( !success )
    return;

  if ( mSieveEditor )
    return;

  mSieveEditor = new SieveEditor( this );
  mSieveEditor->setScript( script );
  connect( mSieveEditor, SIGNAL(okClicked()), this, SLOT(slotSieveEditorOkClicked()) );
  connect( mSieveEditor, SIGNAL(cancelClicked()), this, SLOT(slotSieveEditorCancelClicked()) );
  mSieveEditor->show();
  mWasActive = isActive;
}

void ManageSieveScriptsDialog::slotSieveEditorOkClicked()
{
  if ( !mSieveEditor )
    return;
  KManageSieve::SieveJob * job = KManageSieve::SieveJob::put( mCurrentURL,mSieveEditor->script(), mWasActive, mWasActive );
  connect( job, SIGNAL(result(KManageSieve::SieveJob*,bool,const QString&,bool)),
           this, SLOT(slotPutResult(KManageSieve::SieveJob*,bool)) );
}

void ManageSieveScriptsDialog::slotSieveEditorCancelClicked()
{
  mSieveEditor->deleteLater(); mSieveEditor = 0;
  mCurrentURL = KUrl();
  slotRefresh();
}

void ManageSieveScriptsDialog::slotPutResult( KManageSieve::SieveJob *, bool success )
{
  if ( success ) {
    KMessageBox::information( this, i18n( "The Sieve script was successfully uploaded." ),
                              i18n( "Sieve Script Upload" ) );
    mSieveEditor->deleteLater(); mSieveEditor = 0;
    mCurrentURL = KUrl();
  } else {
    mSieveEditor->show();
  }
}

#include "managesievescriptsdialog.moc"
#include "managesievescriptsdialog_p.moc"
