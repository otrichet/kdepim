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

#include "hubicstorageservice.h"
#include "hubicjob.h"

#include <KLocalizedString>
#include <KConfig>
#include <KGlobal>
#include <KConfigGroup>


using namespace PimCommon;

HubicStorageService::HubicStorageService(QObject *parent)
    : PimCommon::StorageServiceAbstract(parent)
{
    readConfig();
}

HubicStorageService::~HubicStorageService()
{
}

void HubicStorageService::readConfig()
{
    KConfigGroup grp(KGlobal::config(), "Hubic Settings");
    mRefreshToken = grp.readEntry("Refresh Token");
    mToken = grp.readEntry("Token");
    if (grp.hasKey("Expire Time"))
        mExpireDateTime = grp.readEntry("Expire Time", QDateTime::currentDateTime());
}

void HubicStorageService::removeConfig()
{
    KConfigGroup grp(KGlobal::config(), "Hubic Settings");
    grp.deleteGroup();
    KGlobal::config()->sync();
}

void HubicStorageService::authentication()
{
    HubicJob *job = new HubicJob(this);
    connect(job, SIGNAL(authorizationDone(QString,QString,qint64)), this, SLOT(slotAuthorizationDone(QString,QString,qint64)));
    connect(job, SIGNAL(authorizationFailed(QString)), this, SLOT(slotAuthorizationFailed(QString)));
    job->requestTokenAccess();
}

void HubicStorageService::slotAuthorizationFailed(const QString &errorMessage)
{
    mRefreshToken.clear();
    Q_EMIT authenticationFailed(serviceName(), errorMessage);
}

void HubicStorageService::slotAuthorizationDone(const QString &refreshToken, const QString &token, qint64 expireTime)
{
    mRefreshToken = refreshToken;
    mToken = token;
    KConfigGroup grp(KGlobal::config(), "Hubic Settings");
    grp.writeEntry("Refresh Token", mRefreshToken);
    grp.writeEntry("Token", mToken);
    grp.writeEntry("Expire Time", QDateTime::currentDateTime().addSecs(expireTime));
    grp.sync();
    emitAuthentificationDone();
}

void HubicStorageService::listFolder()
{
    if (mRefreshToken.isEmpty()) {
        mNextAction = ListFolder;
        authentication();
    } else {
        HubicJob *job = new HubicJob(this);
        job->initializeToken(mRefreshToken, mToken, mExpireDateTime);
        connect(job, SIGNAL(listFolderDone(QStringList)), this, SLOT(slotListFolderDone(QStringList)));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->listFolder();
    }
}

void HubicStorageService::createFolder(const QString &folder)
{
    if (mRefreshToken.isEmpty()) {
        mNextAction = CreateFolder;
        authentication();
    } else {
        HubicJob *job = new HubicJob(this);
        job->initializeToken(mRefreshToken, mToken, mExpireDateTime);
        connect(job, SIGNAL(createFolderDone(QString)), this, SLOT(slotCreateFolderDone(QString)));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->createFolder(folder);
    }
}

void HubicStorageService::accountInfo()
{
    if (mRefreshToken.isEmpty()) {
        mNextAction = AccountInfo;
        authentication();
    } else {
        HubicJob *job = new HubicJob(this);
        job->initializeToken(mRefreshToken, mToken, mExpireDateTime);
        connect(job,SIGNAL(accountInfoDone(PimCommon::AccountInfo)), this, SLOT(slotAccountInfoDone(PimCommon::AccountInfo)));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->accountInfo();
    }
}

QString HubicStorageService::name()
{
    return i18n("Hubic");
}

void HubicStorageService::uploadFile(const QString &filename)
{
    if (mRefreshToken.isEmpty()) {
        mNextAction = UploadFile;
        authentication();
    } else {
        HubicJob *job = new HubicJob(this);
        job->initializeToken(mRefreshToken, mToken, mExpireDateTime);
        connect(job, SIGNAL(uploadFileDone(QString)), this, SLOT(slotUploadFileDone(QString)));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        connect(job, SIGNAL(uploadFileProgress(qint64,qint64)), SLOT(slotUploadFileProgress(qint64,qint64)));
        job->uploadFile(filename);
    }
}

QString HubicStorageService::description()
{
    return i18n("Hubic is a file hosting service operated by Ovh, Inc. that offers cloud storage, file synchronization, and client software.");
}

QUrl HubicStorageService::serviceUrl()
{
    return QUrl(QLatin1String("https://hubic.com"));
}

QString HubicStorageService::serviceName()
{
    return QLatin1String("hubic");
}

QString HubicStorageService::iconName()
{
    return QString();
}

void HubicStorageService::shareLink(const QString &root, const QString &path)
{
    if (mRefreshToken.isEmpty()) {
        mNextAction = ShareLink;
        authentication();
    } else {
        HubicJob *job = new HubicJob(this);
        job->initializeToken(mRefreshToken, mToken, mExpireDateTime);
        connect(job, SIGNAL(shareLinkDone(QString)), this, SLOT(slotShareLinkDone(QString)));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->shareLink(root, path);
    }
}

QString HubicStorageService::storageServiceName() const
{
    return serviceName();
}

void HubicStorageService::downloadFile(const QString &filename)
{
    if (mRefreshToken.isEmpty()) {
        mNextAction = DownLoadFile;
        authentication();
    } else {
        HubicJob *job = new HubicJob(this);
        job->initializeToken(mRefreshToken, mToken, mExpireDateTime);
        connect(job, SIGNAL(downLoadFileDone(QString)), this, SLOT(slotDownLoadFileDone(QString)));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->downloadFile(filename);
    }
}

void HubicStorageService::createServiceFolder()
{
    if (mRefreshToken.isEmpty()) {
        mNextAction = CreateServiceFolder;
        authentication();
    } else {
        HubicJob *job = new HubicJob(this);
        job->initializeToken(mRefreshToken, mToken, mExpireDateTime);
        connect(job, SIGNAL(createFolderDone(QString)), this, SLOT(slotCreateFolderDone(QString)));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->createServiceFolder();
    }
}

void HubicStorageService::deleteFile(const QString &filename)
{
    if (mRefreshToken.isEmpty()) {
        mNextAction = DeleteFile;
        authentication();
    } else {
        HubicJob *job = new HubicJob(this);
        job->initializeToken(mRefreshToken, mToken, mExpireDateTime);
        connect(job, SIGNAL(deleteFileDone(QString)), SLOT(slotDeleteFileDone(QString)));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->deleteFile(filename);
    }
}

void HubicStorageService::deleteFolder(const QString &foldername)
{
    if (mRefreshToken.isEmpty()) {
        mNextAction = DeleteFolder;
        authentication();
    } else {
        HubicJob *job = new HubicJob(this);
        job->initializeToken(mRefreshToken, mToken, mExpireDateTime);
        connect(job, SIGNAL(deleteFolderDone(QString)), SLOT(slotDeleteFolderDone(QString)));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->deleteFolder(foldername);
    }
}

KIcon HubicStorageService::icon() const
{
    return KIcon();
}
