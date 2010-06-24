/*
    Copyright (c) 2010 Bertjan Broeksema <broeksema@kde.org>
    Copyright (C) 2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "eventortododialogng.h"

#include <KCal/Incidence>
#include <KCal/Event>
#include <KCal/Todo>

#include <Akonadi/CollectionComboBox>
#include <Akonadi/CollectionComboBox>
#include <Akonadi/Item>
#include <Akonadi/KCal/IncidenceMimeTypeVisitor>

#include "combinedincidenceeditor.h"
#include "editoritemmanager.h"
#include "incidencealarm.h"
#include "incidenceattachment.h"
#include "incidencecategories.h"
#include "incidencedatetime.h"
#include "incidencedescription.h"
#include "incidencegeneral.h"
#include "incidencerecurrence.h"
#include "incidencesecrecy.h"
#include "incidenceattendee.h"
#include "ui_eventortododesktop.h"

using namespace IncidenceEditorsNG;

class EventOrTodoDialogNGPrivate : public Akonadi::ItemEditorUi
{
  EventOrTodoDialogNG *q_ptr;
  Q_DECLARE_PUBLIC( EventOrTodoDialogNG )

public:
  Ui::EventOrTodoDesktop *mUi;
  Akonadi::CollectionComboBox *mCalSelector;

  Akonadi::EditorItemManager *mItemManager;
  CombinedIncidenceEditor *mEditor;

public:
  EventOrTodoDialogNGPrivate( EventOrTodoDialogNG *qq );
  ~EventOrTodoDialogNGPrivate();

  /// General methods
  void updateButtonStatus( bool isDirty );

  /// ItemEditorUi methods
  virtual bool containsPayloadIdentifiers( const QSet<QByteArray> &partIdentifiers ) const;
  virtual bool hasSupportedPayload( const Akonadi::Item &item ) const;
  virtual bool isDirty() const;
  virtual bool isValid();
  virtual void load( const Akonadi::Item &item );
  virtual Akonadi::Item save( const Akonadi::Item &item );
  virtual Akonadi::Collection selectedCollection() const;
  virtual void reject( RejectReason reason, const QString &errorMessage = QString() );
};

EventOrTodoDialogNGPrivate::EventOrTodoDialogNGPrivate( EventOrTodoDialogNG *qq )
  : q_ptr( qq )
  , mUi( new Ui::EventOrTodoDesktop )
  , mCalSelector( new Akonadi::CollectionComboBox )
  , mItemManager( new Akonadi::EditorItemManager( this ) )
  , mEditor( new CombinedIncidenceEditor )
{
  Q_Q( EventOrTodoDialogNG );
  mUi->setupUi( q->mainWidget() );

  QGridLayout *layout = new QGridLayout( mUi->mCalSelectorPlaceHolder );
  layout->addWidget( mCalSelector );

  mCalSelector->setAccessRightsFilter( Akonadi::Collection::CanCreateItem );

  // Now instantiate the logic of the dialog. These editors update the ui, validate
  // fields and load/store incidences in the ui.
  IncidenceGeneral *ieGeneral = new IncidenceGeneral( mUi );
  mEditor->combine( ieGeneral );

  IncidenceCategories *ieCategories = new IncidenceCategories( mUi );
  mEditor->combine( ieCategories );

  IncidenceDateTime *ieDateTime = new IncidenceDateTime( mUi );
  mEditor->combine( ieDateTime );

  IncidenceDescription *ieDescription = new IncidenceDescription( mUi );
  mEditor->combine( ieDescription );

  IncidenceAlarm *ieAlarm = new IncidenceAlarm( mUi );
  mEditor->combine( ieAlarm );

  IncidenceAttachment *ieAttachments = new IncidenceAttachment( mUi );
  mEditor->combine( ieAttachments );

  IncidenceRecurrence *ieRecurrence = new IncidenceRecurrence( ieDateTime, mUi );
  mEditor->combine( ieRecurrence );

  IncidenceSecrecy *ieSecrecy = new IncidenceSecrecy( mUi );
  mEditor->combine( ieSecrecy );

  IncidenceAttendee *ieAttendee= new IncidenceAttendee( mUi );
  mEditor->combine( ieAttendee );

  q->connect( mEditor, SIGNAL(dirtyStatusChanged(bool)),
             SLOT(updateButtonStatus(bool)) );
//  connect( d->mAttachtmentPage, SIGNAL(attachmentCountChanged(int)),
//           SLOT(updateAttachmentCount(int)) );
}

EventOrTodoDialogNGPrivate::~EventOrTodoDialogNGPrivate()
{
  delete mItemManager;
  delete mEditor;
}

void EventOrTodoDialogNGPrivate::updateButtonStatus( bool isDirty )
{
  Q_Q( EventOrTodoDialogNG );
  qDebug() << "EventOrTodoDialogNGPrivate::updateButtonStatus" << isDirty;
  q->enableButton( KDialog::Apply, isDirty );
  q->enableButton( KDialog::Ok, isDirty );
}


bool EventOrTodoDialogNGPrivate::containsPayloadIdentifiers( const QSet<QByteArray> &partIdentifiers ) const
{
  return partIdentifiers.contains( QByteArray( "PLD:RFC822" ) );
}

bool EventOrTodoDialogNGPrivate::hasSupportedPayload( const Akonadi::Item &item ) const
{
  return item.hasPayload() && item.hasPayload<KCal::Incidence::Ptr>()
    && ( item.hasPayload<KCal::Event::Ptr>() || item.hasPayload<KCal::Todo::Ptr>() );
}

bool EventOrTodoDialogNGPrivate::isDirty() const
{
  return mEditor->isDirty();
}

bool EventOrTodoDialogNGPrivate::isValid()
{
  return mEditor->isValid();
}

void EventOrTodoDialogNGPrivate::load( const Akonadi::Item &item )
{
  Q_ASSERT( hasSupportedPayload( item ) );
  mEditor->load( item.payload<KCal::Incidence::Ptr>() );

  if ( item.hasPayload<KCal::Event::Ptr>() ) {
    mCalSelector->setMimeTypeFilter(
      QStringList() << Akonadi::IncidenceMimeTypeVisitor::eventMimeType() );
  } else {
    mCalSelector->setMimeTypeFilter(
      QStringList() << Akonadi::IncidenceMimeTypeVisitor::todoMimeType() );
  }
}

Akonadi::Item EventOrTodoDialogNGPrivate::save( const Akonadi::Item &item )
{
  // TODO: Add support for todos
  KCal::Event::Ptr event( new KCal::Event );
  event->setUid( mEditor->incidence<KCal::Incidence>()->uid() );
  mEditor->save( event );

  Akonadi::Item result = item;
  result.setMimeType( Akonadi::IncidenceMimeTypeVisitor::eventMimeType() );
  result.setPayload<KCal::Event::Ptr>( event );
  return result;
}

Akonadi::Collection EventOrTodoDialogNGPrivate::selectedCollection() const
{
  return mCalSelector->currentCollection();
}

void EventOrTodoDialogNGPrivate::reject( RejectReason /*reason*/, const QString &errorMessage )
{
  Q_Q( EventOrTodoDialogNG );
  kDebug() << "Rejecting:" << errorMessage;
  q->deleteLater();
}

/// EventOrTodoDialog

EventOrTodoDialogNG::EventOrTodoDialogNG()
  : d_ptr( new EventOrTodoDialogNGPrivate( this ) )
{
  setButtons( KDialog::Ok | KDialog::Apply | KDialog::Cancel );
  setButtonText( KDialog::Apply, i18nc( "@action:button", "&Save" ) );
  setButtonToolTip( KDialog::Apply, i18nc( "@info:tooltip", "Save current changes" ) );
  setButtonToolTip( KDialog::Ok, i18nc( "@action:button", "Save changes and close dialog" ) );
  setButtonToolTip( KDialog::Cancel, i18nc( "@action:button", "Discard changes and close dialog" ) );
  setDefaultButton( Ok );
  enableButton( Ok, false );
  enableButton( Apply, false );

  setModal( false );
  showButtonSeparator( false );
}

EventOrTodoDialogNG::~EventOrTodoDialogNG()
{
  delete d_ptr;
}

void EventOrTodoDialogNG::load( const Akonadi::Item &item )
{
  Q_D( EventOrTodoDialogNG );
  Q_ASSERT( d->hasSupportedPayload( item ) );

  if ( item.isValid() ) {
    d->mItemManager->load( item );
  } else {
    d->mEditor->load( item.payload<KCal::Incidence::Ptr>() );
  }

  if ( item.hasPayload<KCal::Event::Ptr>() ) {
    d->mCalSelector->setMimeTypeFilter(
      QStringList() << Akonadi::IncidenceMimeTypeVisitor::eventMimeType() );
  } else {
    d->mCalSelector->setMimeTypeFilter(
      QStringList() << Akonadi::IncidenceMimeTypeVisitor::todoMimeType() );
  }
}

#include "eventortododialogng.moc"
