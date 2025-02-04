#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kglobal.h>
/*
    This file is part of kdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <klocale.h>

#include "groupwarewizard.h"

int main( int argc, char **argv )
{
  KLocale::setMainCatalog( "kdepimwizards" );

  KAboutData aboutData( "groupwarewizard", 0,
                        ki18n( "KDE-PIM Groupware Configuration Wizard" ), "0.1" );
  KCmdLineArgs::init( argc, argv, &aboutData );

  KCmdLineOptions options;
  options.add("serverType <type>", ki18n("The server type"));
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication app;

  KGlobal::locale()->insertCatalog( "libkdepim" );

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  QString serverType;
  if ( args->isSet( "serverType" ) )
    serverType = args->getOption( "serverType" );
  args->clear();

  GroupwareWizard wizard( 0 );
  wizard.show();

  return app.exec();
}
