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

#ifndef KNFOLDERMANAGER_H
#define KNFOLDERMANAGER_H

#include "knfolder.h"

// #include <QList>
#include <QObject>

// class KNCleanUp;
class KNMemoryManager;


/** Folder manager. */
class KNFolderManager : public QObject
{
  Q_OBJECT

  public:
    KNFolderManager();
    ~KNFolderManager();

    //folder access
    KNFolder::Ptr folder( int id );
    KNFolder::List folders() const { return mFolderList; }

    /// Returns the root folder.
    KNFolder::Ptr root() const { return mFolderList[0]; }
    /// Returns the drafts folder.
    KNFolder::Ptr drafts() const { return mFolderList[1]; }
    /// Returns the outbox folder.
    KNFolder::Ptr outbox() const { return mFolderList[2]; }
    /// Returns the sent folder.
    KNFolder::Ptr sent() const { return mFolderList[3]; }

//     //header loading
    bool loadHeaders( KNFolder::Ptr f );
//     bool unloadHeaders( KNFolder::Ptr f, bool force = true );

//     // returns the new folder
//     KNFolder::Ptr newFolder( KNFolder::Ptr p );
//     bool deleteFolder( KNFolder::Ptr f );
//     void emptyFolder( KNFolder::Ptr f );
//
//     /**
//       Returns true if the folder @p f can be moved under
//       a new parent @p p.
//     */
//     bool canMoveFolder( KNFolder::Ptr f, KNFolder::Ptr p );
//     /**
//       Move the folder @p f to a new parent @p p.
//       @returns false if the move is not possible.
//     */
//     bool moveFolder( KNFolder::Ptr f, KNFolder::Ptr p );
//
//     //unsent articles
//     int unsentForAccount(int accId);
//
//     //compacting
//     void compactFolder( KNFolder::Ptr f );
//     void compactAll(KNCleanUp *cup);
//     void compactAll();
//
//     // import + export
//     void importFromMBox( KNFolder::Ptr f );
//     void exportToMBox( KNFolder::Ptr f );
//
//     //synchronization
//     void syncFolders();

  protected:
    int loadCustomFolders();

    KNFolder::List mFolderList;
    int l_astId;

};

#endif
