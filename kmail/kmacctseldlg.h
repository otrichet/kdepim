/*
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, <espen@kde.org>
 *   Contains code segments and ideas from earlier kmail dialog code
 *   by Stefan Taferner <taferner@alpin.or.at>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

// Select account from given list of account types

#ifndef kmacctseldlg_h
#define kmacctseldlg_h

#include <kdialogbase.h>

class KMAcctSelDlg: public KDialogBase
{
  Q_OBJECT

  public:
    KMAcctSelDlg( QWidget *parent=0, const char *name=0, bool modal=true );

    /** 
     * Returns selected button from the account selection group:
     * 0=local mail, 1=pop3. 
     */
    int selected(void) const;

  private slots:
    void buttonClicked(int);

  private:
    int mSelectedButton;
};


#endif /*kmacctseldlg_h*/
