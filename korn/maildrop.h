/*
* maildrop.h -- Declaration of class KMailDrop.
* Generated by newclass on Sat Nov 29 20:07:45 EST 1997.
*/
#ifndef SSK_MAILDROP_H
#define SSK_MAILDROP_H

#include<qobject.h>
#include<qstring.h>
#include<qcolor.h>
#include<qvector.h>

class AccountSettings;
class Protocol;

class KDropCfgDialog;
class KornMailSubject;

template< class T > class QList;
template< class T, class R > class QMap;
class QVariant;

/**
* Abstract base class for all mailbox monitors.
* @author Sirtaj Singh Kang (taj@kde.org)
* @version $Id$
*/
class KMailDrop : public QObject
{
  Q_OBJECT

  public:

    //TODO: still used?
    /**
     * This enum is used to determe the type of the email box
     */
    enum  Style { Plain, Colour, Icon };

  protected:
    int _lastCount;
    AccountSettings *_settings;

  public:
    /**
     * KMailDrop Constructor
     */
    KMailDrop();

    /**
     * KMailDrop Destructor
     */
    virtual ~KMailDrop();

    /** 
     * @return TRUE if the mailbox and its configuration are valid.
     */
    virtual bool valid() = 0;

    /** 
     * Number of messages in the mailbox at the last count.
     * @return The number of messages in the mailbox since last count.
     */
    int count() {return _lastCount;};

    QString realName() const;

    QString soundFile() const;

    QString newMailCmd() const;

    /** 
     * Recheck the number of letters in this mailbox. Raises the
     * changed(int) signal if new mail is found.
     *
     * Concrete subclasses MUST reimplement this method.
     */
    virtual void recheck()=0;

    /**
     * Force a recheck
     */
    virtual void forceRecheck() { recheck(); }

    /** 
     * This function starts the monitor
     *
     * @return true if succesfull; false otherwise
     */
    virtual bool startMonitor()=0;

    /**
     * This function stops the monitor.
     *
     * @return true if succesfull; false otherwise
     */
    virtual bool stopMonitor()=0;

    /** 
     * Check monitor run status.
     * @return true if monitor is running.
     */
    virtual bool running()=0;

    /** 
     * Add a configuration page to the configuration dialog.
     * Each reimplementation should first call the inherited implementation,
     * then call @ref KDropCfgDialog::addConfigPage with a custom
     * @ref KMonitorCfg object.
     */
//    virtual void addConfigPage( KDropCfgDialog * );

    /** 
     * Returns a newly created KBoxFactory object initialized to
     * be equivalent to this object (prototype pattern). 
     *
     * Deletion of the returned object becomes the responsibility of 
     * the caller.
     *
     * Subclasses should override this to return objects of their
     * own type.
     */
    virtual KMailDrop *clone() const = 0;

    /** 
     * Read box configuration from a config group. Subclasses that
     * reimplement this should call the overridden method.
     *
     * @param cfg  A configuration object with the group already set to
     *     the configuration for this box.
     * @return true if read was successful, false otherwise.
     */
    virtual bool readConfig( AccountSettings *settings );
    /**
     * This function have a mapping to the configuration, and a protocol for the type of protocol.
     *
     * @param map the configuration mapping containing the configuration entries of this account
     * @param protocol a pointer to a class which contains the information about the used type of protocol
     * @return true if succesfull; false otherwise
     */
    virtual bool readConfigGroup( const QMap< QString, QString > &, const Protocol * ) { return true; }

    /** 
     * Write box configuration to a config group. Subclasses that
     * reimplement this should call the overridden method.
     *
     * @param cfg  A configuration object with the group already set to
     *     the configuration for this box.
     * @return true if read was successful, false otherwise.
     */
    virtual bool writeConfigGroup( AccountSettings *settings ) const;

    /** 
     * Return the type of this monitor, for display and
     * configuration purposes. Each concrete subclass should return a 
     * unique identifier.
     */
    virtual QString type() const = 0;

    /**
     * Return if the maildrop is synchrone (true) or asynchrone (false).
     * This way, classes like KornSubjectDlg know if functions like
     * readSubject() return a result immediately.
     * @return true by a synchrone type; false by an asynchrone (like KKkioDrop) type.
     */
    virtual bool synchrone() const { return true; }
    
