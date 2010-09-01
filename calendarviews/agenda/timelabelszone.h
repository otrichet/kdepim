/*
  This file is part of KOrganizer.

  Copyright (c) 2007 Bruno Virlet <bruno@virlet.org>

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
#ifndef TIMELABELSZONE_H
#define TIMELABELSZONE_H

#include "timelabels.h"

#include <KDateTime>

#include <QWidget>
#include <QList>

class QHBoxLayout;
class QScrollArea;

namespace EventViews
{

class AgendaView;
class Agenda;

class TimeLabelsZone : public QWidget
{
  Q_OBJECT
  public:
    explicit TimeLabelsZone( QWidget *parent, const PrefsPtr &preferences, Agenda *agenda = 0 );

    /** Add a new time label with the given spec.
        If spec is not valid, use the display timespec.
    */
    void addTimeLabels( const KDateTime::Spec &spec );

    /**
       Returns the best width for each TimeLabels widget
    */
    int preferedTimeLabelsWidth() const;

    void updateAll();
    void reset();
    void init();
    void setAgendaView( AgendaView *agenda );

    QList<QScrollArea*> timeLabels() const;

  private:
    void setupTimeLabel( QScrollArea *area );
    Agenda *mAgenda;
    PrefsPtr mPrefs;
    AgendaView *mParent;

    QHBoxLayout *mTimeLabelsLayout;
    QList<QScrollArea*> mTimeLabelsList;
};

} // namespace EventViews

#endif
