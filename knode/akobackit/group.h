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


#ifndef KNODE_GROUP_H
#define KNODE_GROUP_H

#include "configuration/settings_container_interface.h"

#include <Akonadi/Collection>
#include <boost/shared_ptr.hpp>
#include <QList>

class KConfigGroup;

namespace KNode {

namespace Akobackit {
  class GroupManager;
}

/**
 * Wrapper around an Akonadi::Collection representing a news group.
 */
class Group : public SettingsContainerInterface
{
  friend class Akobackit::GroupManager;

  public:
    /**
     * Posting status on a news group.
     *
     * (Enum values must be kept in sync with the enum in PostingStatus
     * ${kdepim-dir}/runtime/resources/nntp/nntpcollectionattribute.h).
     */
    enum PostingStatus {
      Unknown = 0,    ///< Unknown (not retrieve or not available).
      ReadOnly,       ///< Posting is not allowed.
      PostingAllowed, ///< Posting is (should be) allowed without restriction.
      Moderated,      ///< Posting will go through a moderation process.
    };

    /**
     * Shared pointer to a Group. To be used instead of raw Group*.
     */
    typedef boost::shared_ptr<Group> Ptr;
    /**
     * List of groups.
     */
    typedef QList<Group::Ptr> List;



    /**
     * Create a new wrapper around @p collection.
     */
    Group( const Akonadi::Collection &collection );
    /**
     * Destroys this wrapper.
     */
    virtual ~Group();


    /**
     * Returns displayName() if it is not empty, otherwise returns
     * groupName().
     */
    QString name();


    /**
     * Returns the group's name (e.g. misc.test).
     */
    QString groupName();

    /**
     * Returns a user defined name for this group.
     */
    QString displayName();
    /**
     * Changes the name used for display in the UI.
     * @param name The new name to display. An empty string reset this name
     * to its default (the group name).
     */
    void setDisplayName( const QString &name );

    /**
     * Returns the description of this group.
     */
    QString description();
    /**
     * Returns the posting right in this group.
     */
    PostingStatus postingStatus();


    /**
     * Returns true if a specific charset is to be used for this group.
     */
    bool useCharset();
    /**
     * Sets the use of a default charset specific to this group.
     */
    void setUseCharset( bool useCharset );
    /**
     * Returns the default charset specified for this group.
     */
    QByteArray defaultCharset();
    /**
     * Sets the default charset for this group.
     */
    void setDefaultCharset( const QByteArray &charset );




    //-- Implementation of SettingsContainerInterface -- //

    /**
     * Returns this account's specific identity or
     * the null identity if there is none.
     */
    virtual const KPIMIdentities::Identity & identity() const;
    /**
     * Sets this account's specific identity
     * @param identity this server's identity of a null identity to unset.
     */
    virtual void setIdentity( const KPIMIdentities::Identity &identity );

    /** Reimplemented from SettingsContainerInterface. */
    virtual void writeConfig();

  private:
    /**
     * Save modification made to this group into the configuration.
     * Used by GroupManager.
     */
    void save();


    /**
     * Wrapped collection.
     */
    Akonadi::Collection mCollection;

    /**
     * Contains properties values before they are synchronized into the backend.
     */
    QHash<QString, QVariant> mProperties;


    /**
     * Read a property from the property cache mProperties or from the configuration
     * in the config group [Akonadi][Group][${AkonadiCollectionID}].
     */
    template<class T>
    QVariant loadProperty( const QString &key, const T &defaultValue ) const;
};


}

#endif // KNODE_GROUP_H
