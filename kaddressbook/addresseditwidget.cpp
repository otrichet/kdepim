/*
    This file is part of KAddressBook.
    Copyright (c) 2002 Mike Pilone <mpilone@slac.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/


#include <qbuttongroup.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qlistview.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <qtextedit.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qsignal.h>
#include <qstring.h>
#include <qhbox.h>

#include <kaccelmanager.h>
#include <kapplication.h>
#include <kbuttonbox.h>
#include <kconfig.h>
#include <klineedit.h>
#include <klistview.h>
#include <kcombobox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kdialog.h>
#include <kseparator.h>

#include "addresseditwidget.h"


AddressEditWidget::AddressEditWidget( QWidget *parent, const char *name )
  : QWidget( parent, name )
{
  QBoxLayout *layout = new QVBoxLayout( this, 4, 2 );
  layout->setSpacing( KDialog::spacingHint() );

  mTypeCombo = new AddressTypeCombo( mAddressList, this );
  connect( mTypeCombo, SIGNAL( activated( int ) ),
           SLOT( updateAddressEdit() ) );
  layout->addWidget( mTypeCombo );

  mAddressTextEdit = new QTextEdit( this );
  mAddressTextEdit->setReadOnly( true );
  mAddressTextEdit->setMinimumHeight( 20 );
  layout->addWidget( mAddressTextEdit );

  QPushButton *editButton = new QPushButton( i18n( "&Edit Addresses..." ),
                                             this );
  connect( editButton, SIGNAL( clicked() ), SLOT( edit() ) );
  layout->addWidget( editButton );
}

AddressEditWidget::~AddressEditWidget()
{
}

KABC::Address::List AddressEditWidget::addresses()
{
  KABC::Address::List retList;

  KABC::Address::List::Iterator it;
  for ( it = mAddressList.begin(); it != mAddressList.end(); ++it )
    if ( !(*it).isEmpty() )
      retList.append( *it );

  return retList;
}

void AddressEditWidget::setAddresses(const KABC::Address::List &list)
{
  mAddressList.clear();

  // Insert types for existing numbers.
  mTypeCombo->insertTypeList( list );

  QValueList<int> defaultTypes;
  defaultTypes << KABC::Address::Home;
  defaultTypes << KABC::Address::Work;

  // Insert default types.
  // Doing this for mPrefCombo is enough because the list is shared by all
  // combos.
  QValueList<int>::ConstIterator it;
  for( it = defaultTypes.begin(); it != defaultTypes.end(); ++it ) {
    if ( !mTypeCombo->hasType( *it ) )
      mTypeCombo->insertType( list, *it, Address( *it ) );
  }

  mTypeCombo->updateTypes();

  // find preferred address which will be shown
  int preferred = KABC::Address::Home;  // default if no preferred address set
  uint i;
  for (i = 0; i < list.count(); i++)
    if ( list[i].type() & KABC::Address::Pref ) {
      preferred = list[i].type();
      break;
    }

  mTypeCombo->selectType( preferred );

  updateAddressEdit();
}

void AddressEditWidget::edit()
{
  AddressEditDialog dialog( mAddressList, mTypeCombo->currentItem(), this );
  if ( dialog.exec() ) {
    if ( dialog.changed() ) {
      mAddressList = dialog.addresses();
      mTypeCombo->updateTypes();
      updateAddressEdit();
      emit modified();
    }
  }
}

void AddressEditWidget::updateAddressEdit()
{
  KABC::Address::List::Iterator it = mTypeCombo->selectedElement();

  bool block = signalsBlocked();
  blockSignals( true );

  mAddressTextEdit->setText( "" );

  if ( it != mAddressList.end() ) {
    KABC::Address a = *it;

    if ( !a.isEmpty() ) {
      QString text;
      if ( !a.street().isEmpty() )
        text += a.street() + "\n";

      if ( !a.postOfficeBox().isEmpty() )
        text += a.postOfficeBox() + "\n";

      text += a.locality() + QString(" ") + a.region();

      if ( !a.postalCode().isEmpty() )
        text += QString(", ") + a.postalCode();

      text += "\n";

      if ( !a.country().isEmpty() )
        text += a.country() + "\n";

      text += a.extended();

      mAddressTextEdit->setText(text);
    }
  }

  blockSignals( block );
}


AddressEditDialog::AddressEditDialog( const KABC::Address::List &list,
                                      int selected, QWidget *parent,
                                      const char *name )
  : KDialogBase( KDialogBase::Plain, i18n( "Edit Address" ),
                 KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok,
                 parent, name, true, true )
{
  mAddressList = list;

  QWidget *page = plainPage();
  
  QGridLayout *topLayout = new QGridLayout(page, 8, 2);
  topLayout->setSpacing(spacingHint());

  mTypeCombo = new AddressTypeCombo( mAddressList, page );
  topLayout->addMultiCellWidget( mTypeCombo, 0, 0, 0, 1 );

  QLabel *label = new QLabel(i18n("Street:"), page);
  label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  topLayout->addWidget(label, 1, 0);
  mStreetTextEdit = new QTextEdit(page, "mStreetTextEdit");
  label->setBuddy( mStreetTextEdit );
  topLayout->addWidget(mStreetTextEdit, 1, 1);

  label = new QLabel(i18n("Post office box:"), page);
  topLayout->addWidget(label, 2 , 0);
  mPOBoxEdit = new KLineEdit(page, "mPOBoxEdit");
  label->setBuddy( mPOBoxEdit );
  topLayout->addWidget(mPOBoxEdit, 2, 1);

  label = new QLabel(i18n("Locality:"), page);
  topLayout->addWidget(label, 3, 0);
  mLocalityEdit = new KLineEdit(page, "mLocalityEdit");
  label->setBuddy( mLocalityEdit );
  topLayout->addWidget(mLocalityEdit, 3, 1);

  label = new QLabel(i18n("Region:"), page);
  topLayout->addWidget(label, 4, 0);
  mRegionEdit = new KLineEdit(page, "mRegionEdit");
  label->setBuddy( mRegionEdit );
  topLayout->addWidget(mRegionEdit, 4, 1);

  label = new QLabel(i18n("Postal code:"), page);
  topLayout->addWidget(label, 5, 0);
  mPostalCodeEdit = new KLineEdit(page, "mPostalCodeEdit");
  label->setBuddy( mPostalCodeEdit );
  topLayout->addWidget(mPostalCodeEdit, 5, 1);

  label = new QLabel(i18n("Country:"), page);
  topLayout->addWidget(label, 6, 0);
  mCountryCombo = new KComboBox( true, page, "mCountryCombo" );
  mCountryCombo->setDuplicatesEnabled(false);
  mCountryCombo->setAutoCompletion(true);
  fillCountryCombo( mCountryCombo );
  label->setBuddy( mCountryCombo );
  topLayout->addWidget(mCountryCombo, 6, 1);
  
  mPreferredCheckBox = new QCheckBox( i18n( "This is the preferred address" ), page );
  topLayout->addMultiCellWidget( mPreferredCheckBox, 7, 7, 0, 1 );

  KSeparator *sep = new KSeparator( KSeparator::HLine, page );
  topLayout->addMultiCellWidget( sep, 8, 8, 0, 1 );

  QHBox *buttonBox = new QHBox( page );
  buttonBox->setSpacing( spacingHint() );
  topLayout->addMultiCellWidget( buttonBox, 9, 9, 0, 1 );

  QPushButton *addButton = new QPushButton( i18n( "&Add..." ), buttonBox );
  connect( addButton, SIGNAL( clicked() ), SLOT( addAddress() ) );

  removeButton = new QPushButton( i18n( "&Remove" ), buttonBox );
  connect( removeButton, SIGNAL( clicked() ), SLOT( removeAddress() ) );

  mTypeCombo->updateTypes();
  mTypeCombo->setCurrentItem( selected );

  updateAddressEdits();

  connect( mTypeCombo, SIGNAL( activated( int ) ),
           SLOT( updateAddressEdits() ) );
  connect( mStreetTextEdit, SIGNAL( textChanged() ), SLOT( modified() ) );
  connect( mPOBoxEdit, SIGNAL( textChanged( const QString& ) ), SLOT( modified() ) );
  connect( mLocalityEdit, SIGNAL( textChanged( const QString& ) ), SLOT( modified() ) );
  connect( mRegionEdit, SIGNAL( textChanged( const QString& ) ), SLOT( modified() ) );
  connect( mPostalCodeEdit, SIGNAL( textChanged( const QString& ) ), SLOT( modified() ) );
  connect( mCountryCombo, SIGNAL( textChanged( const QString& ) ), SLOT( modified() ) );
  connect( mPreferredCheckBox, SIGNAL( toggled( bool ) ), SLOT( modified() ) );
  connect( removeButton, SIGNAL( clicked() ), SLOT( modified() ) );

  KAcceleratorManager::manage( this );

  mChanged = false;
  removeButton->setEnabled( mAddressList.count() > 1 );
}

AddressEditDialog::~AddressEditDialog()
{
}

void AddressEditDialog::updateAddressEdits()
{
  KABC::Address::List::Iterator it = mTypeCombo->selectedElement();
  KABC::Address a = *it;

  bool tmp = mChanged;

  mStreetTextEdit->setText( a.street() );
  mRegionEdit->setText( a.region() );
  mLocalityEdit->setText( a.locality() );
  mPostalCodeEdit->setText( a.postalCode() );
  mPOBoxEdit->setText( a.postOfficeBox() );
  mCountryCombo->setCurrentText( a.country() );

  mPreferredCheckBox->setChecked( a.type() & KABC::Address::Pref );

  mStreetTextEdit->setFocus();

  mChanged = tmp;
}

KABC::Address::List AddressEditDialog::addresses()
{
  saveAddress();

  return mAddressList;
}

void AddressEditDialog::saveAddress()
{
  KABC::Address::List::Iterator a = mTypeCombo->selectedElement();

  (*a).setLocality( mLocalityEdit->text() );
  (*a).setRegion( mRegionEdit->text() );
  (*a).setPostalCode( mPostalCodeEdit->text() );
  (*a).setCountry( mCountryCombo->currentText() );
  (*a).setPostOfficeBox( mPOBoxEdit->text() );
  (*a).setStreet( mStreetTextEdit->text() );

  if ( mPreferredCheckBox->isChecked() )
    (*a).setType( (*a).type() | KABC::Address::Pref );
  else
    (*a).setType( (*a).type() & ~KABC::Address::Pref );
}

void AddressEditDialog::addAddress()
{
  AddressTypeDialog dlg( mTypeCombo->selectedType(), this );
  if ( dlg.exec() ) {
    mAddressList.append( Address( dlg.type() ) );

    mTypeCombo->updateTypes();
    mTypeCombo->setCurrentItem( mTypeCombo->count() - 1 );
    updateAddressEdits();

    modified();
  }
  removeButton->setEnabled( true );
}

void AddressEditDialog::removeAddress()
{
    if ( mAddressList.count()>1 )
    {
        mAddressList.remove( mTypeCombo->selectedElement() );
        mTypeCombo->updateTypes();
        updateAddressEdits();
    }
    removeButton->setEnabled( mAddressList.count()>1 );
}

void AddressEditDialog::fillCountryCombo(KComboBox *combo)
{
  QString sCountry[] = {
    i18n( "Afghanistan" ), i18n( "Albania" ), i18n( "Algeria" ),
    i18n( "American Samoa" ), i18n( "Andorra" ), i18n( "Angola" ),
    i18n( "Anguilla" ), i18n( "Antarctica" ), i18n( "Antigua and Barbuda" ),
    i18n( "Argentina" ), i18n( "Armenia" ), i18n( "Aruba" ),
    i18n( "Ashmore and Cartier Islands" ), i18n( "Australia" ),
    i18n( "Austria" ), i18n( "Azerbaijan" ), i18n( "Bahamas" ),
    i18n( "Bahrain" ), i18n( "Bangladesh" ), i18n( "Barbados" ),
    i18n( "Belarus" ), i18n( "Belgium" ), i18n( "Belize" ),
    i18n( "Benin" ), i18n( "Bermuda" ), i18n( "Bhutan" ),
    i18n( "Bolivia" ), i18n( "Bosnia and Herzegovina" ), i18n( "Botswana" ),
    i18n( "Brazil" ), i18n( "Brunei" ), i18n( "Bulgaria" ),
    i18n( "Burkina Faso" ), i18n( "Burundi" ), i18n( "Cambodia" ),
    i18n( "Cameroon" ), i18n( "Canada" ), i18n( "Cape Verde" ),
    i18n( "Cayman Islands" ), i18n( "Central African Republic" ),
    i18n( "Chad" ), i18n( "Chile" ), i18n( "China" ), i18n( "Colombia" ),
    i18n( "Comoros" ), i18n( "Congo" ), i18n( "Congo, Dem. Rep." ),
    i18n( "Costa Rica" ), i18n( "Croatia" ),
    i18n( "Cuba" ), i18n( "Cyprus" ), i18n( "Czech Republic" ),
    i18n( "Denmark" ), i18n( "Djibouti" ),
    i18n( "Dominica" ), i18n( "Dominican Republic" ), i18n( "Ecuador" ),
    i18n( "Egypt" ), i18n( "El Salvador" ), i18n( "Equatorial Guinea" ),
    i18n( "Eritrea" ), i18n( "Estonia" ), i18n( "England" ),
    i18n( "Ethiopia" ), i18n( "European Union" ), i18n( "Faroe Islands" ),
    i18n( "Fiji" ), i18n( "Finland" ), i18n( "France" ),
    i18n( "French Polynesia" ), i18n( "Gabon" ), i18n( "Gambia" ),
    i18n( "Georgia" ), i18n( "Germany" ), i18n( "Ghana" ),
    i18n( "Greece" ), i18n( "Greenland" ), i18n( "Grenada" ),
    i18n( "Guam" ), i18n( "Guatemala" ), i18n( "Guinea" ),
    i18n( "Guinea-Bissau" ), i18n( "Guyana" ), i18n( "Haiti" ),
    i18n( "Honduras" ), i18n( "Hong Kong" ), i18n( "Hungary" ),
    i18n( "Iceland" ), i18n( "India" ), i18n( "Indonesia" ),
    i18n( "Iran" ), i18n( "Iraq" ), i18n( "Ireland" ),
    i18n( "Israel" ), i18n( "Italy" ), i18n( "Ivory Coast" ),
    i18n( "Jamaica" ), i18n( "Japan" ), i18n( "Jordan" ),
    i18n( "Kazakhstan" ), i18n( "Kenya" ), i18n( "Kiribati" ),
    i18n( "Korea, North" ), i18n( "Korea, South" ),
    i18n( "Kuwait" ), i18n( "Kyrgyzstan" ), i18n( "Laos" ),
    i18n( "Latvia" ), i18n( "Lebanon" ), i18n( "Lesotho" ),
    i18n( "Liberia" ), i18n( "Libya" ), i18n( "Liechtenstein" ),
    i18n( "Lithuania" ), i18n( "Luxembourg" ), i18n( "Macau" ),
    i18n( "Madagascar" ), i18n( "Malawi" ), i18n( "Malaysia" ),
    i18n( "Maldives" ), i18n( "Mali" ), i18n( "Malta" ),
    i18n( "Marshall Islands" ), i18n( "Martinique" ), i18n( "Mauritania" ),
    i18n( "Mauritius" ), i18n( "Mexico" ),
    i18n( "Micronesia, Federated States Of" ), i18n( "Moldova" ),
    i18n( "Monaco" ), i18n( "Mongolia" ), i18n( "Montserrat" ),
    i18n( "Morocco" ), i18n( "Mozambique" ), i18n( "Myanmar" ),
    i18n( "Namibia" ),
    i18n( "Nauru" ), i18n( "Nepal" ), i18n( "Netherlands" ),
    i18n( "Netherlands Antilles" ), i18n( "New Caledonia" ),
    i18n( "New Zealand" ), i18n( "Nicaragua" ), i18n( "Niger" ),
    i18n( "Nigeria" ), i18n( "Niue" ), i18n( "North Korea" ),
    i18n( "Northern Ireland" ), i18n( "Northern Mariana Islands" ),
    i18n( "Norway" ), i18n( "Oman" ), i18n( "Pakistan" ), i18n( "Palau" ),
    i18n( "Palestinian" ), i18n( "Panama" ), i18n( "Papua New Guinea" ),
    i18n( "Paraguay" ), i18n( "Peru" ), i18n( "Philippines" ),
    i18n( "Poland" ), i18n( "Portugal" ), i18n( "Puerto Rico" ),
    i18n( "Qatar" ), i18n( "Romania" ), i18n( "Russia" ), i18n( "Rwanda" ),
    i18n( "St. Kitts and Nevis" ), i18n( "St. Lucia" ),
    i18n( "St. Vincent and the Grenadines" ), i18n( "San Marino" ),
    i18n( "Sao Tome and Principe" ), i18n( "Saudi Arabia" ),
    i18n( "Senegal" ), i18n( "Serbia & Montenegro" ), i18n( "Seychelles" ),
    i18n( "Sierra Leone" ), i18n( "Singapore" ), i18n( "Slovakia" ),
    i18n( "Slovenia" ), i18n( "Solomon Islands" ), i18n( "Somalia" ),
    i18n( "South Africa" ), i18n( "South Korea" ), i18n( "Spain" ),
    i18n( "Sri Lanka" ), i18n( "St. Kitts and Nevis" ), i18n( "Sudan" ),
    i18n( "Suriname" ), i18n( "Swaziland" ), i18n( "Sweden" ),
    i18n( "Switzerland" ), i18n( "Syria" ), i18n( "Taiwan" ),
    i18n( "Tajikistan" ), i18n( "Tanzania" ), i18n( "Thailand" ),
    i18n( "Tibet" ), i18n( "Togo" ), i18n( "Tonga" ),
    i18n( "Trinidad and Tobago" ), i18n( "Tunisia" ), i18n( "Turkey" ),
    i18n( "Turkmenistan" ), i18n( "Turks and Caicos Islands" ),
    i18n( "Tuvalu" ), i18n( "Uganda " ), i18n( "Ukraine" ),
    i18n( "United Arab Emirates" ), i18n( "United Kingdom" ),
    i18n( "United States" ), i18n( "Uruguay" ), i18n( "Uzbekistan" ),
    i18n( "Vanuatu" ), i18n( "Vatican City" ), i18n( "Venezuela" ),
    i18n( "Vietnam" ), i18n( "Western Samoa" ), i18n( "Yemen" ),
    i18n( "Yugoslavia" ), i18n( "Zaire" ), i18n( "Zambia" ),
    i18n( "Zimbabwe" ),
    ""
  };
  
  QStringList countries;
  for (int i =0; sCountry[i] != ""; ++i )
    countries.append( sCountry[i] );

  countries.sort();
  
  combo->insertStringList( countries );
}

bool AddressEditDialog::changed() const
{
  return mChanged;
}

void AddressEditDialog::modified()
{
  mChanged = true;
}

AddressTypeDialog::AddressTypeDialog( int type, QWidget *parent )
  : KDialogBase( KDialogBase::Plain, i18n( "Edit Address Type" ),
                 KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok,
                 parent, "AddressTypeDialog" )
{
  QWidget *page = plainPage();
  QVBoxLayout *layout = new QVBoxLayout( page );

  mGroup = new QButtonGroup( 2, Horizontal, i18n( "Address Types" ), page );
  layout->addWidget( mGroup );

  mTypeList = KABC::Address::typeList();
  mTypeList.remove( KABC::Address::Pref );

  KABC::Address::TypeList::Iterator it;
  for ( it = mTypeList.begin(); it != mTypeList.end(); ++it )
    new QCheckBox( KABC::Address::typeLabel( *it ), mGroup );

  for ( int i = 0; i < mGroup->count(); ++i ) {
    QCheckBox *box = (QCheckBox*)mGroup->find( i );
    box->setChecked( type & mTypeList[ i ] );
  }
}

AddressTypeDialog::~AddressTypeDialog()
{
}

int AddressTypeDialog::type()
{
  int type = 0;
  for ( int i = 0; i < mGroup->count(); ++i ) {
    QCheckBox *box = (QCheckBox*)mGroup->find( i );
    if ( box->isChecked() )
      type += mTypeList[ i ];
  }

  return type;
}

#include "addresseditwidget.moc"
