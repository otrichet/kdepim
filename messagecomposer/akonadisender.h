/*
 * This file is part of KMail.
 * Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef AKONADISENDER_H
#define AKONADISENDER_H

#include "messagesender.h"
#include "messagecomposer_export.h"

#include <QObject>
#include <QString>
#include <QSet>

#include <kmime/kmime_message.h>

class KJob;


class MESSAGECOMPOSER_EXPORT AkonadiSender: public QObject, public MessageSender
{
  Q_OBJECT

public:
  AkonadiSender();

protected:
  /**
    Send given message. The message is either queued or sent
    immediately. The default behaviour, as selected with
    setSendImmediate(), can be overwritten with the parameter
    sendNow (by specifying true or false).
    The sender takes ownership of the given message on success,
    so DO NOT DELETE OR MODIFY the message further.
    Returns true on success.

    TODO cberzan: update docu...
   */
  virtual bool doSend( const KMime::Message::Ptr &msg, short sendNow );

  /**
    Send queued messages, using the specified transport or the
    default, if none is given.
   */
  virtual bool doSendQueued( const QString &transport = QString() );

private:
  /**
    Queue or send immediately one message using MailTransport::MessageQueueJob.
  */
  void sendOrQueueMessage( const KMime::Message::Ptr &msg, MessageSender::SendMethod method = MessageSender::SendDefault );

private slots:
  void queueJobResult( KJob *job );

private:
  QString mCustomTransport;
  QSet<KJob*> mPendingJobs;

};

#endif /* AKONADISENDER_H */
