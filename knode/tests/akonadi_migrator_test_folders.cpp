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

#include "update/akonadi_migrator.h"

#include <Akonadi/AgentManager>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/Entity>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/KMime/SpecialMailCollections>
#include <akonadi/qtest_akonadi.h>
#include <KConfig>
#include <KConfigGroup>
#include <KMime/NewsArticle>
#include <KStandardDirs>


using namespace KNode::Update;

static const int ARTICLE_COUNT = 10;

class AkonadiMigratorTestFolderHierarchy : public QObject
{
  Q_OBJECT

  private:
    /**
     * Create a local folder (i.e. the xxxx.info file that describe it).
     * @param id Id of the folder (0 for root, 1 for drafts, 2 for outbox, 3 for sent-mail, above for standard folder).
     * @param parentId Id of the parent folder (used only for standard folders).
     * @param name Name of the folder (used only for standard folders)
     */
    void createFolder( int id, int parentId, const QString &name );

    /**
     * Returns the Akonadi collection id of a 'legacy' folder's @p id.
     * Read it from the migration file.
     */
    int collectionIdOfFolder( int id );

    /**
     * Returns an article content.
     */
    QByteArray loadArticleContent( int id );


    /**
     * Map legacy folder's id to the associated Akonadi Collection.
     */
    QMap<int, Akonadi::Collection> id2Collection;
    /**
     * Absolute path of the ${kdehome}/share/apps/knode/ directory.
     */
    QString mKnodeFoldersDir;


  private slots:
    /**
     * @note Run by the qtest framework before each test method.
     */
    void initTestCase();


    /**
     * Test migration with no 'legacy' folder.
     */
    void testNoFolder();

    /**
     * Create folders as done before the Akonadi migration.
     */
    void createLegacyFolders();
    /**
     * Create an Mbox file along with its index as done by KNode.
     */
    void initMbox();

    /**
     * Run the migration tool.
     */
    void runMigrator();

    /**
     * Load the map id2Collection.
     */
    void loadId2Collection();

    /**
     * Tests the folder hierarchy after the migration
     */
    void testFolders();
    /**
     * Test the migration the content and attribute of an article.
     */
    void testArticles();



// TODO:
//     /**
//      * Test migration.
//      *
//      * Input data:
//      * @li not empty folder;
//      * @li message with all possible flags;
//      * @li folder mbox containing deleted articles.Data
//      *
//      * Check output data:
//      * @li hierarchy is correct;
//      * @li special folders (check that each one contains a specific message);
//      * @li check all message are moved correctly;
//      * @li check flags
//      */
//     void test02();
//
//     /**
//      * Test resume of previously failing migration.
//      * @
//      */
//     void test03();
    // TODO 1) add a new legacy folder
    // TODO 2) re-run the migration tool
    // TODO 3) check that the new folder is not migrated
    // TODO 4) modified the file knode-migrationrc to remove "done=false".
    // TODO 5) re-run the migration tools
    // TODO 6) check that the new folder is migrated
};



//-----------------------------------------------------------
//---- Helpers ----------------------------------------------
//-----------------------------------------------------------


void AkonadiMigratorTestFolderHierarchy::createFolder( int id, int parentId, const QString &name )
{
  Q_ASSERT( id >= 0 );

  QString info = mKnodeFoldersDir;
  switch ( id ) {
    case 0:
      info += "root_";
      break;
    case 1:
      info += "drafts_";
      break;
    case 2:
      info += "outbox_";
      break;
    case 3:
      info += "sent_";
      break;
    default:
      info += "custom_";
      break;
  }
  info += QString::number( id ) + ".info";

  KConfig folderConf( info, KConfig::SimpleConfig );
  KConfigGroup group = folderConf.group( "" );
  if ( id > 3 ) { // standard folders
    group.writeEntry( "id", id );
    group.writeEntry( "parentId", parentId );
    group.writeEntry( "name", name );
  }
  group.writeEntry( "wasOpen", true ); // not used but is present in the migrated data.
  //qDebug() << "Adding" << id << "under" << parentId << "with .info file" << info;
  group.sync();
}


