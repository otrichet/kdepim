/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "collectionmodifyjob.h"
#include "imapparser.h"

using namespace Akonadi;

class Akonadi::CollectionModifyJobPrivate
{
  public:
    QString path;
    QList<QByteArray> mimeTypes;
    bool setMimeTypes;
};

CollectionModifyJob::CollectionModifyJob(const QString &path, QObject * parent) :
    Job( parent ), d( new CollectionModifyJobPrivate )
{
  d->path = path;
  d->setMimeTypes = false;
}

CollectionModifyJob::~ CollectionModifyJob()
{
  delete d;
}

void CollectionModifyJob::doStart()
{
  QByteArray command = newTag() + " MODIFY \"" + d->path.toUtf8() + '\"';
  if ( d->setMimeTypes )
    command += " MIMETYPES (" + ImapParser::join( d->mimeTypes, " " ) + ')';
  writeData( command );
}

void CollectionModifyJob::setContentTypes(const QList< QByteArray > & mimeTypes)
{
  d->setMimeTypes = true;
  d->mimeTypes = mimeTypes;
}

#include "collectionmodifyjob.moc"
