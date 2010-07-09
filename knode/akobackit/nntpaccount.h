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

#ifndef KNODE_NNTPACCOUNT_H
#define KNODE_NNTPACCOUNT_H

#include "configuration/settings_container_interface.h"

#include <Akonadi/AgentInstance>
#include <boost/shared_ptr.hpp>
#include <QHash>


namespace KNode {

namespace Akobackit {
  class NntpAccountManager;
}

/**
 * Wrapper around an Akonadi::AgentInstance representing a NNTP account.
 * It provides access over DBus to the configuration of the remote Agent.
 */
class NntpAccount : public SettingsContainerInterface
{
  friend class Akobackit::NntpAccountManager;

  public:
    /**
     * Encryption value for NNTP connection.
     * (Enum values must be kept with the enum in $build-dir/runtime/resources/nntp/settingsbase.h).
     */
    enum Encryption {
      None,
      SSL,
      TLS
    };

    /**
     * Shared pointer to a NntpAccount. To be used instead of raw NntpAccount*.
     */
    typedef boost::shared_ptr<NntpAccount> Ptr;
    /**
     * List of accounts.
     */
    typedef QList<NntpAccount::Ptr> List;

    /**
     * Create a new wrapper around @p agent.
     */
    NntpAccount( const Akonadi::AgentInstance &agent = Akonadi::AgentInstance() );
    /**
     * Destroys this wrapper.
     */
    virtual ~NntpAccount();


    /**
     * Is the underlying agent valid (i.e. exist in the Akonadi storage).
     */
    bool isValid()
    {
      return mAgent.isValid();
    }


    /**
     * The wrapped AgentInstance.
     */
    Akonadi::AgentInstance agent();


    /** Returns the name of this account. */
    QString name();
    /** Changes the name of this account. */
    void setName( const QString &name );

    /** Returns the server name of this account. */
    QString server();
    /** Changes the server name of this account. */
    void setServer( const QString &server );
    /** Returns the port of this account. */
    uint port();
    /** Changes the port. */
    void setPort( uint port );
    /** Returns the encryption mode of the connection. */
    Encryption encryption();
    /** Changes the encryption mode. */
    void setEncryption( Encryption encryption );

    /** Indicates if groups descriptions should be fetched. */
    bool fetchDescriptions();
    /** When @p fetch is true, description of groups are fetched. */
    void setFetchDescriptions( bool fetch );

    /** Indicates if this account requires authentication. */
    bool requiresAuthentication();
    /** Changes the authentication requirement state. */
    void setRequiresAuthentication( bool required );
    /** Returns the username used to login into this account. */
    QString user();
    /** Changes the user loging of this account. */
    void setUser( const QString &user );
    /** Returns the password used to login. */
    QString password();
    /** Changes the password used to login. */
    void setPassword( const QString &password );



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
    Akonadi::AgentInstance mAgent;
    QHash<QString, QVariant> mProperties;

    /**
     * Load the configuration property @p key of the resource mAgent over DBus.
     * The template class is the type used in the remote Dbus method call.
     */
    template<class T>
    QVariant loadProperty( const QString &key );

    /**
     * Save modification made to this account into Akonadi.
     * Called by NntpAccountManager.
     */
    void save();


    /**
      Unique object identifier of the identity of this server.
      -1 means there is no specific identity for this group
      (because KPIMIdentities::Identity::uoid() returns an unsigned int.
    */
    int mIdentityUoid;
};

}

#endif