int AkonadiMigratorTestFolderHierarchy::collectionIdOfFolder( int id )
{
  KSharedConfig::Ptr config = KSharedConfig::openConfig( "knode-migrationrc" );
  KConfigGroup group = config->group( "AkonadiMigrator" ).group( "Folders" );
  const QString configKey = QString( "collection for folder %1" ).arg( id );
  return group.readEntry( configKey, -1 );
}


QByteArray AkonadiMigratorTestFolderHierarchy::loadArticleContent( int id )
{
  QByteArray content =
      "From: Test ament <test-##identifier##@example.net>\n"
      "User-Agent: Edonk 6.4\n"
      "MIME-Version: 1.0\n"
      "Newsgroups: alt.test.##identifier##\n"
      "Subject: =?GB2312?B?1eLAz8qmvczRp9KyzKvDu7XAtcLBy7DJo78=?=\n"
      "Content-Type: text/plain; charset=\"UTF-8\"\n"
      "Content-Transfer-Encoding: 8bit\n"
      "X-TEST-ARTICLE-ID: ##identifier##\n"
      "NNTP-Posting-Host: 1.2.3.4\n"
      "Message-Id: <fds52e5z4ff7zae37sd3-@jskd.sdf75.sdf.example.net>\n"
      "X-Trace: migration.knode.test.example.net 12649875222 1.2.3.4 (26 Mar 2010 11:30:22 +0800)\n"
      "Organization: None\n"
      "Lines: 20\n"
      "Date: Wed, 30 Jun 2010 08:15:59 +0800\n"
      "Path: migration.knode.test.example.net!not-for-mail\n"
      "\n"
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed id ante velit.\n"
      "Vestibulum orci eros, suscipit id porta sagittis,\n"
      "aliquam in libero. Ut interdum commodo eros, nec ornare purus ornare eget. Cras erat dolor, gravida interdum sagittis facilisis, pharetra nec tortor.\n"
      "Proin erat enim, malesuada eu imperdiet sed, ultricies eu nisi. Maecenas ##identifier## eget libero ultricies erat rhoncus\n"
      "laoreet. Suspendisse varius, purus vel placerat aliquam,\n"
      "augue metus accumsan ipsum, cursus dignissim eros neque sit\n"
      "\n"
      "amet mi. Suspendisse eu orci id ligula luctus volutpat vel et ##identifier##\n"
      "turpis. Class aptent taciti sociosqu ad litora torquent per conu\n"
      "\n"
      "bia nostra, per inceptos himenaeos. Aliquam fringilla viverra facilisis. Suspendisse\n"
      "semper tincidunt leo, id dictum mi gravida non. Sed ut nibh sem, quis ultrices\n"
      "\n";
  content.replace( "##identifier##", QByteArray::number( id ) );
  return content;
}


// void AkonadiMigratorTestFolderHierarchy::addArticleTestColumns()
// {
//   QTest::addColumn<QByteArray>( "articleContentId" ); // Argument of loadArticleData()
//   QTest::addColumn<bool>( "presentInIndex" ); // was not deleted?
//   // flags
//   QTest::addColumn<bool>( "doPost" );
//   QTest::addColumn<bool>( "posted" );
//   QTest::addColumn<bool>( "doMail" );
//   QTest::addColumn<bool>( "mailed" );
//   QTest::addColumn<bool>( "editDisabled" );
//   QTest::addColumn<bool>( "canceld" );
// }




//-----------------------------------------------------------
//---- Pre/post tests ---------------------------------------
//-----------------------------------------------------------

void AkonadiMigratorTestFolderHierarchy::initTestCase()
{
  mKnodeFoldersDir = KStandardDirs::locateLocal( "data", "knode/folders/" );
  QVERIFY( !mKnodeFoldersDir.isEmpty() );
  QDir dir( mKnodeFoldersDir );

  const QStringList entries = dir.entryList( QDir::NoDotAndDotDot );
  foreach ( const QString &entry, entries ) {
    qDebug() << entry;
  }
  QVERIFY2( entries.isEmpty(),
            QString( "The directory %1 contained files or directory before the test begin!" ).arg( mKnodeFoldersDir ).toUtf8() );
}