    /**
     * Return true if the concrete subclass can read the subjects of
     * all new mails. This will enable the "Read Subjects" menu item.
     */
    virtual bool canReadSubjects() {return false;}

    /** 
     * Read the subjects of all new mails.
     * NOTE: the default implementation stops the timer, calls
     * doReadSubjects, restarts the time if necessary and updates
     * the displayed mail count. Concrete implementations should not
     * touch readSubjects() but implement doReadSubjects() instead!
     * @param stop: stop flag. If it is set to true during the execution,
     * readSubjects shoulkd return as soon as possible. The return value
     * is invalid in this case. If stop is 0, readSubjects will not
     * terminate before all mail subjects are loaded.
     * @return all new mails subjects as a vector.
     */
    virtual QVector<KornMailSubject> * readSubjects(bool * stop);

    /**
     * Read the subjects of all new mails. The concrete subclass has
     * to implement it, if canReadSubjects() returns true.
     * @param stop: stop flag. If it is set to true during the execution,
     * readSubjects should return as soon as possible. The return value
     * is invalid in this case. If stop is 0, readSubjects will not
     * terminate before all mail subjects are loaded.
     * @return all new mails subjects as a vector.
     */
    virtual QVector<KornMailSubject> * doReadSubjects(bool * stop);

    /**
     * Return true if the concrete subclass can delete individual mails.
     * This will enable the "Delete" button in the mail subjects dialog.
     */
    virtual bool canDeleteMails() {return false;}

    /**
     * Delete some mails in the mailbox. The concrete subclass has
     * to implement it, if canDeleteMails() returns true.
     * @param ids list of mail ids to delete. The ids are taken from
     * the corresponding KornMailSubject instances returned by a previous
     * call to doReadSubjects().
     * @param stop: stop flag. If it is set to true during the execution,
     * deleteMails() should return as soon as possible. The return value
     * is invalid in this case. If stop is 0, deleteMails() will not
     * terminate before the mails are deleted.
     * @return true, if the mail ids of the remaining mails might have changed.
     * The corresponding KornMailSubject instances returned by a previous
     * call to doReadSubjects() have to be discarded and readSubjects() must
     * be called again to get the correct mail ids. If false is returned,
     * the KornMailSubject instances of the remaining mails might be used
     * further more.
     */
    virtual bool deleteMails(QList<QVariant> *ids, bool * stop);

    /**
     * Return true if the concrete subclass can load individual mails fully.
     * This will enable the "Full Message" button in the mail dialog.
     */
    virtual bool canReadMail() const {return false;}

    /**
     * Load a mail from the mailbox fulle . The concrete subclass has
     * to implement it, if deleteMails() returns true.
     * @param id id of the mail to load. The id is taken from the corresponding 
     * KornMailSubject instances returned by a previous call to doReadSubjects().
     * @param stop: stop flag. If it is set to true during the execution,
     * readMail() should return as soon as possible. The return value
     * is invalid in this case. If stop is 0, readMail() will not
     * terminate before the mail is loaded.
     * @return the fully loaded mail (header and body) or "" on error.
     */
    virtual QString readMail(const QVariant id, bool * stop);

    /**
     * Set function for reset counter.
     *
     * @param val the new value for this setting
     */
    void setResetCounter(int val);

    /** 
     * This is called by the manager when it wishes to delete
     * a monitor. Clients should connect to the notifyDisconnect()
     * signal and ensure that the monitor is not accessed after
     * the signal has been received.
     *
     * Reimplementations should call this implementation too.
     */
    virtual void notifyClients();

    public slots:

    /**
     * Forcibly set the count to zero;
     */
    virtual void forceCountZero();
    
    /**
     * The next slots are used by the kio maildrop; the present at this places
     * prevent warnings at runtime.
     */
    virtual void readSubjectsCanceled() {}
    /**
     * The next slots are used by the kio maildrop; the present at this places
     * prevent warnings at runtime.
     */
    virtual void readMailCanceled() {}
    /**
     * The next slots are used by the kio maildrop; the present at this places
     * prevent warnings at runtime.
     */
    virtual void deleteMailsCanceled() {}

protected slots:

