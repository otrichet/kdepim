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

#include "knscoring.h"

#include "knglobals.h"
#include "kscoringeditor.h"
#include "utilities.h"

#include <kwindowsystem.h>

//----------------------------------------------------------------------------
NotifyCollection* KNScorableArticle::notifyC = 0;

KNScorableArticle::KNScorableArticle( KNRemoteArticle::Ptr a )
  : ScorableArticle(), _a(a)
{
#if 0
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


KNScorableArticle::~KNScorableArticle()
{
#if 0
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNScorableArticle::addScore(short s)
{
#if 0
  _a->setScore(_a->score()+s);
  _a->setChanged(true);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}

void KNScorableArticle::changeColor(const QColor& c)
{
#if 0
  _a->setColor(c);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}

void KNScorableArticle::displayMessage(const QString& s)
{
#if 0
  if (!_a->isNew()) return;
  if (!notifyC) notifyC = new NotifyCollection();
  notifyC->addNote(*this,s);
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}

QString KNScorableArticle::from() const
{
#if 0
  return _a->from()->asUnicodeString();
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


QString KNScorableArticle::subject() const
{
#if 0
  return _a->subject()->asUnicodeString();
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


QString KNScorableArticle::getHeaderByType(const QString& s) const
{
#if 0
  KMime::Headers::Base *h = _a->headerByType(s.toLatin1());
  if (!h) return "";
  QString t = _a->headerByType(s.toLatin1())->asUnicodeString();
  Q_ASSERT( !t.isEmpty() );
  return t;
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNScorableArticle::markAsRead()
{
#if 0
  _a->setRead();
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}

//----------------------------------------------------------------------------

KNScorableGroup::KNScorableGroup()
{
#if 0
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


KNScorableGroup::~KNScorableGroup()
{
#if 0
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}

//----------------------------------------------------------------------------

KNScoringManager::KNScoringManager() : KScoringManager("knode")
{
#if 0
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


KNScoringManager::~KNScoringManager()
{
#if 0
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


QStringList KNScoringManager::getGroups() const
{
#if 0
  QStringList res;
  KNNntpAccount::List list = knGlobals.accountManager()->accounts();
  for ( KNNntpAccount::List::Iterator it = list.begin(); it != list.end(); ++it ) {
    QStringList groups;
    knGlobals.groupManager()->getSubscribed( (*it), groups);
    res += groups;
  }
  res.sort();
  return res;
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


QStringList KNScoringManager::getDefaultHeaders() const
{
#if 0
  QStringList l = KScoringManager::getDefaultHeaders();
  l << "Lines";
  l << "References";
  return l;
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}


void KNScoringManager::configure()
{
#if 0
  KScoringEditor *dlg = KScoringEditor::createEditor(this, knGlobals.topWidget);

  if (dlg) {
    dlg->show();
#ifdef Q_OS_UNIX
    KWindowSystem::activateWindow(dlg->winId());
#endif
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in" << Q_FUNC_INFO;
#endif
}

#include "knscoring.moc"