// void AkonadiMigratorTestFolderHierarchy::cleanup()
// {
//   bool b = rmdir( QDir( mKnodeFoldersDir ) );
//   QVERIFY2( b, "Deletion of KNode's folder dir failed." );
//   mKnodeFoldersDir.clear();
//
//   Akonadi::AgentManager *manager = Akonadi::AgentManager::self();
//   foreach ( const Akonadi::AgentInstance &agent, manager->instances() ) {
//     qDebug() << "Removing Akonadi agent" << agent.identifier() << agent.name();
//     manager->removeInstance( agent );
//     QTest::kWaitForSignal( manager, SIGNAL( instanceRemoved( const Akonadi::AgentInstance & ) ), 2000/*ms*/ );
//   }
//   QVERIFY( manager->instances().isEmpty() );
// }




//-----------------------------------------------------------
//---- Tests ------------------------------------------------
//-----------------------------------------------------------


void AkonadiMigratorTestFolderHierarchy::testNoFolder()
{
  AkonadiMigrator *am = new AkonadiMigrator();
  am->update();
  QVERIFY2( am->error() == UpdaterBase::NoError, "Error code:"+QByteArray::number( am->error() ) );
  delete am;

  KSharedConfig::Ptr config = KSharedConfig::openConfig( "knode-migrationrc" );
  KConfigGroup group = config->group( "AkonadiMigrator" ).group( "Folders" );
  QVERIFY2( group.readEntry( "done", false ), "The configuration knode-migrationrc contains done=true for folders" );

  // Allow the following test to run
  // This also test implicitly the recovery of failed migration.
  group.writeEntry( "done", false );
  group.sync();
}


void AkonadiMigratorTestFolderHierarchy::createLegacyFolders()
{
  // Many levels deep
  createFolder( 11, 0, "test11" );
  createFolder( 12, 11, "test12" );
  createFolder( 13, 12, "test13" );
  createFolder( 14, 13, "test14" );
  createFolder( 15, 14, "test15" );
  // Special folders
  createFolder( 0, 0, "" );
  createFolder( 1, 0, "" );
  createFolder( 2, 0, "" );
  createFolder( 3, 0, "" );
  // Not sequential id
  createFolder( 21, 0, "test21" );
  createFolder( 23, 21, "test23" );
  createFolder( 25, 21, "test25" );
  // Sibling with same name
  createFolder( 31, 0, "test31" );
  createFolder( 32, 31, "test3x" );
  createFolder( 33, 31, "test3x" );
  createFolder( 34, 31, "test3x " );
  createFolder( 35, 31, "TEST3x" );
  // Child id lower than parent id
  createFolder( 43, 0, "test43" );
  createFolder( 41, 43, "test41" );
  // Missing parent
  createFolder( 51, 52, "test51" );
  // Folders name
  createFolder( 61, 0, QString::fromUtf8( "日本語" ) );
  createFolder( 62, 0, "a name for testing" );
}

void AkonadiMigratorTestFolderHierarchy::initMbox()
{
  createFolder( 71, 0, "test71" );
  createFolder( 72, 71, "test index migration" );

  QFile mbox( mKnodeFoldersDir + "custom_72.mbox" );
  QVERIFY( mbox.open( QIODevice::WriteOnly ) );
  QFile index( mKnodeFoldersDir + "custom_72.idx" );
  QVERIFY( index.open( QIODevice::WriteOnly ) );

  QList< QPair<qint64, qint64> > startEndOffsets;
  // Format of an article in the mbox
  // 1st line: From aaa@aaa Mon Jan 01 00:00:00 1997
  // 2nd line: X-KNode-Overview: <<subject>>\t<<Newsgroup: header>>\t<<To: header>>\t<<line count>>
  // next lines: Article content
  for ( int id = 0 ; id < ARTICLE_COUNT ; ++id ) {
    mbox.write( "From aaa@aaa Mon Jan 01 00:00:00 1997\n" );
    qint64 start = mbox.pos();
    mbox.write( "X-KNode-Overview: test\tgroup.test\trecipient@test\t0\n" );
    mbox.write( loadArticleContent( id ) );
    mbox.write( "\n" );
    qint64 end = mbox.pos();
    startEndOffsets.insert( id, qMakePair( start, end ) );
  }

  // Format of the index: sequence of DynData serialization
  struct DynData { // struct copied from knfolder.h
    int articleId; //
    int startOffset; // start offset in the mbox
    int endOffset; // end offset in the mbox
    int serverId; // id of the server that should be used to send the message
    time_t articleDate; // Date: header of the message (used for display)
    bool flags[ 6 ]; // { doMail, mailed, doPost, posted, canceled, editDisabled }
  };
  for ( int id = 0 ; id < ARTICLE_COUNT ; ++id ) {
    // TODO: test the serverId migration.
    const DynData indexData = { id, startEndOffsets[ id ].first, startEndOffsets[ id ].second,
                                -1, 1278006139, { true, true, true, false, false, false } };
    qint64 byteNumber = index.write( reinterpret_cast<const char*>( &indexData ), sizeof( indexData ) );
    QVERIFY( byteNumber != -1 );
  }


  QVERIFY( index.flush() );
  index.close();
  QVERIFY( mbox.flush() );
  mbox.close();
}


