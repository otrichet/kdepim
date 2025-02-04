/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKONADI_NEPOMUK_EMAIL_FEEDER_H
#define AKONADI_NEPOMUK_EMAIL_FEEDER_H

#include <nepomukfeederagent.h>

#include <mailbox.h>
#include <contact.h>

#include <QtCore/QList>
#include <akonadi/agentsearchinterface.h>
#include <kmime/kmime_header_parsing.h>

namespace Akonadi {

class NepomukEMailFeeder : public NepomukFeederAgent<NepomukFast::Mailbox>, public AgentSearchInterface
{
  Q_OBJECT
  public:
    NepomukEMailFeeder( const QString &id );
    void configure(WId windowId);

    void updateItem( const Akonadi::Item &item, const QUrl &graphUri );

    void addSearch(const QString& query, const QString& queryLanguage, const Akonadi::Collection& resultCollection);
    void removeSearch(const Akonadi::Collection& resultCollection);

  protected:
    ItemFetchScope fetchScopeForCollection(const Akonadi::Collection& collection);
};

}

#endif
