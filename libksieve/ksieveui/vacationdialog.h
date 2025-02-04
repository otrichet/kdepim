/*  -*- c++ -*-
    vacationdialog.h

    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#ifndef KSIEVEUI_VACATIONDIALOG_H
#define KSIEVEUI_VACATIONDIALOG_H

#include <kdialog.h>

class KIntSpinBox;
class KLineEdit;
class KTextEdit;

class QString;
class QCheckBox;
template <typename T> class QList;

namespace KMime {
  namespace Types {
    struct AddrSpec;
    typedef QList<AddrSpec> AddrSpecList;
  }
}

namespace KSieveUi {

class VacationDialog : public KDialog
{
  Q_OBJECT

  public:
    explicit VacationDialog( const QString & caption, QWidget * parent=0,
                             const char * name=0, bool modal=true );
    virtual ~VacationDialog();

    virtual void enableDomainAndSendForSpam( bool enable = true );

    bool activateVacation() const;
    virtual void setActivateVacation( bool activate );

    bool domainCheck() const;
    virtual void setDomainCheck( bool check );

    QString messageText() const;
    virtual void setMessageText( const QString & text );

    int notificationInterval() const;
    virtual void setNotificationInterval( int days );

    KMime::Types::AddrSpecList mailAliases() const;
    virtual void setMailAliases( const KMime::Types::AddrSpecList & aliases );
    virtual void setMailAliases( const QString & aliases );

    QString domainName() const;
    virtual void setDomainName( const QString & domain );

    bool sendForSpam() const;
    virtual void setSendForSpam( bool enable );

  private slots:
    void slotIntervalSpinChanged( int value );

  protected:
    QCheckBox   * mActiveCheck;
    KIntSpinBox * mIntervalSpin;
    KLineEdit   * mMailAliasesEdit;
    KTextEdit   * mTextEdit;
    QCheckBox   * mSpamCheck;
    QCheckBox   * mDomainCheck;
    KLineEdit   * mDomainEdit;

};

}

#endif
