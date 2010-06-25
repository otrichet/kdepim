/*
    KNode, the KDE newsreader
    Copyright (c) 1999-2005 the KNode authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#include "knstringfilter.h"

#include "knglobals.h"
#include "settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <kconfig.h>
#include <kdialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <KPIMIdentities/Identity>


using namespace KNode;

StringFilter& KNode::StringFilter::operator=( const StringFilter &sf )
{
#if 0
  con=sf.con;
  data=sf.data;
  regExp=sf.regExp;

  return (*this);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



bool KNode::StringFilter::doFilter( const QString &s )
{
#if 0
  bool ret=true;

  if(!expanded.isEmpty()) {
    if(regExp) {
      QRegExp matcher(expanded);
      ret = ( matcher.indexIn(s) >= 0 );
    } else
      ret = ( s.indexOf( expanded, 0, Qt::CaseInsensitive ) != -1 );

    if(!con) ret=!ret;
  }

  return ret;
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



// replace placeholders
void KNode::StringFilter::expand( KNGroup *g )
{
#if 0
  KPIMIdentities::Identity id;
  if ( g ) {
    if ( !g->identity().isNull() ) {
      id = g->identity();
    } else if ( !g->account()->identity().isNull() ) {
      id = g->account()->identity();
    }
  }
  if ( id.isNull() ) {
    id = KNGlobals::self()->settings()->identity();
  }

  expanded = data;
  expanded.replace( QRegExp("%MYNAME"), id.fullName() );
  expanded.replace( QRegExp("%MYEMAIL"), id.primaryEmailAddress() );
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



void KNode::StringFilter::load( const KConfigGroup &group )
{
#if 0
  con=group.readEntry("contains", true);
  data=group.readEntry("Data");
  regExp=group.readEntry("regX", false);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



void KNode::StringFilter::save( KConfigGroup &group )
{
#if 0
  group.writeEntry("contains", con);
  group.writeEntry("Data", data);
  group.writeEntry("regX", regExp);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


//===============================================================================

KNode::StringFilterWidget::StringFilterWidget( const QString& title, QWidget *parent )
  : QGroupBox( title, parent )
{
#if 0
  fType=new QComboBox(this);
  fType->addItem(i18n("Does Contain"));
  fType->addItem(i18n("Does NOT Contain"));

  fString=new KLineEdit(this);

  regExp=new QCheckBox(i18n("Regular expression"), this);

  QGridLayout *topL = new QGridLayout( this );
  topL->setSpacing( KDialog::spacingHint() );
  topL->addWidget( fType, 0, 0 );
  topL->addWidget( regExp, 0, 1 );
  topL->addWidget( fString, 1, 0, 1, 2 );
  topL->setColumnStretch( 2, 1 );
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



KNode::StringFilterWidget::~StringFilterWidget()
{
#if 0
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



KNode::StringFilter StringFilterWidget::filter()
{
#if 0
  StringFilter ret;
  ret.con=(fType->currentIndex()==0);
  ret.data=fString->text();
  ret.regExp=regExp->isChecked();

  return ret;
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



void KNode::StringFilterWidget::setFilter( StringFilter &f )
{
#if 0
  if(f.con) fType->setCurrentIndex(0);
  else fType->setCurrentIndex(1);
  fString->setText(f.data);
  regExp->setChecked(f.regExp);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



void KNode::StringFilterWidget::clear()
{
#if 0
  fString->clear();
  fType->setCurrentIndex(0);
  regExp->setChecked(false);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNode::StringFilterWidget::setStartFocus()
{
#if 0
  fString->setFocus();
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


// -----------------------------------------------------------------------------+

#include "knstringfilter.moc"
