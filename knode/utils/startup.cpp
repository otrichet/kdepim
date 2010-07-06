/*
 Copyright 2009 Olivier Trichet <nive@nivalis.org>

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

#include "utils/startup.h"

#include "akobackit/akonadi_manager.h"
#include "akobackit/folder_manager.h"

#include <Akonadi/AgentInstance>
#include <KDebug>
#include <KIconLoader>
#include <KLocale>
#include <KProcess>
#include <stdlib.h>

namespace KNode {
namespace Utilities {

void Startup::loadLibrariesIconsAndTranslations() const
{
  KIconLoader::global()->addAppDir( "knode" );
  KIconLoader::global()->addAppDir( "libkdepim" );

  KGlobal::locale()->insertCatalog( "libkdepim" );
  KGlobal::locale()->insertCatalog( "libkpgp" );
  KGlobal::locale()->insertCatalog( "libmessagecomposer" );
  KGlobal::locale()->insertCatalog( "libmessageviewer" );

}

void Startup::updateDataAndConfiguration() const
{
  kDebug();

#ifdef __GNUC__
#warning ENABLE THE MIGRATION TOOL!
return;
#endif

  Q_ASSERT_X( false,
              "Startup::updateDataAndConfiguration",
              "Check this is working once the migration tools is finished" );

  KProcess migrator;
  migrator.setProgram( "knode-migrator" );
  migrator.start();
  if ( !migrator.waitForFinished( -1 ) ) {
    kError() << "Error while running the updater:\n"
             << "exit code =" << migrator.exitCode() << '\n'
             << "qt error code =" << migrator.error() << '\n'
             << "qt exist status =" << migrator.exitStatus() << '\n';
    exit( migrator.exitCode() );
  }
}

void Startup::initData()
{
  // Side effect: register special folder to SpecialMailCollections.
  Akobackit::manager()->folderManager()->foldersResource( true );
}



}
}
