/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "dropboxstorageservice.h"
#include "dropboxjob.h"

#include <KLocale>
#include <KConfig>
#include <KGlobal>
#include <KConfigGroup>
#include <KMessageBox>

#include <QDebug>


using namespace PimCommon;

DropBoxStorageService::DropBoxStorageService(QObject *parent)
    : PimCommon::StorageServiceAbstract(parent)
{
    readConfig();
}

DropBoxStorageService::~DropBoxStorageService()
{
}

void DropBoxStorageService::removeConfig()
{
    KConfigGroup grp(KGlobal::config(), "Dropbox Settings");
    grp.deleteGroup();
    KGlobal::config()->sync();
}

void DropBoxStorageService::authentification()
{
    if (mAccessToken.isEmpty()) {
        DropBoxJob *job = new DropBoxJob(this);
        connect(job, SIGNAL(authorizationDone(QString,QString,QString)), this, SLOT(slotAuthorizationDone(QString,QString,QString)));
        connect(job, SIGNAL(authorizationFailed()), this, SLOT(slotAuthorizationFailed()));
        job->requestTokenAccess();
    }
}

void DropBoxStorageService::shareLink(const QString &root, const QString &path)
{

}

void DropBoxStorageService::readConfig()
{
    KConfigGroup grp(KGlobal::config(), "Dropbox Settings");
    mAccessToken = grp.readEntry("Access Token");
    mAccessTokenSecret = grp.readEntry("Access Token Secret");
    mAccessOauthSignature = grp.readEntry("Access Oauth Signature");
}

void DropBoxStorageService::slotAuthorizationDone(const QString &accessToken, const QString &accessTokenSecret, const QString &accessOauthSignature)
{
    mAccessToken = accessToken;
    mAccessTokenSecret = accessTokenSecret;
    mAccessOauthSignature = accessOauthSignature;
    KConfigGroup grp(KGlobal::config(), "Dropbox Settings");
    grp.writeEntry("Access Token", mAccessToken);
    grp.writeEntry("Access Token Secret", mAccessTokenSecret);
    grp.writeEntry("Access Oauth Signature", mAccessOauthSignature);
    grp.sync();
    KGlobal::config()->sync();
}

void DropBoxStorageService::slotActionFailed(const QString &error)
{
    qDebug()<<" error found "<<error;
    Q_EMIT actionFailed(serviceName(), error);
}

void DropBoxStorageService::slotUploadFileProgress(qint64 done, qint64 total)
{
    Q_EMIT uploadFileProgress(serviceName(), done, total);
}

void DropBoxStorageService::listFolder()
{
    DropBoxJob *job = new DropBoxJob(this);
    if (mAccessToken.isEmpty()) {
        connect(job, SIGNAL(authorizationDone(QString,QString,QString)), this, SLOT(slotAuthorizationDone(QString,QString,QString)));
        connect(job, SIGNAL(authorizationFailed()), this, SLOT(slotAuthorizationFailed()));
        job->requestTokenAccess();
    } else {
        job->initializeToken(mAccessToken,mAccessTokenSecret,mAccessOauthSignature);
        connect(job, SIGNAL(listFolderDone()), this, SLOT(slotListFolderDone()));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->listFolder();
    }
}

void DropBoxStorageService::slotListFolderDone()
{
    //TODO
}

void DropBoxStorageService::accountInfo()
{
    DropBoxJob *job = new DropBoxJob(this);
    if (mAccessToken.isEmpty()) {
        connect(job, SIGNAL(authorizationDone(QString,QString,QString)), this, SLOT(slotAuthorizationDone(QString,QString,QString)));
        connect(job, SIGNAL(authorizationFailed()), this, SLOT(slotAuthorizationFailed()));
        job->requestTokenAccess();
    } else {
        job->initializeToken(mAccessToken,mAccessTokenSecret,mAccessOauthSignature);
        connect(job, SIGNAL(accountInfoDone(PimCommon::AccountInfo)), this, SLOT(slotAccountInfoDone(PimCommon::AccountInfo)));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->accountInfo();
    }
}

void DropBoxStorageService::slotAccountInfoDone(const PimCommon::AccountInfo &info)
{
    Q_EMIT accountInfoDone(serviceName(), info);
}

void DropBoxStorageService::createFolder(const QString &folder)
{
    DropBoxJob *job = new DropBoxJob(this);
    if (mAccessToken.isEmpty()) {
        connect(job, SIGNAL(authorizationDone(QString,QString,QString)), this, SLOT(slotAuthorizationDone(QString,QString,QString)));
        connect(job, SIGNAL(authorizationFailed()), this, SLOT(slotAuthorizationFailed()));
        job->requestTokenAccess();
    } else {
        job->initializeToken(mAccessToken,mAccessTokenSecret,mAccessOauthSignature);
        connect(job, SIGNAL(createFolderDone()), this, SLOT(slotCreateFolderDone()));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->createFolder(folder);
    }
}

void DropBoxStorageService::slotCreateFolderDone()
{
    //TODO
    qDebug()<<" folder created";
}

void DropBoxStorageService::uploadFile(const QString &filename)
{
    DropBoxJob *job = new DropBoxJob(this);
    if (mAccessToken.isEmpty()) {
        connect(job, SIGNAL(authorizationDone(QString,QString,QString)), this, SLOT(slotAuthorizationDone(QString,QString,QString)));
        connect(job, SIGNAL(authorizationFailed()), this, SLOT(slotAuthorizationFailed()));
        job->requestTokenAccess();
    } else {
        job->initializeToken(mAccessToken,mAccessTokenSecret,mAccessOauthSignature);
        connect(job, SIGNAL(uploadFileDone()), this, SLOT(slotUploadFileDone()));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        connect(job, SIGNAL(uploadFileProgress(qint64,qint64)), SLOT(slotUploadFileProgress(qint64,qint64)));
        job->uploadFile(filename);
    }
}

void DropBoxStorageService::slotUploadFileDone()
{
    qDebug()<<" Upload file done";
}


void DropBoxStorageService::slotAuthorizationFailed()
{
    mAccessToken.clear();
    mAccessTokenSecret.clear();
    mAccessOauthSignature.clear();
}

QString DropBoxStorageService::name()
{
    return i18n("Dropbox");
}

QString DropBoxStorageService::description()
{
    return i18n("Dropbox is a file hosting service operated by Dropbox, Inc. that offers cloud storage, file synchronization, and client software.");
}

QUrl DropBoxStorageService::serviceUrl()
{
    return QUrl(QLatin1String("https://www.dropbox.com/"));
}

QString DropBoxStorageService::serviceName()
{
    return QLatin1String("dropbox");
}

