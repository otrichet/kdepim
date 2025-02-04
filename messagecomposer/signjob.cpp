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

#include "signjob.h"

#include "contentjobbase_p.h"
#include "kleo/cryptobackendfactory.h"
#include "kleo/cryptobackend.h"
#include "kleo/enum.h"
#include "kleo/signjob.h"
#include "util.h"

#include <kdebug.h>
#include <kmime/kmime_message.h>
#include <kmime/kmime_content.h>
#include <kmime/kmime_util.h>
#include <QBuffer>

#include <gpgme++/global.h>
#include <gpgme++/signingresult.h>
#include <gpgme++/encryptionresult.h>
#include <sstream>

using namespace Message;

class Message::SignJobPrivate : public ContentJobBasePrivate
{
  public:
    SignJobPrivate( SignJob *qq )
      : ContentJobBasePrivate( qq )
      , content( 0 )
    {
    }

    KMime::Content* content;
    std::vector<GpgME::Key> signers;
    Kleo::CryptoMessageFormat format;

    // copied from messagecomposer.cpp
    bool binaryHint( Kleo::CryptoMessageFormat f )
    {
      switch ( f ) {
      case Kleo::SMIMEFormat:
      case Kleo::SMIMEOpaqueFormat:
        return true;
      default:
      case Kleo::OpenPGPMIMEFormat:
      case Kleo::InlineOpenPGPFormat:
        return false;
      }
    }


    GpgME::SignatureMode signingMode( Kleo::CryptoMessageFormat f )
    {
      switch ( f ) {
      case Kleo::SMIMEOpaqueFormat:
        return GpgME::NormalSignatureMode;
      case Kleo::InlineOpenPGPFormat:
        return GpgME::Clearsigned;
      default:
      case Kleo::SMIMEFormat:
      case Kleo::OpenPGPMIMEFormat:
        return GpgME::Detached;
      }
    }

    Q_DECLARE_PUBLIC( SignJob )
};

SignJob::SignJob( QObject *parent )
  : ContentJobBase( *new SignJobPrivate( this ), parent )
{
}

SignJob::~SignJob()
{
}

void SignJob::setContent( KMime::Content* content )
{
  Q_D( SignJob );

  d->content = content;
}

void SignJob::setCryptoMessageFormat( Kleo::CryptoMessageFormat format)
{
  Q_D( SignJob );

  // There *must* be a concrete format set at this point.
  Q_ASSERT( format == Kleo::OpenPGPMIMEFormat
            || format == Kleo::InlineOpenPGPFormat
            || format == Kleo::SMIMEFormat
            || format == Kleo::SMIMEOpaqueFormat );
  d->format = format;
}

void SignJob::setSigningKeys( std::vector<GpgME::Key>& signers )
{
  Q_D( SignJob );

  d->signers = signers;
}

KMime::Content* SignJob::origContent()
{
  Q_D( SignJob );

  return d->content;

}

void SignJob::process()
{
  Q_D( SignJob );
  Q_ASSERT( d->resultContent == 0 ); // Not processed before.

  // if setContent hasn't been called, we assume that a subjob was added
  // and we want to use that
  if( !d->content ) {
    Q_ASSERT( d->subjobContents.size() == 1 );
    d->content = d->subjobContents.first();
  }
  
  d->resultContent = new KMime::Content;

  const Kleo::CryptoBackend::Protocol *proto = 0;
  if( d->format & Kleo::AnyOpenPGP ) {
    proto = Kleo::CryptoBackendFactory::instance()->openpgp();
  } else if( d->format & Kleo::AnySMIME ) {
    proto = Kleo::CryptoBackendFactory::instance()->smime();
  }

  Q_ASSERT( proto );


  kDebug() << "creating signJob from:" << proto->name() << proto->displayName();
  std::auto_ptr<Kleo::SignJob> job( proto->signJob( !d->binaryHint( d->format ), d->format == Kleo::InlineOpenPGPFormat ) );
  // for now just do the main recipients
  QByteArray signature;

  // replace simple LFs by CRLFs for all MIME supporting CryptPlugs
  // according to RfC 2633, 3.1.1 Canonicalization
  d->content->assemble();
  QByteArray content;
  // 

  if( d->format & Kleo::InlineOpenPGPFormat  &&
    !( d->format & Kleo::SMIMEOpaqueFormat ) ) {
    content = KMime::LFtoCRLF( d->content->body() );
  } else if( !( d->format & Kleo::SMIMEOpaqueFormat ) ) {
    content = KMime::LFtoCRLF( d->content->encodedContent() );
  } else { // SMimeOpaque doesn't need LFtoCRLF, else it gets munged
    content = d->content->encodedContent();
  }
  
  // FIXME: Make this async
  GpgME::SigningResult res = job->exec( d->signers,
                                        content,
                                        d->signingMode( d->format ),
                                        signature );

  if ( res.error() ) {
    kDebug() << "signing failed:" << res.error().asString();
    //        job->showErrorDialog( globalPart()->parentWidgetForGui() );
    setError( res.error().code() );
    setErrorText( QString::fromLocal8Bit( res.error().asString() ) );
  }

  // exec'ed jobs don't delete themselves
  job->deleteLater();

  QByteArray signatureHashAlgo =  res.createdSignature( 0 ).hashAlgorithmAsString();
  
  d->resultContent = Message::Util::composeHeadersAndBody( d->content, signature, d->format, true, signatureHashAlgo );
  emitResult();
}

#include "signjob.moc"
