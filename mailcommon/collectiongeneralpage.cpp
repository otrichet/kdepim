/* -*- mode: C++; c-file-style: "gnu" -*-
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "collectiongeneralpage.h"

#include "collectionannotationsattribute.h"

#include <akonadi/agentmanager.h>
#include <akonadi/attributefactory.h>
#include <akonadi/collection.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/collectionmodifyjob.h>
#include <kcombobox.h>
#include <kdialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpimidentities/identitycombo.h>
#include <mailcommon/foldercollection.h>
#include <mailcommon/mailkernel.h>
#include <mailcommon/mailutil.h>

#include <QtGui/QCheckBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>

// TODO Where should these be?
#define KOLAB_FOLDERTYPE "/vendor/kolab/folder-type"
#define KOLAB_INCIDENCESFOR "/vendor/kolab/incidences-for"
#define KOLAB_SHAREDSEEN "/vendor/cmu/cyrus-imapd/sharedseen"

using namespace Akonadi;
using namespace MailCommon;

CollectionGeneralPage::CollectionGeneralPage( QWidget *parent )
  : CollectionPropertiesPage( parent ), mFolderCollection( 0 )
{
  setObjectName( QLatin1String( "MailCommon::CollectionGeneralPage" ) );
  setPageTitle( i18nc( "@title:tab General settings for a folder.", "General" ) );
}

CollectionGeneralPage::~CollectionGeneralPage()
{
}

static void addLine( QWidget *parent, QVBoxLayout *layout )
{
  QFrame *line = new QFrame( parent );
  line->setObjectName( "line" );
  line->setGeometry( QRect( 80, 150, 250, 20 ) );
  line->setFrameShape( QFrame::HLine );
  line->setFrameShadow( QFrame::Sunken );
  line->setFrameShape( QFrame::HLine );
  layout->addWidget( line );
}

static QString incidencesForToString( CollectionGeneralPage::IncidencesFor type )
{
  switch ( type ) {
    case CollectionGeneralPage::IncForNobody:
      return "nobody";
    case CollectionGeneralPage::IncForAdmins:
      return "admins";
    case CollectionGeneralPage::IncForReaders:
      return "readers";
  }

  return QString(); // can't happen
}

static CollectionGeneralPage::IncidencesFor incidencesForFromString( const QString &string )
{
  if ( string == "nobody" ) {
    return CollectionGeneralPage::IncForNobody;
  }
  if ( string == "admins" ) {
    return CollectionGeneralPage::IncForAdmins;
  }
  if ( string == "readers" ) {
    return CollectionGeneralPage::IncForReaders;
  }

  return CollectionGeneralPage::IncForAdmins; // by default
}

static QString folderContentDescription( CollectionGeneralPage::FolderContentsType type )
{
  switch ( type ) {
    case CollectionGeneralPage::ContentsTypeMail:
      return ( i18nc( "type of folder content", "Mail" ) );
    case CollectionGeneralPage::ContentsTypeCalendar:
      return ( i18nc( "type of folder content", "Calendar" ) );
    case CollectionGeneralPage::ContentsTypeContact:
      return ( i18nc( "type of folder content", "Contacts" ) );
    case CollectionGeneralPage::ContentsTypeNote:
      return ( i18nc( "type of folder content", "Notes" ) );
    case CollectionGeneralPage::ContentsTypeTask:
      return ( i18nc( "type of folder content", "Tasks" ) );
    case CollectionGeneralPage::ContentsTypeJournal:
      return ( i18nc( "type of folder content", "Journal" ) );
    default:
      return ( i18nc( "type of folder content", "Unknown" ) );
  }
}

static CollectionGeneralPage::FolderContentsType contentsTypeFromString( const QString &type )
{
  if ( type == i18nc( "type of folder content", "Mail" ) )
    return CollectionGeneralPage::ContentsTypeMail;
  if ( type == i18nc( "type of folder content", "Calendar" ) )
    return CollectionGeneralPage::ContentsTypeCalendar;
  if ( type == i18nc( "type of folder content", "Contacts" ) )
    return CollectionGeneralPage::ContentsTypeContact;
  if ( type == i18nc( "type of folder content", "Notes" ) )
    return CollectionGeneralPage::ContentsTypeNote;
  if ( type == i18nc( "type of folder content", "Tasks" ) )
    return CollectionGeneralPage::ContentsTypeTask;
  if ( type == i18nc( "type of folder content", "Journal" ) )
    return CollectionGeneralPage::ContentsTypeJournal;

  return CollectionGeneralPage::ContentsTypeMail; //safety return value
}

static QString typeNameFromKolabType( const QByteArray &type )
{
  if ( type == "task" || type == "task.default" )
    return i18nc( "type of folder content", "Tasks" );
  if ( type == "event" || type == "event.default" )
    return i18nc( "type of folder content", "Calendar" );
  if ( type == "contact" || type == "contact.default" )
    return i18nc( "type of folder content", "Contacts" );
  if ( type == "note" || type == "note.default" )
    return i18nc( "type of folder content", "Notes" );
  if ( type == "journal" || type == "journal.default" )
    return i18nc( "type of folder content", "Journal" );

  return i18nc( "type of folder content", "Mail" );
}

static QByteArray kolabNameFromType( CollectionGeneralPage::FolderContentsType type )
{
  switch ( type ) {
    case CollectionGeneralPage::ContentsTypeCalendar:
      return "event";
    case CollectionGeneralPage::ContentsTypeContact:
      return "contact";
    case CollectionGeneralPage::ContentsTypeNote:
      return "note";
    case CollectionGeneralPage::ContentsTypeTask:
      return "task";
    case CollectionGeneralPage::ContentsTypeJournal:
      return "journal";
    default:
      return QByteArray();
  }
}

static CollectionGeneralPage::FolderContentsType typeFromKolabName( const QByteArray &name )
{
  if ( name == "task" || name == "task.default" )
    return CollectionGeneralPage::ContentsTypeTask;
  if ( name == "event" || name == "event.default" )
    return CollectionGeneralPage::ContentsTypeCalendar;
  if ( name == "contact" || name == "contact.default" )
    return CollectionGeneralPage::ContentsTypeContact;
  if ( name == "note" || name == "note.default" )
    return CollectionGeneralPage::ContentsTypeNote;
  if ( name == "journal" || name == "journal.default" )
    return CollectionGeneralPage::ContentsTypeJournal;

  return CollectionGeneralPage::ContentsTypeMail;
}

void CollectionGeneralPage::init( const Akonadi::Collection &collection )
{
  mIsLocalSystemFolder = CommonKernel->isSystemFolderCollection( collection );
  mIsImapFolder = collection.resource().contains( IMAP_RESOURCE_IDENTIFIER );

  mIsResourceFolder = (collection.parentCollection() == Akonadi::Collection::root());
  QLabel *label;

  QVBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setSpacing( KDialog::spacingHint() );
  topLayout->setMargin( 0 );

  // Musn't be able to edit details for a non-resource, system folder.
  if ( ( !mIsLocalSystemFolder || mIsResourceFolder )
       && !mFolderCollection->isReadOnly() ) {

    QHBoxLayout *hl = new QHBoxLayout();
    topLayout->addItem( hl );
    hl->setSpacing( KDialog::spacingHint() );
    hl->setMargin( KDialog::marginHint() );
    label = new QLabel( i18nc( "@label:textbox Name of the folder.", "&Name:" ), this );
    hl->addWidget( label );

    mNameEdit = new KLineEdit( this );
    mNameEdit->setEnabled( collection.rights() & Collection::CanChangeCollection );
    label->setBuddy( mNameEdit );
    hl->addWidget( mNameEdit );
  }


  // should new mail in this folder be ignored?
  QHBoxLayout *hbl = new QHBoxLayout();
  topLayout->addItem( hbl );
  hbl->setSpacing( KDialog::spacingHint() );
  hbl->setMargin( KDialog::marginHint() );
  mNotifyOnNewMailCheckBox =
    new QCheckBox( i18n("Act on new/unread mail in this folder" ), this );
  mNotifyOnNewMailCheckBox->setWhatsThis(
      i18n( "<qt><p>If this option is enabled then you will be notified about "
            "new/unread mail in this folder. Moreover, going to the "
            "next/previous folder with unread messages will stop at this "
            "folder.</p>"
            "<p>Uncheck this option if you do not want to be notified about "
            "new/unread mail in this folder and if you want this folder to "
            "be skipped when going to the next/previous folder with unread "
            "messages. This is useful for ignoring any new/unread mail in "
            "your trash and spam folder.</p></qt>" ) );
  hbl->addWidget( mNotifyOnNewMailCheckBox );
#if 0
  if ( collection.resource().contains( IMAP_RESOURCE_IDENTIFIER ) ) {
    // should this folder be included in new-mail-checks?

    QHBoxLayout *nml = new QHBoxLayout();
    topLayout->addItem( nml );
    nml->setMargin( KDialog::marginHint() );
    nml->setSpacing( KDialog::spacingHint() );
    mNewMailCheckBox = new QCheckBox( i18n("Include this folder in mail checks"), this );
    mNewMailCheckBox->setWhatsThis(
        i18n("<qt><p>If this option is enabled this folder will be included "
              "while checking new emails.</p>"
              "<p>Uncheck this option if you want to skip this folder "
              "while checking new emails.</p></qt>") );
    // default is on
    mNewMailCheckBox->setChecked(true);
    nml->addWidget( mNewMailCheckBox );
    nml->addStretch( 1 );
  }
#endif
  // should replies to mails in this folder be kept in this same folder?
  hbl = new QHBoxLayout();
  topLayout->addItem( hbl );
  hbl->setSpacing( KDialog::spacingHint() );
  hbl->setMargin( KDialog::marginHint() );
  mKeepRepliesInSameFolderCheckBox =
    new QCheckBox( i18n( "Keep replies in this folder" ), this );
  mKeepRepliesInSameFolderCheckBox->setWhatsThis(
                   i18n( "Check this option if you want replies you write "
                         "to mails in this folder to be put in this same folder "
                         "after sending, instead of in the configured sent-mail folder." ) );
  hbl->addWidget( mKeepRepliesInSameFolderCheckBox );
  hbl->addStretch( 1 );
  // should this folder be shown in the folder selection dialog?
  hbl = new QHBoxLayout();
  topLayout->addItem( hbl );
  hbl->setSpacing( KDialog::spacingHint() );
  hbl->setMargin( KDialog::marginHint() );
  mHideInSelectionDialogCheckBox =
      new QCheckBox( i18n( "Hide this folder in the folder selection dialog" ), this );
  mHideInSelectionDialogCheckBox->setWhatsThis(
                  i18nc( "@info:whatsthis", "Check this option if you do not want this folder "
                         "to be shown in folder selection dialogs, such as the <interface>"
                          "Jump to Folder</interface> dialog." ) );
  hbl->addWidget( mHideInSelectionDialogCheckBox );
  hbl->addStretch( 1 );

  addLine( this, topLayout );
  // use grid layout for the following combobox settings
  QGridLayout *gl = new QGridLayout();
  gl->setMargin( KDialog::marginHint() );
  topLayout->addItem( gl );
  gl->setSpacing( KDialog::spacingHint() );
  gl->setColumnStretch( 1, 100 ); // make the second column use all available space
  int row = -1;

  // sender identity
  ++row;
  mUseDefaultIdentityCheckBox = new QCheckBox( i18n( "Use &default identity" ), this );
  gl->addWidget( mUseDefaultIdentityCheckBox );
  connect( mUseDefaultIdentityCheckBox, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotIdentityCheckboxChanged() ) );
  ++row;
  label = new QLabel( i18n( "&Sender identity:" ), this );
  gl->addWidget( label, row, 0 );
  mIdentityComboBox = new KPIMIdentities::IdentityCombo( KernelIf->identityManager(), this );
  label->setBuddy( mIdentityComboBox );
  gl->addWidget( mIdentityComboBox, row, 1 );
  mIdentityComboBox->setWhatsThis(
      i18n( "Select the sender identity to be used when writing new mail "
            "or replying to mail in this folder. This means that if you are in "
            "one of your work folders, you can make KMail use the corresponding "
            "sender email address, signature and signing or encryption keys "
            "automatically. Identities can be set up in the main configuration "
            "dialog. (Settings -> Configure KMail)") );

  CollectionGeneralPage::FolderContentsType contentsType = CollectionGeneralPage::ContentsTypeMail;

  const CollectionAnnotationsAttribute *annotationAttribute = collection.attribute<CollectionAnnotationsAttribute>();
  const QMap<QByteArray, QByteArray> annotations = (annotationAttribute ? annotationAttribute->annotations() : QMap<QByteArray, QByteArray>());

  const bool sharedSeen = (annotations.value( KOLAB_SHAREDSEEN ) == "true");
  const IncidencesFor incidencesFor = incidencesForFromString( annotations.value( KOLAB_INCIDENCESFOR ) );
  const FolderContentsType folderType = typeFromKolabName( annotations.value( KOLAB_FOLDERTYPE ) );

  // Only do make this settable, if the IMAP resource is enabled
  // and it's not the personal folders (those must not be changed)
  if ( collection.resource().contains( IMAP_RESOURCE_IDENTIFIER ) ) {
    ++row;
    label = new QLabel( i18n( "&Folder contents:" ), this );
    gl->addWidget( label, row, 0 );
    mContentsComboBox = new KComboBox( this );
    label->setBuddy( mContentsComboBox );
    gl->addWidget( mContentsComboBox, row, 1 );
    mContentsComboBox->addItem( folderContentDescription( ContentsTypeMail ) );
    mContentsComboBox->addItem( folderContentDescription( ContentsTypeCalendar ) );
    mContentsComboBox->addItem( folderContentDescription( ContentsTypeContact ) );
    mContentsComboBox->addItem( folderContentDescription( ContentsTypeNote ) );
    mContentsComboBox->addItem( folderContentDescription( ContentsTypeTask ) );
    mContentsComboBox->addItem( folderContentDescription( ContentsTypeJournal ) );

    mContentsComboBox->setCurrentIndex( contentsType );

    connect( mContentsComboBox, SIGNAL ( activated( int ) ),
             this, SLOT( slotFolderContentsSelectionChanged( int ) ) );

    if ( mFolderCollection->isReadOnly() || mIsResourceFolder )
      mContentsComboBox->setEnabled( false );

  } else {
    mContentsComboBox = 0;
  }

  // Kolab incidences-for annotation.
  // Show incidences-for combobox if the contents type can be changed (new folder),
  // or if it's set to calendar or task (existing folder)
  if ( folderType == ContentsTypeCalendar || folderType == ContentsTypeTask ) {
    ++row;
    QLabel* label = new QLabel( i18n( "Generate free/&busy and activate alarms for:" ), this );
    gl->addWidget( label, row, 0 );
    mIncidencesForComboBox = new KComboBox( this );
    label->setBuddy( mIncidencesForComboBox );
    gl->addWidget( mIncidencesForComboBox, row, 1 );

    mIncidencesForComboBox->addItem( i18n( "Nobody" ) );
    mIncidencesForComboBox->addItem( i18n( "Admins of This Folder" ) );
    mIncidencesForComboBox->addItem( i18n( "All Readers of This Folder" ) );
    const QString whatsThisForMyOwnFolders =
      i18n( "This setting defines which users sharing "
            "this folder should get \"busy\" periods in their freebusy lists "
            "and should see the alarms for the events or tasks in this folder. "
            "The setting applies to Calendar and Task folders only "
            "(for tasks, this setting is only used for alarms).\n\n"
            "Example use cases: if the boss shares a folder with his secretary, "
            "only the boss should be marked as busy for his meetings, so he should "
            "select \"Admins\", since the secretary has no admin rights on the folder.\n"
            "On the other hand if a working group shares a Calendar for "
            "group meetings, all readers of the folders should be marked "
            "as busy for meetings.\n"
            "A company-wide folder with optional events in it would use \"Nobody\" "
            "since it is not known who will go to those events." );

    mIncidencesForComboBox->setWhatsThis( whatsThisForMyOwnFolders );
    mIncidencesForComboBox->setCurrentIndex( incidencesFor );
  } else {
    mIncidencesForComboBox = 0;
  }

  if ( collection.resource().contains( IMAP_RESOURCE_IDENTIFIER ) ) {
    mSharedSeenFlagsCheckBox = new QCheckBox( this );
    mSharedSeenFlagsCheckBox->setText( i18n( "Share unread state with all users" ) );
    mSharedSeenFlagsCheckBox->setChecked( sharedSeen );
    ++row;
    gl->addWidget( mSharedSeenFlagsCheckBox, row, 0, 1, 1 );
    mSharedSeenFlagsCheckBox->setWhatsThis( i18n( "If enabled, the unread state of messages in this folder will be the same "
        "for all users having access to this folder. If disabled (the default), every user with access to this folder has their "
        "own unread state." ) );
  } else {
    mSharedSeenFlagsCheckBox = 0;
  }

  topLayout->addStretch( 100 ); // eat all superfluous space
}


void CollectionGeneralPage::load( const Akonadi::Collection &collection )
{
  mFolderCollection = FolderCollection::forCollection( collection );
  init( collection );

  QString displayName;
  if ( collection.hasAttribute<Akonadi::EntityDisplayAttribute>() ) {
    displayName = collection.attribute<Akonadi::EntityDisplayAttribute>()->displayName();
  }

  if ( !mIsLocalSystemFolder || mIsResourceFolder ) {
    if ( displayName.isEmpty() )
      mNameEdit->setText( collection.name() );
    else
      mNameEdit->setText( displayName );
  }

  // folder identity
  mIdentityComboBox->setCurrentIdentity( mFolderCollection->identity() );
  mUseDefaultIdentityCheckBox->setChecked( mFolderCollection->useDefaultIdentity() );

  // ignore new mail
  mNotifyOnNewMailCheckBox->setChecked( !mFolderCollection->ignoreNewMail() );

  const bool keepInFolder = mFolderCollection->canCreateMessages() && mFolderCollection->putRepliesInSameFolder();
  mKeepRepliesInSameFolderCheckBox->setChecked( keepInFolder );
  mKeepRepliesInSameFolderCheckBox->setEnabled( mFolderCollection->canCreateMessages() );
  mHideInSelectionDialogCheckBox->setChecked( mFolderCollection->hideInSelectionDialog() );

  if ( mContentsComboBox ) {
    const CollectionAnnotationsAttribute *annotationsAttribute = collection.attribute<CollectionAnnotationsAttribute>();
    if ( annotationsAttribute ) {
      const QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();
      if ( annotations.contains( KOLAB_FOLDERTYPE ) )
        mContentsComboBox->setCurrentItem( typeNameFromKolabType( annotations[ KOLAB_FOLDERTYPE ] ) );
    }
  }
}

void CollectionGeneralPage::save( Collection &collection )
{
  if ( !mIsLocalSystemFolder || mIsResourceFolder ) {
    if ( collection.hasAttribute<Akonadi::EntityDisplayAttribute>() &&
         !collection.attribute<Akonadi::EntityDisplayAttribute>()->displayName().isEmpty() )
      collection.attribute<Akonadi::EntityDisplayAttribute>()->setDisplayName( mNameEdit->text() );
    else if( !mNameEdit->text().isEmpty() )
      collection.setName( mNameEdit->text() );
  }

  CollectionAnnotationsAttribute *annotationsAttribute = collection.attribute<CollectionAnnotationsAttribute>( Entity::AddIfMissing );

  QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();
  if ( mSharedSeenFlagsCheckBox && mSharedSeenFlagsCheckBox->isEnabled() ) {
    annotations[ KOLAB_SHAREDSEEN ] = mSharedSeenFlagsCheckBox->isChecked() ? "true" : "false";
  }

  if ( mIncidencesForComboBox && mIncidencesForComboBox->isEnabled() )
    annotations[ KOLAB_INCIDENCESFOR ] = incidencesForToString( static_cast<IncidencesFor>( mIncidencesForComboBox->currentIndex() ) ).toLatin1();

  if ( mContentsComboBox ) {
      const CollectionGeneralPage::FolderContentsType type = contentsTypeFromString( mContentsComboBox->currentText() );
    const QByteArray kolabName = kolabNameFromType( type ) ;
    if ( !kolabName.isEmpty() ) {
      QString iconName;
      switch( type ) {
      case ContentsTypeCalendar:
        iconName= QString::fromLatin1( "view-calendar" );
        break;
      case ContentsTypeContact:
        iconName= QString::fromLatin1( "view-pim-contacts" );
        break;
      case ContentsTypeNote:
        iconName = QString::fromLatin1( "view-pim-notes" );
        break;
      case ContentsTypeTask:
        iconName = QString::fromLatin1( "view-pim-tasks" );
        break;
      case ContentsTypeJournal:
        iconName = QString::fromLatin1( "view-pim-journal" );
        break;
      case ContentsTypeMail:
      default:
        break;
      }

      Akonadi::EntityDisplayAttribute *attribute =  collection.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Entity::AddIfMissing );
      attribute->setIconName( iconName );
      new Akonadi::CollectionModifyJob( collection );
      annotations[ KOLAB_FOLDERTYPE ] = kolabName;
    }
  }

  annotationsAttribute->setAnnotations( annotations );

  if ( mFolderCollection ) {
    mFolderCollection->setIdentity( mIdentityComboBox->currentIdentity() );
    mFolderCollection->setUseDefaultIdentity( mUseDefaultIdentityCheckBox->isChecked() );

    mFolderCollection->setIgnoreNewMail( !mNotifyOnNewMailCheckBox->isChecked() );
    mFolderCollection->setPutRepliesInSameFolder( mKeepRepliesInSameFolderCheckBox->isChecked() );
    mFolderCollection->setHideInSelectionDialog( mHideInSelectionDialogCheckBox->isChecked() );
  }

  mFolderCollection->writeConfig();
}

void CollectionGeneralPage::slotIdentityCheckboxChanged()
{
  mIdentityComboBox->setEnabled( !mUseDefaultIdentityCheckBox->isChecked() );
}

void CollectionGeneralPage::slotFolderContentsSelectionChanged( int )
{
  const CollectionGeneralPage::FolderContentsType type = contentsTypeFromString( mContentsComboBox->currentText() );

  if ( type != CollectionGeneralPage::ContentsTypeMail  ) {
    const QString message = i18n( "You have configured this folder to contain groupware information "
                                  "That means that this folder will disappear once the configuration "
                                  "dialog is closed." );

    KMessageBox::information( this, message );
  }

  const bool enable = (type == CollectionGeneralPage::ContentsTypeCalendar || type == CollectionGeneralPage::ContentsTypeTask);
  if ( mIncidencesForComboBox )
    mIncidencesForComboBox->setEnabled( enable );
}

#include "collectiongeneralpage.moc"
