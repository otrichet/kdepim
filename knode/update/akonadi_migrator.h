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


#ifndef KNODE_UPDATE_AKONADIMIGRATOR_H
#define KNODE_UPDATE_AKONADIMIGRATOR_H

#include "updater_base.h"

namespace KNode {
namespace Update {

/**
 * @brief Migrates the folders, accounts and groups to Akonadi.
 * @since 4.6
 */
class AkonadiMigrator : public UpdaterBase
{
  public:
    AkonadiMigrator( QObject *parent = 0 );
    virtual ~AkonadiMigrator();

    /**
     * @reimp
     */
    virtual void update();

    /**
     * Returns 2.
     */
    virtual int id() const
    {
      return 2;
    };

    /**
     * @reimp
     */
    virtual QString name() const;
    /**
     * @reimp
     */
    virtual Status error() const;

  private:
    /**
     * Migrates folders from the pre-4.6 mbox storage into maildirs
     * managed by Akonadi.
     * @param config Configuration group [AkonadiMigrator][Folders] in the
     * migration config file.
     */
    void migrateFolders( KConfigGroup &config );

    /**
     * Add "done=XXX" in the configuration group @p config.
     */
    void markDone( KConfigGroup &config, bool done );

    Status mError;

};

}
}

#endif
