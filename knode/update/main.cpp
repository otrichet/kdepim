/*
  Copyright 2010 Olivier Trichet <nive@nivalis.org>

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include "aboutdata.h"
#include "legacy_identity.h"
#include "akonadi_migrator.h"

#include <KApplication>
#include <KCmdLineArgs>
#include <KConfig>
#include <KDebug>
#include <KLocale>
#include <stdlib.h>

using namespace KNode::Update;


void run( UpdaterBase *updater )
{
  Q_ASSERT( updater );
  kDebug() << "Running updater" << updater->id() << updater->name();


  static QList<int> seenUpdaterIds;
  Q_ASSERT( !seenUpdaterIds.contains( updater->id() ) );
  seenUpdaterIds.append( updater->id() );


  KSharedConfig::Ptr conf = KSharedConfig::openConfig( "knode-migrationrc" );
  KConfigGroup cg( conf, "Update" );
  QList<int> updateIds = cg.readEntry( "updateIds", QList<int>() );

  if ( !updateIds.contains( updater->id() ) ) {
    updater->update();
    // TODO: check error
    updateIds.append( updater->id() );

    cg.writeEntry( "updateIds", updateIds );
    cg.sync();
  }

  if ( updater->error() == UpdaterBase::Error ) {
    delete updater;
    exit( 10 );
  }
  delete updater;
}


int main( int argc, char *argv[] )
{
  KNode::AboutData aboutData;
  aboutData.setAppName( "knode-migrator" );
  aboutData.setCatalogName( "knode" );
  aboutData.setProgramName( ki18n( "KNode's data and configuration migrator" ) );
  aboutData.setVersion( "1.0" );

  KCmdLineArgs::init( argc, argv, &aboutData );

  KApplication app;


  // Force a kconf_update run (ensure the identity converter is not run again)
  KConfig knodeConf( "knode" );
  knodeConf.checkUpdate( "002", "knode.upd" );

  run( new LegacyIdentity() );
  run( new AkonadiMigrator() );

  return 0;
}
