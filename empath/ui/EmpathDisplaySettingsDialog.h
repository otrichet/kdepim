/*
    Empath - Mailer for KDE
    
    Copyright 1999, 2000
        Rik Hemsley <rik@kde.org>
        Wilco Greven <j.w.greven@student.utwente.nl>
    
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
*/

#ifdef __GNUG__
# pragma interface "EmpathDisplaySettingsDialog.h"
#endif

#ifndef EMPATHDISPLAYSETTINGSDIALOG_H
#define EMPATHDISPLAYSETTINGSDIALOG_H

// Qt includes
#include <qwidget.h>
#include <qlabel.h>
#include <qbuttongroup.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qcombobox.h>
#include <qspinbox.h>

// KDE includes
#include <kdialog.h>
#include <kcolorbtn.h>
#include <kbuttonbox.h>

// Local includes
#include "EmpathDefines.h"

/**
 * Configure the appearance of various stuff.
 */
class EmpathDisplaySettingsDialog : public KDialog
{
    Q_OBJECT

    public:
        
        EmpathDisplaySettingsDialog(QWidget * = 0);
        ~EmpathDisplaySettingsDialog();

        void saveData();
        void loadData();

    protected slots:
        
        void s_chooseFixedFont();

        void s_OK();
        void s_cancel();
        void s_help();
        void s_default();
        void s_apply();

    private:

        QPushButton     * pb_chooseFixedFont_;
        
        KColorButton    * kcb_quoteColorTwo_;
        KColorButton    * kcb_quoteColorOne_;
        KColorButton    * kcb_linkColor_;
        KColorButton    * kcb_newMessageColor_;
        
        QCheckBox       * cb_underlineLinks_;
        
        QCheckBox       * cb_threadMessages_;
        
        QLineEdit       * le_displayHeaders_;

        KButtonBox      * buttonBox_;
        QPushButton     * pb_help_;
        QPushButton     * pb_default_;
        QPushButton     * pb_apply_;
        QPushButton     * pb_OK_;
        QPushButton     * pb_cancel_;
        
        QCheckBox       * cb_timer_;
        QSpinBox        * sb_timer_;
        
        bool            applied_;
};

#endif
// vim:ts=4:sw=4:tw=78
