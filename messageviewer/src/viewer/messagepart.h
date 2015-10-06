/*
   Copyright (c) 2015 Sandro Knauß <sknauss@kde.org>

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

#ifndef _MESSAGEVIEWER_MESSAGEPART_H_
#define _MESSAGEVIEWER_MESSAGEPART_H_

#include "objecttreeparser.h"

#include <QString>

class QTextCodec;

namespace MessageViewer {
class HTMLBlock
{
public:
    HTMLBlock()
    : entered(false)
    { }

    virtual ~HTMLBlock() { }

protected:
    bool entered;
};

class CryptoBlock: public HTMLBlock
{
public:
    CryptoBlock(ObjectTreeParser *otp,
                PartMetaData *block,
                const Kleo::CryptoBackend::Protocol *cryptoProto,
                const QString &fromAddress,
                KMime::Content *node);
    virtual ~CryptoBlock();

private:
    void internalEnter();
    void internalExit();

    ObjectTreeParser *mOtp;
    PartMetaData *mMetaData;
    const Kleo::CryptoBackend::Protocol *mCryptoProto;
    QString mFromAddress;
    KMime::Content *mNode;
};

class MessagePart
{
public:
    typedef QSharedPointer<MessagePart> Ptr;
    MessagePart(ObjectTreeParser *otp,
                PartMetaData *block,
                const QString &text);

    virtual QString text() const;
    void setText(const QString &text);
    virtual void html(bool decorate) const;

    PartMetaData *partMetaData() const;

protected:
    QString mText;
    ObjectTreeParser *mOtp;
    PartMetaData *mMetaData;
};

class CryptoMessagePart : public MessagePart
{
public:
    typedef QSharedPointer<CryptoMessagePart> Ptr;
    CryptoMessagePart(ObjectTreeParser *otp,
                PartMetaData *block,
                const QString &text,
                const Kleo::CryptoBackend::Protocol *cryptoProto,
                const QString &fromAddress,
                KMime::Content *node);

    void startDecryption(const QByteArray &text, const QTextCodec *aCodec);
    void startDecryption(KMime::Content *data = 0);
    void startVerification(const QByteArray &text, const QTextCodec *aCodec);
    void startVerificationDetached(const QByteArray& text, KMime::Content* textNode, const QByteArray& signature );
    void html(bool decorate) const Q_DECL_OVERRIDE;

    bool mPassphraseError;
    QByteArray mDecryptedData;
    std::vector<GpgME::Signature> mSignatures;

private:
    /** Writes out the block that we use when the node is encrypted,
      but we're deferring decryption for later. */
    void writeDeferredDecryptionBlock() const;

protected:
    const Kleo::CryptoBackend::Protocol *mCryptoProto;
    QString mFromAddress;
    KMime::Content *mNode, *mTextNode;
    bool mDecryptMessage;
    QByteArray mVerifiedText;
};

}
#endif //_MESSAGEVIEWER_MESSAGEPART_H_
