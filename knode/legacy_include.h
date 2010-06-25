/*
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


/*
 * This file contains classes declarations to allow
 * classes that include headers moved into update/legacy
 * to keep building.
 */

#ifndef KNODE_LEGACYINCLUDE_H
#define KNODE_LEGACYINCLUDE_H

#include "configuration/settings_container_interface.h"

#include <boost/shared_ptr.hpp>
#include <KDebug>
#include <KMime/NewsArticle>


/***************************************
 *           knjobdata.h
 ***************************************/
class KNJobData : public QObject
{
};

class KNJobConsumer
{
};

class KNJobItem
{
};

/***************************************
 *           knarticle.h
 ***************************************/
class KNArticle : public KMime::NewsArticle, public KNJobItem
{
  public:
    typedef boost::shared_ptr<KNArticle> Ptr;
    typedef QList<KNArticle::Ptr> List;
};

class KNRemoteArticle : public KNArticle
{
  public:
    typedef boost::shared_ptr<KNRemoteArticle> Ptr;
    typedef QList<KNRemoteArticle::Ptr> List;
};

class KNLocalArticle : public KNArticle
{
  public:
    typedef boost::shared_ptr<KNLocalArticle> Ptr;
    typedef QList<KNLocalArticle::Ptr> List;
};

class KNAttachment
{
  public:
    typedef boost::shared_ptr<KNAttachment> Ptr;

};

/***************************************
 *           kncollection.h
 ***************************************/
class KNCollection
{
  public:
    typedef boost::shared_ptr<KNCollection> Ptr;
};

/***************************************
 *           knarticlecollection.h
 ***************************************/
class KNArticleCollection : public KNCollection
{
  public:
    typedef boost::shared_ptr<KNArticleCollection> Ptr;
    typedef QList<KNArticleCollection::Ptr> List;
};

/***************************************
 *           knserverinfo.h
 ***************************************/
class KNServerInfo
{
  public:
    typedef boost::shared_ptr<KNServerInfo> Ptr;
};

/***************************************
 *           knntpaccount.h
 ***************************************/
class KNNntpAccount : public KNCollection , public KNServerInfo, public KNode::SettingsContainerInterface
{
  public:
    typedef boost::shared_ptr<KNNntpAccount> Ptr;
    typedef QList<KNNntpAccount::Ptr> List;
};

/***************************************
 *           kngroup.h
 ***************************************/
class KNGroup : public KNArticleCollection, public KNJobItem, public KNode::SettingsContainerInterface
{
  public:
    typedef boost::shared_ptr<KNGroup> Ptr;
};

/***************************************
 *           knfolder.h
 ***************************************/
class KNFolder : public KNArticleCollection
{
  public:
    typedef boost::shared_ptr<KNFolder> Ptr;
    typedef QList<KNFolder::Ptr> List;
};

/***************************************
 *           kngroupmanager.h
 ***************************************/
class KNGroupInfo
{
};
class KNGroupListData : public KNJobItem
{
  public:
    typedef boost::shared_ptr<KNGroupListData> Ptr;
};

#endif
