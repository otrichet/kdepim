/*
    knapplication.cpp

    KNode, the KDE newsreader
    Copyright (c) 1999-2010 the KNode authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#include "knapplication.h"

#include <kwindowsystem.h>
#include <kdebug.h>
#include <kcmdlineargs.h>

#include "knode.h"
#include "knmainwidget.h"


int KNApplication::newInstance()
{
  kDebug() << "KNApplication::newInstance()";

  KNMainWindow *main = findPrimaryWindow();

  if (!main) {
    if ( isSessionRestored() ) {
      int n = 1;
      while (KNMainWindow::canBeRestored(n)){
        if (KNMainWindow::classNameOfToplevel(n)=="KNMainWindow") {
          main = new KNMainWindow;
          main->restore(n);
          break;
        }
        n++;
      }
    }

    if (!main) {
      main = new KNMainWindow;
      main->show();
    }
  }

  // Handle window activation and startup notification
  KUniqueApplication::newInstance();

  // process URLs...
  KNMainWidget *w = main->mainWidget();
  w->handleCommandLine();

  kDebug() << "KNApplication::newInstance() done";
  return 0;
}

KNMainWindow* KNApplication::findPrimaryWindow()
{
    Q_FOREACH(QWidget* w, KNApplication::topLevelWidgets()) {
        KNMainWindow *window = dynamic_cast<KNMainWindow*>(w);
        if(window) {
            return window;
        }
    }
    return 0;
}


