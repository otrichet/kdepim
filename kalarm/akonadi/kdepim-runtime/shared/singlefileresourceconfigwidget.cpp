/*
    Copyright (c) 2008 Bertjan Broeksema <b.broeksema@kdemail.org>
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 David Jarvie <djarvie@kde.org>

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

#include "singlefileresourceconfigwidget.h"

#include <KFileItem>
#include <KIO/Job>

#include <QTimer>

using namespace Akonadi;

SingleFileResourceConfigWidget::SingleFileResourceConfigWidget( QWidget *parent )
  : QWidget( parent ),
    mStatJob( 0 ),
    mDirUrlChecked( false ),
    mMonitorEnabled( true ),
    mLocalFileOnly( false )
{
  ui.setupUi( this );
  ui.kcfg_Path->setMode( KFile::File );
#ifndef KDEPIM_MOBILE_UI
  ui.statusLabel->setText( QString() );
#endif

  connect( ui.kcfg_Path, SIGNAL(textChanged(QString)), SLOT(validate()) );
  connect( ui.kcfg_ReadOnly, SIGNAL(toggled(bool)), SLOT(validate()) );
  connect( ui.kcfg_MonitorFile, SIGNAL(toggled(bool)), SLOT(validate()) );
  ui.kcfg_Path->setFocus();
  QTimer::singleShot( 0, this, SLOT(validate()) );
}

void SingleFileResourceConfigWidget::setFilter(const QString & filter)
{
  ui.kcfg_Path->setFilter( filter );
}

void SingleFileResourceConfigWidget::setMonitorEnabled( bool enable )
{
  mMonitorEnabled = enable;
#ifdef KDEPIM_MOBILE_UI
  ui.kcfg_MonitorFile->setVisible( mMonitorEnabled );
#else
  ui.groupBox_MonitorFile->setVisible( mMonitorEnabled );
#endif
}

void SingleFileResourceConfigWidget::setUrl(const KUrl &url )
{
  ui.kcfg_Path->setUrl( url );
}

KUrl SingleFileResourceConfigWidget::url() const
{
  return ui.kcfg_Path->url();
}

void SingleFileResourceConfigWidget::setLocalFileOnly( bool local )
{
  mLocalFileOnly = local;
  ui.kcfg_Path->setMode( mLocalFileOnly ? KFile::File | KFile::LocalOnly : KFile::File );
}

void SingleFileResourceConfigWidget::validate()
{
  const KUrl currentUrl = ui.kcfg_Path->url();
  if ( currentUrl.isEmpty() ) {
    emit validated( false );
    return;
  }

  if ( currentUrl.isLocalFile() ) {
    if ( mMonitorEnabled ) {
      ui.kcfg_MonitorFile->setEnabled( true );
    }
#ifndef KDEPIM_MOBILE_UI
    ui.statusLabel->setText( QString() );
#endif

    const QFileInfo file( currentUrl.toLocalFile() );
    if ( file.exists() && file.isFile() && !file.isWritable() ) {
      ui.kcfg_ReadOnly->setEnabled( false );
      ui.kcfg_ReadOnly->setChecked( true );
    } else {
      ui.kcfg_ReadOnly->setEnabled( true );
    }
    emit validated( true );
  } else {
    if ( mLocalFileOnly ) {
      emit validated( false );
      return;
    }
    if ( mMonitorEnabled ) {
      ui.kcfg_MonitorFile->setEnabled( false );
    }
#ifndef KDEPIM_MOBILE_UI
    ui.statusLabel->setText( i18nc( "@info:status", "Checking file information..." ) );
#endif

    if ( mStatJob )
      mStatJob->kill();

    mStatJob = KIO::stat( currentUrl, KIO::DefaultFlags | KIO::HideProgressInfo );
    mStatJob->setDetails( 2 ); // All details.
    mStatJob->setSide( KIO::StatJob::SourceSide );

    connect( mStatJob, SIGNAL( result( KJob * ) ),
             SLOT( slotStatJobResult( KJob * ) ) );

    // Allow the OK button to be disabled until the MetaJob is finished.
    emit validated( false );
  }
}

void SingleFileResourceConfigWidget::slotStatJobResult( KJob* job )
{
  if ( job->error() == KIO::ERR_DOES_NOT_EXIST && !mDirUrlChecked ) {
    // The file did not exists, so let's see if the directory the file should
    // reside supports writing.
    const KUrl dirUrl = ui.kcfg_Path->url().upUrl();

    mStatJob = KIO::stat( dirUrl, KIO::DefaultFlags | KIO::HideProgressInfo );
    mStatJob->setDetails( 2 ); // All details.
    mStatJob->setSide( KIO::StatJob::SourceSide );

    connect( mStatJob, SIGNAL( result( KJob * ) ),
             SLOT( slotStatJobResult( KJob * ) ) );

    // Make sure we don't check the whole path upwards.
    mDirUrlChecked = true;
    return;
  } else if ( job->error() ) {
    // It doesn't seem possible to read nor write from the location so leave the
    // ok button disabled
#ifndef KDEPIM_MOBILE_UI
    ui.statusLabel->setText( QString() );
#endif
    emit validated( false );
    mDirUrlChecked = false;
    mStatJob = 0;
    return;
  }

  KIO::StatJob* statJob = qobject_cast<KIO::StatJob *>( job );
  const KFileItem item( statJob->statResult(), KUrl() );

  if ( item.isWritable() ) {
    ui.kcfg_ReadOnly->setEnabled( true );
  } else {
    ui.kcfg_ReadOnly->setEnabled( false );
    ui.kcfg_ReadOnly->setChecked( true );
  }

#ifndef KDEPIM_MOBILE_UI
  ui.statusLabel->setText( QString() );
#endif
  emit validated( true );

  mDirUrlChecked = false;
  mStatJob = 0;
}
