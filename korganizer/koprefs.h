/*
  This file is part of KOrganizer.

  Copyright (c) 2000,2001 Cornelius Schumacher <schumacher@kde.org>

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
#ifndef KOPREFS_H
#define KOPREFS_H

#include "korganizer_export.h"
#include "koprefs_base.h"

#include <calendarviews/eventviews/prefs.h>

#include <KDateTime>

#include <QHash>

#include <boost/shared_ptr.hpp>

class QFont;
class QColor;
class QStringList;

class KORGANIZER_CORE_EXPORT KOPrefs : public KOPrefsBase
{
  public:
    virtual ~KOPrefs();

    /** Get instance of KOPrefs. It is made sure that there is only one
    instance. */
    static KOPrefs *instance();

    EventViews::PrefsPtr eventViewsPreferences() const;

    /** Set preferences to default values */
    void usrSetDefaults();

    /** Read preferences from config file */
    void usrReadConfig();

    /** Write preferences to config file */
    void usrWriteConfig();

  private:
    /** Constructor disabled for public. Use instance() to create a KOPrefs
    object. */
    KOPrefs();
    friend class KOPrefsPrivate;

  public:
    void setResourceColor ( const QString &, const QColor & );
    QColor resourceColor( const QString & );

    void setHtmlExportFile( const QString &fileName );
    QString htmlExportFile() const;

    QStringList timeScaleTimezones() const;
    void setTimeScaleTimezones( const QStringList &list );

  private:
    QHash<QString,QColor> mCategoryColors;
    QColor mDefaultCategoryColor;

    QHash<QString,QColor> mResourceColors;
    QColor mDefaultResourceColor;

    QFont mDefaultMonthViewFont;

    QStringList mTimeScaleTimeZones;

    QString mHtmlExportFile;

    EventViews::PrefsPtr mEventViewsPrefs;

  public: // Do not use - except in KOPrefsDialogMain
    QString mName;
    QString mEmail;
};

#endif
