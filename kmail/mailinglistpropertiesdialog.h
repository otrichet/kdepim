/*******************************************************************************
**
** Filename   : mailinglistpropertiesdialog.h
** Created on : 30 January, 2005
** Copyright  : (c) 2005 Till Adam
** Email      : adam@kde.org
**
*******************************************************************************/

/*******************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   In addition, as a special exception, the copyright holders give
**   permission to link the code of this program with any edition of
**   the Qt library by Trolltech AS, Norway (or with modified versions
**   of Qt that use the same license as Qt), and distribute linked
**   combinations including the two.  You must obey the GNU General
**   Public License in all respects for all of the code used other than
**   Qt.  If you modify this file, you may extend this exception to
**   your version of the file, but you are not obligated to do so.  If
**   you do not wish to do so, delete this exception statement from
**   your version.
*******************************************************************************/

#ifndef MAILINGLISTPROPERTIESDIALOG_H
#define MAILINGLISTPROPERTIESDIALOG_H

#include "messagecore/mailinglist.h"
#include <kdeversion.h>
#include <kdialog.h>
#include "foldercollection.h"
class QCheckBox;
class QPushButton;
class QLabel;
class KComboBox;
class KEditListBox;
class KEditListWidget;

namespace KMail
{

class MailingListFolderPropertiesDialog : public KDialog
{
  Q_OBJECT
public:
  MailingListFolderPropertiesDialog( QWidget *parent, const QSharedPointer<MailCommon::FolderCollection>& folder );
  ~MailingListFolderPropertiesDialog() {}

protected:
  void load();
  void save();

protected slots:
  void slotOk();
  void slotFetchDone( KJob* job );

private slots:
  /*
   * Detects mailing-list related stuff
   */
  void slotDetectMailingList();
  void slotInvokeHandler();
  void slotMLHandling( int element );
  void slotHoldsML( bool holdsML );
  void slotAddressChanged( int addr );

private:
  QSharedPointer<MailCommon::FolderCollection> mFolder;
  void fillMLFromWidgets();
  void fillEditBox();

  bool          mMLInfoChanged;
  QCheckBox    *mHoldsMailingList;
  KComboBox    *mMLHandlerCombo;
  QPushButton  *mDetectButton;
  KComboBox    *mAddressCombo;
  int           mLastItem;
#if KDE_IS_VERSION( 4, 5, 74 )
  KEditListWidget *mEditList;
#else
  KEditListBox *mEditList;
#endif
  QLabel       *mMLId;
  MailingList   mMailingList;
}; // End of class MailingListFolderProperties

} // End of namespace KMail

#endif // MAILINGLISTPROPERTIESDIALOG_H