void AkonadiMigratorTestFolderHierarchy::runMigrator()
{
  AkonadiMigrator *am = new AkonadiMigrator();
  am->update();
  QVERIFY2( am->error() == UpdaterBase::NoError, "Error code:"+QByteArray::number( am->error() ) );
  delete am;

  KSharedConfig::Ptr config = KSharedConfig::openConfig( "knode-migrationrc" );
  KConfigGroup group = config->group( "AkonadiMigrator" ).group( "Folders" );
  QVERIFY2( group.readEntry( "done", false ), "The configuration knode-migrationrc contains done=true for folders" );
}

void AkonadiMigratorTestFolderHierarchy::loadId2Collection()
{
  Akonadi::Collection::List collections;
  QList<int> ids;
  ids << 11 << 12 << 13 << 14 << 15
      << 1  << 2  << 3
      << 21 << 23 << 25
      << 31 << 32 << 33 << 34 << 35
      << 41 << 43
      << 51
      << 61 << 62
      << 71 << 72;
  foreach ( int id, ids ) {
    const Akonadi::Collection col = Akonadi::Collection( collectionIdOfFolder( id ) );
    QVERIFY2( col.isValid(), "Folders id:"+QByteArray::number( id ) );
    collections << col;
  }

  Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob( collections, this );
  QVERIFY2( job->exec(), job->errorString().toUtf8() );
  QCOMPARE( job->collections().size(), collections.size() );
  collections = job->collections();
  foreach ( const Akonadi::Collection &col, collections ) {
    QVERIFY2( col.isValid(), "Checking collection fetched from the Akonadi store" );
    QVERIFY2( col.parentCollection().isValid(), "Child collection name "+col.name().toUtf8() );
  }

  foreach ( int id, ids ) {
    Akonadi::Entity::Id collectionId = collectionIdOfFolder( id );
    foreach ( const Akonadi::Collection &col, collections ) {
      if ( col.id() == collectionId ) {
        id2Collection.insert( id, col );
        break;
      }
    }
  }
  QCOMPARE( id2Collection.size(), collections.size() );

  // Adding the root
  job = new Akonadi::CollectionFetchJob( id2Collection[ 1 ].parentCollection(),
                                         Akonadi::CollectionFetchJob::Base, this );
  QVERIFY2( job->exec(), job->errorString().toUtf8() );
  id2Collection.insert( 0, job->collections().at( 0 ) );
}


