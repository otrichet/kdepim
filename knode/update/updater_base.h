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


#ifndef KNODE_UPDATE_UPDATERBASE_H
#define KNODE_UPDATE_UPDATERBASE_H

#include <KSharedConfig>
#include <QObject>

class KConfig;

namespace KNode {
namespace Update {

/**
 * Base class for updater.
 *
 * The update scheduler call the update() method and wait for the
 * finished() signal to be emitted.
 */
class UpdaterBase : public QObject
{
  Q_OBJECT

  public:
    /**
     * Status of the job.
     */
    enum Status {
      NoError = 0, ///< Everything OK.
      Warning,     ///< "Unimportant" errors.
      Error,       ///< Error that should stop the whole migration process.
    };

    /**
     * Level of message displayed to the user.
     */
    enum LogLevel {
      LogInfo,
      LogWarn,
      LogError
    };

  protected:
    UpdaterBase( QObject *parent = 0 );

  public:
    virtual ~UpdaterBase();

  public:
    /**
     * Run the update process.
     */
    virtual void update() = 0;
    /**
     * Returns the unique updater id.
     */
    virtual int id() const = 0;
    /**
     * Internationalized name.
     */
    virtual QString name() const = 0;

    virtual Status error() const = 0;

  signals:
    void progress( int );
    void message( const QString &message, LogLevel level = LogInfo ) const;

  protected:
    /**
     * Returns a config pointing to "knoderc".
     */
    KConfig * knodeConfig();

    /**
     * Return the knode-migrationrc configuration pointer.
     */
    KSharedConfig::Ptr migratorConfig();

  private:
    KConfig *mKnodeConfig;
};

}

}

#endif
