/*
  This file is part of KOrganizer.

  Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/
#ifndef KOJOURNALVIEW_H
#define KOJOURNALVIEW_H

#include <korganizer/baseview.h>
#include "journalview.h"

#include <Akonadi/Item>

class QScrollArea;
class KVBox;

/**
 * This class provides a journal view.

 * @short View for Journal components.
 * @author Cornelius Schumacher <schumacher@kde.org>, Reinhold Kainhofer <reinhold@kainhofer.com>
 * @see KOBaseView
 */
class KOJournalView : public KOrg::BaseView
{
  Q_OBJECT
  public:
    explicit KOJournalView( QWidget *parent = 0 );
    ~KOJournalView();

    virtual int currentDateCount() const;
    virtual Akonadi::Item::List selectedIncidences();
    DateList selectedIncidenceDates() { return DateList(); }
    void appendJournal( const Akonadi::Item &journal, const QDate &dt );

    /** documentation in baseview.h */
    void getHighlightMode( bool &highlightEvents,
                           bool &highlightTodos,
                           bool &highlightJournals );

    bool eventFilter ( QObject *, QEvent * );
    virtual KOrg::CalPrinterBase::PrintType printType() const;

  public slots:
    // Don't update the view when midnight passed, otherwise we'll have data loss (bug 79145)
    virtual void dayPassed( const QDate & ) {}
    void updateView();
    void flushView();

    void showDates( const QDate &start, const QDate &end );
    void showIncidences( const Akonadi::Item::List &incidences, const QDate &date );

    void changeIncidenceDisplay( const Akonadi::Item &incidence, int );
    void setIncidenceChanger( CalendarSupport::IncidenceChanger *changer );
    void newJournal();
  signals:
    void flushEntries();
    void setIncidenceChangerSignal( CalendarSupport::IncidenceChanger * );
    void journalEdited( const Akonadi::Item &journal );
    void journalDeleted( const Akonadi::Item &journal );

  protected:
    void clearEntries();

  private:
    QScrollArea *mSA;
    KVBox *mVBox;
    QMap<QDate, JournalDateView*> mEntries;
//    DateList mSelectedDates;  // List of dates to be displayed
};

#endif
