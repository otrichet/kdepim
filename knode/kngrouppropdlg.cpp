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

#include "kngrouppropdlg.h"


#include "akobackit/akonadi_manager.h"
#include "akobackit/group_manager.h"
#include "configuration/identity_widget.h"
#include "knconfigmanager.h"
#include "knconfigwidgets.h"
#include "knglobals.h"
#include "utilities.h"
#include "utils/locale.h"

#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QComboBox>

#include <KCharsets>
#include <klocale.h>
#include <klineedit.h>
#include <kvbox.h>

using namespace KNode;


KNGroupPropDlg::KNGroupPropDlg( Group::Ptr group, QWidget *parent )
  : KPageDialog( parent ),
    g_rp( group )
{
  setFaceType( Tabbed );
  setCaption( i18nc( "@title:window %1=newsgroup name", "Properties of %1", group->groupName() ) );
  setButtons( Ok|Cancel|Help );
  setDefaultButton( Ok );

  // General tab ===============================================

  QWidget *page = new QWidget( this );
  addPage( page, i18nc( "@title:tab", "General" ) );
  QVBoxLayout *pageL = new QVBoxLayout(page);
  pageL->setSpacing(3);

  // settings
  QGroupBox *gb = new QGroupBox( i18nc( "@title:group", "Settings"), page );
  pageL->addWidget(gb);
  QGridLayout *grpL=new QGridLayout(gb);
  grpL->setSpacing(5);
  grpL->setMargin(15);

  grpL->addItem( new QSpacerItem( 0, fontMetrics().lineSpacing()-9), 0, 0 );

  n_ick=new KLineEdit(gb);
  n_ick->setText( g_rp->displayName() );
  QLabel *l = new QLabel( i18nc( "@label:textbox Alternative name of a newsgroup", "Nickname:" ), gb );
  l->setBuddy(n_ick);
  grpL->addWidget(l,1,0);
  grpL->addWidget(n_ick,1,1, 1,2);

  u_seCharset = new QCheckBox( i18nc( "@label:listbox", "Use different default charset:" ), gb );
  u_seCharset->setChecked(g_rp->useCharset());
  grpL->addWidget(u_seCharset,2,0, 1, 2 );

  c_harset=new QComboBox(gb);
  c_harset->setEditable(false);
  c_harset->addItems( KNode::Utilities::Locale::encodings() );
  QString defaultCsDesc = KGlobal::charsets()->descriptionForEncoding( QString::fromAscii( g_rp->defaultCharset() ) );
  c_harset->setCurrentIndex( c_harset->findText( defaultCsDesc ) );
  c_harset->setEnabled(g_rp->useCharset());
  connect(u_seCharset, SIGNAL(toggled(bool)), c_harset, SLOT(setEnabled(bool)));

  grpL->addWidget(c_harset, 2,2);
  grpL->setColumnStretch(1,1);
  grpL->setColumnStretch(2,2);

  // group name & description
  gb = new QGroupBox( i18nc( "@title:group", "Description" ), page );
  pageL->addWidget(gb);
  grpL=new QGridLayout(gb);
  grpL->setSpacing(5);
  grpL->setMargin(15);

  grpL->addItem( new QSpacerItem( 0, fontMetrics().lineSpacing()-9), 0, 0 );

  l = new QLabel( i18nc( "@label name of a newsgroup", "Name:" ), gb );
  grpL->addWidget(l,1,0);
  l = new QLabel( group->groupName(), gb );
  grpL->addWidget(l,1,2);

  l = new QLabel( i18nc( "@label description of a newsgroup", "Description:" ), gb );
  grpL->addWidget(l,2,0);
  l=new QLabel(g_rp->description(),gb);
  grpL->addWidget(l,2,2);

  l = new QLabel( i18nc( "@label status of posting to a newsgroup", "Status:" ), gb );
  grpL->addWidget(l,3,0);
  QString status;
  switch ( g_rp->postingStatus() ) {
    case Group::Unknown:
      status = i18nc( "posting status", "unknown" );
      break;
    case Group::ReadOnly:
      status = i18nc( "posting status", "posting forbidden" );
      break;
    case Group::PostingAllowed:
      status = i18nc( "posting status", "posting allowed" );
      break;
    case Group::Moderated:
      status = i18nc( "posting status", "moderated" );
      break;
  }
  l=new QLabel(status,gb);
  grpL->addWidget(l,3,2);

  grpL->addItem( new QSpacerItem(20, 0 ), 0, 1 );
  grpL->setColumnStretch(2,1);

#if 0
  // statistics
  gb = new QGroupBox( i18nc( "@title:group", "Statistics" ), page );
  pageL->addWidget(gb);
  grpL=new QGridLayout(gb);
  grpL->setSpacing(5);
  grpL->setMargin(15);

  grpL->addItem( new QSpacerItem( 0, fontMetrics().lineSpacing()-9), 0, 0 );

  l=new QLabel(i18n("Articles:"), gb);
  grpL->addWidget(l,1,0);
  l=new QLabel(QString::number(g_rp->count()),gb);
  grpL->addWidget(l,1,2);

  l=new QLabel(i18n("Unread articles:"), gb);
  grpL->addWidget(l,2,0);
  l=new QLabel(QString::number(g_rp->count()-g_rp->readCount()),gb);
  grpL->addWidget(l,2,2);

  l=new QLabel(i18n("New articles:"), gb);
  grpL->addWidget(l,3,0);
  l=new QLabel(QString::number(g_rp->newCount()),gb);
  grpL->addWidget(l,3,2);

  l=new QLabel(i18n("Threads with unread articles:"), gb);
  grpL->addWidget(l,4,0);
  l=new QLabel(QString::number(g_rp->statThrWithUnread()),gb);
  grpL->addWidget(l,4,2);

  l=new QLabel(i18n("Threads with new articles:"), gb);
  grpL->addWidget(l,5,0);
  l=new QLabel(QString::number(g_rp->statThrWithNew()),gb);
  grpL->addWidget(l,5,2);

  grpL->addItem( new QSpacerItem(20, 0 ), 0, 1 );
  grpL->setColumnStretch(2,1);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
  pageL->addStretch(1);

  // Specific Identity tab =========================================
  i_dWidget = new KNode::IdentityWidget( g_rp.get(), knGlobals.componentData(), this );
  addPage( i_dWidget, i18nc( "@title:tab", "Identity" ) );

#if 0
  // per server cleanup configuration
  mCleanupWidget = new KNode::GroupCleanupWidget( g_rp->cleanupConfig(), this );
  addPage( mCleanupWidget, i18nc( "@title:tab", "Cleanup" ) );
  mCleanupWidget->load();
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif

  KNHelper::restoreWindowSize("groupPropDLG", this, sizeHint());
  connect(this,SIGNAL(okClicked()),SLOT(slotOk()));
}



KNGroupPropDlg::~KNGroupPropDlg()
{
  KNHelper::saveWindowSize("groupPropDLG", size());
}



void KNGroupPropDlg::slotOk()
{
  if( g_rp->name() != n_ick->text() ) {
    g_rp->setDisplayName( n_ick->text() );
  } else {
    g_rp->setDisplayName( QString() );
  }
  g_rp->setUseCharset(u_seCharset->isChecked());
  g_rp->setDefaultCharset( KGlobal::charsets()->encodingForName( c_harset->currentText() ).toLatin1() );


  i_dWidget->save();
#if 0
  mCleanupWidget->save();
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
  Akobackit::manager()->groupManager()->saveGroup( g_rp );

  accept();
}
