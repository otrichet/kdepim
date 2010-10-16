/*
    KNode, the KDE newsreader
    Copyright (c) 1999-2006 the KNode authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#include "kngroupselectdialog.h"

#include "utilities.h"

#include <q3header.h>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <klocale.h>
#include <kmessagebox.h>
#include <QPushButton>


KNGroupSelectDialog::KNGroupSelectDialog( QWidget *parent, KNNntpAccount::Ptr a, const QStringList &groups ) :
  KNGroupBrowser(parent, i18n("Select Destinations"), a)
{
#if 0
  selView=new Q3ListView(page);
  selView->addColumn( QString() );
  selView->header()->hide();
  listL->addWidget(selView, 1,2);
  rightLabel->setText(i18n("Groups for this article:"));
  subCB->setChecked(true);

  KNGroupInfo info;
  foreach ( const QString &group, groups ) {
    info.name = group;
    new GroupItem(selView, info);
  }

  connect(selView, SIGNAL(selectionChanged(Q3ListViewItem*)),
    this, SLOT(slotItemSelected(Q3ListViewItem*)));
  connect(groupView, SIGNAL(selectionChanged(Q3ListViewItem*)),
    this, SLOT(slotItemSelected(Q3ListViewItem*)));
  connect(groupView, SIGNAL(selectionChanged()),
    this, SLOT(slotSelectionChanged()));
  connect(arrowBtn1, SIGNAL(clicked()), this, SLOT(slotArrowBtn1()));
  connect(arrowBtn2, SIGNAL(clicked()), this, SLOT(slotArrowBtn2()));

  KNHelper::restoreWindowSize("groupSelDlg", this, QSize(659,364));  // optimized for 800x600
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



KNGroupSelectDialog::~KNGroupSelectDialog()
{
#if 0
  KNHelper::saveWindowSize("groupSelDlg", this->size());
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



void KNGroupSelectDialog::itemChangedState(CheckItem *it, bool s)
{
#if 0
  if(s)
    new GroupItem(selView, it->info);
  else
    removeListItem(selView, it->info);
  arrowBtn1->setEnabled(!s);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



void KNGroupSelectDialog::updateItemState(CheckItem *it)
{
#if 0
  it->setChecked(itemInListView(selView, it->info));
  if(it->info.subscribed && it->pixmap(0)==0)
    it->setPixmap(0, pmGroup);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



QString KNGroupSelectDialog::selectedGroups()const
{
#if 0
  QString ret;
  Q3ListViewItemIterator it(selView);
  bool moderated=false;
  int count=0;
  bool isFirst=true;

  for(; it.current(); ++it) {
    if(!isFirst)
      ret+=',';
    ret+=(static_cast<GroupItem*>(it.current()))->info.name;
    isFirst=false;
    count++;
    if ((static_cast<GroupItem*>(it.current()))->info.status == KNGroup::moderated)
      moderated=true;
  }

  if (moderated && (count>=2))   // warn the user
     KMessageBox::information(parentWidget(),i18n("You are crossposting to a moderated newsgroup.\nPlease be aware that your article will not appear in any group\nuntil it has been approved by the moderators of the moderated group."),
                              QString(),"crosspostModeratedWarning");

  return ret;
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
  return QString();
#endif
}



void KNGroupSelectDialog::slotItemSelected(Q3ListViewItem *it)
{
#if 0
  const QObject *s=sender();

  if(s==groupView) {
    selView->clearSelection();
    arrowBtn2->setEnabled(false);
    if(it)
      arrowBtn1->setEnabled(!(static_cast<CheckItem*>(it))->isOn());
    else
      arrowBtn1->setEnabled(false);
  }
  else {
    groupView->clearSelection();
    arrowBtn1->setEnabled(false);
    arrowBtn2->setEnabled((it!=0));
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



void KNGroupSelectDialog::slotSelectionChanged()
{
#if 0
  if (!groupView->selectedItem())
    arrowBtn1->setEnabled(false);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



void KNGroupSelectDialog::slotArrowBtn1()
{
#if 0
  CheckItem *i=static_cast<CheckItem*>(groupView->selectedItem());

  if(i) {
    new GroupItem(selView, i->info);
    arrowBtn1->setEnabled(false);
    i->setChecked(true);
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}



void KNGroupSelectDialog::slotArrowBtn2()
{
#if 0
  GroupItem *i=static_cast<GroupItem*>(selView->selectedItem());

  if(i) {
    changeItemState(i->info, false);
    delete i;
    arrowBtn2->setEnabled(false);
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


// -----------------------------------------------------------------------------

#include "kngroupselectdialog.moc"

