/*
 *  kaevent.cpp  -  represents calendar events
 *  Program:  kalarm
 *  Copyright © 2001-2011 by David Jarvie <djarvie@kde.org>
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

#include "kaevent.h"

#ifndef USE_AKONADI
#include "alarmresource.h"
#endif
#include "alarmtext.h"
#include "identities.h"
#include "version.h"

#ifdef USE_AKONADI
#include <kcalcore/memorycalendar.h>
#else
#include <kcal/calendarlocal.h>
#endif
#include <kholidays/holidays.h>
using namespace KHolidays;

#include <ksystemtimezone.h>
#include <klocale.h>
#ifndef USE_AKONADI
#include <kconfiggroup.h>
#endif
#include <kdebug.h>

#ifdef USE_AKONADI
using namespace KCalCore;
#else
using namespace KCal;
#endif
using namespace KHolidays;

// KAlarm version which first used the current calendar/event format.
// If this changes, KAEvent::convertKCalEvents() must be changed correspondingly.
// The string version is the KAlarm version string used in the calendar file.
QByteArray KAEvent::currentCalendarVersionString()  { return QByteArray("2.2.9"); }
int        KAEvent::currentCalendarVersion()        { return KAlarm::Version(2,2,9); }

// Custom calendar properties.
// Note that all custom property names are prefixed with X-KDE-KALARM- in the calendar file.

// Event properties
const QByteArray KAEvent::Private::FLAGS_PROPERTY("FLAGS");              // X-KDE-KALARM-FLAGS property
const QString    KAEvent::Private::DATE_ONLY_FLAG        = QLatin1String("DATE");
const QString    KAEvent::Private::EMAIL_BCC_FLAG        = QLatin1String("BCC");
const QString    KAEvent::Private::CONFIRM_ACK_FLAG      = QLatin1String("ACKCONF");
const QString    KAEvent::Private::KORGANIZER_FLAG       = QLatin1String("KORG");
const QString    KAEvent::Private::EXCLUDE_HOLIDAYS_FLAG = QLatin1String("EXHOLIDAYS");
const QString    KAEvent::Private::WORK_TIME_ONLY_FLAG   = QLatin1String("WORKTIME");
const QString    KAEvent::Private::DEFER_FLAG            = QLatin1String("DEFER");   // default defer interval for this alarm
const QString    KAEvent::Private::LATE_CANCEL_FLAG      = QLatin1String("LATECANCEL");
const QString    KAEvent::Private::AUTO_CLOSE_FLAG       = QLatin1String("LATECLOSE");
const QString    KAEvent::Private::TEMPL_AFTER_TIME_FLAG = QLatin1String("TMPLAFTTIME");
const QString    KAEvent::Private::KMAIL_SERNUM_FLAG     = QLatin1String("KMAIL");

const QByteArray KAEvent::Private::NEXT_RECUR_PROPERTY("NEXTRECUR");     // X-KDE-KALARM-NEXTRECUR property
const QByteArray KAEvent::Private::REPEAT_PROPERTY("REPEAT");            // X-KDE-KALARM-REPEAT property
const QByteArray KAEvent::Private::ARCHIVE_PROPERTY("ARCHIVE");          // X-KDE-KALARM-ARCHIVE property
const QString    KAEvent::Private::ARCHIVE_REMINDER_ONCE_TYPE = QLatin1String("ONCE");
const QByteArray KAEvent::Private::LOG_PROPERTY("LOG");                  // X-KDE-KALARM-LOG property
const QString    KAEvent::Private::xtermURL = QLatin1String("xterm:");
const QString    KAEvent::Private::displayURL = QLatin1String("display:");

// - General alarm properties
const QByteArray KAEvent::Private::TYPE_PROPERTY("TYPE");                // X-KDE-KALARM-TYPE property
const QString    KAEvent::Private::FILE_TYPE                  = QLatin1String("FILE");
const QString    KAEvent::Private::AT_LOGIN_TYPE              = QLatin1String("LOGIN");
const QString    KAEvent::Private::REMINDER_TYPE              = QLatin1String("REMINDER");
const QString    KAEvent::Private::REMINDER_ONCE_TYPE         = QLatin1String("REMINDER_ONCE");
const QString    KAEvent::Private::TIME_DEFERRAL_TYPE         = QLatin1String("DEFERRAL");
const QString    KAEvent::Private::DATE_DEFERRAL_TYPE         = QLatin1String("DATE_DEFERRAL");
const QString    KAEvent::Private::DISPLAYING_TYPE            = QLatin1String("DISPLAYING");   // used only in displaying calendar
const QString    KAEvent::Private::PRE_ACTION_TYPE            = QLatin1String("PRE");
const QString    KAEvent::Private::POST_ACTION_TYPE           = QLatin1String("POST");
const QString    KAEvent::Private::SOUND_REPEAT_TYPE          = QLatin1String("SOUNDREPEAT");
const QByteArray KAEvent::Private::NEXT_REPEAT_PROPERTY("NEXTREPEAT");   // X-KDE-KALARM-NEXTREPEAT property
// - Display alarm properties
const QByteArray KAEvent::Private::FONT_COLOUR_PROPERTY("FONTCOLOR");    // X-KDE-KALARM-FONTCOLOR property
// - Email alarm properties
const QByteArray KAEvent::Private::EMAIL_ID_PROPERTY("EMAILID");         // X-KDE-KALARM-EMAILID property
// - Audio alarm properties
const QByteArray KAEvent::Private::VOLUME_PROPERTY("VOLUME");            // X-KDE-KALARM-VOLUME property
const QByteArray KAEvent::Private::SPEAK_PROPERTY("SPEAK");              // X-KDE-KALARM-SPEAK property
// - Command alarm properties
const QByteArray KAEvent::Private::CANCEL_ON_ERROR_PROPERTY("ERRCANCEL");// X-KDE-KALARM-ERRCANCEL property
const QByteArray KAEvent::Private::DONT_SHOW_ERROR_PROPERTY("ERRNOSHOW");// X-KDE-KALARM-ERRNOSHOW property

// Event status strings
const QString    KAEvent::Private::DISABLED_STATUS            = QLatin1String("DISABLED");

// Displaying event ID identifier
const QString    KAEvent::Private::DISP_DEFER = QLatin1String("DEFER");
const QString    KAEvent::Private::DISP_EDIT  = QLatin1String("EDIT");

// Command error strings
#ifndef USE_AKONADI
QString          KAEvent::Private::mCmdErrConfigGroup = QLatin1String("CommandErrors");
#endif
const QString    KAEvent::Private::CMD_ERROR_VALUE      = QLatin1String("MAIN");
const QString    KAEvent::Private::CMD_ERROR_PRE_VALUE  = QLatin1String("PRE");
const QString    KAEvent::Private::CMD_ERROR_POST_VALUE = QLatin1String("POST");

const QString    KAEvent::Private::SC = QLatin1String(";");

QFont                           KAEvent::Private::mDefaultFont;
const KHolidays::HolidayRegion* KAEvent::Private::mHolidays = 0;
QBitArray                       KAEvent::Private::mWorkDays(7);
QTime                           KAEvent::Private::mWorkDayStart(9, 0, 0);
QTime                           KAEvent::Private::mWorkDayEnd(17, 0, 0);


struct AlarmData
{
#ifdef USE_AKONADI
    ConstAlarmPtr          alarm;
#else
    const Alarm*           alarm;
#endif
    QString                cleanText;       // text or audio file name
    uint                   emailFromId;
    QFont                  font;
    QColor                 bgColour, fgColour;
    float                  soundVolume;
    float                  fadeVolume;
    int                    fadeSeconds;
    int                    nextRepeat;
    bool                   speak;
    KAAlarm::SubType       type;
    KAAlarmEventBase::Type action;
    int                    displayingFlags;
    bool                   defaultFont;
    bool                   reminderOnceOnly;
    bool                   isEmailText;
    bool                   commandScript;
    bool                   cancelOnPreActErr;
    bool                   dontShowPreActErr;
    bool                   repeatSound;
};
typedef QMap<KAAlarm::SubType, AlarmData> AlarmMap;

#ifdef USE_AKONADI
static void setProcedureAlarm(const Alarm::Ptr&, const QString& commandLine);
#else
static void setProcedureAlarm(Alarm*, const QString& commandLine);
#endif


/*=============================================================================
= Class KAEvent
= Corresponds to a KCal::Event instance.
=============================================================================*/

inline void KAEvent::Private::set_deferral(DeferType type)
{
    if (type)
    {
        if (!mDeferral)
            ++mAlarmCount;
    }
    else
    {
        if (mDeferral)
            --mAlarmCount;
    }
    mDeferral = type;
}

inline void KAEvent::Private::activate_reminder(bool activate)
{
    if (activate  &&  !mReminderActive  &&  mReminderMinutes)
    {
        mReminderActive = true;
        ++mAlarmCount;
    }
    else if (!activate  &&  mReminderActive)
    {
        mReminderActive = false;
        --mAlarmCount;
    }
}

KAEvent::KAEvent()
    : d(new Private)
{ }

KAEvent::Private::Private()
    :
#ifndef USE_AKONADI
      mResource(0),
#endif
      mCommandError(CMD_NO_ERROR),
#ifdef USE_AKONADI
      mItemId(-1),
#endif
      mReminderMinutes(0),
      mReminderActive(false),
      mRevision(0),
      mRecurrence(0),
      mAlarmCount(0),
      mDeferral(NO_DEFERRAL),
      mChangeCount(0),
      mChanged(false),
      mCategory(KAlarm::CalEvent::EMPTY),
#ifdef USE_AKONADI
      mCompatibility(KAlarm::Calendar::Current),
      mReadOnly(false),
#endif
      mConfirmAck(false),
      mEmailBcc(false),
      mBeep(false),
      mExcludeHolidays(false),
      mWorkTimeOnly(false),
      mDisplaying(false)
{ }

KAEvent::KAEvent(const KDateTime& dt, const QString& message, const QColor& bg, const QColor& fg, const QFont& f,
                 Action action, int lateCancel, int flags, bool changesPending)
    : d(new Private(dt, message, bg, fg, f, action, lateCancel, flags, changesPending))
{
}

KAEvent::Private::Private(const KDateTime& dt, const QString& message, const QColor& bg, const QColor& fg, const QFont& f,
                          Action action, int lateCancel, int flags, bool changesPending)
    : mRecurrence(0)
{
    set(dt, message, bg, fg, f, action, lateCancel, flags, changesPending);
    calcTriggerTimes();
}

#ifdef USE_AKONADI
KAEvent::KAEvent(const ConstEventPtr& e)
#else
KAEvent::KAEvent(const Event* e)
#endif
    : d(new Private(e))
{
}

#ifdef USE_AKONADI
KAEvent::Private::Private(const ConstEventPtr& e)
#else
KAEvent::Private::Private(const Event* e)
#endif
    : mRecurrence(0)
{
    set(e);
    calcTriggerTimes();
}

KAEvent::Private::Private(const KAEvent::Private& e)
    : KAAlarmEventBase(e),
      QSharedData(e),
      mRecurrence(0)
{
    copy(e);
    calcTriggerTimes();
}

/******************************************************************************
* Copies the data from another instance.
*/
void KAEvent::Private::copy(const KAEvent::Private& event)
{
    KAAlarmEventBase::copy(event);
#ifndef USE_AKONADI
    mResource                = event.mResource;
#endif
    mAllTrigger              = event.mAllTrigger;
    mMainTrigger             = event.mMainTrigger;
    mAllWorkTrigger          = event.mAllWorkTrigger;
    mMainWorkTrigger         = event.mMainWorkTrigger;
    mCommandError            = event.mCommandError;
    mTemplateName            = event.mTemplateName;
#ifdef USE_AKONADI
    mCustomProperties        = event.mCustomProperties;
    mItemId                  = event.mItemId;
    mCollectionId            = event.mCollectionId;
#else
    mResourceId              = event.mResourceId;
#endif
    mAudioFile               = event.mAudioFile;
    mPreAction               = event.mPreAction;
    mPostAction              = event.mPostAction;
    mStartDateTime           = event.mStartDateTime;
    mSaveDateTime            = event.mSaveDateTime;
    mAtLoginDateTime         = event.mAtLoginDateTime;
    mDeferralTime            = event.mDeferralTime;
    mDisplayingTime          = event.mDisplayingTime;
    mDisplayingFlags         = event.mDisplayingFlags;
    mReminderMinutes         = event.mReminderMinutes;
    mReminderActive          = event.mReminderActive;
    mDeferDefaultMinutes     = event.mDeferDefaultMinutes;
    mDeferDefaultDateOnly    = event.mDeferDefaultDateOnly;
    mRevision                = event.mRevision;
    mAlarmCount              = event.mAlarmCount;
    mDeferral                = event.mDeferral;
    mKMailSerialNumber       = event.mKMailSerialNumber;
    mTemplateAfterTime       = event.mTemplateAfterTime;
    mEmailFromIdentity       = event.mEmailFromIdentity;
    mEmailAddresses          = event.mEmailAddresses;
    mEmailSubject            = event.mEmailSubject;
    mEmailAttachments        = event.mEmailAttachments;
    mLogFile                 = event.mLogFile;
    mSoundVolume             = event.mSoundVolume;
    mFadeVolume              = event.mFadeVolume;
    mFadeSeconds             = event.mFadeSeconds;
    mCategory                = event.mCategory;
#ifdef USE_AKONADI
    mCompatibility           = event.mCompatibility;
    mReadOnly                = event.mReadOnly;
#endif
    mCancelOnPreActErr       = event.mCancelOnPreActErr;
    mDontShowPreActErr       = event.mDontShowPreActErr;
    mConfirmAck              = event.mConfirmAck;
    mCommandXterm            = event.mCommandXterm;
    mCommandDisplay          = event.mCommandDisplay;
    mEmailBcc                = event.mEmailBcc;
    mBeep                    = event.mBeep;
    mRepeatSound             = event.mRepeatSound;
    mSpeak                   = event.mSpeak;
    mCopyToKOrganizer        = event.mCopyToKOrganizer;
    mExcludeHolidays         = event.mExcludeHolidays;
    mWorkTimeOnly            = event.mWorkTimeOnly;
    mReminderOnceOnly        = event.mReminderOnceOnly;
    mMainExpired             = event.mMainExpired;
    mArchiveRepeatAtLogin    = event.mArchiveRepeatAtLogin;
    mArchive                 = event.mArchive;
    mDisplaying              = event.mDisplaying;
    mDisplayingDefer         = event.mDisplayingDefer;
    mDisplayingEdit          = event.mDisplayingEdit;
    mEnabled                 = event.mEnabled;
    mChangeCount             = 0;
    mChanged                 = false;
    delete mRecurrence;
    if (event.mRecurrence)
        mRecurrence = new KARecurrence(*event.mRecurrence);
    else
        mRecurrence = 0;
    if (event.mChanged)
        calcTriggerTimes();
}

/******************************************************************************
* Initialise the KAEvent::Private from a KCal::Event.
*/
#ifdef USE_AKONADI
void KAEvent::Private::set(const ConstEventPtr& event)
#else
void KAEvent::Private::set(const Event* event)
#endif
{
    startChanges();
    // Extract status from the event
    mCommandError           = CMD_NO_ERROR;
#ifndef USE_AKONADI
    mResource               = 0;
#endif
    mEventID                = event->uid();
    mRevision               = event->revision();
    mTemplateName.clear();
    mLogFile.clear();
#ifdef USE_AKONADI
    mItemId                 = -1;
    mCollectionId           = -1;
#else
    mResourceId.clear();
#endif
    mTemplateAfterTime      = -1;
    mBeep                   = false;
    mSpeak                  = false;
    mEmailBcc               = false;
    mCommandXterm           = false;
    mCommandDisplay         = false;
    mCopyToKOrganizer       = false;
    mExcludeHolidays        = false;
    mWorkTimeOnly           = false;
    mConfirmAck             = false;
    mArchive                = false;
    mReminderOnceOnly       = false;
    mAutoClose              = false;
    mArchiveRepeatAtLogin   = false;
    mDisplayingDefer        = false;
    mDisplayingEdit         = false;
    mDeferDefaultDateOnly   = false;
    mReminderActive         = false;
    mReminderMinutes        = 0;
    mDeferDefaultMinutes    = 0;
    mLateCancel             = 0;
    mKMailSerialNumber      = 0;
    mChangeCount            = 0;
    mChanged                = false;
    mBgColour               = QColor(255, 255, 255);    // missing/invalid colour - return white background
    mFgColour               = QColor(0, 0, 0);          // and black foreground
#ifdef USE_AKONADI
    mCompatibility          = KAlarm::Calendar::Current;
    mReadOnly               = event->isReadOnly();
#endif
    mUseDefaultFont         = true;
    mEnabled                = true;
    clearRecur();
    QString param;
    bool ok;
    mCategory               = KAlarm::CalEvent::status(event, &param);
    if (mCategory == KAlarm::CalEvent::DISPLAYING)
    {
        // It's a displaying calendar event - set values specific to displaying alarms
        QStringList params = param.split(SC, QString::KeepEmptyParts);
        int n = params.count();
        if (n)
        {
#ifdef USE_AKONADI
            qlonglong id = params[0].toLongLong(&ok);
            if (ok)
                mCollectionId = id;
#else
            mResourceId = params[0];
#endif
            for (int i = 1;  i < n;  ++i)
            {
                if (params[i] == DISP_DEFER)
                    mDisplayingDefer = true;
                if (params[i] == DISP_EDIT)
                    mDisplayingEdit = true;
            }
        }
    }
#ifdef USE_AKONADI
    // Store the non-KAlarm custom properties of the event
    QByteArray kalarmKey = "X-KDE-" + KAlarm::Calendar::APPNAME + '-';
    mCustomProperties = event->customProperties();
    for (QMap<QByteArray, QString>::Iterator it = mCustomProperties.begin();  it != mCustomProperties.end(); )
    {
        if (it.key().startsWith(kalarmKey))
            it = mCustomProperties.erase(it);
        else
            ++it;
    }
#endif

    bool dateOnly = false;
    QStringList flags = event->customProperty(KAlarm::Calendar::APPNAME, FLAGS_PROPERTY).split(SC, QString::SkipEmptyParts);
    flags += QString();    // to avoid having to check for end of list
    for (int i = 0, end = flags.count() - 1;  i < end;  ++i)
    {
        if (flags[i] == DATE_ONLY_FLAG)
            dateOnly = true;
        else if (flags[i] == CONFIRM_ACK_FLAG)
            mConfirmAck = true;
        else if (flags[i] == EMAIL_BCC_FLAG)
            mEmailBcc = true;
        else if (flags[i] == KORGANIZER_FLAG)
            mCopyToKOrganizer = true;
        else if (flags[i] == EXCLUDE_HOLIDAYS_FLAG)
            mExcludeHolidays = true;
        else if (flags[i] == WORK_TIME_ONLY_FLAG)
            mWorkTimeOnly = true;
        else if (flags[i]== KMAIL_SERNUM_FLAG)
        {
            unsigned long n = flags[i + 1].toULong(&ok);
            if (!ok)
                continue;
            mKMailSerialNumber = n;
            ++i;
        }
        else if (flags[i] == DEFER_FLAG)
        {
            QString mins = flags[i + 1];
            if (mins.endsWith('D'))
            {
                mDeferDefaultDateOnly = true;
                mins.truncate(mins.length() - 1);
            }
            int n = static_cast<int>(mins.toUInt(&ok));
            if (!ok)
                continue;
            mDeferDefaultMinutes = n;
            ++i;
        }
        else if (flags[i] == TEMPL_AFTER_TIME_FLAG)
        {
            int n = static_cast<int>(flags[i + 1].toUInt(&ok));
            if (!ok)
                continue;
            mTemplateAfterTime = n;
            ++i;
        }
        else if (flags[i] == LATE_CANCEL_FLAG)
        {
            mLateCancel = static_cast<int>(flags[i + 1].toUInt(&ok));
            if (ok)
                ++i;
            if (!ok  ||  !mLateCancel)
                mLateCancel = 1;    // invalid parameter defaults to 1 minute
        }
        else if (flags[i] == AUTO_CLOSE_FLAG)
        {
            mLateCancel = static_cast<int>(flags[i + 1].toUInt(&ok));
            if (ok)
                ++i;
            if (!ok  ||  !mLateCancel)
                mLateCancel = 1;    // invalid parameter defaults to 1 minute
            mAutoClose = true;
        }
    }

    QString prop = event->customProperty(KAlarm::Calendar::APPNAME, LOG_PROPERTY);
    if (!prop.isEmpty())
    {
        if (prop == xtermURL)
            mCommandXterm = true;
        else if (prop == displayURL)
            mCommandDisplay = true;
        else
            mLogFile = prop;
    }
    prop = event->customProperty(KAlarm::Calendar::APPNAME, REPEAT_PROPERTY);
    if (!prop.isEmpty())
    {
        // This property is used when the main alarm has expired
        QStringList list = prop.split(QLatin1Char(':'));
        if (list.count() >= 2)
        {
            int interval = static_cast<int>(list[0].toUInt());
            int count = static_cast<int>(list[1].toUInt());
            if (interval && count)
            {
                if (interval % (24*60))
                    mRepetition.set(Duration(interval * 60, Duration::Seconds), count);
                else
                    mRepetition.set(Duration(interval / (24*60), Duration::Days), count);
            }
        }
    }
    prop = event->customProperty(KAlarm::Calendar::APPNAME, ARCHIVE_PROPERTY);
    if (!prop.isEmpty())
    {
        mArchive = true;
        if (prop != QLatin1String("0"))
        {
            // It's the archive property containing a reminder time and/or repeat-at-login flag
            QStringList list = prop.split(SC, QString::SkipEmptyParts);
            for (int j = 0;  j < list.count();  ++j)
            {
                if (list[j] == AT_LOGIN_TYPE)
                    mArchiveRepeatAtLogin = true;
                else if (list[j] == ARCHIVE_REMINDER_ONCE_TYPE)
                    mReminderOnceOnly = true;
                else
                {
                    char ch;
                    const char* cat = list[j].toLatin1().constData();
                    while ((ch = *cat) != 0  &&  (ch < '0' || ch > '9'))
                        ++cat;
                    if (ch)
                    {
                        mReminderMinutes = ch - '0';
                        while ((ch = *++cat) >= '0'  &&  ch <= '9')
                            mReminderMinutes = mReminderMinutes * 10 + ch - '0';
                        switch (ch)
                        {
                            case 'M':  break;
                            case 'H':  mReminderMinutes *= 60;    break;
                            case 'D':  mReminderMinutes *= 1440;  break;
                        }
                    }
                }
            }
        }
    }
    mNextMainDateTime = readDateTime(event, dateOnly, mStartDateTime);
    mSaveDateTime = event->created();
    if (dateOnly  &&  !mRepetition.isDaily())
        mRepetition.set(Duration(mRepetition.intervalDays(), Duration::Days));
    if (mCategory == KAlarm::CalEvent::TEMPLATE)
        mTemplateName = event->summary();
#ifdef USE_AKONADI
    if (event->customStatus() == DISABLED_STATUS)
#else
    if (event->statusStr() == DISABLED_STATUS)
#endif
        mEnabled = false;

    // Extract status from the event's alarms.
    // First set up defaults.
    mActionType        = T_MESSAGE;
    mMainExpired       = true;
    mRepeatAtLogin     = false;
    mDisplaying        = false;
    mRepeatSound       = false;
    mCommandScript     = false;
    mCancelOnPreActErr = false;
    mDontShowPreActErr = false;
    mDeferral          = NO_DEFERRAL;
    mSoundVolume       = -1;
    mFadeVolume        = -1;
    mFadeSeconds       = 0;
    mEmailFromIdentity = 0;
    mText.clear();
    mAudioFile.clear();
    mPreAction.clear();
    mPostAction.clear();
    mEmailSubject.clear();
    mEmailAddresses.clear();
    mEmailAttachments.clear();

    // Extract data from all the event's alarms and index the alarms by sequence number
    AlarmMap alarmMap;
    readAlarms(event, &alarmMap, mCommandDisplay);

    // Incorporate the alarms' details into the overall event
    mAlarmCount = 0;       // initialise as invalid
    DateTime alTime;
    bool set = false;
    bool isEmailText = false;
    bool setDeferralTime = false;
    Duration deferralOffset;
    for (AlarmMap::ConstIterator it = alarmMap.constBegin();  it != alarmMap.constEnd();  ++it)
    {
        const AlarmData& data = it.value();
        DateTime dateTime = data.alarm->hasStartOffset() ? data.alarm->startOffset().end(mNextMainDateTime.effectiveKDateTime()) : data.alarm->time();
        switch (data.type)
        {
            case KAAlarm::MAIN__ALARM:
                mMainExpired = false;
                alTime = dateTime;
                alTime.setDateOnly(mStartDateTime.isDateOnly());
                if (data.alarm->repeatCount()  &&  data.alarm->snoozeTime())
                {
                    mRepetition.set(data.alarm->snoozeTime(), data.alarm->repeatCount());   // values may be adjusted in setRecurrence()
                    mNextRepeat = data.nextRepeat;
                }
                if (data.action != T_AUDIO)
                    break;
                // Fall through to AUDIO__ALARM
            case KAAlarm::AUDIO__ALARM:
                mAudioFile   = data.cleanText;
                mSpeak       = data.speak  &&  mAudioFile.isEmpty();
                mBeep        = !mSpeak  &&  mAudioFile.isEmpty();
                mSoundVolume = (!mBeep && !mSpeak) ? data.soundVolume : -1;
                mFadeVolume  = (mSoundVolume >= 0  &&  data.fadeSeconds > 0) ? data.fadeVolume : -1;
                mFadeSeconds = (mFadeVolume >= 0) ? data.fadeSeconds : 0;
                mRepeatSound = (!mBeep && !mSpeak)  &&  (data.alarm->repeatCount() < 0);
                break;
            case KAAlarm::AT_LOGIN__ALARM:
                mRepeatAtLogin   = true;
                mAtLoginDateTime = dateTime.kDateTime();
                alTime = mAtLoginDateTime;
                break;
            case KAAlarm::REMINDER__ALARM:
                // N.B. there can be a start offset but no valid date/time (e.g. in template)
                mReminderMinutes = -(data.alarm->startOffset().asSeconds() / 60);
                if (mReminderMinutes < 0)
                    mReminderMinutes = 0;   // reminders currently must be BEFORE the main alarm
                else if (mReminderMinutes)
                    mReminderActive = true;
                break;
            case KAAlarm::DEFERRED_REMINDER_DATE__ALARM:
            case KAAlarm::DEFERRED_DATE__ALARM:
                mDeferral = (data.type == KAAlarm::DEFERRED_REMINDER_DATE__ALARM) ? REMINDER_DEFERRAL : NORMAL_DEFERRAL;
                mDeferralTime = dateTime;
                mDeferralTime.setDateOnly(true);
                if (data.alarm->hasStartOffset())
                    deferralOffset = data.alarm->startOffset();
                break;
            case KAAlarm::DEFERRED_REMINDER_TIME__ALARM:
            case KAAlarm::DEFERRED_TIME__ALARM:
                mDeferral = (data.type == KAAlarm::DEFERRED_REMINDER_TIME__ALARM) ? REMINDER_DEFERRAL : NORMAL_DEFERRAL;
                mDeferralTime = dateTime;
                if (data.alarm->hasStartOffset())
                    deferralOffset = data.alarm->startOffset();
                break;
            case KAAlarm::DISPLAYING__ALARM:
            {
                mDisplaying      = true;
                mDisplayingFlags = data.displayingFlags;
                bool dateOnly = (mDisplayingFlags & DEFERRAL) ? !(mDisplayingFlags & TIMED_FLAG)
                              : mStartDateTime.isDateOnly();
                mDisplayingTime = dateTime;
                mDisplayingTime.setDateOnly(dateOnly);
                alTime = mDisplayingTime;
                break;
            }
            case KAAlarm::PRE_ACTION__ALARM:
                mPreAction         = data.cleanText;
                mCancelOnPreActErr = data.cancelOnPreActErr;
                mDontShowPreActErr = data.dontShowPreActErr;
                break;
            case KAAlarm::POST_ACTION__ALARM:
                mPostAction = data.cleanText;
                break;
            case KAAlarm::INVALID__ALARM:
            default:
                break;
        }

        if (data.reminderOnceOnly)
            mReminderOnceOnly = true;
        bool noSetNextTime = false;
        switch (data.type)
        {
            case KAAlarm::DEFERRED_REMINDER_DATE__ALARM:
            case KAAlarm::DEFERRED_DATE__ALARM:
            case KAAlarm::DEFERRED_REMINDER_TIME__ALARM:
            case KAAlarm::DEFERRED_TIME__ALARM:
                if (!set)
                {
                    // The recurrence has to be evaluated before we can
                    // calculate the time of a deferral alarm.
                    setDeferralTime = true;
                    noSetNextTime = true;
                }
                // fall through to AT_LOGIN__ALARM etc.
            case KAAlarm::AT_LOGIN__ALARM:
            case KAAlarm::REMINDER__ALARM:
            case KAAlarm::DISPLAYING__ALARM:
                if (!set  &&  !noSetNextTime)
                    mNextMainDateTime = alTime;
                // fall through to MAIN__ALARM
            case KAAlarm::MAIN__ALARM:
                // Ensure that the basic fields are set up even if there is no main
                // alarm in the event (if it has expired and then been deferred)
                if (!set)
                {
                    mActionType = data.action;
                    mText = (mActionType == T_COMMAND) ? data.cleanText.trimmed() : data.cleanText;
                    switch (data.action)
                    {
                        case T_COMMAND:
                            mCommandScript = data.commandScript;
                            if (!mCommandDisplay)
                                break;
                            // fall through to T_MESSAGE
                        case T_MESSAGE:
                            mFont           = data.font;
                            mUseDefaultFont = data.defaultFont;
                            if (data.isEmailText)
                                isEmailText = true;
                            // fall through to T_FILE
                        case T_FILE:
                            mBgColour = data.bgColour;
                            mFgColour = data.fgColour;
                            break;
                        case T_EMAIL:
                            mEmailFromIdentity = data.emailFromId;
                            mEmailAddresses    = data.alarm->mailAddresses();
                            mEmailSubject      = data.alarm->mailSubject();
                            mEmailAttachments  = data.alarm->mailAttachments();
                            break;
                        case T_AUDIO:
                            // Already mostly handled above
                            mRepeatSound = data.repeatSound;
                            break;
                        default:
                            break;
                    }
                    set = true;
                }
                if (data.action == T_FILE  &&  mActionType == T_MESSAGE)
                    mActionType = T_FILE;
                ++mAlarmCount;
                break;
            case KAAlarm::AUDIO__ALARM:
            case KAAlarm::PRE_ACTION__ALARM:
            case KAAlarm::POST_ACTION__ALARM:
            case KAAlarm::INVALID__ALARM:
            default:
                break;
        }
    }
    if (!isEmailText)
        mKMailSerialNumber = 0;
    if (mRepeatAtLogin)
        mArchiveRepeatAtLogin = false;

    Recurrence* recur = event->recurrence();
    if (recur  &&  recur->recurs())
    {
        int nextRepeat = mNextRepeat;    // setRecurrence() clears mNextRepeat
        setRecurrence(*recur);
        if (nextRepeat <= mRepetition.count())
            mNextRepeat = nextRepeat;
    }
    else if (mRepetition)
    {
        // Convert a repetition with no recurrence into a recurrence
        if (mRepetition.isDaily())
            recur->setDaily(mRepetition.intervalDays());
        else
            recur->setMinutely(mRepetition.intervalMinutes());
        recur->setDuration(mRepetition.count() + 1);
        mRepetition.set(0, 0);
    }

    if (mMainExpired  &&  deferralOffset  &&  checkRecur() != KARecurrence::NO_RECUR)
    {
        // Adjust the deferral time for an expired recurrence, since the
        // offset is relative to the first actual occurrence.
        DateTime dt = mRecurrence->getNextDateTime(mStartDateTime.addDays(-1).kDateTime());
        dt.setDateOnly(mStartDateTime.isDateOnly());
        if (mDeferralTime.isDateOnly())
        {
            mDeferralTime = deferralOffset.end(dt.kDateTime());
            mDeferralTime.setDateOnly(true);
        }
        else
            mDeferralTime = deferralOffset.end(dt.effectiveKDateTime());
    }
    if (mDeferral)
    {
        if (setDeferralTime)
            mNextMainDateTime = mDeferralTime;
    }
    mChanged = true;
    endChanges();
}

/******************************************************************************
* Fetch the start and next date/time for a KCal::Event.
* Reply = next main date/time.
*/
#ifdef USE_AKONADI
DateTime KAEvent::readDateTime(const ConstEventPtr& event, bool dateOnly, DateTime& start)
#else
DateTime KAEvent::readDateTime(const Event* event, bool dateOnly, DateTime& start)
#endif
{
    start = event->dtStart();
    if (dateOnly)
    {
        // A date-only event is indicated by the X-KDE-KALARM-FLAGS:DATE property, not
        // by a date-only start date/time (for the reasons given in updateKCalEvent()).
        start.setDateOnly(true);
    }
    DateTime next = start;
    QString prop = event->customProperty(KAlarm::Calendar::APPNAME, Private::NEXT_RECUR_PROPERTY);
    if (prop.length() >= 8)
    {
        // The next due recurrence time is specified
        QDate d(prop.left(4).toInt(), prop.mid(4,2).toInt(), prop.mid(6,2).toInt());
        if (d.isValid())
        {
            if (dateOnly  &&  prop.length() == 8)
                next.setDate(d);
            else if (!dateOnly  &&  prop.length() == 15  &&  prop[8] == QChar('T'))
            {
                QTime t(prop.mid(9,2).toInt(), prop.mid(11,2).toInt(), prop.mid(13,2).toInt());
                if (t.isValid())
                {
                    next.setDate(d);
                    next.setTime(t);
                }
            }
            if (next < start)
                next = start;   // ensure next recurrence time is valid
        }
    }
    return next;
}

/******************************************************************************
* Parse the alarms for a KCal::Event.
* Reply = map of alarm data, indexed by KAAlarm::Type
*/
#ifdef USE_AKONADI
void KAEvent::readAlarms(const ConstEventPtr& event, void* almap, bool cmdDisplay)
#else
void KAEvent::readAlarms(const Event* event, void* almap, bool cmdDisplay)
#endif
{
    AlarmMap* alarmMap = (AlarmMap*)almap;
    Alarm::List alarms = event->alarms();

    // Check if it's an audio event with no display alarm
    bool audioOnly = false;
    for (int i = 0, end = alarms.count();  i < end;  ++i)
    {
        switch (alarms[i]->type())
        {
            case Alarm::Display:
            case Alarm::Procedure:
                audioOnly = false;
                i = end;   // exit from the 'for' loop
                break;
            case Alarm::Audio:
                audioOnly = true;
                break;
            default:
                break;
        }
    }

    for (int i = 0, end = alarms.count();  i < end;  ++i)
    {
        // Parse the next alarm's text
        AlarmData data;
        readAlarm(alarms[i], data, audioOnly, cmdDisplay);
        if (data.type != KAAlarm::INVALID__ALARM)
            alarmMap->insert(data.type, data);
    }
}

/******************************************************************************
* Parse a KCal::Alarm.
* If 'audioMain' is true, the event contains an audio alarm but no display alarm.
* Reply = alarm ID (sequence number)
*/
#ifdef USE_AKONADI
void KAEvent::readAlarm(const ConstAlarmPtr& alarm, AlarmData& data, bool audioMain, bool cmdDisplay)
#else
void KAEvent::readAlarm(const Alarm* alarm, AlarmData& data, bool audioMain, bool cmdDisplay)
#endif
{
    // Parse the next alarm's text
    data.alarm           = alarm;
    data.displayingFlags = 0;
    data.isEmailText     = false;
    data.speak           = false;
    data.nextRepeat      = 0;
    if (alarm->repeatCount())
    {
        bool ok;
        QString property = alarm->customProperty(KAlarm::Calendar::APPNAME, Private::NEXT_REPEAT_PROPERTY);
        int n = static_cast<int>(property.toUInt(&ok));
        if (ok)
            data.nextRepeat = n;
    }
    switch (alarm->type())
    {
        case Alarm::Procedure:
            data.action        = KAAlarmEventBase::T_COMMAND;
            data.cleanText     = alarm->programFile();
            data.commandScript = data.cleanText.isEmpty();   // blank command indicates a script
            if (!alarm->programArguments().isEmpty())
            {
                if (!data.commandScript)
                    data.cleanText += ' ';
                data.cleanText += alarm->programArguments();
            }
            data.cancelOnPreActErr = !alarm->customProperty(KAlarm::Calendar::APPNAME, Private::CANCEL_ON_ERROR_PROPERTY).isNull();
            data.dontShowPreActErr = !alarm->customProperty(KAlarm::Calendar::APPNAME, Private::DONT_SHOW_ERROR_PROPERTY).isNull();
            if (!cmdDisplay)
                break;
            // fall through to Display
        case Alarm::Display:
        {
            if (alarm->type() == Alarm::Display)
            {
                data.action    = KAAlarmEventBase::T_MESSAGE;
                data.cleanText = AlarmText::fromCalendarText(alarm->text(), data.isEmailText);
            }
            QString property = alarm->customProperty(KAlarm::Calendar::APPNAME, Private::FONT_COLOUR_PROPERTY);
            QStringList list = property.split(QLatin1Char(';'), QString::KeepEmptyParts);
            data.bgColour = QColor(255, 255, 255);   // white
            data.fgColour = QColor(0, 0, 0);         // black
            int n = list.count();
            if (n > 0)
            {
                if (!list[0].isEmpty())
                {
                    QColor c(list[0]);
                    if (c.isValid())
                        data.bgColour = c;
                }
                if (n > 1  &&  !list[1].isEmpty())
                {
                    QColor c(list[1]);
                    if (c.isValid())
                        data.fgColour = c;
                }
            }
            data.defaultFont = (n <= 2 || list[2].isEmpty());
            if (!data.defaultFont)
                data.font.fromString(list[2]);
            break;
        }
        case Alarm::Email:
            data.action      = KAAlarmEventBase::T_EMAIL;
            data.emailFromId = alarm->customProperty(KAlarm::Calendar::APPNAME, Private::EMAIL_ID_PROPERTY).toUInt();
            data.cleanText   = alarm->mailText();
            break;
        case Alarm::Audio:
        {
            data.action      = KAAlarmEventBase::T_AUDIO;
            data.cleanText   = alarm->audioFile();
            data.soundVolume = -1;
            data.fadeVolume  = -1;
            data.fadeSeconds = 0;
            QString property = alarm->customProperty(KAlarm::Calendar::APPNAME, Private::VOLUME_PROPERTY);
            if (!property.isEmpty())
            {
                bool ok;
                float fadeVolume;
                int   fadeSecs = 0;
                QStringList list = property.split(QLatin1Char(';'), QString::KeepEmptyParts);
                data.soundVolume = list[0].toFloat(&ok);
                if (!ok)
                    data.soundVolume = -1;
                if (data.soundVolume >= 0  &&  list.count() >= 3)
                {
                    fadeVolume = list[1].toFloat(&ok);
                    if (ok)
                        fadeSecs = static_cast<int>(list[2].toUInt(&ok));
                    if (ok  &&  fadeVolume >= 0  &&  fadeSecs > 0)
                    {
                        data.fadeVolume  = fadeVolume;
                        data.fadeSeconds = fadeSecs;
                    }
                }
            }
            if (!audioMain)
            {
                data.type  = KAAlarm::AUDIO__ALARM;
                data.speak = !alarm->customProperty(KAlarm::Calendar::APPNAME, Private::SPEAK_PROPERTY).isNull();
                return;
            }
            break;
        }
        case Alarm::Invalid:
            data.type = KAAlarm::INVALID__ALARM;
            return;
    }

    bool atLogin          = false;
    bool reminder         = false;
    bool deferral         = false;
    bool dateDeferral     = false;
    data.reminderOnceOnly = false;
    data.repeatSound      = false;
    data.type = KAAlarm::MAIN__ALARM;
    QString property = alarm->customProperty(KAlarm::Calendar::APPNAME, Private::TYPE_PROPERTY);
    QStringList types = property.split(QLatin1Char(','), QString::SkipEmptyParts);
    for (int i = 0, end = types.count();  i < end;  ++i)
    {
        QString type = types[i];
        if (type == Private::AT_LOGIN_TYPE)
            atLogin = true;
        else if (type == Private::FILE_TYPE  &&  data.action == KAAlarmEventBase::T_MESSAGE)
            data.action = KAAlarmEventBase::T_FILE;
        else if (type == Private::REMINDER_TYPE)
            reminder = true;
        else if (type == Private::REMINDER_ONCE_TYPE)
            reminder = data.reminderOnceOnly = true;
        else if (type == Private::TIME_DEFERRAL_TYPE)
            deferral = true;
        else if (type == Private::DATE_DEFERRAL_TYPE)
            dateDeferral = deferral = true;
        else if (type == Private::DISPLAYING_TYPE)
            data.type = KAAlarm::DISPLAYING__ALARM;
        else if (type == Private::PRE_ACTION_TYPE  &&  data.action == KAAlarmEventBase::T_COMMAND)
            data.type = KAAlarm::PRE_ACTION__ALARM;
        else if (type == Private::POST_ACTION_TYPE  &&  data.action == KAAlarmEventBase::T_COMMAND)
            data.type = KAAlarm::POST_ACTION__ALARM;
        else if (type == Private::SOUND_REPEAT_TYPE  &&  data.action == KAAlarmEventBase::T_AUDIO)
            data.repeatSound = true;
    }

    if (reminder)
    {
        if (data.type == KAAlarm::MAIN__ALARM)
            data.type = dateDeferral ? KAAlarm::DEFERRED_REMINDER_DATE__ALARM
                      : deferral ? KAAlarm::DEFERRED_REMINDER_TIME__ALARM : KAAlarm::REMINDER__ALARM;
        else if (data.type == KAAlarm::DISPLAYING__ALARM)
            data.displayingFlags = dateDeferral ? REMINDER | DATE_DEFERRAL
                                 : deferral ? REMINDER | TIME_DEFERRAL : REMINDER;
    }
    else if (deferral)
    {
        if (data.type == KAAlarm::MAIN__ALARM)
            data.type = dateDeferral ? KAAlarm::DEFERRED_DATE__ALARM : KAAlarm::DEFERRED_TIME__ALARM;
        else if (data.type == KAAlarm::DISPLAYING__ALARM)
            data.displayingFlags = dateDeferral ? DATE_DEFERRAL : TIME_DEFERRAL;
    }
    if (atLogin)
    {
        if (data.type == KAAlarm::MAIN__ALARM)
            data.type = KAAlarm::AT_LOGIN__ALARM;
        else if (data.type == KAAlarm::DISPLAYING__ALARM)
            data.displayingFlags = REPEAT_AT_LOGIN;
    }
//kDebug()<<"text="<<alarm->text()<<", time="<<alarm->time().toString()<<", valid time="<<alarm->time().isValid();
}

/******************************************************************************
* Initialise the instance with the specified parameters.
*/
void KAEvent::Private::set(const KDateTime& dateTime, const QString& text, const QColor& bg, const QColor& fg,
                           const QFont& font, Action action, int lateCancel, int flags, bool changesPending)
{
    clearRecur();
    mStartDateTime = dateTime;
    mStartDateTime.setDateOnly(flags & ANY_TIME);
    mNextMainDateTime = mStartDateTime;
    switch (action)
    {
        case MESSAGE:
        case FILE:
        case COMMAND:
        case EMAIL:
        case AUDIO:
            mActionType = (KAAlarmEventBase::Type)action;
            break;
        default:
            mActionType = T_MESSAGE;
            break;
    }
    mEventID.clear();
    mTemplateName.clear();
#ifdef USE_AKONADI
    mItemId                 = -1;
    mCollectionId           = -1;
#else
    mResource               = 0;
    mResourceId.clear();
#endif
    mPreAction.clear();
    mPostAction.clear();
    mText                   = (mActionType == T_COMMAND) ? text.trimmed()
                            : (mActionType == T_AUDIO) ? QString() : text;
    mCategory               = KAlarm::CalEvent::ACTIVE;
    mAudioFile              = (mActionType == T_AUDIO) ? text : QString();
    mSoundVolume            = -1;
    mFadeVolume             = -1;
    mTemplateAfterTime      = -1;
    mFadeSeconds            = 0;
    mBgColour               = bg;
    mFgColour               = fg;
    mFont                   = font;
    mAlarmCount             = 1;
    mLateCancel             = lateCancel;     // do this before setting flags
    mDeferral               = NO_DEFERRAL;    // do this before setting flags

    KAAlarmEventBase::set(flags & ~READ_ONLY_FLAGS);
    if (mRepeatAtLogin)                       // do this after setting flags
        ++mAlarmCount;
    mStartDateTime.setDateOnly(flags & ANY_TIME);
    set_deferral((flags & DEFERRAL) ? NORMAL_DEFERRAL : NO_DEFERRAL);
    mConfirmAck             = flags & CONFIRM_ACK;
    mCommandXterm           = flags & EXEC_IN_XTERM;
    mCommandDisplay         = flags & DISPLAY_COMMAND;
    mCopyToKOrganizer       = flags & COPY_KORGANIZER;
    mExcludeHolidays        = flags & EXCL_HOLIDAYS;
    mWorkTimeOnly           = flags & WORK_TIME_ONLY;
    mEmailBcc               = flags & EMAIL_BCC;
    mEnabled                = !(flags & DISABLED);
    mDisplaying             = flags & DISPLAYING_;
    mRepeatSound            = flags & REPEAT_SOUND;
    mBeep                   = (flags & BEEP) && action != AUDIO;
    mSpeak                  = (flags & SPEAK) && action != AUDIO;
    if (mSpeak)
        mBeep               = false;

    mKMailSerialNumber      = 0;
    mReminderMinutes        = 0;
    mDeferDefaultMinutes    = 0;
    mDeferDefaultDateOnly   = false;
    mArchiveRepeatAtLogin   = false;
    mReminderActive         = false;
    mReminderOnceOnly       = false;
    mDisplaying             = false;
    mMainExpired            = false;
    mDisplayingDefer        = false;
    mDisplayingEdit         = false;
    mArchive                = false;
    mCancelOnPreActErr      = false;
    mDontShowPreActErr      = false;
#ifdef USE_AKONADI
    mCompatibility          = KAlarm::Calendar::Current;
    mReadOnly               = false;
#endif
    mCommandError           = CMD_NO_ERROR;
    mChangeCount            = changesPending ? 1 : 0;
    mChanged                = true;
    calcTriggerTimes();
}

void KAEvent::setLogFile(const QString& logfile)
{
    d->mLogFile = logfile;
    if (!logfile.isEmpty())
        d->mCommandDisplay = d->mCommandXterm = false;
}

void KAEvent::setEmail(uint from, const EmailAddressList& addresses, const QString& subject, const QStringList& attachments)
{
    d->mEmailFromIdentity = from;
    d->mEmailAddresses    = addresses;
    d->mEmailSubject      = subject;
    d->mEmailAttachments  = attachments;
}

void KAEvent::Private::setAudioFile(const QString& filename, float volume, float fadeVolume, int fadeSeconds, bool allowEmptyFile)
{
    mAudioFile = filename;
    mSoundVolume = (!allowEmptyFile && filename.isEmpty()) ? -1 : volume;
    if (mSoundVolume >= 0)
    {
        mFadeVolume  = (fadeSeconds > 0) ? fadeVolume : -1;
        mFadeSeconds = (mFadeVolume >= 0) ? fadeSeconds : 0;
    }
    else
    {
        mFadeVolume  = -1;
        mFadeSeconds = 0;
    }
}

/******************************************************************************
* Change the type of an event.
* If it is being set to archived, set the archived indication in the event ID;
* otherwise, remove the archived indication from the event ID.
*/
void KAEvent::Private::setCategory(KAlarm::CalEvent::Type s)
{
    if (s == mCategory)
        return;
    mEventID = KAlarm::CalEvent::uid(mEventID, s);
    mCategory = s;
}

/******************************************************************************
* Set the event to be an alarm template.
*/
void KAEvent::setTemplate(const QString& name, int afterTime)
{
    d->setCategory(KAlarm::CalEvent::TEMPLATE);
    d->mTemplateName = name;
    d->mTemplateAfterTime = afterTime;
    // Templates don't need trigger times to be calculated
    d->mChangeCount = 0;
    d->calcTriggerTimes();   // invalidate all trigger times
}

/******************************************************************************
* Set or clear repeat-at-login.
*/
void KAEvent::Private::setRepeatAtLogin(bool rl)
{
    if (rl  &&  !mRepeatAtLogin)
        ++mAlarmCount;
    else if (!rl  &&  mRepeatAtLogin)
        --mAlarmCount;
    mRepeatAtLogin = rl;
    if (mRepeatAtLogin)
    {
        // Cancel reminder, late-cancel and copy-to-KOrganizer
        setReminder(0, false);
        mLateCancel = 0;
        mAutoClose = false;
        mCopyToKOrganizer = false;
    }
}

/******************************************************************************
* Set a reminder.
* 'minutes' = number of minutes BEFORE the main alarm.
*/
void KAEvent::Private::setReminder(int minutes, bool onceOnly)
{
    if (minutes != mReminderMinutes  ||  (minutes && !mReminderActive))
    {
        if (minutes  &&  !mReminderActive)
            ++mAlarmCount;
        else if (!minutes  &&  mReminderActive)
            --mAlarmCount;
        mReminderMinutes  = minutes;
        mReminderActive   = minutes;
        mReminderOnceOnly = onceOnly;
        calcTriggerTimes();
    }
}

void KAEvent::setHolidays(const HolidayRegion& h)
{
    Private::mHolidays = &h;
}

DateTime KAEvent::nextTrigger(TriggerType type) const
{
    switch (type)
    {
        case ALL_TRIGGER:       return d->mAllTrigger;
        case MAIN_TRIGGER:      return d->mMainTrigger;
        case ALL_WORK_TRIGGER:  return d->mAllWorkTrigger;
        case WORK_TRIGGER:      return d->mMainWorkTrigger;
        case DISPLAY_TRIGGER:   return (d->mWorkTimeOnly || d->mExcludeHolidays) ? d->mMainWorkTrigger : d->mMainTrigger;
        default:                return DateTime();
    }
}

/******************************************************************************
* Indicate that changes to the instance are complete.
* Recalculate the trigger times if any changes have occurred.
*/
void KAEvent::Private::endChanges()
{
    if (mChangeCount > 0)
        --mChangeCount;
    if (!mChangeCount  &&  mChanged)
        calcTriggerTimes();
}

/******************************************************************************
* Calculate the next trigger times of the alarm.
* This should only be called when changes have actually occurred which might
* affect the event's trigger times.
* mMainTrigger is set to the next scheduled recurrence/sub-repetition, or the
*              deferral time if a deferral is pending.
* mAllTrigger is the same as mMainTrigger, but takes account of reminders.
* mMainWorkTrigger is set to the next scheduled recurrence/sub-repetition
*                  which occurs in working hours, if working-time-only is set.
* mAllWorkTrigger is the same as mMainWorkTrigger, but takes account of reminders.
*/
void KAEvent::Private::calcTriggerTimes() const
{
    if (mChangeCount)
    {
        mChanged = true;   // note that changes have actually occurred
        return;
    }
    mChanged = false;
    if (mCategory == KAlarm::CalEvent::ARCHIVED  ||  !mTemplateName.isEmpty())
    {
        // It's a template or archived
        mAllTrigger = mMainTrigger = mAllWorkTrigger = mMainWorkTrigger = KDateTime();
    }
    else if (mDeferral > 0  &&  mDeferral != REMINDER_DEFERRAL)
    {
        // For a deferred alarm, working time setting is ignored
        mAllTrigger = mMainTrigger = mAllWorkTrigger = mMainWorkTrigger = mDeferralTime;
    }
    else
    {
        mMainTrigger = mainDateTime(true);   // next recurrence or sub-repetition
        mAllTrigger = (mDeferral == REMINDER_DEFERRAL) ? mDeferralTime
                    : mReminderActive ? mMainTrigger.addMins(-mReminderMinutes) : mMainTrigger;
        // It's not deferred.
        // If only-during-working-time is set and it recurs, it won't actually trigger
        // unless it falls during working hours.
        if ((!mWorkTimeOnly && !mExcludeHolidays)
        ||  checkRecur() == KARecurrence::NO_RECUR
        ||  isWorkingTime(mMainTrigger.kDateTime()))
        {
            // It only occurs once, or it complies with any working hours/holiday
            // restrictions.
            mMainWorkTrigger = mMainTrigger;
            mAllWorkTrigger = mAllTrigger;
        }
        else if (mWorkTimeOnly)
        {
            // The alarm is restricted to working hours.
            // Finding the next occurrence during working hours can sometimes take a long time,
            // so mark the next actual trigger as invalid until the calculation completes.
            // Note that reminders are only triggered if the main alarm is during working time.
            if (!mExcludeHolidays)
            {
                // There are no holiday restrictions.
                calcNextWorkingTime(mMainTrigger);
            }
            else if (mHolidays)
            {
                // Holidays are excluded.
                DateTime nextTrigger = mMainTrigger;
                KDateTime kdt;
                for (int i = 0;  i < 20;  ++i)
                {
                    calcNextWorkingTime(nextTrigger);
                    if (!mHolidays->isHoliday(mMainWorkTrigger.date()))
                        return;   // found a non-holiday occurrence
                    kdt = mMainWorkTrigger.effectiveKDateTime();
                    kdt.setTime(QTime(23,59,59));
                    OccurType type = nextOccurrence(kdt, nextTrigger, RETURN_REPETITION);
                    if (!nextTrigger.isValid())
                        break;
                    if (isWorkingTime(nextTrigger.kDateTime()))
                    {
                        int reminder = mReminderMinutes;
                        mMainWorkTrigger = nextTrigger;
                        mAllWorkTrigger = (type & OCCURRENCE_REPEAT) ? mMainWorkTrigger : mMainWorkTrigger.addMins(-reminder);
                        return;   // found a non-holiday occurrence
                    }
                }
                mMainWorkTrigger = mAllWorkTrigger = DateTime();
            }
        }
        else if (mExcludeHolidays  &&  mHolidays)
        {
            // Holidays are excluded.
            DateTime nextTrigger = mMainTrigger;
            KDateTime kdt;
            for (int i = 0;  i < 20;  ++i)
            {
                kdt = nextTrigger.effectiveKDateTime();
                kdt.setTime(QTime(23,59,59));
                OccurType type = nextOccurrence(kdt, nextTrigger, RETURN_REPETITION);
                if (!nextTrigger.isValid())
                    break;
                if (!mHolidays->isHoliday(nextTrigger.date()))
                {
                    int reminder = mReminderMinutes;
                    mMainWorkTrigger = nextTrigger;
                    mAllWorkTrigger = (type & OCCURRENCE_REPEAT) ? mMainWorkTrigger : mMainWorkTrigger.addMins(-reminder);
                    return;   // found a non-holiday occurrence
                }
            }
            mMainWorkTrigger = mAllWorkTrigger = DateTime();
        }
    }
}

/******************************************************************************
* Return the time of the next scheduled occurrence of the event during working
* hours, for an alarm which is restricted to working hours.
* On entry, 'nextTrigger' = the next recurrence or repetition (as returned by
* mainDateTime(true) ).
*/
void KAEvent::Private::calcNextWorkingTime(const DateTime& nextTrigger) const
{
    kDebug() << "next=" << nextTrigger.kDateTime().dateTime();
    mMainWorkTrigger = mAllWorkTrigger = DateTime();

    for (int i = 0;  ;  ++i)
    {
        if (i >= 7)
            return;   // no working days are defined
        if (mWorkDays.testBit(i))
            break;
    }
    KARecurrence::Type recurType = checkRecur();
    KDateTime kdt = nextTrigger.effectiveKDateTime();
    int reminder = mReminderMinutes;
    // Check if it always falls on the same day(s) of the week.
    RecurrenceRule* rrule = mRecurrence->defaultRRuleConst();
    if (!rrule)
        return;   // no recurrence rule!
    unsigned allDaysMask = 0x7F;  // mask bits for all days of week
    bool noWorkPos = false;  // true if no recurrence day position is working day
    QList<RecurrenceRule::WDayPos> pos = rrule->byDays();
    int nDayPos = pos.count();  // number of day positions
    if (nDayPos)
    {
        noWorkPos = true;
        allDaysMask = 0;
        for (int i = 0;  i < nDayPos;  ++i)
        {
            int day = pos[i].day() - 1;  // Monday = 0
            if (mWorkDays.testBit(day))
                noWorkPos = false;   // found a working day occurrence
            allDaysMask |= 1 << day;
        }
        if (noWorkPos  &&  !mRepetition)
            return;   // never occurs on a working day
    }
    DateTime newdt;

    if (mStartDateTime.isDateOnly())
    {
        // It's a date-only alarm.
        // Sub-repetitions also have to be date-only.
        int repeatFreq = mRepetition.intervalDays();
        bool weeklyRepeat = mRepetition && !(repeatFreq % 7);
        Duration interval = mRecurrence->regularInterval();
        if ((interval  &&  !(interval.asDays() % 7))
        ||  nDayPos == 1)
        {
            // It recurs on the same day each week
            if (!mRepetition || weeklyRepeat)
                return;   // any repetitions are also weekly

            // It's a weekly recurrence with a non-weekly sub-repetition.
            // Check one cycle of repetitions for the next one that lands
            // on a working day.
            KDateTime dt(nextTrigger.kDateTime().addDays(1));
            dt.setTime(QTime(0,0,0));
            previousOccurrence(dt, newdt, false);
            if (!newdt.isValid())
                return;   // this should never happen
            kdt = newdt.effectiveKDateTime();
            int day = kdt.date().dayOfWeek() - 1;   // Monday = 0
            for (int repeatNum = mNextRepeat + 1;  ;  ++repeatNum)
            {
                if (repeatNum > mRepetition.count())
                    repeatNum = 0;
                if (repeatNum == mNextRepeat)
                    break;
                if (!repeatNum)
                {
                    nextOccurrence(newdt.kDateTime(), newdt, IGNORE_REPETITION);
                    if (mWorkDays.testBit(day))
                    {
                        mMainWorkTrigger = newdt;
                        mAllWorkTrigger  = mMainWorkTrigger.addMins(-reminder);
                        return;
                    }
                    kdt = newdt.effectiveKDateTime();
                }
                else
                {
                    int inc = repeatFreq * repeatNum;
                    if (mWorkDays.testBit((day + inc) % 7))
                    {
                        kdt = kdt.addDays(inc);
                        kdt.setDateOnly(true);
                        mMainWorkTrigger = mAllWorkTrigger = kdt;
                        return;
                    }
                }
            }
            return;
        }
        if (!mRepetition  ||  weeklyRepeat)
        {
            // It's a date-only alarm with either no sub-repetition or a
            // sub-repetition which always falls on the same day of the week
            // as the recurrence (if any).
            unsigned days = 0;
            for ( ; ; )
            {
                kdt.setTime(QTime(23,59,59));
                nextOccurrence(kdt, newdt, IGNORE_REPETITION);
                if (!newdt.isValid())
                    return;
                kdt = newdt.effectiveKDateTime();
                int day = kdt.date().dayOfWeek() - 1;
                if (mWorkDays.testBit(day))
                    break;   // found a working day occurrence
                // Prevent indefinite looping (which should never happen anyway)
                if ((days & allDaysMask) == allDaysMask)
                    return;  // found a recurrence on every possible day of the week!?!
                days |= 1 << day;
            }
            kdt.setDateOnly(true);
            mMainWorkTrigger = kdt;
            mAllWorkTrigger  = kdt.addSecs(-60 * reminder);
            return;
        }

        // It's a date-only alarm which recurs on different days of the week,
        // as does the sub-repetition.
        // Find the previous recurrence (as opposed to sub-repetition)
        unsigned days = 1 << (kdt.date().dayOfWeek() - 1);
        KDateTime dt(nextTrigger.kDateTime().addDays(1));
        dt.setTime(QTime(0,0,0));
        previousOccurrence(dt, newdt, false);
        if (!newdt.isValid())
            return;   // this should never happen
        kdt = newdt.effectiveKDateTime();
        int day = kdt.date().dayOfWeek() - 1;   // Monday = 0
        for (int repeatNum = mNextRepeat;  ;  repeatNum = 0)
        {
            while (++repeatNum <= mRepetition.count())
            {
                int inc = repeatFreq * repeatNum;
                if (mWorkDays.testBit((day + inc) % 7))
                {
                    kdt = kdt.addDays(inc);
                    kdt.setDateOnly(true);
                    mMainWorkTrigger = mAllWorkTrigger = kdt;
                    return;
                }
                if ((days & allDaysMask) == allDaysMask)
                    return;  // found an occurrence on every possible day of the week!?!
                days |= 1 << day;
            }
            nextOccurrence(kdt, newdt, IGNORE_REPETITION);
            if (!newdt.isValid())
                return;
            kdt = newdt.effectiveKDateTime();
            day = kdt.date().dayOfWeek() - 1;
            if (mWorkDays.testBit(day))
            {
                kdt.setDateOnly(true);
                mMainWorkTrigger = kdt;
                mAllWorkTrigger  = kdt.addSecs(-60 * reminder);
                return;
            }
            if ((days & allDaysMask) == allDaysMask)
                return;  // found an occurrence on every possible day of the week!?!
            days |= 1 << day;
        }
        return;
    }

    // It's a date-time alarm.

    /* Check whether the recurrence or sub-repetition occurs at the same time
     * every day. Note that because of seasonal time changes, a recurrence
     * defined in terms of minutes will vary its time of day even if its value
     * is a multiple of a day (24*60 minutes). Sub-repetitions are considered
     * to repeat at the same time of day regardless of time changes if they
     * are multiples of a day, which doesn't strictly conform to the iCalendar
     * format because this only allows their interval to be recorded in seconds.
     */
    bool recurTimeVaries = (recurType == KARecurrence::MINUTELY);
    bool repeatTimeVaries = (mRepetition  &&  !mRepetition.isDaily());

    if (!recurTimeVaries  &&  !repeatTimeVaries)
    {
        // The alarm always occurs at the same time of day.
        // Check whether it can ever occur during working hours.
        if (!mayOccurDailyDuringWork(kdt))
            return;   // never occurs during working hours

        // Find the next working day it occurs on
        bool repetition = false;
        unsigned days = 0;
        for ( ; ; )
        {
            OccurType type = nextOccurrence(kdt, newdt, RETURN_REPETITION);
            if (!newdt.isValid())
                return;
            repetition = (type & OCCURRENCE_REPEAT);
            kdt = newdt.effectiveKDateTime();
            int day = kdt.date().dayOfWeek() - 1;
            if (mWorkDays.testBit(day))
                break;   // found a working day occurrence
            // Prevent indefinite looping (which should never happen anyway)
            if (!repetition)
            {
                if ((days & allDaysMask) == allDaysMask)
                    return;  // found a recurrence on every possible day of the week!?!
                days |= 1 << day;
            }
        }
        mMainWorkTrigger = nextTrigger;
        mMainWorkTrigger.setDate(kdt.date());
        mAllWorkTrigger = repetition ? mMainWorkTrigger : mMainWorkTrigger.addMins(-reminder);
        return;
    }

    // The alarm occurs at different times of day.
    // We may need to check for a full annual cycle of seasonal time changes, in
    // case it only occurs during working hours after a time change.
    KTimeZone tz = kdt.timeZone();
    if (tz.isValid()  &&  tz.type() == "KSystemTimeZone")
    {
        // It's a system time zone, so fetch full transition information
        KTimeZone ktz = KSystemTimeZones::readZone(tz.name());
        if (ktz.isValid())
            tz = ktz;
    }
    QList<KTimeZone::Transition> tzTransitions = tz.transitions();

    if (recurTimeVaries)
    {
        /* The alarm recurs at regular clock intervals, at different times of day.
         * Note that for this type of recurrence, it's necessary to avoid the
         * performance overhead of Recurrence class calls since these can in the
         * worst case cause the program to hang for a significant length of time.
         * In this case, we can calculate the next recurrence by simply adding the
         * recurrence interval, since KAlarm offers no facility to regularly miss
         * recurrences. (But exception dates/times need to be taken into account.)
         */
        KDateTime kdtRecur;
        int repeatFreq = 0;
        int repeatNum = 0;
        if (mRepetition)
        {
            // It's a repetition inside a recurrence, each of which occurs
            // at different times of day (bearing in mind that the repetition
            // may occur at daily intervals after each recurrence).
            // Find the previous recurrence (as opposed to sub-repetition)
            repeatFreq = mRepetition.intervalSeconds();
            previousOccurrence(kdt.addSecs(1), newdt, false);
            if (!newdt.isValid())
                return;   // this should never happen
            kdtRecur = newdt.effectiveKDateTime();
            repeatNum = kdtRecur.secsTo(kdt) / repeatFreq;
            kdt = kdtRecur.addSecs(repeatNum * repeatFreq);
        }
        else
        {
            // There is no sub-repetition.
            // (N.B. Sub-repetitions can't exist without a recurrence.)
            // Check until the original time wraps round, but ensure that
            // if there are seasonal time changes, that all other subsequent
            // time offsets within the next year are checked.
            // This does not guarantee to find the next working time,
            // particularly if there are exceptions, but it's a
            // reasonable try.
            kdtRecur = kdt;
        }
        QTime firstTime = kdtRecur.time();
        int firstOffset = kdtRecur.utcOffset();
        int currentOffset = firstOffset;
        int dayRecur = kdtRecur.date().dayOfWeek() - 1;   // Monday = 0
        int firstDay = dayRecur;
        QDate finalDate;
        bool subdaily = (repeatFreq < 24*3600);
//        int period = mRecurrence->frequency() % (24*60);  // it is by definition a MINUTELY recurrence
//        int limit = (24*60 + period - 1) / period;  // number of times until recurrence wraps round
        int transitionIndex = -1;
        for (int n = 0;  n < 7*24*60;  ++n)
        {
            if (mRepetition)
            {
                // Check the sub-repetitions for this recurrence
                for ( ; ; )
                {
                    // Find the repeat count to the next start of the working day
                    int inc = subdaily ? nextWorkRepetition(kdt) : 1;
                    repeatNum += inc;
                    if (repeatNum > mRepetition.count())
                        break;
                    kdt = kdt.addSecs(inc * repeatFreq);
                    QTime t = kdt.time();
                    if (t >= mWorkDayStart  &&  t < mWorkDayEnd)
                    {
                        if (mWorkDays.testBit(kdt.date().dayOfWeek() - 1))
                        {
                            mMainWorkTrigger = mAllWorkTrigger = kdt;
                            return;
                        }
                    }
                }
                repeatNum = 0;
            }
            nextOccurrence(kdtRecur, newdt, IGNORE_REPETITION);
            if (!newdt.isValid())
                return;
            kdtRecur = newdt.effectiveKDateTime();
            dayRecur = kdtRecur.date().dayOfWeek() - 1;   // Monday = 0
            QTime t = kdtRecur.time();
            if (t >= mWorkDayStart  &&  t < mWorkDayEnd)
            {
                if (mWorkDays.testBit(dayRecur))
                {
                    mMainWorkTrigger = kdtRecur;
                    mAllWorkTrigger  = kdtRecur.addSecs(-60 * reminder);
                    return;
                }
            }
            if (kdtRecur.utcOffset() != currentOffset)
                currentOffset = kdtRecur.utcOffset();
            if (t == firstTime  &&  dayRecur == firstDay  &&  currentOffset == firstOffset)
            {
                // We've wrapped round to the starting day and time.
                // If there are seasonal time changes, check for up
                // to the next year in other time offsets in case the
                // alarm occurs inside working hours then.
                if (!finalDate.isValid())
                    finalDate = kdtRecur.date();
                int i = tz.transitionIndex(kdtRecur.toUtc().dateTime());
                if (i < 0)
                    return;
                if (i > transitionIndex)
                    transitionIndex = i;
                if (++transitionIndex >= static_cast<int>(tzTransitions.count()))
                    return;
                previousOccurrence(KDateTime(tzTransitions[transitionIndex].time(), KDateTime::UTC), newdt, IGNORE_REPETITION);
                kdtRecur = newdt.effectiveKDateTime();
                if (finalDate.daysTo(kdtRecur.date()) > 365)
                    return;
                firstTime = kdtRecur.time();
                firstOffset = kdtRecur.utcOffset();
                currentOffset = firstOffset;
                firstDay = kdtRecur.date().dayOfWeek() - 1;
            }
            kdt = kdtRecur;
        }
//kDebug()<<"-----exit loop: count="<<limit<<endl;
        return;   // too many iterations
    }

    if (repeatTimeVaries)
    {
        /* There's a sub-repetition which occurs at different times of
         * day, inside a recurrence which occurs at the same time of day.
         * We potentially need to check recurrences starting on each day.
         * Then, it is still possible that a working time sub-repetition
         * could occur immediately after a seasonal time change.
         */
        // Find the previous recurrence (as opposed to sub-repetition)
        int repeatFreq = mRepetition.intervalSeconds();
        previousOccurrence(kdt.addSecs(1), newdt, false);
        if (!newdt.isValid())
            return;   // this should never happen
        KDateTime kdtRecur = newdt.effectiveKDateTime();
        bool recurDuringWork = (kdtRecur.time() >= mWorkDayStart  &&  kdtRecur.time() < mWorkDayEnd);

        // Use the previous recurrence as a base for checking whether
        // our tests have wrapped round to the same time/day of week.
        bool subdaily = (repeatFreq < 24*3600);
        unsigned days = 0;
        bool checkTimeChangeOnly = false;
        int transitionIndex = -1;
        for (int limit = 10;  --limit >= 0;  )
        {
            // Check the next seasonal time change (for an arbitrary 10 times,
            // even though that might not guarantee the correct result)
            QDate dateRecur = kdtRecur.date();
            int dayRecur = dateRecur.dayOfWeek() - 1;   // Monday = 0
            int repeatNum = kdtRecur.secsTo(kdt) / repeatFreq;
            kdt = kdtRecur.addSecs(repeatNum * repeatFreq);

            // Find the next recurrence, which sets the limit on possible sub-repetitions.
            // Note that for a monthly recurrence, for example, a sub-repetition could
            // be defined which is longer than the recurrence interval in short months.
            // In these cases, the sub-repetition is truncated by the following
            // recurrence.
            nextOccurrence(kdtRecur, newdt, IGNORE_REPETITION);
            KDateTime kdtNextRecur = newdt.effectiveKDateTime();

            int repeatsToCheck = mRepetition.count();
            int repeatsDuringWork = 0;  // 0=unknown, 1=does, -1=never
            for ( ; ; )
            {
                // Check the sub-repetitions for this recurrence
                if (repeatsDuringWork >= 0)
                {
                    for ( ; ; )
                    {
                        // Find the repeat count to the next start of the working day
                        int inc = subdaily ? nextWorkRepetition(kdt) : 1;
                        repeatNum += inc;
                        bool pastEnd = (repeatNum > mRepetition.count());
                        if (pastEnd)
                            inc -= repeatNum - mRepetition.count();
                        repeatsToCheck -= inc;
                        kdt = kdt.addSecs(inc * repeatFreq);
                        if (kdtNextRecur.isValid()  &&  kdt >= kdtNextRecur)
                        {
                            // This sub-repetition is past the next recurrence,
                            // so start the check again from the next recurrence.
                            repeatsToCheck = mRepetition.count();
                            break;
                        }
                        if (pastEnd)
                            break;
                        QTime t = kdt.time();
                        if (t >= mWorkDayStart  &&  t < mWorkDayEnd)
                        {
                            if (mWorkDays.testBit(kdt.date().dayOfWeek() - 1))
                            {
                                mMainWorkTrigger = mAllWorkTrigger = kdt;
                                return;
                            }
                            repeatsDuringWork = 1;
                        }
                        else if (!repeatsDuringWork  &&  repeatsToCheck <= 0)
                        {
                            // Sub-repetitions never occur during working hours
                            repeatsDuringWork = -1;
                            break;
                        }
                    }
                }
                repeatNum = 0;
                if (repeatsDuringWork < 0  &&  !recurDuringWork)
                    break;   // it never occurs during working hours

                // Check the next recurrence
                if (!kdtNextRecur.isValid())
                    return;
                if (checkTimeChangeOnly  ||  (days & allDaysMask) == allDaysMask)
                    break;  // found a recurrence on every possible day of the week!?!
                kdtRecur = kdtNextRecur;
                nextOccurrence(kdtRecur, newdt, IGNORE_REPETITION);
                kdtNextRecur = newdt.effectiveKDateTime();
                dateRecur = kdtRecur.date();
                dayRecur = dateRecur.dayOfWeek() - 1;
                if (recurDuringWork  &&  mWorkDays.testBit(dayRecur))
                {
                    mMainWorkTrigger = kdtRecur;
                    mAllWorkTrigger  = kdtRecur.addSecs(-60 * reminder);
                    return;
                }
                days |= 1 << dayRecur;
                kdt = kdtRecur;
            }

            // Find the next recurrence before a seasonal time change,
            // and ensure the time change is after the last one processed.
            checkTimeChangeOnly = true;
            int i = tz.transitionIndex(kdtRecur.toUtc().dateTime());
            if (i < 0)
                return;
            if (i > transitionIndex)
                transitionIndex = i;
            if (++transitionIndex >= static_cast<int>(tzTransitions.count()))
                return;
            kdt = KDateTime(tzTransitions[transitionIndex].time(), KDateTime::UTC);
            previousOccurrence(kdt, newdt, IGNORE_REPETITION);
            kdtRecur = newdt.effectiveKDateTime();
        }
        return;  // not found - give up
    }
}

/******************************************************************************
* Find the repeat count to the next start of a working day.
* This allows for possible daylight saving time changes during the repetition.
* Use for repetitions which occur at different times of day.
*/
int KAEvent::Private::nextWorkRepetition(const KDateTime& pre) const
{
    KDateTime nextWork(pre);
    if (pre.time() < mWorkDayStart)
        nextWork.setTime(mWorkDayStart);
    else
    {
        int preDay = pre.date().dayOfWeek() - 1;   // Monday = 0
        for (int n = 1;  ;  ++n)
        {
            if (n >= 7)
                return mRepetition.count() + 1;  // should never happen
            if (mWorkDays.testBit((preDay + n) % 7))
            {
                nextWork = nextWork.addDays(n);
                nextWork.setTime(mWorkDayStart);
                break;
            }
        }
    }
    return (pre.secsTo(nextWork) - 1) / mRepetition.intervalSeconds() + 1;
}

/******************************************************************************
* Check whether an alarm which recurs at the same time of day can possibly
* occur during working hours.
* This does not determine whether it actually does, but rather whether it could
* potentially given enough repetitions.
* Reply = false if it can never occur during working hours, true if it might.
*/
bool KAEvent::Private::mayOccurDailyDuringWork(const KDateTime& kdt) const
{
    if (!kdt.isDateOnly()
    &&  (kdt.time() < mWorkDayStart || kdt.time() >= mWorkDayEnd))
        return false;   // its time is outside working hours
    // Check if it always occurs on the same day of the week
    Duration interval = mRecurrence->regularInterval();
    if (interval  &&  interval.isDaily()  &&  !(interval.asDays() % 7))
    {
        // It recurs weekly
        if (!mRepetition  ||  (mRepetition.isDaily() && !(mRepetition.intervalDays() % 7)))
            return false;   // any repetitions are also weekly
        // Repetitions are daily. Check if any occur on working days
        // by checking the first recurrence and up to 6 repetitions.
        int day = mRecurrence->startDateTime().date().dayOfWeek() - 1;   // Monday = 0
        int repeatDays = mRepetition.intervalDays();
        int maxRepeat = (mRepetition.count() < 6) ? mRepetition.count() : 6;
        for (int i = 0;  !mWorkDays.testBit(day);  ++i, day = (day + repeatDays) % 7)
        {
            if (i >= maxRepeat)
                return false;  // no working day occurrences
        }
    }
    return true;
}

/******************************************************************************
* Check whether a date/time is during working hours and/or holidays, depending
* on the flags set for the specified event.
*/
bool KAEvent::Private::isWorkingTime(const KDateTime& dt) const
{
    if ((mWorkTimeOnly  &&  !mWorkDays.testBit(dt.date().dayOfWeek() - 1))
    ||  (mExcludeHolidays  &&  mHolidays  &&  mHolidays->isHoliday(dt.date())))
        return false;
    if (!mWorkTimeOnly)
        return true;
    return dt.isDateOnly()
       ||  (dt.time() >= mWorkDayStart  &&  dt.time() < mWorkDayEnd);
}

int KAEvent::Private::flags() const
{
    if (mSpeak)
        const_cast<KAEvent::Private*>(this)->mBeep = false;
    return baseFlags()
         | (mBeep                       ? BEEP : 0)
         | (mRepeatSound                ? REPEAT_SOUND : 0)
         | (mEmailBcc                   ? EMAIL_BCC : 0)
         | (mStartDateTime.isDateOnly() ? ANY_TIME : 0)
         | (mDeferral > 0               ? DEFERRAL : 0)
         | (mSpeak                      ? SPEAK : 0)
         | (mConfirmAck                 ? CONFIRM_ACK : 0)
         | (mCommandXterm               ? EXEC_IN_XTERM : 0)
         | (mCommandDisplay             ? DISPLAY_COMMAND : 0)
         | (mCopyToKOrganizer           ? COPY_KORGANIZER : 0)
         | (mExcludeHolidays            ? EXCL_HOLIDAYS : 0)
         | (mWorkTimeOnly               ? WORK_TIME_ONLY : 0)
         | (mDisplaying                 ? DISPLAYING_ : 0)
         | (mEnabled                    ? 0 : DISABLED);
}

KAEvent::Actions KAEvent::actions() const
{
    switch (d->mActionType)
    {
        case KAAlarmEventBase::T_MESSAGE:
        case KAAlarmEventBase::T_FILE:     return ACT_DISPLAY;
        case KAAlarmEventBase::T_COMMAND:  return d->mCommandDisplay ? ACT_DISPLAY_COMMAND : ACT_COMMAND;
        case KAAlarmEventBase::T_EMAIL:    return ACT_EMAIL;
        case KAAlarmEventBase::T_AUDIO:    return ACT_AUDIO;
        default:                           return ACT_NONE;
    }
}

/******************************************************************************
* Update an existing KCal::Event with the KAEvent::Private data.
* If 'setCustomProperties' is true, all the KCal::Event's existing custom
* properties are cleared and replaced with the KAEvent's custom properties. If
* false, the KCal::Event's non-KAlarm custom properties are left untouched.
*/
#ifdef USE_AKONADI
bool KAEvent::Private::updateKCalEvent(const Event::Ptr& ev, UidAction uidact, bool setCustomProperties) const
#else
bool KAEvent::Private::updateKCalEvent(Event* ev, UidAction uidact) const
#endif
{
    // If it's an archived event, the event start date/time will be adjusted to its original
    // value instead of its next occurrence, and the expired main alarm will be reinstated.
    bool archived = (mCategory == KAlarm::CalEvent::ARCHIVED);

    if (!ev
    ||  (uidact == UID_CHECK  &&  !mEventID.isEmpty()  &&  mEventID != ev->uid())
    ||  (!mAlarmCount  &&  (!archived || !mMainExpired)))
        return false;

    ev->startUpdates();   // prevent multiple update notifications
    checkRecur();         // ensure recurrence/repetition data is consistent
    bool readOnly = ev->isReadOnly();
    if (uidact == KAEvent::UID_SET)
        ev->setUid(mEventID);
#ifdef USE_AKONADI
    ev->setReadOnly(mReadOnly);
#else
    ev->setReadOnly(false);
#endif
    ev->setTransparency(Event::Transparent);

    // Set up event-specific data

    // Set up custom properties.
#ifdef USE_AKONADI
    if (setCustomProperties)
        ev->setCustomProperties(mCustomProperties);
#endif
    ev->removeCustomProperty(KAlarm::Calendar::APPNAME, FLAGS_PROPERTY);
    ev->removeCustomProperty(KAlarm::Calendar::APPNAME, NEXT_RECUR_PROPERTY);
    ev->removeCustomProperty(KAlarm::Calendar::APPNAME, REPEAT_PROPERTY);
    ev->removeCustomProperty(KAlarm::Calendar::APPNAME, LOG_PROPERTY);
    ev->removeCustomProperty(KAlarm::Calendar::APPNAME, ARCHIVE_PROPERTY);

    QString param;
    if (mCategory == KAlarm::CalEvent::DISPLAYING)
    {
#ifdef USE_AKONADI
        param = QString::number(mCollectionId);
#else
        param = mResourceId;
#endif
        if (mDisplayingDefer)
            param += SC + DISP_DEFER;
        if (mDisplayingEdit)
            param += SC + DISP_EDIT;
    }
#ifdef USE_AKONADI
    KAlarm::CalEvent::setStatus(ev, mCategory, param);
#else
    KAlarm::CalEvent::setStatus(ev, mCategory, param);
#endif
    QStringList flags;
    if (mStartDateTime.isDateOnly())
        flags += DATE_ONLY_FLAG;
    if (mConfirmAck)
        flags += CONFIRM_ACK_FLAG;
    if (mEmailBcc)
        flags += EMAIL_BCC_FLAG;
    if (mCopyToKOrganizer)
        flags += KORGANIZER_FLAG;
    if (mExcludeHolidays)
        flags += EXCLUDE_HOLIDAYS_FLAG;
    if (mWorkTimeOnly)
        flags += WORK_TIME_ONLY_FLAG;
    if (mLateCancel)
        (flags += (mAutoClose ? AUTO_CLOSE_FLAG : LATE_CANCEL_FLAG)) += QString::number(mLateCancel);
    if (mDeferDefaultMinutes)
    {
        QString param = QString::number(mDeferDefaultMinutes);
        if (mDeferDefaultDateOnly)
            param += 'D';
        (flags += DEFER_FLAG) += param;
    }
    if (!mTemplateName.isEmpty()  &&  mTemplateAfterTime >= 0)
        (flags += TEMPL_AFTER_TIME_FLAG) += QString::number(mTemplateAfterTime);
    if (mKMailSerialNumber)
        (flags += KMAIL_SERNUM_FLAG) += QString::number(mKMailSerialNumber);
    if (!flags.isEmpty())
        ev->setCustomProperty(KAlarm::Calendar::APPNAME, FLAGS_PROPERTY, flags.join(SC));

    if (mCommandXterm)
        ev->setCustomProperty(KAlarm::Calendar::APPNAME, LOG_PROPERTY, xtermURL);
    else if (mCommandDisplay)
        ev->setCustomProperty(KAlarm::Calendar::APPNAME, LOG_PROPERTY, displayURL);
    else if (!mLogFile.isEmpty())
        ev->setCustomProperty(KAlarm::Calendar::APPNAME, LOG_PROPERTY, mLogFile);
    if (mArchive  &&  !archived)
    {
        QStringList params;
        if (mReminderMinutes  &&  !mReminderActive)
        {
            if (mReminderOnceOnly)
                params += ARCHIVE_REMINDER_ONCE_TYPE;
            char unit = 'M';
            int count = mReminderMinutes;
            if (count % 1440 == 0)
            {
                unit = 'D';
                count /= 1440;
            }
            else if (count % 60 == 0)
            {
                unit = 'H';
                count /= 60;
            }
            params += QString("%1%2").arg(count).arg(unit);
        }
        if (mArchiveRepeatAtLogin)
            params += AT_LOGIN_TYPE;
        QString param;
        if (params.count() > 0)
            param = params.join(SC);
        else
            param = QLatin1String("0");
        ev->setCustomProperty(KAlarm::Calendar::APPNAME, ARCHIVE_PROPERTY, param);
    }

    ev->setCustomStatus(mEnabled ? QString() : DISABLED_STATUS);
    ev->setRevision(mRevision);
    ev->clearAlarms();

    /* Always set DTSTART as date/time, and use the category "DATE" to indicate
     * a date-only event, instead of calling setAllDay(). This is necessary to
     * allow a time zone to be specified for a date-only event. Also, KAlarm
     * allows the alarm to float within the 24-hour period defined by the
     * start-of-day time (which is user-dependent and therefore can't be
     * written into the calendar) rather than midnight to midnight, and there
     * is no RFC2445 conformant way to specify this. 
     * RFC2445 states that alarm trigger times specified in absolute terms
     * (rather than relative to DTSTART or DTEND) can only be specified as a
     * UTC DATE-TIME value. So always use a time relative to DTSTART instead of
     * an absolute time.
     */
    ev->setDtStart(mStartDateTime.calendarKDateTime());
    ev->setAllDay(false);
    ev->setHasEndDate(false);

    DateTime dtMain = archived ? mStartDateTime : mNextMainDateTime;
    int      ancillaryType = 0;   // 0 = invalid, 1 = time, 2 = offset
    DateTime ancillaryTime;       // time for ancillary alarms (pre-action, extra audio, etc)
    int      ancillaryOffset = 0; // start offset for ancillary alarms
    if (!mMainExpired  ||  archived)
    {
        /* The alarm offset must always be zero for the main alarm. To determine
         * which recurrence is due, the property X-KDE-KALARM_NEXTRECUR is used.
         * If the alarm offset was non-zero, exception dates and rules would not
         * work since they apply to the event time, not the alarm time.
         */
        if (!archived  &&  checkRecur() != KARecurrence::NO_RECUR)
        {
            QDateTime dt = mNextMainDateTime.kDateTime().toTimeSpec(mStartDateTime.timeSpec()).dateTime();
            ev->setCustomProperty(KAlarm::Calendar::APPNAME, NEXT_RECUR_PROPERTY,
                                  dt.toString(mNextMainDateTime.isDateOnly() ? "yyyyMMdd" : "yyyyMMddThhmmss"));
        }
        // Add the main alarm
        initKCalAlarm(ev, 0, QStringList(), KAAlarm::MAIN_ALARM);
        ancillaryOffset = 0;
        ancillaryType = dtMain.isValid() ? 2 : 0;
    }
    else if (mRepetition)
    {
        // Alarm repetition is normally held in the main alarm, but since
        // the main alarm has expired, store in a custom property.
        QString param = QString("%1:%2").arg(mRepetition.intervalMinutes()).arg(mRepetition.count());
        ev->setCustomProperty(KAlarm::Calendar::APPNAME, REPEAT_PROPERTY, param);
    }

    // Add subsidiary alarms
    if (mRepeatAtLogin  ||  (mArchiveRepeatAtLogin && archived))
    {
        DateTime dtl;
        if (mArchiveRepeatAtLogin)
            dtl = mStartDateTime.calendarKDateTime().addDays(-1);
        else if (mAtLoginDateTime.isValid())
            dtl = mAtLoginDateTime;
        else if (mStartDateTime.isDateOnly())
            dtl = DateTime(KDateTime::currentLocalDate().addDays(-1), mStartDateTime.timeSpec());
        else
            dtl = KDateTime::currentUtcDateTime();
        initKCalAlarm(ev, dtl, QStringList(AT_LOGIN_TYPE));
        if (!ancillaryType  &&  dtl.isValid())
        {
            ancillaryTime = dtl;
            ancillaryType = 1;
        }
    }
    if (mReminderMinutes  &&  (mReminderActive || archived))
    {
        initKCalAlarm(ev, -mReminderMinutes * 60, QStringList(mReminderOnceOnly ? REMINDER_ONCE_TYPE : REMINDER_TYPE));
        if (!ancillaryType)
        {
            ancillaryOffset = -mReminderMinutes * 60;
            ancillaryType = 2;
        }
    }
    if (mDeferral > 0)
    {
        DateTime nextDateTime = mNextMainDateTime;
        if (mMainExpired)
        {
            if (checkRecur() == KARecurrence::NO_RECUR)
                nextDateTime = mStartDateTime;
            else if (!archived)
            {
                // It's a deferral of an expired recurrence.
                // Need to ensure that the alarm offset is to an occurrence
                // which isn't excluded by an exception - otherwise, it will
                // never be triggered. So choose the first recurrence which
                // isn't an exception.
                KDateTime dt = mRecurrence->getNextDateTime(mStartDateTime.addDays(-1).kDateTime());
                dt.setDateOnly(mStartDateTime.isDateOnly());
                nextDateTime = dt;
            }
        }
        int startOffset;
        QStringList list;
        if (mDeferralTime.isDateOnly())
        {
            startOffset = nextDateTime.secsTo(mDeferralTime.calendarKDateTime());
            list += DATE_DEFERRAL_TYPE;
        }
        else
        {
            startOffset = nextDateTime.calendarKDateTime().secsTo(mDeferralTime.calendarKDateTime());
            list += TIME_DEFERRAL_TYPE;
        }
        if (mDeferral == REMINDER_DEFERRAL)
            list += mReminderOnceOnly ? REMINDER_ONCE_TYPE : REMINDER_TYPE;
        initKCalAlarm(ev, startOffset, list);
        if (!ancillaryType  &&  mDeferralTime.isValid())
        {
            ancillaryOffset = startOffset;
            ancillaryType = 2;
        }
    }
    if (!mTemplateName.isEmpty())
        ev->setSummary(mTemplateName);
    else if (mDisplaying)
    {
        QStringList list(DISPLAYING_TYPE);
        if (mDisplayingFlags & REPEAT_AT_LOGIN)
            list += AT_LOGIN_TYPE;
        else if (mDisplayingFlags & DEFERRAL)
        {
            if (mDisplayingFlags & TIMED_FLAG)
                list += TIME_DEFERRAL_TYPE;
            else
                list += DATE_DEFERRAL_TYPE;
        }
        if (mDisplayingFlags & REMINDER)
            list += mReminderOnceOnly ? REMINDER_ONCE_TYPE : REMINDER_TYPE;
        initKCalAlarm(ev, mDisplayingTime, list);
        if (!ancillaryType  &&  mDisplayingTime.isValid())
        {
            ancillaryTime = mDisplayingTime;
            ancillaryType = 1;
        }
    }
    if ((mBeep  ||  mSpeak  ||  !mAudioFile.isEmpty())  &&  mActionType != T_AUDIO)
    {
        // A sound is specified
        if (ancillaryType == 2)
            initKCalAlarm(ev, ancillaryOffset, QStringList(), KAAlarm::AUDIO_ALARM);
        else
            initKCalAlarm(ev, ancillaryTime, QStringList(), KAAlarm::AUDIO_ALARM);
    }
    if (!mPreAction.isEmpty())
    {
        // A pre-display action is specified
        if (ancillaryType == 2)
            initKCalAlarm(ev, ancillaryOffset, QStringList(PRE_ACTION_TYPE), KAAlarm::PRE_ACTION_ALARM);
        else
            initKCalAlarm(ev, ancillaryTime, QStringList(PRE_ACTION_TYPE), KAAlarm::PRE_ACTION_ALARM);
    }
    if (!mPostAction.isEmpty())
    {
        // A post-display action is specified
        if (ancillaryType == 2)
            initKCalAlarm(ev, ancillaryOffset, QStringList(POST_ACTION_TYPE), KAAlarm::POST_ACTION_ALARM);
        else
            initKCalAlarm(ev, ancillaryTime, QStringList(POST_ACTION_TYPE), KAAlarm::POST_ACTION_ALARM);
    }

    if (mRecurrence)
        mRecurrence->writeRecurrence(*ev->recurrence());
    else
        ev->clearRecurrence();
    if (mSaveDateTime.isValid())
        ev->setCreated(mSaveDateTime);
    ev->setReadOnly(readOnly);
    ev->endUpdates();     // finally issue an update notification
    return true;
}

/******************************************************************************
* Create a new alarm for a libkcal event, and initialise it according to the
* alarm action. If 'types' is non-null, it is appended to the X-KDE-KALARM-TYPE
* property value list.
*/
#ifdef USE_AKONADI
Alarm::Ptr KAEvent::Private::initKCalAlarm(const Event::Ptr& event, const DateTime& dt, const QStringList& types, KAAlarm::Type type) const
#else
Alarm* KAEvent::Private::initKCalAlarm(Event* event, const DateTime& dt, const QStringList& types, KAAlarm::Type type) const
#endif
{
    int startOffset = dt.isDateOnly() ? mStartDateTime.secsTo(dt)
                                      : mStartDateTime.calendarKDateTime().secsTo(dt.calendarKDateTime());
    return initKCalAlarm(event, startOffset, types, type);
}

#ifdef USE_AKONADI
Alarm::Ptr KAEvent::Private::initKCalAlarm(const Event::Ptr& event, int startOffsetSecs, const QStringList& types, KAAlarm::Type type) const
#else
Alarm* KAEvent::Private::initKCalAlarm(Event* event, int startOffsetSecs, const QStringList& types, KAAlarm::Type type) const
#endif
{
    QStringList alltypes;
#ifdef USE_AKONADI
    Alarm::Ptr alarm = event->newAlarm();
#else
    Alarm* alarm = event->newAlarm();
#endif
    alarm->setEnabled(true);
    if (type != KAAlarm::MAIN_ALARM)
    {
        // RFC2445 specifies that absolute alarm times must be stored as a UTC DATE-TIME value.
        // Set the alarm time as an offset to DTSTART for the reasons described in updateKCalEvent().
        alarm->setStartOffset(startOffsetSecs);
    }

    switch (type)
    {
        case KAAlarm::AUDIO_ALARM:
            setAudioAlarm(alarm);
            if (mSpeak)
                alarm->setCustomProperty(KAlarm::Calendar::APPNAME, SPEAK_PROPERTY, QLatin1String("Y"));
            if (mRepeatSound)
            {
                alarm->setRepeatCount(-1);
                alarm->setSnoozeTime(0);
            }
            break;
        case KAAlarm::PRE_ACTION_ALARM:
            setProcedureAlarm(alarm, mPreAction);
            if (mCancelOnPreActErr)
                alarm->setCustomProperty(KAlarm::Calendar::APPNAME, CANCEL_ON_ERROR_PROPERTY, QLatin1String("Y"));
            if (mDontShowPreActErr)
                alarm->setCustomProperty(KAlarm::Calendar::APPNAME, DONT_SHOW_ERROR_PROPERTY, QLatin1String("Y"));
            break;
        case KAAlarm::POST_ACTION_ALARM:
            setProcedureAlarm(alarm, mPostAction);
            break;
        case KAAlarm::MAIN_ALARM:
            alarm->setSnoozeTime(mRepetition.interval());
            alarm->setRepeatCount(mRepetition.count());
            if (mRepetition)
                alarm->setCustomProperty(KAlarm::Calendar::APPNAME, NEXT_REPEAT_PROPERTY,
                                         QString::number(mNextRepeat));
            // fall through to INVALID_ALARM
        case KAAlarm::INVALID_ALARM:
        {
            bool display = false;
            switch (mActionType)
            {
                case T_FILE:
                    alltypes += FILE_TYPE;
                    // fall through to T_MESSAGE
                case T_MESSAGE:
                    alarm->setDisplayAlarm(AlarmText::toCalendarText(mText));
                    display = true;
                    break;
                case T_COMMAND:
                    if (mCommandScript)
                        alarm->setProcedureAlarm("", mText);
                    else
                        setProcedureAlarm(alarm, mText);
                    display = mCommandDisplay;
                    break;
                case T_EMAIL:
                    alarm->setEmailAlarm(mEmailSubject, mText, mEmailAddresses, mEmailAttachments);
                    if (mEmailFromIdentity)
                        alarm->setCustomProperty(KAlarm::Calendar::APPNAME, EMAIL_ID_PROPERTY, QString::number(mEmailFromIdentity));
                    break;
                case T_AUDIO:
                    setAudioAlarm(alarm);
                    if (mRepeatSound)
                        alltypes += SOUND_REPEAT_TYPE;
                    break;
            }
            if (display)
                alarm->setCustomProperty(KAlarm::Calendar::APPNAME, FONT_COLOUR_PROPERTY,
                                         QString::fromLatin1("%1;%2;%3").arg(mBgColour.name())
                                                                        .arg(mFgColour.name())
                                                                        .arg(mUseDefaultFont ? QString() : mFont.toString()));
            break;
        }
        case KAAlarm::REMINDER_ALARM:
        case KAAlarm::DEFERRED_ALARM:
        case KAAlarm::DEFERRED_REMINDER_ALARM:
        case KAAlarm::AT_LOGIN_ALARM:
        case KAAlarm::DISPLAYING_ALARM:
            break;
    }
    alltypes += types;
    if (alltypes.count() > 0)
        alarm->setCustomProperty(KAlarm::Calendar::APPNAME, TYPE_PROPERTY, alltypes.join(","));
    return alarm;
}

/******************************************************************************
* Set the specified alarm to be an audio alarm with the given file name.
*/
#ifdef USE_AKONADI
void KAEvent::Private::setAudioAlarm(const Alarm::Ptr& alarm) const
#else
void KAEvent::Private::setAudioAlarm(Alarm* alarm) const
#endif
{
    alarm->setAudioAlarm(mAudioFile);  // empty for a beep or for speaking
    if (mSoundVolume >= 0)
        alarm->setCustomProperty(KAlarm::Calendar::APPNAME, VOLUME_PROPERTY,
                      QString::fromLatin1("%1;%2;%3").arg(QString::number(mSoundVolume, 'f', 2))
                                                     .arg(QString::number(mFadeVolume, 'f', 2))
                                                     .arg(mFadeSeconds));
}

/******************************************************************************
* Return the alarm of the specified type.
*/
KAAlarm KAEvent::Private::alarm(KAAlarm::Type type) const
{
    checkRecur();     // ensure recurrence/repetition data is consistent
    KAAlarm al;       // this sets type to INVALID_ALARM
    if (mAlarmCount)
    {
        al.mEventID        = mEventID;
        al.mActionType     = mActionType;
        al.mText           = mText;
        al.mBgColour       = mBgColour;
        al.mFgColour       = mFgColour;
        al.mFont           = mFont;
        al.mUseDefaultFont = mUseDefaultFont;
        al.mRepeatAtLogin  = false;
        al.mDeferred       = false;
        al.mLateCancel     = mLateCancel;
        al.mAutoClose      = mAutoClose;
        al.mCommandScript  = mCommandScript;
        switch (type)
        {
            case KAAlarm::MAIN_ALARM:
                if (!mMainExpired)
                {
                    al.mType             = KAAlarm::MAIN__ALARM;
                    al.mNextMainDateTime = mNextMainDateTime;
                    al.mRepetition       = mRepetition;
                    al.mNextRepeat       = mNextRepeat;
                }
                break;
            case KAAlarm::REMINDER_ALARM:
                if (mReminderActive)
                {
                    al.mType = KAAlarm::REMINDER__ALARM;
                    if (mReminderOnceOnly)
                        al.mNextMainDateTime = mStartDateTime.addMins(-mReminderMinutes);
                    else
                        al.mNextMainDateTime = mNextMainDateTime.addMins(-mReminderMinutes);
                }
                break;
            case KAAlarm::DEFERRED_REMINDER_ALARM:
                if (mDeferral != REMINDER_DEFERRAL)
                    break;
                // fall through to DEFERRED_ALARM
            case KAAlarm::DEFERRED_ALARM:
                if (mDeferral > 0)
                {
                    al.mType = static_cast<KAAlarm::SubType>((mDeferral == REMINDER_DEFERRAL ? KAAlarm::DEFERRED_REMINDER_ALARM : KAAlarm::DEFERRED_ALARM)
                                                             | (mDeferralTime.isDateOnly() ? 0 : KAAlarm::TIMED_DEFERRAL_FLAG));
                    al.mNextMainDateTime = mDeferralTime;
                    al.mDeferred         = true;
                }
                break;
            case KAAlarm::AT_LOGIN_ALARM:
                if (mRepeatAtLogin)
                {
                    al.mType             = KAAlarm::AT_LOGIN__ALARM;
                    al.mNextMainDateTime = mAtLoginDateTime;
                    al.mRepeatAtLogin    = true;
                    al.mLateCancel       = 0;
                    al.mAutoClose        = false;
                }
                break;
            case KAAlarm::DISPLAYING_ALARM:
                if (mDisplaying)
                {
                    al.mType             = KAAlarm::DISPLAYING__ALARM;
                    al.mNextMainDateTime = mDisplayingTime;
                }
                break;
            case KAAlarm::AUDIO_ALARM:
            case KAAlarm::PRE_ACTION_ALARM:
            case KAAlarm::POST_ACTION_ALARM:
            case KAAlarm::INVALID_ALARM:
            default:
                break;
        }
    }
    return al;
}

/******************************************************************************
* Return the main alarm for the event.
* If the main alarm does not exist, one of the subsidiary ones is returned if
* possible.
* N.B. a repeat-at-login alarm can only be returned if it has been read from/
* written to the calendar file.
*/
KAAlarm KAEvent::Private::firstAlarm() const
{
    if (mAlarmCount)
    {
        if (!mMainExpired)
            return alarm(KAAlarm::MAIN_ALARM);
        return nextAlarm(KAAlarm::MAIN_ALARM);
    }
    return KAAlarm();
}

/******************************************************************************
* Return the next alarm for the event, after the specified alarm.
* N.B. a repeat-at-login alarm can only be returned if it has been read from/
* written to the calendar file.
*/
KAAlarm KAEvent::Private::nextAlarm(KAAlarm::Type prevType) const
{
    switch (prevType)
    {
        case KAAlarm::MAIN_ALARM:
            if (mReminderActive)
                return alarm(KAAlarm::REMINDER_ALARM);
            // fall through to REMINDER_ALARM
        case KAAlarm::REMINDER_ALARM:
            // There can only be one deferral alarm
            if (mDeferral == REMINDER_DEFERRAL)
                return alarm(KAAlarm::DEFERRED_REMINDER_ALARM);
            if (mDeferral == NORMAL_DEFERRAL)
                return alarm(KAAlarm::DEFERRED_ALARM);
            // fall through to DEFERRED_ALARM
        case KAAlarm::DEFERRED_REMINDER_ALARM:
        case KAAlarm::DEFERRED_ALARM:
            if (mRepeatAtLogin)
                return alarm(KAAlarm::AT_LOGIN_ALARM);
            // fall through to AT_LOGIN_ALARM
        case KAAlarm::AT_LOGIN_ALARM:
            if (mDisplaying)
                return alarm(KAAlarm::DISPLAYING_ALARM);
            // fall through to DISPLAYING_ALARM
        case KAAlarm::DISPLAYING_ALARM:
            // fall through to default
        case KAAlarm::AUDIO_ALARM:
        case KAAlarm::PRE_ACTION_ALARM:
        case KAAlarm::POST_ACTION_ALARM:
        case KAAlarm::INVALID_ALARM:
        default:
            break;
    }
    return KAAlarm();
}

/******************************************************************************
* Remove the alarm of the specified type from the event.
* This must only be called to remove an alarm which has expired, not to
* reconfigure the event.
*/
void KAEvent::Private::removeExpiredAlarm(KAAlarm::Type type)
{
    int count = mAlarmCount;
    switch (type)
    {
        case KAAlarm::MAIN_ALARM:
            mAlarmCount = 0;    // removing main alarm - also remove subsidiary alarms
            break;
        case KAAlarm::AT_LOGIN_ALARM:
            if (mRepeatAtLogin)
            {
                // Remove the at-login alarm, but keep a note of it for archiving purposes
                mArchiveRepeatAtLogin = true;
                mRepeatAtLogin = false;
                --mAlarmCount;
            }
            break;
        case KAAlarm::REMINDER_ALARM:
            // Remove any reminder alarm, but keep a note of it for archiving purposes
            // and for restoration after the next recurrence.
            activate_reminder(false);
            break;
        case KAAlarm::DEFERRED_REMINDER_ALARM:
        case KAAlarm::DEFERRED_ALARM:
            set_deferral(NO_DEFERRAL);
            break;
        case KAAlarm::DISPLAYING_ALARM:
            if (mDisplaying)
            {
                mDisplaying = false;
                --mAlarmCount;
            }
            break;
        case KAAlarm::AUDIO_ALARM:
        case KAAlarm::PRE_ACTION_ALARM:
        case KAAlarm::POST_ACTION_ALARM:
        case KAAlarm::INVALID_ALARM:
        default:
            break;
    }
    if (mAlarmCount != count)
        calcTriggerTimes();
}

/******************************************************************************
* Defer the event to the specified time.
* If the main alarm time has passed, the main alarm is marked as expired.
* If 'adjustRecurrence' is true, ensure that the next scheduled recurrence is
* after the current time.
* Reply = true if a repetition has been deferred.
*/
bool KAEvent::Private::defer(const DateTime& dateTime, bool reminder, bool adjustRecurrence)
{
    startChanges();   // prevent multiple trigger time evaluation here
    bool result = false;
    bool setNextRepetition = false;
    bool checkRepetition = false;
    if (checkRecur() == KARecurrence::NO_RECUR)
    {
        // Deferring a non-recurring alarm
        if (mReminderMinutes)
        {
            if (dateTime < mNextMainDateTime.effectiveKDateTime())
            {
                set_deferral(REMINDER_DEFERRAL);   // defer reminder alarm
                mDeferralTime = dateTime;
                mChanged = true;
            }
            else
            {
                // Deferring past the main alarm time, so adjust any existing deferral
                if (mReminderActive  ||  mDeferral == REMINDER_DEFERRAL)
                {
                    set_deferral(NO_DEFERRAL);
                    mChanged = true;
                }
            }
            if (mReminderActive)
            {
                activate_reminder(false);
                mChanged = true;
            }
        }
        if (mDeferral != REMINDER_DEFERRAL)
        {
            // We're deferring the main alarm, not a reminder.
            // Main alarm has now expired.
            mNextMainDateTime = mDeferralTime = dateTime;
            set_deferral(NORMAL_DEFERRAL);
            mChanged = true;
            if (!mMainExpired)
            {
                // Mark the alarm as expired now
                mMainExpired = true;
                --mAlarmCount;
                if (mRepeatAtLogin)
                {
                    // Remove the repeat-at-login alarm, but keep a note of it for archiving purposes
                    mArchiveRepeatAtLogin = true;
                    mRepeatAtLogin = false;
                    --mAlarmCount;
                }
            }
        }
    }
    else if (reminder)
    {
        // Deferring a reminder for a recurring alarm
        if (dateTime >= mNextMainDateTime.effectiveKDateTime())
            set_deferral(NO_DEFERRAL);    // (error)
        else
        {
            set_deferral(REMINDER_DEFERRAL);
            mDeferralTime = dateTime;
            checkRepetition = true;
        }
        mChanged = true;
    }
    else
    {
        // Deferring a recurring alarm
        mDeferralTime = dateTime;
        mChanged = true;
        if (mDeferral <= 0)
            set_deferral(NORMAL_DEFERRAL);
        if (adjustRecurrence)
        {
            KDateTime now = KDateTime::currentUtcDateTime();
            if (mainEndRepeatTime() < now)
            {
                // The last repetition (if any) of the current recurrence has already passed.
                // Adjust to the next scheduled recurrence after now.
                if (!mMainExpired  &&  setNextOccurrence(now) == NO_OCCURRENCE)
                {
                    mMainExpired = true;
                    --mAlarmCount;
                }
            }
            else
                setNextRepetition = mRepetition;
        }
        else
            checkRepetition = true;
    }
    if (checkRepetition)
        setNextRepetition = (mRepetition  &&  mDeferralTime < mainEndRepeatTime());
    if (setNextRepetition)
    {
        // The alarm is repeated, and we're deferring to a time before the last repetition.
        // Set the next scheduled repetition to the one after the deferral.
        if (mNextMainDateTime >= mDeferralTime)
            mNextRepeat = 0;
        else
            mNextRepeat = mRepetition.nextRepeatCount(mNextMainDateTime.kDateTime(), mDeferralTime.kDateTime());
        mChanged = true;
    }
    endChanges();
    return result;
}

/******************************************************************************
* Cancel any deferral alarm.
*/
void KAEvent::Private::cancelDefer()
{
    if (mDeferral > 0)
    {
        mDeferralTime = DateTime();
        set_deferral(NO_DEFERRAL);
        calcTriggerTimes();
    }
}

/******************************************************************************
* Find the latest time which the alarm can currently be deferred to.
*/
DateTime KAEvent::Private::deferralLimit(DeferLimitType* limitType) const
{
    DeferLimitType ltype = LIMIT_NONE;
    DateTime endTime;
    bool recurs = (checkRecur() != KARecurrence::NO_RECUR);
    if (recurs)
    {
        // It's a recurring alarm. Don't allow it to be deferred past its
        // next occurrence or repetition.
        DateTime reminderTime;
        KDateTime now = KDateTime::currentUtcDateTime();
        OccurType type = nextOccurrence(now, endTime, RETURN_REPETITION);
        if (type & OCCURRENCE_REPEAT)
            ltype = LIMIT_REPETITION;
        else if (type == NO_OCCURRENCE)
            ltype = LIMIT_NONE;
        else if (mReminderActive  &&  (now < (reminderTime = endTime.addMins(-mReminderMinutes))))
        {
            endTime = reminderTime;
            ltype = LIMIT_REMINDER;
        }
        else if (type == FIRST_OR_ONLY_OCCURRENCE  &&  !recurs)
            ltype = LIMIT_REPETITION;
        else
            ltype = LIMIT_RECURRENCE;
    }
    else if (mReminderMinutes
         &&  KDateTime::currentUtcDateTime() < mNextMainDateTime.effectiveKDateTime())
    {
        // It's a reminder alarm. Don't allow it to be deferred past its main alarm time.
        endTime = mNextMainDateTime;
        ltype = LIMIT_REMINDER;
    }
    if (ltype != LIMIT_NONE)
        endTime = endTime.addMins(-1);
    if (limitType)
        *limitType = ltype;
    return endTime;
}

#ifndef USE_AKONADI
/******************************************************************************
* Initialise the command last error status of the alarm from the config file.
*/
void KAEvent::Private::setCommandError(const QString& configString)
{
    mCommandError = CMD_NO_ERROR;
    const QStringList errs = configString.split(',');
    if (errs.indexOf(CMD_ERROR_VALUE) >= 0)
        mCommandError = CMD_ERROR;
    else
    {
        if (errs.indexOf(CMD_ERROR_PRE_VALUE) >= 0)
            mCommandError = CMD_ERROR_PRE;
        if (errs.indexOf(CMD_ERROR_POST_VALUE) >= 0)
            mCommandError = static_cast<CmdErrType>(mCommandError | CMD_ERROR_POST);
    }
}

/******************************************************************************
* Set the command last error status.
* If 'writeConfig' is true, the status is written to the config file.
*/
void KAEvent::Private::setCommandError(CmdErrType error, bool writeConfig) const
{
    kDebug() << mEventID << "," << error;
    if (error == mCommandError)
        return;
    mCommandError = error;
    if (writeConfig)
    {
        KConfigGroup config(KGlobal::config(), mCmdErrConfigGroup);
        if (mCommandError == CMD_NO_ERROR)
            config.deleteEntry(mEventID);
        else
        {
            QString errtext;
            switch (mCommandError)
            {
                case CMD_ERROR:       errtext = CMD_ERROR_VALUE;  break;
                case CMD_ERROR_PRE:   errtext = CMD_ERROR_PRE_VALUE;  break;
                case CMD_ERROR_POST:  errtext = CMD_ERROR_POST_VALUE;  break;
                case CMD_ERROR_PRE_POST:
                    errtext = CMD_ERROR_PRE_VALUE + ',' + CMD_ERROR_POST_VALUE;
                    break;
                default:
                    break;
            }
            config.writeEntry(mEventID, errtext);
        }
        config.sync();
    }
}
#endif

/******************************************************************************
* Set the event to be a copy of the specified event, making the specified
* alarm the 'displaying' alarm.
* The purpose of setting up a 'displaying' alarm is to be able to reinstate
* the alarm message in case of a crash, or to reinstate it should the user
* choose to defer the alarm. Note that even repeat-at-login alarms need to be
* saved in case their end time expires before the next login.
* Reply = true if successful, false if alarm was not copied.
*/
#ifdef USE_AKONADI
bool KAEvent::Private::setDisplaying(const KAEvent::Private& event, KAAlarm::Type alarmType, Akonadi::Collection::Id collectionId,
                                     const KDateTime& repeatAtLoginTime, bool showEdit, bool showDefer)
#else
bool KAEvent::Private::setDisplaying(const KAEvent::Private& event, KAAlarm::Type alarmType, const QString& resourceID,
                                     const KDateTime& repeatAtLoginTime, bool showEdit, bool showDefer)
#endif
{
    if (!mDisplaying
    &&  (alarmType == KAAlarm::MAIN_ALARM
      || alarmType == KAAlarm::REMINDER_ALARM
      || alarmType == KAAlarm::DEFERRED_REMINDER_ALARM
      || alarmType == KAAlarm::DEFERRED_ALARM
      || alarmType == KAAlarm::AT_LOGIN_ALARM))
    {
//kDebug()<<event.id()<<","<<(alarmType==KAAlarm::MAIN_ALARM?"MAIN":alarmType==KAAlarm::REMINDER_ALARM?"REMINDER":alarmType==KAAlarm::DEFERRED_REMINDER_ALARM?"REMINDER_DEFERRAL":alarmType==KAAlarm::DEFERRED_ALARM?"DEFERRAL":"LOGIN")<<"): time="<<repeatAtLoginTime.toString();
        KAAlarm al = event.alarm(alarmType);
        if (al.isValid())
        {
            *this = event;
            // Change the event ID to avoid duplicating the same unique ID as the original event
            setCategory(KAlarm::CalEvent::DISPLAYING);
#ifdef USE_AKONADI
            mItemId          = -1;    // the display event doesn't have an associated Item
            mCollectionId    = collectionId;;
#else
            mResourceId      = resourceID;
#endif
            mDisplayingDefer = showDefer;
            mDisplayingEdit  = showEdit;
            mDisplaying      = true;
            mDisplayingTime  = (alarmType == KAAlarm::AT_LOGIN_ALARM) ? repeatAtLoginTime : al.dateTime().kDateTime();
            switch (al.type())
            {
                case KAAlarm::AT_LOGIN__ALARM:                mDisplayingFlags = REPEAT_AT_LOGIN;  break;
                case KAAlarm::REMINDER__ALARM:                mDisplayingFlags = REMINDER;  break;
                case KAAlarm::DEFERRED_REMINDER_TIME__ALARM:  mDisplayingFlags = REMINDER | TIME_DEFERRAL;  break;
                case KAAlarm::DEFERRED_REMINDER_DATE__ALARM:  mDisplayingFlags = REMINDER | DATE_DEFERRAL;  break;
                case KAAlarm::DEFERRED_TIME__ALARM:           mDisplayingFlags = TIME_DEFERRAL;  break;
                case KAAlarm::DEFERRED_DATE__ALARM:           mDisplayingFlags = DATE_DEFERRAL;  break;
                default:                                      mDisplayingFlags = 0;  break;
            }
            ++mAlarmCount;
            return true;
        }
    }
    return false;
}

/******************************************************************************
* Reinstate the original event from the 'displaying' event.
*/
#ifdef USE_AKONADI
void KAEvent::Private::reinstateFromDisplaying(const ConstEventPtr& kcalEvent, Akonadi::Collection::Id collectionId, bool& showEdit, bool& showDefer)
#else
void KAEvent::Private::reinstateFromDisplaying(const Event* kcalEvent, QString& resourceID, bool& showEdit, bool& showDefer)
#endif
{
    set(kcalEvent);
    if (mDisplaying)
    {
        // Retrieve the original event's unique ID
        setCategory(KAlarm::CalEvent::ACTIVE);
#ifdef USE_AKONADI
        collectionId = mCollectionId;
#else
        resourceID   = mResourceId;
#endif
        showDefer    = mDisplayingDefer;
        showEdit     = mDisplayingEdit;
        mDisplaying  = false;
        --mAlarmCount;
    }
}

/******************************************************************************
* Return the original alarm which the displaying alarm refers to.
* Note that the caller is responsible for ensuring that the event was a
* displaying event, since this is normally called after
* reinstateFromDisplaying(), which clears mDisplaying.
*/
KAAlarm KAEvent::convertDisplayingAlarm() const
{
    KAAlarm al = alarm(KAAlarm::DISPLAYING_ALARM);
    int displayingFlags = d->mDisplayingFlags;
    if (displayingFlags & REPEAT_AT_LOGIN)
    {
        al.mRepeatAtLogin = true;
        al.mType = KAAlarm::AT_LOGIN__ALARM;
    }
    else if (displayingFlags & DEFERRAL)
    {
        al.mDeferred = true;
        al.mType = (displayingFlags == (REMINDER | DATE_DEFERRAL)) ? KAAlarm::DEFERRED_REMINDER_DATE__ALARM
             : (displayingFlags == (REMINDER | TIME_DEFERRAL)) ? KAAlarm::DEFERRED_REMINDER_TIME__ALARM
             : (displayingFlags == DATE_DEFERRAL) ? KAAlarm::DEFERRED_DATE__ALARM
             : KAAlarm::DEFERRED_TIME__ALARM;
    }
    else if (displayingFlags & REMINDER)
        al.mType = KAAlarm::REMINDER__ALARM;
    else
        al.mType = KAAlarm::MAIN__ALARM;
    return al;
}

/******************************************************************************
* Determine whether the event will occur after the specified date/time.
* If 'includeRepetitions' is true and the alarm has a sub-repetition, it
* returns true if any repetitions occur after the specified date/time.
*/
bool KAEvent::Private::occursAfter(const KDateTime& preDateTime, bool includeRepetitions) const
{
    KDateTime dt;
    if (checkRecur() != KARecurrence::NO_RECUR)
    {
        if (mRecurrence->duration() < 0)
            return true;    // infinite recurrence
        dt = mRecurrence->endDateTime();
    }
    else
        dt = mNextMainDateTime.effectiveKDateTime();
    if (mStartDateTime.isDateOnly())
    {
        QDate pre = preDateTime.date();
        if (preDateTime.toTimeSpec(mStartDateTime.timeSpec()).time() < DateTime::startOfDay())
            pre = pre.addDays(-1);    // today's recurrence (if today recurs) is still to come
        if (pre < dt.date())
            return true;
    }
    else if (preDateTime < dt)
        return true;

    if (includeRepetitions  &&  mRepetition)
    {
        if (preDateTime < mRepetition.duration().end(dt))
            return true;
    }
    return false;
}

/******************************************************************************
* Get the date/time of the next occurrence of the event, after the specified
* date/time.
* 'result' = date/time of next occurrence, or invalid date/time if none.
*/
KAEvent::OccurType KAEvent::Private::nextOccurrence(const KDateTime& preDateTime, DateTime& result,
                                                    OccurOption includeRepetitions) const
{
    KDateTime pre = preDateTime;
    if (includeRepetitions != IGNORE_REPETITION)
    {                   // RETURN_REPETITION or ALLOW_FOR_REPETITION
        if (!mRepetition)
            includeRepetitions = IGNORE_REPETITION;
        else
            pre = mRepetition.duration(-mRepetition.count()).end(preDateTime);
    }

    OccurType type;
    bool recurs = (checkRecur() != KARecurrence::NO_RECUR);
    if (recurs)
        type = nextRecurrence(pre, result);
    else if (pre < mNextMainDateTime.effectiveKDateTime())
    {
        result = mNextMainDateTime;
        type = FIRST_OR_ONLY_OCCURRENCE;
    }
    else
    {
        result = DateTime();
        type = NO_OCCURRENCE;
    }

    if (type != NO_OCCURRENCE  &&  result <= preDateTime  &&  includeRepetitions != IGNORE_REPETITION)
    {                   // RETURN_REPETITION or ALLOW_FOR_REPETITION
        // The next occurrence is a sub-repetition
        int repetition = mRepetition.nextRepeatCount(result.kDateTime(), preDateTime);
        DateTime repeatDT = mRepetition.duration(repetition).end(result.kDateTime());
        if (recurs)
        {
            // We've found a recurrence before the specified date/time, which has
            // a sub-repetition after the date/time.
            // However, if the intervals between recurrences vary, we could possibly
            // have missed a later recurrence which fits the criterion, so check again.
            DateTime dt;
            OccurType newType = previousOccurrence(repeatDT.effectiveKDateTime(), dt, false);
            if (dt > result)
            {
                type = newType;
                result = dt;
                if (includeRepetitions == RETURN_REPETITION  &&  result <= preDateTime)
                {
                    // The next occurrence is a sub-repetition
                    int repetition = mRepetition.nextRepeatCount(result.kDateTime(), preDateTime);
                    result = mRepetition.duration(repetition).end(result.kDateTime());
                    type = static_cast<OccurType>(type | OCCURRENCE_REPEAT);
                }
                return type;
            }
        }
        if (includeRepetitions == RETURN_REPETITION)
        {
            // The next occurrence is a sub-repetition
            result = repeatDT;
            type = static_cast<OccurType>(type | OCCURRENCE_REPEAT);
        }
    }
    return type;
}

/******************************************************************************
* Get the date/time of the last previous occurrence of the event, before the
* specified date/time.
* If 'includeRepetitions' is true and the alarm has a sub-repetition, the
* last previous repetition is returned if appropriate.
* 'result' = date/time of previous occurrence, or invalid date/time if none.
*/
KAEvent::OccurType KAEvent::Private::previousOccurrence(const KDateTime& afterDateTime, DateTime& result,
                                                        bool includeRepetitions) const
{
    Q_ASSERT(!afterDateTime.isDateOnly());
    if (mStartDateTime >= afterDateTime)
    {
        result = KDateTime();
        return NO_OCCURRENCE;     // the event starts after the specified date/time
    }

    // Find the latest recurrence of the event
    OccurType type;
    if (checkRecur() == KARecurrence::NO_RECUR)
    {
        result = mStartDateTime;
        type = FIRST_OR_ONLY_OCCURRENCE;
    }
    else
    {
        KDateTime recurStart = mRecurrence->startDateTime();
        KDateTime after = afterDateTime.toTimeSpec(mStartDateTime.timeSpec());
        if (mStartDateTime.isDateOnly()  &&  afterDateTime.time() > DateTime::startOfDay())
            after = after.addDays(1);    // today's recurrence (if today recurs) has passed
        KDateTime dt = mRecurrence->getPreviousDateTime(after);
        result = dt;
        result.setDateOnly(mStartDateTime.isDateOnly());
        if (!dt.isValid())
            return NO_OCCURRENCE;
        if (dt == recurStart)
            type = FIRST_OR_ONLY_OCCURRENCE;
        else if (mRecurrence->getNextDateTime(dt).isValid())
            type = result.isDateOnly() ? RECURRENCE_DATE : RECURRENCE_DATE_TIME;
        else
            type = LAST_RECURRENCE;
    }

    if (includeRepetitions  &&  mRepetition)
    {
        // Find the latest repetition which is before the specified time.
        int repetition = mRepetition.previousRepeatCount(result.effectiveKDateTime(), afterDateTime);
        if (repetition > 0)
        {
            result = mRepetition.duration(qMin(repetition, mRepetition.count())).end(result.kDateTime());
            return static_cast<OccurType>(type | OCCURRENCE_REPEAT);
        }
    }
    return type;
}

/******************************************************************************
* Set the date/time of the event to the next scheduled occurrence after the
* specified date/time, provided that this is later than its current date/time.
* Any reminder alarm is adjusted accordingly.
* If the alarm has a sub-repetition, and a repetition of a previous recurrence
* occurs after the specified date/time, that repetition is set as the next
* occurrence.
*/
KAEvent::OccurType KAEvent::Private::setNextOccurrence(const KDateTime& preDateTime)
{
    if (preDateTime < mNextMainDateTime.effectiveKDateTime())
        return FIRST_OR_ONLY_OCCURRENCE;    // it might not be the first recurrence - tant pis
    KDateTime pre = preDateTime;
    // If there are repetitions, adjust the comparison date/time so that
    // we find the earliest recurrence which has a repetition falling after
    // the specified preDateTime.
    if (mRepetition)
        pre = mRepetition.duration(-mRepetition.count()).end(preDateTime);

    DateTime dt;
    OccurType type;
    bool changed = false;
    if (pre < mNextMainDateTime.effectiveKDateTime())
    {
        dt = mNextMainDateTime;
        type = FIRST_OR_ONLY_OCCURRENCE;   // may not actually be the first occurrence
    }
    else if (checkRecur() != KARecurrence::NO_RECUR)
    {
        type = nextRecurrence(pre, dt);
        if (type == NO_OCCURRENCE)
            return NO_OCCURRENCE;
        if (type != FIRST_OR_ONLY_OCCURRENCE  &&  dt != mNextMainDateTime)
        {
            // Need to reschedule the next trigger date/time
            mNextMainDateTime = dt;
            // Reinstate the reminder (if any) for the rescheduled recurrence
            if (mReminderMinutes  &&  (mDeferral == REMINDER_DEFERRAL || !mReminderActive))
                activate_reminder(!mReminderOnceOnly);
            if (mDeferral == REMINDER_DEFERRAL)
                set_deferral(NO_DEFERRAL);
            changed = true;
        }
    }
    else
        return NO_OCCURRENCE;

    if (mRepetition)
    {
        if (dt <= preDateTime)
        {
            // The next occurrence is a sub-repetition.
            type = static_cast<OccurType>(type | OCCURRENCE_REPEAT);
            mNextRepeat = mRepetition.nextRepeatCount(dt.effectiveKDateTime(), preDateTime);
            // Repetitions can't have a reminder, so remove any.
            activate_reminder(false);
            if (mDeferral == REMINDER_DEFERRAL)
                set_deferral(NO_DEFERRAL);
            changed = true;
        }
        else if (mNextRepeat)
        {
            // The next occurrence is the main occurrence, not a repetition
            mNextRepeat = 0;
            changed = true;
        }
    }
    if (changed)
        calcTriggerTimes();
    return type;
}

/******************************************************************************
* Get the date/time of the next recurrence of the event, after the specified
* date/time.
* 'result' = date/time of next occurrence, or invalid date/time if none.
*/
KAEvent::OccurType KAEvent::Private::nextRecurrence(const KDateTime& preDateTime, DateTime& result) const
{
    KDateTime recurStart = mRecurrence->startDateTime();
    KDateTime pre = preDateTime.toTimeSpec(mStartDateTime.timeSpec());
    if (mStartDateTime.isDateOnly()  &&  !pre.isDateOnly()  &&  pre.time() < DateTime::startOfDay())
    {
        pre = pre.addDays(-1);    // today's recurrence (if today recurs) is still to come
        pre.setTime(DateTime::startOfDay());
    }
    KDateTime dt = mRecurrence->getNextDateTime(pre);
    result = dt;
    result.setDateOnly(mStartDateTime.isDateOnly());
    if (!dt.isValid())
        return NO_OCCURRENCE;
    if (dt == recurStart)
        return FIRST_OR_ONLY_OCCURRENCE;
    if (mRecurrence->duration() >= 0  &&  dt == mRecurrence->endDateTime())
        return LAST_RECURRENCE;
    return result.isDateOnly() ? RECURRENCE_DATE : RECURRENCE_DATE_TIME;
}

/******************************************************************************
* Return the recurrence interval as text suitable for display.
*/
QString KAEvent::recurrenceText(bool brief) const
{
    if (d->mRepeatAtLogin)
        return brief ? i18nc("@info/plain Brief form of 'At Login'", "Login") : i18nc("@info/plain", "At login");
    if (d->mRecurrence)
    {
        int frequency = d->mRecurrence->frequency();
        switch (d->mRecurrence->defaultRRuleConst()->recurrenceType())
        {
            case RecurrenceRule::rMinutely:
                if (frequency < 60)
                    return i18ncp("@info/plain", "1 Minute", "%1 Minutes", frequency);
                else if (frequency % 60 == 0)
                    return i18ncp("@info/plain", "1 Hour", "%1 Hours", frequency/60);
                else
                {
                    QString mins;
                    return i18nc("@info/plain Hours and minutes", "%1h %2m", frequency/60, mins.sprintf("%02d", frequency%60));
                }
            case RecurrenceRule::rDaily:
                return i18ncp("@info/plain", "1 Day", "%1 Days", frequency);
            case RecurrenceRule::rWeekly:
                return i18ncp("@info/plain", "1 Week", "%1 Weeks", frequency);
            case RecurrenceRule::rMonthly:
                return i18ncp("@info/plain", "1 Month", "%1 Months", frequency);
            case RecurrenceRule::rYearly:
                return i18ncp("@info/plain", "1 Year", "%1 Years", frequency);
            case RecurrenceRule::rNone:
            default:
                break;
        }
    }
    return brief ? QString() : i18nc("@info/plain No recurrence", "None");
}

/******************************************************************************
* Return the repetition interval as text suitable for display.
*/
QString KAEvent::repetitionText(bool brief) const
{
    if (d->mRepetition)
    {
        if (!d->mRepetition.isDaily())
        {
            int minutes = d->mRepetition.intervalMinutes();
            if (minutes < 60)
                return i18ncp("@info/plain", "1 Minute", "%1 Minutes", minutes);
            if (minutes % 60 == 0)
                return i18ncp("@info/plain", "1 Hour", "%1 Hours", minutes/60);
            QString mins;
            return i18nc("@info/plain Hours and minutes", "%1h %2m", minutes/60, mins.sprintf("%02d", minutes%60));
        }
        int days = d->mRepetition.intervalDays();
        if (days % 7)
            return i18ncp("@info/plain", "1 Day", "%1 Days", days);
        return i18ncp("@info/plain", "1 Week", "%1 Weeks", days / 7);
    }
    return brief ? QString() : i18nc("@info/plain No repetition", "None");
}

/******************************************************************************
* Adjust the event date/time to the first recurrence of the event, on or after
* start date/time. The event start date may not be a recurrence date, in which
* case a later date will be set.
*/
void KAEvent::Private::setFirstRecurrence()
{
    switch (checkRecur())
    {
        case KARecurrence::NO_RECUR:
        case KARecurrence::MINUTELY:
            return;
        case KARecurrence::ANNUAL_DATE:
        case KARecurrence::ANNUAL_POS:
            if (mRecurrence->yearMonths().isEmpty())
                return;    // (presumably it's a template)
            break;
        case KARecurrence::DAILY:
        case KARecurrence::WEEKLY:
        case KARecurrence::MONTHLY_POS:
        case KARecurrence::MONTHLY_DAY:
            break;
    }
    KDateTime recurStart = mRecurrence->startDateTime();
    if (mRecurrence->recursOn(recurStart.date(), recurStart.timeSpec()))
        return;           // it already recurs on the start date

    // Set the frequency to 1 to find the first possible occurrence
    bool changed = false;
    int frequency = mRecurrence->frequency();
    mRecurrence->setFrequency(1);
    DateTime next;
    nextRecurrence(mNextMainDateTime.effectiveKDateTime(), next);
    if (!next.isValid())
        mRecurrence->setStartDateTime(recurStart, mStartDateTime.isDateOnly());   // reinstate the old value
    else
    {
        mRecurrence->setStartDateTime(next.effectiveKDateTime(), next.isDateOnly());
        mStartDateTime = mNextMainDateTime = next;
        changed = true;
    }
    mRecurrence->setFrequency(frequency);    // restore the frequency
    if (changed)
        calcTriggerTimes();
}

/******************************************************************************
* Initialise the event's recurrence from a KCal::Recurrence.
* The event's start date/time is not changed.
*/
void KAEvent::Private::setRecurrence(const KARecurrence& recurrence)
{
    startChanges();   // prevent multiple trigger time evaluation here
    delete mRecurrence;
    if (recurrence.recurs())
    {
        mRecurrence = new KARecurrence(recurrence);
        mRecurrence->setStartDateTime(mStartDateTime.effectiveKDateTime(), mStartDateTime.isDateOnly());
        mChanged = true;
    }
    else
    {
        if (mRecurrence)
            mChanged = true;
        mRecurrence = 0;
    }

    // Adjust sub-repetition values to fit the recurrence.
    setRepetition(mRepetition);

    endChanges();
}

/******************************************************************************
* Called when the user changes the start-of-day time.
* Adjust the start time of a date-only alarm's recurrence.
*/
void KAEvent::adjustRecurrenceStartOfDay()
{
    if (d->mRecurrence)
        d->mRecurrence->setStartDateTime(d->mStartDateTime.effectiveKDateTime(), d->mStartDateTime.isDateOnly());
}

/******************************************************************************
* Initialise the event's sub-repetition.
* The repetition length is adjusted if necessary to fit the recurrence interval.
* If the event doesn't recur, the sub-repetition is cleared.
* Reply = false if a non-daily interval was specified for a date-only recurrence.
*/
bool KAEvent::Private::setRepetition(const Repetition& repetition)
{
    // Don't set mRepetition to zero at the start of this function, in case the
    // 'repetition' parameter passed in is a reference to mRepetition.
    mNextRepeat = 0;
    if (repetition  &&  !mRepeatAtLogin)
    {
        Q_ASSERT(checkRecur() != KARecurrence::NO_RECUR);
        if (!repetition.isDaily()  &&  mStartDateTime.isDateOnly())
        {
            mRepetition.set(0, 0);
            return false;    // interval must be in units of days for date-only alarms
        }
        Duration longestInterval = mRecurrence->longestInterval();
        if (repetition.duration() >= longestInterval)
        {
            int count = mStartDateTime.isDateOnly()
                      ? (longestInterval.asDays() - 1) / repetition.intervalDays()
                      : (longestInterval.asSeconds() - 1) / repetition.intervalSeconds();
            mRepetition.set(repetition.interval(), count);
        }
        else
            mRepetition = repetition;
        calcTriggerTimes();
    }
    else
        mRepetition.set(0, 0);
    return true;
}

/******************************************************************************
* Set the recurrence to recur at a minutes interval.
* Parameters:
*    freq  = how many minutes between recurrences.
*    count = number of occurrences, including first and last.
*          = -1 to recur indefinitely.
*          = 0 to use 'end' instead.
*    end   = end date/time (invalid to use 'count' instead).
* Reply = false if no recurrence was set up.
*/
bool KAEvent::setRecurMinutely(int freq, int count, const KDateTime& end)
{
    bool success = d->setRecur(RecurrenceRule::rMinutely, freq, count, end);
    d->calcTriggerTimes();
    return success;
}

/******************************************************************************
* Set the recurrence to recur daily.
* Parameters:
*    freq  = how many days between recurrences.
*    days  = which days of the week alarms are allowed to occur on.
*    count = number of occurrences, including first and last.
*          = -1 to recur indefinitely.
*          = 0 to use 'end' instead.
*    end   = end date (invalid to use 'count' instead).
* Reply = false if no recurrence was set up.
*/
bool KAEvent::setRecurDaily(int freq, const QBitArray& days, int count, const QDate& end)
{
    bool success = d->setRecur(RecurrenceRule::rDaily, freq, count, end);
    if (success)
    {
        int n = 0;
        for (int i = 0;  i < 7;  ++i)
        {
            if (days.testBit(i))
                ++n;
        }
        if (n < 7)
            d->mRecurrence->addWeeklyDays(days);
    }
    d->calcTriggerTimes();
    return success;
}

/******************************************************************************
* Set the recurrence to recur weekly, on the specified weekdays.
* Parameters:
*    freq  = how many weeks between recurrences.
*    days  = which days of the week alarms should occur on.
*    count = number of occurrences, including first and last.
*          = -1 to recur indefinitely.
*          = 0 to use 'end' instead.
*    end   = end date (invalid to use 'count' instead).
* Reply = false if no recurrence was set up.
*/
bool KAEvent::setRecurWeekly(int freq, const QBitArray& days, int count, const QDate& end)
{
    bool success = d->setRecur(RecurrenceRule::rWeekly, freq, count, end);
    if (success)
        d->mRecurrence->addWeeklyDays(days);
    d->calcTriggerTimes();
    return success;
}

/******************************************************************************
* Set the recurrence to recur monthly, on the specified days within the month.
* Parameters:
*    freq  = how many months between recurrences.
*    days  = which days of the month alarms should occur on.
*    count = number of occurrences, including first and last.
*          = -1 to recur indefinitely.
*          = 0 to use 'end' instead.
*    end   = end date (invalid to use 'count' instead).
* Reply = false if no recurrence was set up.
*/
bool KAEvent::setRecurMonthlyByDate(int freq, const QList<int>& days, int count, const QDate& end)
{
    bool success = d->setRecur(RecurrenceRule::rMonthly, freq, count, end);
    if (success)
    {
        for (int i = 0, end = days.count();  i < end;  ++i)
            d->mRecurrence->addMonthlyDate(days[i]);
    }
    d->calcTriggerTimes();
    return success;
}

/******************************************************************************
* Set the recurrence to recur monthly, on the specified weekdays in the
* specified weeks of the month.
* Parameters:
*    freq  = how many months between recurrences.
*    posns = which days of the week/weeks of the month alarms should occur on.
*    count = number of occurrences, including first and last.
*          = -1 to recur indefinitely.
*          = 0 to use 'end' instead.
*    end   = end date (invalid to use 'count' instead).
* Reply = false if no recurrence was set up.
*/
bool KAEvent::setRecurMonthlyByPos(int freq, const QList<MonthPos>& posns, int count, const QDate& end)
{
    bool success = d->setRecur(RecurrenceRule::rMonthly, freq, count, end);
    if (success)
    {
        for (int i = 0, end = posns.count();  i < end;  ++i)
            d->mRecurrence->addMonthlyPos(posns[i].weeknum, posns[i].days);
    }
    d->calcTriggerTimes();
    return success;
}

/******************************************************************************
* Set the recurrence to recur annually, on the specified start date in each
* of the specified months.
* Parameters:
*    freq   = how many years between recurrences.
*    months = which months of the year alarms should occur on.
*    day    = day of month, or 0 to use start date
*    feb29  = when February 29th should recur in non-leap years.
*    count  = number of occurrences, including first and last.
*           = -1 to recur indefinitely.
*           = 0 to use 'end' instead.
*    end    = end date (invalid to use 'count' instead).
* Reply = false if no recurrence was set up.
*/
bool KAEvent::setRecurAnnualByDate(int freq, const QList<int>& months, int day, KARecurrence::Feb29Type feb29, int count, const QDate& end)
{
    bool success = d->setRecur(RecurrenceRule::rYearly, freq, count, end, feb29);
    if (success)
    {
        for (int i = 0, end = months.count();  i < end;  ++i)
            d->mRecurrence->addYearlyMonth(months[i]);
        if (day)
            d->mRecurrence->addMonthlyDate(day);
    }
    d->calcTriggerTimes();
    return success;
}

/******************************************************************************
* Set the recurrence to recur annually, on the specified weekdays in the
* specified weeks of the specified months.
* Parameters:
*    freq   = how many years between recurrences.
*    posns  = which days of the week/weeks of the month alarms should occur on.
*    months = which months of the year alarms should occur on.
*    count  = number of occurrences, including first and last.
*           = -1 to recur indefinitely.
*           = 0 to use 'end' instead.
*    end    = end date (invalid to use 'count' instead).
* Reply = false if no recurrence was set up.
*/
bool KAEvent::setRecurAnnualByPos(int freq, const QList<MonthPos>& posns, const QList<int>& months, int count, const QDate& end)
{
    bool success = d->setRecur(RecurrenceRule::rYearly, freq, count, end);
    if (success)
    {
        int i = 0;
        int iend;
        for (iend = months.count();  i < iend;  ++i)
            d->mRecurrence->addYearlyMonth(months[i]);
        for (i = 0, iend = posns.count();  i < iend;  ++i)
            d->mRecurrence->addYearlyPos(posns[i].weeknum, posns[i].days);
    }
    d->calcTriggerTimes();
    return success;
}

/******************************************************************************
* Initialise the event's recurrence data.
* Parameters:
*    freq  = how many intervals between recurrences.
*    count = number of occurrences, including first and last.
*          = -1 to recur indefinitely.
*          = 0 to use 'end' instead.
*    end   = end date/time (invalid to use 'count' instead).
* Reply = false if no recurrence was set up.
*/
bool KAEvent::Private::setRecur(RecurrenceRule::PeriodType recurType, int freq, int count, const QDate& end, KARecurrence::Feb29Type feb29)
{
    KDateTime edt = mNextMainDateTime.kDateTime();
    edt.setDate(end);
    return setRecur(recurType, freq, count, edt, feb29);
}
bool KAEvent::Private::setRecur(RecurrenceRule::PeriodType recurType, int freq, int count, const KDateTime& end, KARecurrence::Feb29Type feb29)
{
    if (count >= -1  &&  (count || end.date().isValid()))
    {
        if (!mRecurrence)
            mRecurrence = new KARecurrence;
        if (mRecurrence->init(recurType, freq, count, mNextMainDateTime.kDateTime(), end, feb29))
            return true;
    }
    clearRecur();
    return false;
}

/******************************************************************************
* Clear the event's recurrence and alarm repetition data.
*/
void KAEvent::Private::clearRecur()
{
    delete mRecurrence;
    mRecurrence = 0;
    mRepetition.set(0, 0);
    mNextRepeat = 0;
}

/******************************************************************************
* Validate the event's recurrence data, correcting any inconsistencies (which
* should never occur!).
* Reply = true if a recurrence (as opposed to a login repetition) exists.
*/
KARecurrence::Type KAEvent::Private::checkRecur() const
{
    if (mRecurrence)
    {
        KARecurrence::Type type = mRecurrence->type();
        switch (type)
        {
            case KARecurrence::MINUTELY:     // hourly
            case KARecurrence::DAILY:        // daily
            case KARecurrence::WEEKLY:       // weekly on multiple days of week
            case KARecurrence::MONTHLY_DAY:  // monthly on multiple dates in month
            case KARecurrence::MONTHLY_POS:  // monthly on multiple nth day of week
            case KARecurrence::ANNUAL_DATE:  // annually on multiple months (day of month = start date)
            case KARecurrence::ANNUAL_POS:   // annually on multiple nth day of week in multiple months
                return type;
            default:
                if (mRecurrence)
                    const_cast<KAEvent::Private*>(this)->clearRecur();  // this shouldn't ever be necessary!!
                break;
        }
    }
    if (mRepetition)    // can't have a repetition without a recurrence
        const_cast<KAEvent::Private*>(this)->clearRecur();  // this shouldn't ever be necessary!!
    return KARecurrence::NO_RECUR;
}


/******************************************************************************
* Return the recurrence interval in units of the recurrence period type.
*/
int KAEvent::recurInterval() const
{
    if (d->mRecurrence)
    {
        switch (d->mRecurrence->type())
        {
            case KARecurrence::MINUTELY:
            case KARecurrence::DAILY:
            case KARecurrence::WEEKLY:
            case KARecurrence::MONTHLY_DAY:
            case KARecurrence::MONTHLY_POS:
            case KARecurrence::ANNUAL_DATE:
            case KARecurrence::ANNUAL_POS:
                return d->mRecurrence->frequency();
            default:
                break;
        }
    }
    return 0;
}

#if 0
/******************************************************************************
* Convert a QList<WDayPos> to QList<MonthPos>.
*/
QList<KAEvent::Private::MonthPos> KAEvent::Private::convRecurPos(const QList<KCal::RecurrenceRule::WDayPos>& wdaypos)
{
    QList<MonthPos> mposns;
    for (int i = 0, end = wdaypos.count();  i < end;  ++i)
    {
        int daybit  = wdaypos[i].day() - 1;
        int weeknum = wdaypos[i].pos();
        bool found = false;
        for (int m = 0, mend = mposns.count();  m < mend;  ++m)
        {
            if (mposns[m].weeknum == weeknum)
            {
                mposns[m].days.setBit(daybit);
                found = true;
                break;
            }
        }
        if (!found)
        {
            MonthPos mpos;
            mpos.days.fill(false);
            mpos.days.setBit(daybit);
            mpos.weeknum = weeknum;
            mposns.append(mpos);
        }
    }
    return mposns;
}
#endif

/******************************************************************************
* Set the start-of-day time for date-only alarms.
*/
void KAEvent::setStartOfDay(const QTime& startOfDay)
{
    DateTime::setStartOfDay(startOfDay);
}

/******************************************************************************
* If the calendar was written by a previous version of KAlarm, do any
* necessary format conversions on the events to ensure that when the calendar
* is saved, no information is lost or corrupted.
* Reply = true if any conversions were done.
*/
#ifdef USE_AKONADI
bool KAEvent::convertKCalEvents(const Calendar::Ptr& calendar, int calendarVersion, bool adjustSummerTime)
#else
bool KAEvent::convertKCalEvents(CalendarLocal& calendar, int calendarVersion, bool adjustSummerTime)
#endif
{
    // KAlarm pre-0.9 codes held in the alarm's DESCRIPTION property
    static const QChar   SEPARATOR        = QLatin1Char(';');
    static const QChar   LATE_CANCEL_CODE = QLatin1Char('C');
    static const QChar   AT_LOGIN_CODE    = QLatin1Char('L');   // subsidiary alarm at every login
    static const QChar   DEFERRAL_CODE    = QLatin1Char('D');   // extra deferred alarm
    static const QString TEXT_PREFIX      = QLatin1String("TEXT:");
    static const QString FILE_PREFIX      = QLatin1String("FILE:");
    static const QString COMMAND_PREFIX   = QLatin1String("CMD:");

    // KAlarm pre-0.9.2 codes held in the event's CATEGORY property
    static const QString BEEP_CATEGORY    = QLatin1String("BEEP");

    // KAlarm pre-1.1.1 LATECANCEL category with no parameter
    static const QString LATE_CANCEL_CAT = QLatin1String("LATECANCEL");

    // KAlarm pre-1.3.0 TMPLDEFTIME category with no parameter
    static const QString TEMPL_DEF_TIME_CAT = QLatin1String("TMPLDEFTIME");

    // KAlarm pre-1.3.1 XTERM category
    static const QString EXEC_IN_XTERM_CAT  = QLatin1String("XTERM");

    // KAlarm pre-1.9.0 categories
    static const QString DATE_ONLY_CATEGORY        = QLatin1String("DATE");
    static const QString EMAIL_BCC_CATEGORY        = QLatin1String("BCC");
    static const QString CONFIRM_ACK_CATEGORY      = QLatin1String("ACKCONF");
    static const QString KORGANIZER_CATEGORY       = QLatin1String("KORG");
    static const QString DEFER_CATEGORY            = QLatin1String("DEFER;");
    static const QString ARCHIVE_CATEGORY          = QLatin1String("SAVE");
    static const QString ARCHIVE_CATEGORIES        = QLatin1String("SAVE:");
    static const QString LATE_CANCEL_CATEGORY      = QLatin1String("LATECANCEL;");
    static const QString AUTO_CLOSE_CATEGORY       = QLatin1String("LATECLOSE;");
    static const QString TEMPL_AFTER_TIME_CATEGORY = QLatin1String("TMPLAFTTIME;");
    static const QString KMAIL_SERNUM_CATEGORY     = QLatin1String("KMAIL:");
    static const QString LOG_CATEGORY              = QLatin1String("LOG:");

    // KAlarm pre-1.5.0/1.9.9 properties
    static const QByteArray KMAIL_ID_PROPERTY("KMAILID");    // X-KDE-KALARM-KMAILID property

    if (calendarVersion >= currentCalendarVersion())
        return false;

    kDebug() << "Adjusting version" << calendarVersion;
    bool pre_0_7    = (calendarVersion < KAlarm::Version(0,7,0));
    bool pre_0_9    = (calendarVersion < KAlarm::Version(0,9,0));
    bool pre_0_9_2  = (calendarVersion < KAlarm::Version(0,9,2));
    bool pre_1_1_1  = (calendarVersion < KAlarm::Version(1,1,1));
    bool pre_1_2_1  = (calendarVersion < KAlarm::Version(1,2,1));
    bool pre_1_3_0  = (calendarVersion < KAlarm::Version(1,3,0));
    bool pre_1_3_1  = (calendarVersion < KAlarm::Version(1,3,1));
    bool pre_1_4_14 = (calendarVersion < KAlarm::Version(1,4,14));
    bool pre_1_5_0  = (calendarVersion < KAlarm::Version(1,5,0));
    bool pre_1_9_0  = (calendarVersion < KAlarm::Version(1,9,0));
    bool pre_1_9_2  = (calendarVersion < KAlarm::Version(1,9,2));
    bool pre_1_9_7  = (calendarVersion < KAlarm::Version(1,9,7));
    bool pre_1_9_9  = (calendarVersion < KAlarm::Version(1,9,9));
    bool pre_1_9_10 = (calendarVersion < KAlarm::Version(1,9,10));
    bool pre_2_2_9  = (calendarVersion < KAlarm::Version(2,2,9));
    bool pre_2_3_0  = (calendarVersion < KAlarm::Version(2,3,0));
    bool pre_2_3_2  = (calendarVersion < KAlarm::Version(2,3,2));
    Q_ASSERT(currentCalendarVersion() == KAlarm::Version(2,2,9));

    KTimeZone localZone;
    if (pre_1_9_2)
        localZone = KSystemTimeZones::local();

    bool converted = false;
#ifdef USE_AKONADI
    Event::List events = calendar->rawEvents();
#else
    Event::List events = calendar.rawEvents();
#endif
    for (int ei = 0, eend = events.count();  ei < eend;  ++ei)
    {
#ifdef USE_AKONADI
        Event::Ptr event = events[ei];
#else
        Event* event = events[ei];
#endif
        Alarm::List alarms = event->alarms();
        if (alarms.isEmpty())
            continue;    // KAlarm isn't interested in events without alarms
        event->startUpdates();   // prevent multiple update notifications
        bool readOnly = event->isReadOnly();
        if (readOnly)
            event->setReadOnly(false);
        QStringList cats = event->categories();
        bool addLateCancel = false;
        QStringList flags;

        if (pre_0_7  &&  event->allDay())
        {
            // It's a KAlarm pre-0.7 calendar file.
            // Ensure that when the calendar is saved, the alarm time isn't lost.
            event->setAllDay(false);
        }

        if (pre_0_9)
        {
            /*
             * It's a KAlarm pre-0.9 calendar file.
             * All alarms were of type DISPLAY. Instead of the X-KDE-KALARM-TYPE
             * alarm property, characteristics were stored as a prefix to the
             * alarm DESCRIPTION property, as follows:
             *   SEQNO;[FLAGS];TYPE:TEXT
             * where
             *   SEQNO = sequence number of alarm within the event
             *   FLAGS = C for late-cancel, L for repeat-at-login, D for deferral
             *   TYPE = TEXT or FILE or CMD
             *   TEXT = message text, file name/URL or command
             */
            for (int ai = 0, aend = alarms.count();  ai < aend;  ++ai)
            {
#ifdef USE_AKONADI
                Alarm::Ptr alarm = alarms[ai];
#else
                Alarm* alarm = alarms[ai];
#endif
                bool atLogin    = false;
                bool deferral   = false;
                bool lateCancel = false;
                KAAlarmEventBase::Type action = KAAlarmEventBase::T_MESSAGE;
                QString txt = alarm->text();
                int length = txt.length();
                int i = 0;
                if (txt[0].isDigit())
                {
                    while (++i < length  &&  txt[i].isDigit()) ;
                    if (i < length  &&  txt[i++] == SEPARATOR)
                    {
                        while (i < length)
                        {
                            QChar ch = txt[i++];
                            if (ch == SEPARATOR)
                                break;
                            if (ch == LATE_CANCEL_CODE)
                                lateCancel = true;
                            else if (ch == AT_LOGIN_CODE)
                                atLogin = true;
                            else if (ch == DEFERRAL_CODE)
                                deferral = true;
                        }
                    }
                    else
                        i = 0;     // invalid prefix
                }
                if (txt.indexOf(TEXT_PREFIX, i) == i)
                    i += TEXT_PREFIX.length();
                else if (txt.indexOf(FILE_PREFIX, i) == i)
                {
                    action = KAAlarmEventBase::T_FILE;
                    i += FILE_PREFIX.length();
                }
                else if (txt.indexOf(COMMAND_PREFIX, i) == i)
                {
                    action = KAAlarmEventBase::T_COMMAND;
                    i += COMMAND_PREFIX.length();
                }
                else
                    i = 0;
                txt = txt.mid(i);

                QStringList types;
                switch (action)
                {
                    case KAAlarmEventBase::T_FILE:
                        types += Private::FILE_TYPE;
                        // fall through to T_MESSAGE
                    case KAAlarmEventBase::T_MESSAGE:
                        alarm->setDisplayAlarm(txt);
                        break;
                    case KAAlarmEventBase::T_COMMAND:
                        setProcedureAlarm(alarm, txt);
                        break;
                    case KAAlarmEventBase::T_EMAIL:     // email alarms were introduced in KAlarm 0.9
                    case KAAlarmEventBase::T_AUDIO:     // audio alarms (with no display) were introduced in KAlarm 2.3.2
                        break;
                }
                if (atLogin)
                {
                    types += Private::AT_LOGIN_TYPE;
                    lateCancel = false;
                }
                else if (deferral)
                    types += Private::TIME_DEFERRAL_TYPE;
                if (lateCancel)
                    addLateCancel = true;
                if (types.count() > 0)
                    alarm->setCustomProperty(KAlarm::Calendar::APPNAME, Private::TYPE_PROPERTY, types.join(","));

                if (pre_0_7  &&  alarm->repeatCount() > 0  &&  alarm->snoozeTime().value() > 0)
                {
                    // It's a KAlarm pre-0.7 calendar file.
                    // Minutely recurrences were stored differently.
                    Recurrence* recur = event->recurrence();
                    if (recur  &&  recur->recurs())
                    {
                        recur->setMinutely(alarm->snoozeTime().asSeconds() / 60);
                        recur->setDuration(alarm->repeatCount() + 1);
                        alarm->setRepeatCount(0);
                        alarm->setSnoozeTime(0);
                    }
                }

                if (adjustSummerTime)
                {
                    // The calendar file was written by the KDE 3.0.0 version of KAlarm 0.5.7.
                    // Summer time was ignored when converting to UTC.
                    KDateTime dt = alarm->time();
                    time_t t = dt.toTime_t();
                    struct tm* dtm = localtime(&t);
                    if (dtm->tm_isdst)
                    {
                        dt = dt.addSecs(-3600);
                        alarm->setTime(dt);
                    }
                }
            }
        }

        if (pre_0_9_2)
        {
            /*
             * It's a KAlarm pre-0.9.2 calendar file.
             * For the archive calendar, set the CREATED time to the DTEND value.
             * Convert date-only DTSTART to date/time, and add category "DATE".
             * Set the DTEND time to the DTSTART time.
             * Convert all alarm times to DTSTART offsets.
             * For display alarms, convert the first unlabelled category to an
             * X-KDE-KALARM-FONTCOLOUR property.
             * Convert BEEP category into an audio alarm with no audio file.
             */
            if (KAlarm::CalEvent::status(event) == KAlarm::CalEvent::ARCHIVED)
                event->setCreated(event->dtEnd());
            KDateTime start = event->dtStart();
            if (event->allDay())
            {
                event->setAllDay(false);
                start.setTime(QTime(0, 0));
                flags += Private::DATE_ONLY_FLAG;
            }
            event->setHasEndDate(false);

            for (int ai = 0, aend = alarms.count();  ai < aend;  ++ai)
            {
#ifdef USE_AKONADI
                Alarm::Ptr alarm = alarms[ai];
#else
                Alarm* alarm = alarms[ai];
#endif
                KDateTime dt = alarm->time();
                alarm->setStartOffset(start.secsTo(dt));
            }

            if (!cats.isEmpty())
            {
                for (int ai = 0, aend = alarms.count();  ai < aend;  ++ai)
                {
#ifdef USE_AKONADI
                    Alarm::Ptr alarm = alarms[ai];
#else
                    Alarm* alarm = alarms[ai];
#endif
                    if (alarm->type() == Alarm::Display)
                        alarm->setCustomProperty(KAlarm::Calendar::APPNAME, Private::FONT_COLOUR_PROPERTY,
                                                 QString::fromLatin1("%1;;").arg(cats[0]));
                }
                cats.removeAt(0);
            }

            for (int i = 0, end = cats.count();  i < end;  ++i)
            {
                if (cats[i] == BEEP_CATEGORY)
                {
                    cats.removeAt(i);

#ifdef USE_AKONADI
                    Alarm::Ptr alarm = event->newAlarm();
#else
                    Alarm* alarm = event->newAlarm();
#endif
                    alarm->setEnabled(true);
                    alarm->setAudioAlarm();
                    KDateTime dt = event->dtStart();    // default

                    // Parse and order the alarms to know which one's date/time to use
                    AlarmMap alarmMap;
                    readAlarms(event, &alarmMap);
                    AlarmMap::ConstIterator it = alarmMap.constBegin();
                    if (it != alarmMap.constEnd())
                    {
                        dt = it.value().alarm->time();
                        break;
                    }
                    alarm->setStartOffset(start.secsTo(dt));
                    break;
                }
            }
        }

        if (pre_1_1_1)
        {
            /*
             * It's a KAlarm pre-1.1.1 calendar file.
             * Convert simple LATECANCEL category to LATECANCEL:n where n = minutes late.
             */
            int i;
            while ((i = cats.indexOf(LATE_CANCEL_CAT)) >= 0)
            {
                cats.removeAt(i);
                addLateCancel = true;
            }
        }

        if (pre_1_2_1)
        {
            /*
             * It's a KAlarm pre-1.2.1 calendar file.
             * Convert email display alarms from translated to untranslated header prefixes.
             */
            for (int ai = 0, aend = alarms.count();  ai < aend;  ++ai)
            {
#ifdef USE_AKONADI
                Alarm::Ptr alarm = alarms[ai];
#else
                Alarm* alarm = alarms[ai];
#endif
                if (alarm->type() == Alarm::Display)
                {
                    QString oldtext = alarm->text();
                    QString newtext = AlarmText::toCalendarText(oldtext);
                    if (oldtext != newtext)
                        alarm->setDisplayAlarm(newtext);
                }
            }
        }

        if (pre_1_3_0)
        {
            /*
             * It's a KAlarm pre-1.3.0 calendar file.
             * Convert simple TMPLDEFTIME category to TMPLAFTTIME:n where n = minutes after.
             */
            int i;
            while ((i = cats.indexOf(TEMPL_DEF_TIME_CAT)) >= 0)
            {
                cats.removeAt(i);
                (flags += Private::TEMPL_AFTER_TIME_FLAG) += QLatin1String("0");
            }
        }

        if (pre_1_3_1)
        {
            /*
             * It's a KAlarm pre-1.3.1 calendar file.
             * Convert simple XTERM category to LOG:xterm:
             */
            int i;
            while ((i = cats.indexOf(EXEC_IN_XTERM_CAT)) >= 0)
            {
                cats.removeAt(i);
                event->setCustomProperty(KAlarm::Calendar::APPNAME, Private::LOG_PROPERTY, Private::xtermURL);
            }
        }

        if (pre_1_9_0)
        {
            /*
             * It's a KAlarm pre-1.9 calendar file.
             * Add the X-KDE-KALARM-STATUS custom property.
             * Convert KAlarm categories to custom fields.
             */
            KAlarm::CalEvent::setStatus(event, KAlarm::CalEvent::status(event));
            for (int i = 0;  i < cats.count(); )
            {
                QString cat = cats[i];
                if (cat == DATE_ONLY_CATEGORY)
                    flags += Private::DATE_ONLY_FLAG;
                else if (cat == CONFIRM_ACK_CATEGORY)
                    flags += Private::CONFIRM_ACK_FLAG;
                else if (cat == EMAIL_BCC_CATEGORY)
                    flags += Private::EMAIL_BCC_FLAG;
                else if (cat == KORGANIZER_CATEGORY)
                    flags += Private::KORGANIZER_FLAG;
                else if (cat.startsWith(DEFER_CATEGORY))
                    (flags += Private::DEFER_FLAG) += cat.mid(DEFER_CATEGORY.length());
                else if (cat.startsWith(TEMPL_AFTER_TIME_CATEGORY))
                    (flags += Private::TEMPL_AFTER_TIME_FLAG) += cat.mid(TEMPL_AFTER_TIME_CATEGORY.length());
                else if (cat.startsWith(LATE_CANCEL_CATEGORY))
                    (flags += Private::LATE_CANCEL_FLAG) += cat.mid(LATE_CANCEL_CATEGORY.length());
                else if (cat.startsWith(AUTO_CLOSE_CATEGORY))
                    (flags += Private::AUTO_CLOSE_FLAG) += cat.mid(AUTO_CLOSE_CATEGORY.length());
                else if (cat.startsWith(KMAIL_SERNUM_CATEGORY))
                    (flags += Private::KMAIL_SERNUM_FLAG) += cat.mid(KMAIL_SERNUM_CATEGORY.length());
                else if (cat == ARCHIVE_CATEGORY)
                    event->setCustomProperty(KAlarm::Calendar::APPNAME, Private::ARCHIVE_PROPERTY, QLatin1String("0"));
                else if (cat.startsWith(ARCHIVE_CATEGORIES))
                    event->setCustomProperty(KAlarm::Calendar::APPNAME, Private::ARCHIVE_PROPERTY, cat.mid(ARCHIVE_CATEGORIES.length()));
                else if (cat.startsWith(LOG_CATEGORY))
                    event->setCustomProperty(KAlarm::Calendar::APPNAME, Private::LOG_PROPERTY, cat.mid(LOG_CATEGORY.length()));
                else
                {
                    ++i;   // Not a KAlarm category, so leave it
                    continue;
                }
                cats.removeAt(i);
            }
        }

        if (pre_1_9_2)
        {
            /*
             * It's a KAlarm pre-1.9.2 calendar file.
             * Convert from clock time to the local system time zone.
             */
            event->shiftTimes(KDateTime::ClockTime, localZone);
            converted = true;
        }

        if (addLateCancel)
            (flags += Private::LATE_CANCEL_FLAG) += QLatin1String("1");
        if (!flags.isEmpty())
            event->setCustomProperty(KAlarm::Calendar::APPNAME, Private::FLAGS_PROPERTY, flags.join(Private::SC));
        event->setCategories(cats);


        if ((pre_1_4_14  ||  (pre_1_9_7 && !pre_1_9_0))
        &&  event->recurrence()  &&  event->recurrence()->recurs())
        {
            /*
             * It's a KAlarm pre-1.4.14 or KAlarm 1.9 series pre-1.9.7 calendar file.
             * For recurring events, convert the main alarm offset to an absolute
             * time in the X-KDE-KALARM-NEXTRECUR property, and convert main
             * alarm offsets to zero and deferral alarm offsets to be relative to
             * the next recurrence.
             */
            QStringList flags = event->customProperty(KAlarm::Calendar::APPNAME, Private::FLAGS_PROPERTY).split(Private::SC, QString::SkipEmptyParts);
            bool dateOnly = flags.contains(Private::DATE_ONLY_FLAG);
            KDateTime startDateTime = event->dtStart();
            if (dateOnly)
                startDateTime.setDateOnly(true);
            // Convert the main alarm and get the next main trigger time from it
            KDateTime nextMainDateTime;
            bool mainExpired = true;
            for (int i = 0, alend = alarms.count();  i < alend;  ++i)
            {
#ifdef USE_AKONADI
                Alarm::Ptr alarm = alarms[i];
#else
                Alarm* alarm = alarms[i];
#endif
                if (!alarm->hasStartOffset())
                    continue;
                bool mainAlarm = true;
                QString property = alarm->customProperty(KAlarm::Calendar::APPNAME, Private::TYPE_PROPERTY);
                QStringList types = property.split(QChar(','), QString::SkipEmptyParts);
                for (int i = 0;  i < types.count();  ++i)
                {
                    QString type = types[i];
                    if (type == Private::AT_LOGIN_TYPE
                    ||  type == Private::TIME_DEFERRAL_TYPE
                    ||  type == Private::DATE_DEFERRAL_TYPE
                    ||  type == Private::REMINDER_TYPE
                    ||  type == Private::REMINDER_ONCE_TYPE
                    ||  type == Private::DISPLAYING_TYPE
                    ||  type == Private::PRE_ACTION_TYPE
                    ||  type == Private::POST_ACTION_TYPE)
                        mainAlarm = false;
                }
                if (mainAlarm)
                {
                    mainExpired = false;
                    nextMainDateTime = alarm->time();
                    nextMainDateTime.setDateOnly(dateOnly);
                    nextMainDateTime = nextMainDateTime.toTimeSpec(startDateTime);
                    if (nextMainDateTime != startDateTime)
                    {
                        QDateTime dt = nextMainDateTime.dateTime();
                        event->setCustomProperty(KAlarm::Calendar::APPNAME, Private::NEXT_RECUR_PROPERTY,
                                                 dt.toString(dateOnly ? "yyyyMMdd" : "yyyyMMddThhmmss"));
                    }
                    alarm->setStartOffset(0);
                    converted = true;
                }
            }
            int adjustment;
            if (mainExpired)
            {
                // It's an expired recurrence.
                // Set the alarm offset relative to the first actual occurrence
                // (taking account of possible exceptions).
                KDateTime dt = event->recurrence()->getNextDateTime(startDateTime.addDays(-1));
                dt.setDateOnly(dateOnly);
                adjustment = startDateTime.secsTo(dt);
            }
            else
                adjustment = startDateTime.secsTo(nextMainDateTime);
            if (adjustment)
            {
                // Convert deferred alarms
                for (int i = 0, alend = alarms.count();  i < alend;  ++i)
                {
#ifdef USE_AKONADI
                    Alarm::Ptr alarm = alarms[i];
#else
                    Alarm* alarm = alarms[i];
#endif
                    if (!alarm->hasStartOffset())
                        continue;
                    QString property = alarm->customProperty(KAlarm::Calendar::APPNAME, Private::TYPE_PROPERTY);
                    QStringList types = property.split(QChar(','), QString::SkipEmptyParts);
                    for (int i = 0;  i < types.count();  ++i)
                    {
                        QString type = types[i];
                        if (type == Private::TIME_DEFERRAL_TYPE
                        ||  type == Private::DATE_DEFERRAL_TYPE)
                        {
                            alarm->setStartOffset(alarm->startOffset().asSeconds() - adjustment);
                            converted = true;
                            break;
                        }
                    }
                }
            }
        }

        if (pre_1_5_0  ||  (pre_1_9_9 && !pre_1_9_0))
        {
            /*
             * It's a KAlarm pre-1.5.0 or KAlarm 1.9 series pre-1.9.9 calendar file.
             * Convert email identity names to uoids.
             */
            for (int i = 0, alend = alarms.count();  i < alend;  ++i)
            {
#ifdef USE_AKONADI
                Alarm::Ptr alarm = alarms[i];
#else
                Alarm* alarm = alarms[i];
#endif
                QString name = alarm->customProperty(KAlarm::Calendar::APPNAME, KMAIL_ID_PROPERTY);
                if (name.isEmpty())
                    continue;
                uint id = Identities::identityUoid(name);
                if (id)
                    alarm->setCustomProperty(KAlarm::Calendar::APPNAME, Private::EMAIL_ID_PROPERTY, QString::number(id));
                alarm->removeCustomProperty(KAlarm::Calendar::APPNAME, KMAIL_ID_PROPERTY);
                converted = true;
            }
        }

        if (pre_1_9_10)
        {
            /*
             * It's a KAlarm pre-1.9.10 calendar file.
             * Convert simple repetitions without a recurrence, to a recurrence.
             */
            if (convertRepetition(event))
                converted = true;
        }

#if 0
        if (pre_2_3_0)
        {
            /*
             * It's a KAlarm pre-2.3.0 calendar file.
             * Reminder periods could not be negative, so convert to 0.
             */
            //TODO
        }
#endif

        if (pre_2_2_9  ||  (pre_2_3_2 && !pre_2_3_0))
        {
            /*
             * It's a KAlarm pre-2.2.9 or KAlarm 2.3 series pre-2.3.2 calendar file.
             * Set the time in the calendar for all date-only alarms to 00:00.
             */
            if (convertStartOfDay(event))
                converted = true;
        }

        if (readOnly)
            event->setReadOnly(true);
        event->endUpdates();     // finally issue an update notification
    }
    return converted;
}

/******************************************************************************
* Set the time for a date-only event to 00:00.
* Reply = true if the event was updated.
*/
#ifdef USE_AKONADI
bool KAEvent::convertStartOfDay(const Event::Ptr& event)
#else
bool KAEvent::convertStartOfDay(Event* event)
#endif
{
    bool changed = false;
    QTime midnight(0, 0);
    QStringList flags = event->customProperty(KAlarm::Calendar::APPNAME, Private::FLAGS_PROPERTY).split(Private::SC, QString::SkipEmptyParts);
    if (flags.indexOf(Private::DATE_ONLY_FLAG) >= 0)
    {
        // It's an untimed event, so fix it
        KDateTime oldDt = event->dtStart();
        int adjustment = oldDt.time().secsTo(midnight);
        if (adjustment)
        {
            event->setDtStart(KDateTime(oldDt.date(), midnight, oldDt.timeSpec()));
            int deferralOffset = 0;
            AlarmMap alarmMap;
            readAlarms(event, &alarmMap);
            for (AlarmMap::ConstIterator it = alarmMap.constBegin();  it != alarmMap.constEnd();  ++it)
            {
                const AlarmData& data = it.value();
                if (!data.alarm->hasStartOffset())
                    continue;
                if (data.type & KAAlarm::TIMED_DEFERRAL_FLAG)
                {
                    // Timed deferral alarm, so adjust the offset
                    deferralOffset = data.alarm->startOffset().asSeconds();
#ifdef USE_AKONADI
                    constCast<Alarm::Ptr>(data.alarm)->setStartOffset(deferralOffset - adjustment);
#else
                    const_cast<Alarm*>(data.alarm)->setStartOffset(deferralOffset - adjustment);
#endif
                }
                else if (data.type == KAAlarm::AUDIO__ALARM
                &&       data.alarm->startOffset().asSeconds() == deferralOffset)
                {
                    // Audio alarm is set for the same time as the deferral alarm
#ifdef USE_AKONADI
                    constCast<Alarm::Ptr>(data.alarm)->setStartOffset(deferralOffset - adjustment);
#else
                    const_cast<Alarm*>(data.alarm)->setStartOffset(deferralOffset - adjustment);
#endif
                }
            }
            changed = true;
        }
    }
    else
    {
        // It's a timed event. Fix any untimed alarms.
        int deferralOffset = 0;
        int newDeferralOffset = 0;
        DateTime start;
        KDateTime nextMainDateTime = readDateTime(event, false, start).kDateTime();
        AlarmMap alarmMap;
        readAlarms(event, &alarmMap);
        for (AlarmMap::ConstIterator it = alarmMap.constBegin();  it != alarmMap.constEnd();  ++it)
        {
            const AlarmData& data = it.value();
            if (!data.alarm->hasStartOffset())
                continue;
            if ((data.type & KAAlarm::DEFERRED_ALARM)
            &&  !(data.type & KAAlarm::TIMED_DEFERRAL_FLAG))
            {
                // Date-only deferral alarm, so adjust its time
                KDateTime altime = data.alarm->startOffset().end(nextMainDateTime);
                altime.setTime(midnight);
                deferralOffset = data.alarm->startOffset().asSeconds();
                newDeferralOffset = event->dtStart().secsTo(altime);
#ifdef USE_AKONADI
                constCast<Alarm::Ptr>(data.alarm)->setStartOffset(newDeferralOffset);
#else
                const_cast<Alarm*>(data.alarm)->setStartOffset(newDeferralOffset);
#endif
                changed = true;
            }
            else if (data.type == KAAlarm::AUDIO__ALARM
            &&       data.alarm->startOffset().asSeconds() == deferralOffset)
            {
                // Audio alarm is set for the same time as the deferral alarm
#ifdef USE_AKONADI
                constCast<Alarm::Ptr>(data.alarm)->setStartOffset(newDeferralOffset);
#else
                const_cast<Alarm*>(data.alarm)->setStartOffset(newDeferralOffset);
#endif
                changed = true;
            }
        }
    }
    return changed;
}

/******************************************************************************
* Convert simple repetitions in an event without a recurrence, to a
* recurrence. Repetitions which are an exact multiple of 24 hours are converted
* to daily recurrences; else they are converted to minutely recurrences. Note
* that daily and minutely recurrences produce different results when they span
* a daylight saving time change.
* Reply = true if any conversions were done.
*/
#ifdef USE_AKONADI
bool KAEvent::convertRepetition(const Event::Ptr& event)
#else
bool KAEvent::convertRepetition(Event* event)
#endif
{
    Alarm::List alarms = event->alarms();
    if (alarms.isEmpty())
        return false;
    Recurrence* recur = event->recurrence();   // guaranteed to return non-null
    if (recur->recurs())
        return false;
    bool converted = false;
    bool readOnly = event->isReadOnly();
    for (int ai = 0, aend = alarms.count();  ai < aend;  ++ai)
    {
#ifdef USE_AKONADI
        Alarm::Ptr alarm = alarms[ai];
#else
        Alarm* alarm = alarms[ai];
#endif
        if (alarm->repeatCount() > 0  &&  alarm->snoozeTime().value() > 0)
        {
            if (!converted)
            {
                event->startUpdates();   // prevent multiple update notifications
                if (readOnly)
                    event->setReadOnly(false);
                if ((alarm->snoozeTime().asSeconds() % (24*3600)) != 0)
                    recur->setMinutely(alarm->snoozeTime().asSeconds() / 60);
                else
                    recur->setDaily(alarm->snoozeTime().asDays());
                recur->setDuration(alarm->repeatCount() + 1);
                converted = true;
            }
            alarm->setRepeatCount(0);
            alarm->setSnoozeTime(0);
        }
    }
    if (converted)
    {
        if (readOnly)
            event->setReadOnly(true);
        event->endUpdates();     // finally issue an update notification
    }
    return converted;
}

#ifdef USE_AKONADI
/******************************************************************************
* Return a KCal::Event's archive custom property value.
* Note that a custom property value cannot be empty, so when there are no
* flags in the archive value, it is set to "0" to make it valid.
* Reply = true if the value exists and has significant content,
*       = false if it doesn't exist, or it is "0".
*/
bool KAEvent::archivePropertyValue(const KCalCore::ConstEventPtr& event, QString& value)
{
    value = event->customProperty(KAlarm::Calendar::APPNAME, Private::ARCHIVE_PROPERTY);
    // A value of "0" is just a flag to make the value non-null
    return !value.isEmpty() && value != QLatin1String("0");
}

KAEvent::List KAEvent::ptrList(QList<KAEvent>& objList)
{
    KAEvent::List ptrs;
    for (int i = 0, count = objList.count();  i < count;  ++i)
        ptrs += &objList[i];
    return ptrs;
}
#endif


#ifndef KDE_NO_DEBUG_OUTPUT
void KAEvent::Private::dumpDebug() const
{
    kDebug() << "KAEvent dump:";
#ifndef USE_AKONADI
    if (mResource) { kDebug() << "-- mResource:" << mResource->resourceName(); }
#endif
    kDebug() << "-- mCommandError:" << mCommandError;
    kDebug() << "-- mAllTrigger:" << mAllTrigger.toString();
    kDebug() << "-- mMainTrigger:" << mMainTrigger.toString();
    kDebug() << "-- mAllWorkTrigger:" << mAllWorkTrigger.toString();
    kDebug() << "-- mMainWorkTrigger:" << mMainWorkTrigger.toString();
    kDebug() << "-- mCategory:" << mCategory;
    baseDumpDebug();
    if (!mTemplateName.isEmpty())
    {
        kDebug() << "-- mTemplateName:" << mTemplateName;
        kDebug() << "-- mTemplateAfterTime:" << mTemplateAfterTime;
    }
    if (mActionType == T_MESSAGE  ||  mActionType == T_FILE)
    {
        kDebug() << "-- mSpeak:" << mSpeak;
        kDebug() << "-- mAudioFile:" << mAudioFile;
        kDebug() << "-- mPreAction:" << mPreAction;
        kDebug() << "-- mCancelOnPreActErr:" << mCancelOnPreActErr;
        kDebug() << "-- mDontShowPreActErr:" << mDontShowPreActErr;
        kDebug() << "-- mPostAction:" << mPostAction;
    }
    else if (mActionType == T_COMMAND)
    {
        kDebug() << "-- mCommandXterm:" << mCommandXterm;
        kDebug() << "-- mCommandDisplay:" << mCommandDisplay;
        kDebug() << "-- mLogFile:" << mLogFile;
    }
    else if (mActionType == T_EMAIL)
    {
        kDebug() << "-- mEmail: FromKMail:" << mEmailFromIdentity;
        kDebug() << "--         Addresses:" << mEmailAddresses.join(",");
        kDebug() << "--         Subject:" << mEmailSubject;
        kDebug() << "--         Attachments:" << mEmailAttachments.join(",");
        kDebug() << "--         Bcc:" << mEmailBcc;
    }
    else if (mActionType == T_AUDIO)
        kDebug() << "-- mAudioFile:" << mAudioFile;
    kDebug() << "-- mBeep:" << mBeep;
    if (mActionType == T_AUDIO  ||  !mAudioFile.isEmpty())
    {
        if (mSoundVolume >= 0)
        {
            kDebug() << "-- mSoundVolume:" << mSoundVolume;
            if (mFadeVolume >= 0)
            {
                kDebug() << "-- mFadeVolume:" << mFadeVolume;
                kDebug() << "-- mFadeSeconds:" << mFadeSeconds;
            }
            else
                kDebug() << "-- mFadeVolume:-:";
        }
        else
            kDebug() << "-- mSoundVolume:-:";
        kDebug() << "-- mRepeatSound:" << mRepeatSound;
    }
    kDebug() << "-- mKMailSerialNumber:" << mKMailSerialNumber;
    kDebug() << "-- mCopyToKOrganizer:" << mCopyToKOrganizer;
    kDebug() << "-- mExcludeHolidays:" << mExcludeHolidays;
    kDebug() << "-- mWorkTimeOnly:" << mWorkTimeOnly;
    kDebug() << "-- mStartDateTime:" << mStartDateTime.toString();
    kDebug() << "-- mSaveDateTime:" << mSaveDateTime;
    if (mRepeatAtLogin)
        kDebug() << "-- mAtLoginDateTime:" << mAtLoginDateTime;
    kDebug() << "-- mArchiveRepeatAtLogin:" << mArchiveRepeatAtLogin;
    kDebug() << "-- mConfirmAck:" << mConfirmAck;
    kDebug() << "-- mEnabled:" << mEnabled;
#ifdef USE_AKONADI
    kDebug() << "-- mItemId:" << mItemId;
    kDebug() << "-- mCompatibility:" << mCompatibility;
    kDebug() << "-- mReadOnly:" << mReadOnly;
#endif
    if (mReminderMinutes)
    {
        kDebug() << "-- mReminderMinutes:" << mReminderMinutes;
        kDebug() << "-- mReminderActive:" << mReminderActive;
        kDebug() << "-- mReminderOnceOnly:" << mReminderOnceOnly;
    }
    else if (mDeferral > 0)
    {
        kDebug() << "-- mDeferral:" << (mDeferral == NORMAL_DEFERRAL ? "normal" : "reminder");
        kDebug() << "-- mDeferralTime:" << mDeferralTime.toString();
    }
    kDebug() << "-- mDeferDefaultMinutes:" << mDeferDefaultMinutes;
    if (mDeferDefaultMinutes)
        kDebug() << "-- mDeferDefaultDateOnly:" << mDeferDefaultDateOnly;
    if (mDisplaying)
    {
        kDebug() << "-- mDisplayingTime:" << mDisplayingTime.toString();
        kDebug() << "-- mDisplayingFlags:" << mDisplayingFlags;
        kDebug() << "-- mDisplayingDefer:" << mDisplayingDefer;
        kDebug() << "-- mDisplayingEdit:" << mDisplayingEdit;
    }
    kDebug() << "-- mRevision:" << mRevision;
    kDebug() << "-- mRecurrence:" << mRecurrence;
    kDebug() << "-- mAlarmCount:" << mAlarmCount;
    kDebug() << "-- mMainExpired:" << mMainExpired;
    kDebug() << "-- mDisplaying:" << mDisplaying;
    kDebug() << "KAEvent dump end";
}
#endif


/*=============================================================================
= Class KAAlarm
= Corresponds to a single KCal::Alarm instance.
=============================================================================*/

KAAlarm::KAAlarm(const KAAlarm& alarm)
    : KAAlarmEventBase(alarm),
      mType(alarm.mType),
      mRecurs(alarm.mRecurs),
      mDeferred(alarm.mDeferred)
{ }

#ifndef KDE_NO_DEBUG_OUTPUT
void KAAlarm::dumpDebug() const
{
    kDebug() << "KAAlarm dump:";
    baseDumpDebug();
    const char* altype = 0;
    switch (mType)
    {
        case MAIN__ALARM:                    altype = "MAIN";  break;
        case REMINDER__ALARM:                altype = "REMINDER";  break;
        case DEFERRED_DATE__ALARM:           altype = "DEFERRED(DATE)";  break;
        case DEFERRED_TIME__ALARM:           altype = "DEFERRED(TIME)";  break;
        case DEFERRED_REMINDER_DATE__ALARM:  altype = "DEFERRED_REMINDER(DATE)";  break;
        case DEFERRED_REMINDER_TIME__ALARM:  altype = "DEFERRED_REMINDER(TIME)";  break;
        case AT_LOGIN__ALARM:                altype = "LOGIN";  break;
        case DISPLAYING__ALARM:              altype = "DISPLAYING";  break;
        case AUDIO__ALARM:                   altype = "AUDIO";  break;
        case PRE_ACTION__ALARM:              altype = "PRE_ACTION";  break;
        case POST_ACTION__ALARM:             altype = "POST_ACTION";  break;
        default:                             altype = "INVALID";  break;
    }
    kDebug() << "-- mType:" << altype;
    kDebug() << "-- mRecurs:" << mRecurs;
    kDebug() << "-- mDeferred:" << mDeferred;
    kDebug() << "KAAlarm dump end";
}

const char* KAAlarm::debugType(Type type)
{
    switch (type)
    {
        case MAIN_ALARM:               return "MAIN";
        case REMINDER_ALARM:           return "REMINDER";
        case DEFERRED_ALARM:           return "DEFERRED";
        case DEFERRED_REMINDER_ALARM:  return "DEFERRED_REMINDER";
        case AT_LOGIN_ALARM:           return "LOGIN";
        case DISPLAYING_ALARM:         return "DISPLAYING";
        case AUDIO_ALARM:              return "AUDIO";
        case PRE_ACTION_ALARM:         return "PRE_ACTION";
        case POST_ACTION_ALARM:        return "POST_ACTION";
        default:                       return "INVALID";
    }
}
#endif


/*=============================================================================
= Class KAAlarmEventBase
=============================================================================*/

void KAAlarmEventBase::copy(const KAAlarmEventBase& rhs)
{
    mEventID           = rhs.mEventID;
    mText              = rhs.mText;
    mNextMainDateTime  = rhs.mNextMainDateTime;
    mBgColour          = rhs.mBgColour;
    mFgColour          = rhs.mFgColour;
    mFont              = rhs.mFont;
    mActionType        = rhs.mActionType;
    mCommandScript     = rhs.mCommandScript;
    mRepetition        = rhs.mRepetition;
    mNextRepeat        = rhs.mNextRepeat;
    mRepeatAtLogin     = rhs.mRepeatAtLogin;
    mLateCancel        = rhs.mLateCancel;
    mAutoClose         = rhs.mAutoClose;
    mUseDefaultFont    = rhs.mUseDefaultFont;
}

void KAAlarmEventBase::set(int flags)
{
    mRepeatAtLogin  = flags & KAEvent::REPEAT_AT_LOGIN;
    mAutoClose      = (flags & KAEvent::AUTO_CLOSE) && mLateCancel;
    mUseDefaultFont = flags & KAEvent::DEFAULT_FONT;
    mCommandScript  = flags & KAEvent::SCRIPT;
}

int KAAlarmEventBase::baseFlags() const
{
    return (mRepeatAtLogin  ? KAEvent::REPEAT_AT_LOGIN : 0)
         | (mAutoClose      ? KAEvent::AUTO_CLOSE : 0)
         | (mUseDefaultFont ? KAEvent::DEFAULT_FONT : 0)
         | (mCommandScript  ? KAEvent::SCRIPT : 0);
}

#ifndef KDE_NO_DEBUG_OUTPUT
void KAAlarmEventBase::baseDumpDebug() const
{
    kDebug() << "-- mEventID:" << mEventID;
    kDebug() << "-- mActionType:" << (mActionType == T_MESSAGE ? "MESSAGE" : mActionType == T_FILE ? "FILE" : mActionType == T_COMMAND ? "COMMAND" : mActionType == T_EMAIL ? "EMAIL" : mActionType == T_AUDIO ? "AUDIO" : "??");
    kDebug() << "-- mText:" << mText;
    if (mActionType == T_COMMAND)
        kDebug() << "-- mCommandScript:" << mCommandScript;
    kDebug() << "-- mNextMainDateTime:" << mNextMainDateTime.toString();
    kDebug() << "-- mBgColour:" << mBgColour.name();
    kDebug() << "-- mFgColour:" << mFgColour.name();
    kDebug() << "-- mUseDefaultFont:" << mUseDefaultFont;
    if (!mUseDefaultFont)
        kDebug() << "-- mFont:" << mFont.toString();
    kDebug() << "-- mRepeatAtLogin:" << mRepeatAtLogin;
    if (!mRepetition)
        kDebug() << "-- mRepetition: 0";
    else if (mRepetition.isDaily())
        kDebug() << "-- mRepetition: count:" << mRepetition.count() << ", interval:" << mRepetition.intervalDays() << "days";
    else
        kDebug() << "-- mRepetition: count:" << mRepetition.count() << ", interval:" << mRepetition.intervalMinutes() << "minutes";
    kDebug() << "-- mNextRepeat:" << mNextRepeat;
    kDebug() << "-- mLateCancel:" << mLateCancel;
    kDebug() << "-- mAutoClose:" << mAutoClose;
}
#endif


/*=============================================================================
= Class EmailAddressList
=============================================================================*/

/******************************************************************************
* Sets the list of email addresses, removing any empty addresses.
* Reply = false if empty addresses were found.
*/
#ifdef USE_AKONADI
EmailAddressList& EmailAddressList::operator=(const Person::List& addresses)
#else
EmailAddressList& EmailAddressList::operator=(const QList<Person>& addresses)
#endif
{
    clear();
    for (int p = 0, end = addresses.count();  p < end;  ++p)
    {
#ifdef USE_AKONADI
        if (!addresses[p]->email().isEmpty())
#else
        if (!addresses[p].email().isEmpty())
#endif
            append(addresses[p]);
    }
    return *this;
}

/******************************************************************************
* Return the email address list as a string list of email addresses.
*/
EmailAddressList::operator QStringList() const
{
    QStringList list;
    for (int p = 0, end = count();  p < end;  ++p)
        list += address(p);
    return list;
}

/******************************************************************************
* Return the email address list as a string, each address being delimited by
* the specified separator string.
*/
QString EmailAddressList::join(const QString& separator) const
{
    QString result;
    bool first = true;
    for (int p = 0, end = count();  p < end;  ++p)
    {
        if (first)
            first = false;
        else
            result += separator;
        result += address(p);
    }
    return result;
}

/******************************************************************************
* Convert one item into an email address, including name.
*/
QString EmailAddressList::address(int index) const
{
    if (index < 0  ||  index > count())
        return QString();
    QString result;
    bool quote = false;
#ifdef USE_AKONADI
    Person::Ptr person = (*this)[index];
    QString name = person->name();
#else
    Person person = (*this)[index];
    QString name = person.name();
#endif
    if (!name.isEmpty())
    {
        // Need to enclose the name in quotes if it has any special characters
        int len = name.length();
        for (int i = 0;  i < len;  ++i)
        {
            QChar ch = name[i];
            if (!ch.isLetterOrNumber())
            {
                quote = true;
                result += '\"';
                break;
            }
        }
#ifdef USE_AKONADI
        result += (*this)[index]->name();
#else
        result += (*this)[index].name();
#endif
        result += (quote ? "\" <" : " <");
        quote = true;    // need angle brackets round email address
    }

#ifdef USE_AKONADI
    result += person->email();
#else
    result += person.email();
#endif
    if (quote)
        result += '>';
    return result;
}

/******************************************************************************
* Return a list of the pure email addresses, excluding names.
*/
QStringList EmailAddressList::pureAddresses() const
{
    QStringList list;
    for (int p = 0, end = count();  p < end;  ++p)
#ifdef USE_AKONADI
        list += at(p)->email();
#else
        list += at(p).email();
#endif
    return list;
}

/******************************************************************************
* Return a list of the pure email addresses, excluding names, as a string.
*/
QString EmailAddressList::pureAddresses(const QString& separator) const
{
    QString result;
    bool first = true;
    for (int p = 0, end = count();  p < end;  ++p)
    {
        if (first)
            first = false;
        else
            result += separator;
#ifdef USE_AKONADI
        result += at(p)->email();
#else
        result += at(p).email();
#endif
    }
    return result;
}


/*=============================================================================
= Static functions
=============================================================================*/

/******************************************************************************
* Set the specified alarm to be a procedure alarm with the given command line.
* The command line is first split into its program file and arguments before
* initialising the alarm.
*/
#ifdef USE_AKONADI
static void setProcedureAlarm(const Alarm::Ptr& alarm, const QString& commandLine)
#else
static void setProcedureAlarm(Alarm* alarm, const QString& commandLine)
#endif
{
    QString command;
    QString arguments;
    QChar quoteChar;
    bool quoted = false;
    uint posMax = commandLine.length();
    uint pos;
    for (pos = 0;  pos < posMax;  ++pos)
    {
        QChar ch = commandLine[pos];
        if (quoted)
        {
            if (ch == quoteChar)
            {
                ++pos;    // omit the quote character
                break;
            }
            command += ch;
        }
        else
        {
            bool done = false;
            switch (ch.toAscii())
            {
                case ' ':
                case ';':
                case '|':
                case '<':
                case '>':
                    done = !command.isEmpty();
                    break;
                case '\'':
                case '"':
                    if (command.isEmpty())
                    {
                        // Start of a quoted string. Omit the quote character.
                        quoted = true;
                        quoteChar = ch;
                        break;
                    }
                    // fall through to default
                default:
                    command += ch;
                    break;
            }
            if (done)
                break;
        }
    }

    // Skip any spaces after the command
    for ( ;  pos < posMax  &&  commandLine[pos] == QLatin1Char(' ');  ++pos) ;
    arguments = commandLine.mid(pos);

    alarm->setProcedureAlarm(command, arguments);
}

// vim: et sw=4:
