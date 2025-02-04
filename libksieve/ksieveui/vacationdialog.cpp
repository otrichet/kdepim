/*  -*- c++ -*-
    vacationdialog.cpp

    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/


#include "vacationdialog.h"

#include <kdebug.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmime/kmime_header_parsing.h>
#include <knuminput.h>
#include <ktextedit.h>
#include <kwindowsystem.h>

#include <QtCore/QByteArray>
#include <QtGui/QApplication>
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

using KMime::Types::AddrSpecList;
using KMime::Types::AddressList;
using KMime::Types::MailboxList;
using KMime::HeaderParsing::parseAddressList;

using namespace KSieveUi;

VacationDialog::VacationDialog( const QString & caption, QWidget * parent,
                                const char * name, bool modal )
  : KDialog( parent )
{
  setCaption( caption );
  setObjectName( name );
  setButtons( Ok|Cancel|Default );
  setDefaultButton(  Ok );
  setModal( modal );
  QFrame *frame = new QFrame( this );
  setMainWidget( frame );
  KWindowSystem::setIcons( winId(), qApp->windowIcon().pixmap(IconSize(KIconLoader::Desktop),IconSize(KIconLoader::Desktop)), qApp->windowIcon().pixmap(IconSize(KIconLoader::Small),IconSize(KIconLoader::Small)) );
  static const int rows = 7;
  int row = -1;

  QGridLayout * glay = new QGridLayout( frame );
  glay->setSpacing( spacingHint() );
  glay->setMargin( 0 );
  glay->setColumnStretch( 1, 1 );

  // explanation label:
  ++row;
  glay->addWidget( new QLabel( i18n("Configure vacation "
                                    "notifications to be sent:"),
                               frame ), row, 0, 1, 2 );

  // Activate checkbox:
  ++row;
  mActiveCheck = new QCheckBox( i18n("&Activate vacation notifications"), frame );
  glay->addWidget( mActiveCheck, row, 0, 1, 2 );

  // Message text edit:
  ++row;
  glay->setRowStretch( row, 1 );
  mTextEdit = new KTextEdit( frame );
  mTextEdit->setObjectName( "mTextEdit" );
  mTextEdit->setAcceptRichText( false );
  glay->addWidget( mTextEdit, row, 0, 1, 2 );

  // "Resent only after" spinbox and label:
  ++row;
  int defDayInterval = 7; //default day interval
  mIntervalSpin = new KIntSpinBox( 1, 356, 1, defDayInterval, frame );
  mIntervalSpin->setObjectName( "mIntervalSpin" );
  mIntervalSpin->setSuffix( i18np(" day", " days", defDayInterval) );
  connect(mIntervalSpin, SIGNAL( valueChanged( int )), SLOT( slotIntervalSpinChanged( int ) ) );
  QLabel *label = new QLabel( i18n("&Resend notification only after:"), frame );
  label->setBuddy( mIntervalSpin );
  glay->addWidget( label, row, 0 );
  glay->addWidget( mIntervalSpin, row, 1 );

  // "Send responses for these addresses" lineedit and label:
  ++row;
  mMailAliasesEdit = new KLineEdit( frame );
  mMailAliasesEdit->setObjectName( "mMailAliasesEdit" );
  mMailAliasesEdit->setClearButtonShown( true );
  QLabel *tmpLabel = new QLabel( i18n("&Send responses for these addresses:"), frame );
  tmpLabel->setBuddy( mMailAliasesEdit );
  glay->addWidget( tmpLabel, row, 0 );
  glay->addWidget( mMailAliasesEdit, row, 1 );

  // "Send responses also to SPAM mail" checkbox:
  ++row;
  mSpamCheck = new QCheckBox( i18n("Do not send vacation replies to spam messages"), frame );
  mSpamCheck->setObjectName( "mSpamCheck" );
  mSpamCheck->setChecked( true );
  glay->addWidget( mSpamCheck, row, 0, 1, 2 );

  //  domain checkbox and linedit:
  ++row;
  mDomainCheck = new QCheckBox( i18n("Only react to mail coming from domain"), frame );
  mDomainCheck->setObjectName( "mDomainCheck" );
  mDomainCheck->setChecked( false );
  mDomainEdit = new KLineEdit( frame );
  mDomainEdit->setObjectName( "mDomainEdit" );
  mDomainEdit->setClearButtonShown( true );
  mDomainEdit->setEnabled( false );
  mDomainEdit->setValidator( new QRegExpValidator( QRegExp( "[a-zA-Z0-9+-]+(?:\\.[a-zA-Z0-9+-]+)*" ), mDomainEdit ) );
  glay->addWidget( mDomainCheck, row, 0 );
  glay->addWidget( mDomainEdit, row, 1 );
  connect( mDomainCheck, SIGNAL(toggled(bool)),
           mDomainEdit, SLOT(setEnabled(bool)) );

  Q_ASSERT( row == rows - 1 );
}

VacationDialog::~VacationDialog() {
  kDebug() << "~VacationDialog()";
}

bool VacationDialog::activateVacation() const {
  return mActiveCheck->isChecked();
}

void VacationDialog::setActivateVacation( bool activate ) {
  mActiveCheck->setChecked( activate );
}

QString VacationDialog::messageText() const {
  return mTextEdit->toPlainText().trimmed();
}

void VacationDialog::setMessageText( const QString & text ) {
  mTextEdit->setText( text );
  const int height = ( mTextEdit->fontMetrics().lineSpacing() + 1 ) * 11;
  mTextEdit->setMinimumHeight( height );
}

int VacationDialog::notificationInterval() const {
  return mIntervalSpin->value();
}

void VacationDialog::setNotificationInterval( int days ) {
  mIntervalSpin->setValue( days );
}

AddrSpecList VacationDialog::mailAliases() const {
  QByteArray text = mMailAliasesEdit->text().toLatin1(); // ### IMAA: !ok
  AddressList al;
  const char * s = text.begin();
  parseAddressList( s, text.end(), al );

  AddrSpecList asl;
  for ( AddressList::const_iterator it = al.constBegin() ; it != al.constEnd() ; ++it ) {
    const MailboxList & mbl = (*it).mailboxList;
    for ( MailboxList::const_iterator jt = mbl.constBegin() ; jt != mbl.constEnd() ; ++jt )
      asl.push_back( (*jt).addrSpec() );
  }
  return asl;
}

void VacationDialog::setMailAliases( const AddrSpecList & aliases ) {
  QStringList sl;
  for ( AddrSpecList::const_iterator it = aliases.constBegin() ; it != aliases.constEnd() ; ++it )
    sl.push_back( (*it).asString() );
  mMailAliasesEdit->setText( sl.join(", ") );
}

void VacationDialog::setMailAliases( const QString & aliases ) {
  mMailAliasesEdit->setText( aliases );
}

void VacationDialog::slotIntervalSpinChanged ( int value ) {
  mIntervalSpin->setSuffix( i18np(" day", " days", value) );
}

QString VacationDialog::domainName() const {
  return mDomainCheck->isChecked() ? mDomainEdit->text() : QString() ;
}

void VacationDialog::setDomainName( const QString & domain ) {
  if ( !domain.isEmpty() ) {
    mDomainEdit->setText( domain );
    mDomainCheck->setChecked( true );
  }
}

bool VacationDialog::domainCheck() const {
  return mDomainCheck->isChecked();
}

void VacationDialog::setDomainCheck( bool check ) {
  mDomainCheck->setChecked( check );
}

bool VacationDialog::sendForSpam() const {
  return !mSpamCheck->isChecked();
}

void VacationDialog::setSendForSpam( bool enable ) {
  mSpamCheck->setChecked( !enable );
}

void VacationDialog::enableDomainAndSendForSpam( bool enable )
{
  mDomainCheck->setEnabled( enable );
  mDomainEdit->setEnabled( enable && mDomainCheck->isChecked() );
  mSpamCheck->setEnabled( enable );
}

#include "vacationdialog.moc"
