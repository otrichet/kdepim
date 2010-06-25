/*
    KNode, the KDE newsreader
    Copyright (c) 1999-2005 the KNode authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#include "knglobals.h"

#include "knarticlefactory.h"
#include "knconfigmanager.h"
#include "knarticlemanager.h"
#include "knfiltermanager.h"
#include "knscoring.h"
#include "knmainwidget.h"
#include "settings.h"

#include <kconfig.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <KPIMIdentities/IdentityManager>


class KNGlobalsPrivate
{
  public:
    KNGlobals instance;
};

K_GLOBAL_STATIC( KNGlobalsPrivate, kNGlobalsPrivate )


KNGlobals::KNGlobals() :
  mCfgManager( 0 ),
  mArtManager( 0 ),
  mFilManager( 0 ),
  mScoreManager( 0 ),
  mSettings( 0 ),
  mArticleFactory( 0 ),
  mIdentityManager( 0 )
{
  kDebug();
}

KNGlobals::~KNGlobals( )
{
  kDebug();
  mIdentityManager->deleteLater();
  delete mScoreManager;
  delete mSettings;
}


KNGlobals * KNGlobals::self()
{
  Q_ASSERT ( !kNGlobalsPrivate.isDestroyed() );

  return &kNGlobalsPrivate->instance;
}


const KComponentData &KNGlobals::componentData() const
{
  if ( mInstance.isValid() )
    return mInstance;
  return KGlobal::mainComponent();
}


KConfig* KNGlobals::config()
{
  if (!c_onfig) {
      c_onfig = KSharedConfig::openConfig( "knoderc" );
  }
  return c_onfig.data();
}

KNConfigManager* KNGlobals::configManager()
{
  if (!mCfgManager)
    mCfgManager = new KNConfigManager();
  return mCfgManager;
}

KNArticleManager* KNGlobals::articleManager()
{
  if(!mArtManager)
    mArtManager = new KNArticleManager();
  return mArtManager;
}

KNArticleFactory* KNGlobals::articleFactory()
{
  if ( !mArticleFactory ) {
    mArticleFactory = new KNArticleFactory();
  }
  return mArticleFactory;
}

KNFilterManager* KNGlobals::filterManager()
{
  if (!mFilManager)
    mFilManager = new KNFilterManager();
  return mFilManager;
}

KNScoringManager* KNGlobals::scoringManager()
{
  if (!mScoreManager)
    mScoreManager = new KNScoringManager();
  return mScoreManager;
}

KPIMIdentities::IdentityManager* KNGlobals::identityManager()
{
  if ( !mIdentityManager ) {
    mIdentityManager = new KPIMIdentities::IdentityManager( false, 0, "mIdentityManager" );
  }
  return mIdentityManager;
}


void KNGlobals::setStatusMsg(const QString &text, int id)
{
  if(top)
    top->setStatusMsg(text, id);
}

KNode::Settings * KNGlobals::settings( )
{
  if ( !mSettings ) {
    mSettings = new KNode::Settings();
    mSettings->readConfig();
  }
  return mSettings;
}
