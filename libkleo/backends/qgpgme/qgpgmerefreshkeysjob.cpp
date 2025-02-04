/*
    qgpgmerefreshkeysjob.cpp

    This file is part of libkleopatra, the KDE keymanagement library
    Copyright (c) 2004 Klar�vdalens Datakonsult AB

    Libkleopatra is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    Libkleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#define MAX_CMD_LENGTH 32768

#include "qgpgmerefreshkeysjob.h"

#include "gnupgprocessbase.h"
#include "qgpgmeprogresstokenmapper.h"

#include <kdebug.h>

#include <gpgme++/context.h>

#include <QByteArray>
#include <QStringList>

#include <gpg-error.h>

#include <assert.h>

Kleo::QGpgMERefreshKeysJob::QGpgMERefreshKeysJob()
  : RefreshKeysJob( 0 ),
    mProcess( 0 ),
    mError( 0 )
{

}

Kleo::QGpgMERefreshKeysJob::~QGpgMERefreshKeysJob() {

}

GpgME::Error Kleo::QGpgMERefreshKeysJob::start( const QStringList & patterns ) {
  assert( mPatternsToDo.empty() );

  mPatternsToDo = patterns;
  if ( mPatternsToDo.empty() )
    mPatternsToDo.push_back( " " ); // empty list means all -> mae
				    // sure to fail the first
				    // startAProcess() guard clause

  return startAProcess();
}

#if MAX_CMD_LENGTH < 65 + 128
#error MAX_CMD_LENGTH is too low
#endif

GpgME::Error Kleo::QGpgMERefreshKeysJob::startAProcess() {
  if ( mPatternsToDo.empty() )
    return GpgME::Error();
  // create and start gpgsm process:
  mProcess = new GnuPGProcessBase( this );
  mProcess->setObjectName( "gpgsm -k --with-validation --force-crl-refresh --enable-crl-checks" );

  // FIXME: obbtain the path to gpgsm from gpgme, so we use the same instance.
  *mProcess << "gpgsm" << "-k" << "--with-validation" << "--force-crl-refresh"
	    << "--enable-crl-checks";
  unsigned int commandLineLength = MAX_CMD_LENGTH;
  commandLineLength -=
    strlen("gpgsm") + 1 + strlen("-k") + 1 +
    strlen("--with-validation") + 1 + strlen("--force-crl-refresh") + 1 +
    strlen("--enable-crl-checks") + 1;
  while ( !mPatternsToDo.empty() ) {
    const QByteArray pat = mPatternsToDo.front().toUtf8().trimmed();
    const unsigned int patLength = pat.length();
    if ( patLength >= commandLineLength )
      break;
    mPatternsToDo.pop_front();
    if ( pat.isEmpty() )
      continue;
    *mProcess << pat;
    commandLineLength -= patLength + 1;
  }

  mProcess->setUseStatusFD( true );

  connect( mProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
	   SLOT(slotProcessExited( int, QProcess::ExitStatus)) );
  connect( mProcess, SIGNAL(readyReadStandardOutput()),
	   SLOT(slotStdout()) );
  connect( mProcess, SIGNAL(readyReadStandardError()),
	   SLOT(slotStderr()) );

  connect( mProcess, SIGNAL(status(Kleo::GnuPGProcessBase*,const QString&,const QStringList&)),
	   SLOT(slotStatus(Kleo::GnuPGProcessBase*,const QString&,const QStringList&)) );

  mProcess->setOutputChannelMode( KProcess::SeparateChannels );
  mProcess->start();
  if ( !mProcess->waitForStarted() ) {
    mError = GpgME::Error::fromCode( GPG_ERR_ENOENT, GPG_ERR_SOURCE_GPGSM ); // what else?
    deleteLater();
    return mError;
  } else
    return GpgME::Error();
}

void Kleo::QGpgMERefreshKeysJob::slotCancel() {
  if ( mProcess )
    mProcess->kill();
  mProcess = 0;
  mError = GpgME::Error::fromCode( GPG_ERR_CANCELED, GPG_ERR_SOURCE_GPGSM );
}

void Kleo::QGpgMERefreshKeysJob::slotStatus( GnuPGProcessBase * proc, const QString & type, const QStringList & args ) {
  if ( proc != mProcess )
    return;
  QStringList::const_iterator it = args.begin();
  bool ok = false;

  if ( type == "ERROR" ) {


    if ( args.size() < 2 ) {
      kDebug( 5150 ) <<"not recognising ERROR with < 2 args!";
      return;
    }
    const int source = (*++it).toInt( &ok );
    if ( !ok ) {
      kDebug( 5150 ) <<"expected number for first ERROR arg, got something else";
      return;
    }
    ok = false;
    const int code = (*++it).toInt( &ok );
    if ( !ok ) {
      kDebug( 5150 ) <<"expected number for second ERROR arg, got something else";
      return;
    }
    mError = GpgME::Error::fromCode( code, source );


  } else if ( type == "PROGRESS" ) {


    if ( args.size() < 4 ) {
      kDebug( 5150 ) <<"not recognising PROGRESS with < 4 args!";
      return;
    }
    const QString what = *++it;
    ok = false;
    const int typ = (*++it).toInt( &ok );
    if ( !ok ) {
        kDebug( 5150 ) <<"expected number for \"type\", got something else";
        return;
    }
    ok = false;
    const int cur = (*++it).toInt( &ok );
    if ( !ok ) {
      kDebug( 5150 ) <<"expected number for \"cur\", got something else";
      return;
    }
    ok = false;
    const int total = (*++it).toInt( &ok );
    if ( !ok ) {
      kDebug( 5150 ) <<"expected number for \"total\", got something else";
      return;
    }
    emit progress( QGpgMEProgressTokenMapper::map( what, typ ), cur, total );


  }
}

void Kleo::QGpgMERefreshKeysJob::slotStderr() {
  // implement? or not?
}

void Kleo::QGpgMERefreshKeysJob::slotProcessExited(int exitCode, QProcess::ExitStatus exitStatus) {
  if ( !mError && !mPatternsToDo.empty() ) {
    if ( const GpgME::Error err = startAProcess() )
      mError = err;
    else
      return;
  }

  emit done();
  if ( !mError &&
       ( exitStatus != QProcess::NormalExit || exitCode != 0 ) )
    mError = GpgME::Error::fromCode( GPG_ERR_GENERAL, GPG_ERR_SOURCE_GPGSM );
  emit result( mError );
  deleteLater();
}

#include "qgpgmerefreshkeysjob.moc"