void AkonadiMigratorTestFolderHierarchy::testFolders()
{
#define TEST_AKONADI_FOLDER( childId, parentId ) \
  QCOMPARE( id2Collection[ childId ].parentCollection(), id2Collection[ parentId ] ); \
  QCOMPARE( id2Collection[ childId ].resource(), maildirResourceId );


  // Reading knoderc from disk to ensure this information is not lost
  const KConfig config( "knoderc", KConfig::SimpleConfig );
  const KConfigGroup group = config.group( "Akonadi" ).group( "Folders" );
  const QString maildirResourceId = group.readEntry( "resource", QString() );
  QVERIFY2( !maildirResourceId.isEmpty(), "Identifier of the maildir resource is not present in knoderc" );
  const Akonadi::AgentInstance maildir = Akonadi::AgentManager::self()->instance( maildirResourceId );
  QVERIFY2( maildir.isValid(), "The maildir resource for Knode's folders is invalid" );


  // -- hierarchy of 5 levels is created correctly?
  TEST_AKONADI_FOLDER( 11, 0 );
  TEST_AKONADI_FOLDER( 12, 11 );
  TEST_AKONADI_FOLDER( 13, 12 );
  TEST_AKONADI_FOLDER( 14, 13 );
  TEST_AKONADI_FOLDER( 15, 14 );


  // -- Special folders are created correctly?
  QCOMPARE( id2Collection[ 0 ].resource(), maildirResourceId );
  TEST_AKONADI_FOLDER( 1, 0 );
  TEST_AKONADI_FOLDER( 2, 0 );
  TEST_AKONADI_FOLDER( 3, 0 );
  Akonadi::SpecialMailCollections *smc = Akonadi::SpecialMailCollections::self();
  QCOMPARE( smc->collection( Akonadi::SpecialMailCollections::Root, maildir ), id2Collection[ 0 ] );
  QCOMPARE( smc->collection( Akonadi::SpecialMailCollections::Drafts, maildir ), id2Collection[ 1 ] );
  QCOMPARE( smc->collection( Akonadi::SpecialMailCollections::Outbox, maildir ), id2Collection[ 2 ] );
  QCOMPARE( smc->collection( Akonadi::SpecialMailCollections::SentMail, maildir ), id2Collection[ 3 ] );


  // -- Folders with non sequential id
  TEST_AKONADI_FOLDER( 23, 21 );
  TEST_AKONADI_FOLDER( 25, 21 );


  // -- migration of sibling folders:
  // TODO: check the name
  // * with the same name
  TEST_AKONADI_FOLDER( 32, 31 );
  TEST_AKONADI_FOLDER( 33, 31 );
  // * whose names only differ by the ending space
  TEST_AKONADI_FOLDER( 34, 31 );
  // * whose names only differ by the case of the name
  TEST_AKONADI_FOLDER( 35, 31 );


  // -- Id of a child can be lower than the one of its father
  TEST_AKONADI_FOLDER( 41, 43 );


  // -- Migration of a folder with a missing parent
  // Collection 51 should be reparented to the root collection.
  TEST_AKONADI_FOLDER( 51, 0 );


  // -- Name of folders
  // * Non-latin1 name
  QCOMPARE( id2Collection[ 61 ].name(), QString::fromUtf8( "日本語" ) );
  // * Ascii name
  QCOMPARE( id2Collection[ 62 ].name(), QString::fromUtf8( "a name for testing" ) );


#undef TEST_AKONADI_FOLDER

  QFAIL( "Note: reenable MySql backend in data/unittestenv/xdgconfig/akonadi/akonadiserverrc." );
}

void AkonadiMigratorTestFolderHierarchy::testArticles()
{
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( id2Collection[ 72 ], this );
  job->fetchScope().fetchAllAttributes( true );
  job->fetchScope().fetchFullPayload( true );
  QVERIFY2( job->exec(), job->errorString().toUtf8() );
  QCOMPARE( job->items().size(), ARTICLE_COUNT );
  const Akonadi::Item::List items = job->items();

  const QByteArray xTestHeader = "\nX-TEST-ARTICLE-ID: ";
  foreach ( const Akonadi::Item &item, items ) {
    const QByteArray content = item.payloadData();

    QVERIFY( content.contains( xTestHeader ) );
    int idStart = content.indexOf( xTestHeader ) + xTestHeader.size();
    int idEnd = content.indexOf( '\n', idStart );
    bool ok = false;
    int id = content.mid( idStart, idEnd - idStart ).toUInt( &ok );
    QVERIFY2( ok, "Extracting the identifier of the article" );
    QCOMPARE( content, loadArticleContent( id ) );
    QCOMPARE( item.mimeType(), QString::fromUtf8( "message/news" ) );
  }


  QFAIL( "Test migration of flags of articles" );
}



QTEST_AKONADIMAIN( AkonadiMigratorTestFolderHierarchy, NoGUI )

#include "akonadi_migrator_test_folders.moc"
