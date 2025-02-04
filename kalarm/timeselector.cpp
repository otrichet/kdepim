/*
 *  timeselector.cpp  -  widget to optionally set a time period
 *  Program:  kalarm
 *  Copyright © 2004,2005,2007,2009,2010 by David Jarvie <djarvie@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kalarm.h"

#include "checkbox.h"
#include "timeselector.moc"

#include <klocale.h>
#include <kdialog.h>
#include <khbox.h>
#include <kdebug.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

#ifdef USE_AKONADI
using namespace KCalCore;
#else
using namespace KCal;
#endif


TimeSelector::TimeSelector(const QString& selectText, const QString& postfix, const QString& selectWhatsThis,
                           const QString& valueWhatsThis, bool allowHourMinute, QWidget* parent)
    : QFrame(parent),
      mLabel(0),
      mReadOnly(false)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(KDialog::spacingHint());
    mSelect = new CheckBox(selectText, this);
    mSelect->setFixedSize(mSelect->sizeHint());
    connect(mSelect, SIGNAL(toggled(bool)), SLOT(selectToggled(bool)));
    mSelect->setWhatsThis(selectWhatsThis);
    layout->addWidget(mSelect);

    KHBox* box = new KHBox(this);    // to group widgets for QWhatsThis text
    box->setSpacing(KDialog::spacingHint());
    layout->addWidget(box);
    mPeriod = new TimePeriod(allowHourMinute, box);
    mPeriod->setFixedSize(mPeriod->sizeHint());
    mPeriod->setSelectOnStep(false);
#ifdef USE_AKONADI
    connect(mPeriod, SIGNAL(valueChanged(const KCalCore::Duration&)), SLOT(periodChanged(const KCalCore::Duration&)));
#else
    connect(mPeriod, SIGNAL(valueChanged(const KCal::Duration&)), SLOT(periodChanged(const KCal::Duration&)));
#endif
    mSelect->setFocusWidget(mPeriod);
    mPeriod->setEnabled(false);

    if (!postfix.isEmpty())
    {
        mLabel = new QLabel(postfix, box);
        mLabel->setEnabled(false);
    }
    box->setWhatsThis(valueWhatsThis);
    layout->addStretch();
}

/******************************************************************************
*  Set the read-only status.
*/
void TimeSelector::setReadOnly(bool ro)
{
    if ((int)ro != (int)mReadOnly)
    {
        mReadOnly = ro;
        mSelect->setReadOnly(mReadOnly);
        mPeriod->setReadOnly(mReadOnly);
    }
}

bool TimeSelector::isChecked() const
{
    return mSelect->isChecked();
}

void TimeSelector::setChecked(bool on)
{
    if (on != mSelect->isChecked())
    {
        mSelect->setChecked(on);
        emit valueChanged(period());
    }
}

void TimeSelector::setMaximum(int hourmin, int days)
{
    mPeriod->setMaximum(hourmin, days);
}

void TimeSelector::setDateOnly(bool dateOnly)
{
    mPeriod->setDateOnly(dateOnly);
}

/******************************************************************************
 * Get the specified number of minutes.
 * Reply = 0 if unselected.
 */
Duration TimeSelector::period() const
{
    return mSelect->isChecked() ? mPeriod->period() : Duration(0);
}

/******************************************************************************
*  Initialise the controls with a specified time period.
*  If minutes = 0, it will be deselected.
*  The time unit combo-box is initialised to 'defaultUnits', but if 'dateOnly'
*  is true, it will never be initialised to hours/minutes.
*/
void TimeSelector::setPeriod(const Duration& period, bool dateOnly, TimePeriod::Units defaultUnits)
{
    mSelect->setChecked(period);
    mPeriod->setEnabled(period);
    if (mLabel)
        mLabel->setEnabled(period);
    mPeriod->setPeriod(period, dateOnly, defaultUnits);
}

/******************************************************************************
*  Set the input focus on the count field.
*/
void TimeSelector::setFocusOnCount()
{
    mPeriod->setFocusOnCount();
}

/******************************************************************************
*  Called when the TimeSelector checkbox is toggled.
*/
void TimeSelector::selectToggled(bool on)
{
    mPeriod->setEnabled(on);
    if (mLabel)
        mLabel->setEnabled(on);
    if (on)
        mPeriod->setFocus();
    emit toggled(on);
    emit valueChanged(period());
}

/******************************************************************************
*  Called when the period value changes.
*/
void TimeSelector::periodChanged(const Duration& period)
{
    if (mSelect->isChecked())
        emit valueChanged(period);
}

// vim: et sw=4:
