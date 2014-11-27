/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef NOTE_ALARM_ATTRIBUTE_H
#define NOTE_ALARM_ATTRIBUTE_H

#include "noteshared_export.h"
#include <AkonadiCore/Attribute>

#include <QDateTime>
namespace NoteShared
{
class NOTESHARED_EXPORT NoteAlarmAttribute : public Akonadi::Attribute
{
public:
    NoteAlarmAttribute();
    ~NoteAlarmAttribute();

    QByteArray type() const Q_DECL_OVERRIDE;

    NoteAlarmAttribute *clone() const Q_DECL_OVERRIDE;

    QByteArray serialized() const Q_DECL_OVERRIDE;

    void deserialize(const QByteArray &data) Q_DECL_OVERRIDE;

    void setDateTime(const QDateTime &dateTime);
    QDateTime dateTime() const;

private:
    QDateTime mDateTime;
};
}

#endif
