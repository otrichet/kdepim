/*
  Copyright (C) 2009 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>

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

#ifndef ABSTRACT_ENCRYPT_JOB_H
#define ABSTRACT_ENCRYPT_JOB_H

#include <QStringList>

#include <gpgme++/key.h>
#include <vector>

/**
  * Simple interface that both EncryptJob and SignEncryptJob implement
  * so the composer can extract some encryption-specific job info from them
  */

class AbstractEncryptJob
{
  public:
    virtual ~AbstractEncryptJob() {}

    /**
     * Set the list of encryption keys that should be used.
     */
    virtual void setEncryptionKeys( std::vector<GpgME::Key> keys ) = 0;

    /**
     * Set the recipients that this message should be encrypted to.
     */
    virtual void setRecipients( QStringList rec ) = 0;

    virtual std::vector<GpgME::Key> encryptionKeys() = 0;
    virtual QStringList recipients() = 0;
};

#endif