    /**
     * This function sets the number of messages and the maildrop.
     * This function is called when changed is called.
     *
     * @param count the number of messages
     * @param drop the maildrop
     */
    void setCount( int count, KMailDrop* drop );
      
signals:

    /** 
     * This signal is emitted when the mailbox discovers 
     * new messages in the maildrop.
     *
     * @param count the number of messages found
     * @param drop the MailDrop which found the messages
     */
    void changed( int count, KMailDrop* drop );

    /**
     * This signal is emitted when the valid-status changes.
     * @param isValid true then and only then if the box is valid
     */
    void validChanged( bool isValid );

    /** 
     * This is emitted on configuration change, normally
     * on an updateConfig() but 
     */
    void configChanged();

    /** 
     * Clients should connect to this and discontinue use
     * after it is emitted.
     */
    void notifyDisconnect();

    /**
     * rechecked() is called if an asynchrone maildrop has
     * rechecked the availability of email.
     */
    void rechecked();
    
    /**
     * The next signal is emitted when a passive popup could be displayed.
     * As argument, there is a KornSubject, which contains a subject and
     * some more info that could be used with the popup.
     */
    void showPassivePopup( QList< KornMailSubject >*, int, bool, const QString& realname );

    /**
     * This signal is emitted when a passive error message should be displayed.
     *
     * @param error The error message
     * @param realname The real name of this object.
     */
    void showPassivePopup( const QString& error, const QString& realname );
    
    /**
     * readSubjects() might signal readSubject() if
     * an subject is received. This is only useful in
     * asynchrone situations.
     * @param subject the subject structure which is read
     */
    void readSubject( KornMailSubject * subject );
    
    /**
     * readSubjects() might signal readSubjectsTotalSteps() to
     * send the expected total number of steps to a possible
     * progress bar. See readSubjectsProgress();
     * @param totalSteps expected total number of steps.
     */
    void readSubjectsTotalSteps(int totalSteps);

    /**
     * readSubjects() might signal readSubjectsProgress() to
     * send the current progress in relation to the
     * expected total number of steps (see readSubjectsTotalSteps()).
     * @param progress current progress
     */
    void readSubjectsProgress(int progress);
    
    /**
     * readSubjects() might signal readSubjectsReady() to
     * remove the progress bar in asynchrone situations.
     * @param success true if succes, false if cancelled
     */
    void readSubjectsReady( bool success );

    /**
     * deleteMails() might signal deleteMailsTotalSteps() to
     * send the expected total number of steps to a possible
     * progress bar. See deleteMailsProgress();
     * @param totalSteps expected total number of steps.
     */
    void deleteMailsTotalSteps(int totalSteps);

    /**
     * deleteMails() might signal deleteMailsProgress() to
     * send the current progress in relation to the
     * expected total number of steps (see deleteMailsTotalSteps()).
     * @param progress current progress
     */
    void deleteMailsProgress(int progress);

    /**
     * deleteMails() might signal deleteMailsReady() if
     * it is not going to do something anyway.
     * This could be the case when an email has been succesfully
     * removed, or when the deletions failed. This is useful
     * in asynchrone situations.
     * @param ret true if deletion was succesful; elsewise false.
     */
    void deleteMailsReady( bool ret );
    
    /**
     * readMail() might signal readMailTotalSteps() to
     * send the expected total number of steps to a possible
     * progress bar. See readMailProgress();
     * @param totalSteps expected total number of steps.
     */
    void readMailTotalSteps(int totalSteps);

    /**
     * readMail() might signal readMailProgress() to
     * send the current progress in relation to the
     * expected total number of steps (see readMailTotalSteps()).
     * @param progress current progress.
     */
    void readMailProgress(int progress);
    
    /**
     * readMail() might signal readMailReady() if
     * a email is totally read. This is useful
     * in asynchrone situations.
     * @param msg pointer to the full email-message.
     */
    void readMailReady( QString* msg );
};

#endif // SSK_MAILDROP_H
