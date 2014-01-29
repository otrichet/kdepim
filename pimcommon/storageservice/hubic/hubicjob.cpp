/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

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

#include "hubicjob.h"
#include "pimcommon/storageservice/storageserviceabstract.h"
#include "pimcommon/storageservice/storageservicejobconfig.h"

#include <qjson/parser.h>

#include <QDebug>

using namespace PimCommon;

HubicJob::HubicJob(QObject *parent)
    : PimCommon::OAuth2Job(parent)
{
    mScope = PimCommon::StorageServiceJobConfig::self()->hubicScope();
    mClientId = PimCommon::StorageServiceJobConfig::self()->hubicClientId();
    mClientSecret = PimCommon::StorageServiceJobConfig::self()->hubicClientSecret();
    mRedirectUri = PimCommon::StorageServiceJobConfig::self()->oauth2RedirectUrl();
    mServiceUrl = QLatin1String("https://api.hubic.com");
    mApiUrl = QLatin1String("https://api.hubic.com");
    mAuthorizePath = QLatin1String("/oauth/auth/");
    mPathToken = QLatin1String("/oauth/token/");
    mCurrentAccountInfoPath = QLatin1String("/1.0/account/usage");
    //FIXME
    //mFolderInfoPath = QLatin1String("/2.0/folders/");
    //mFileInfoPath = QLatin1String("/2.0/files/");
}

HubicJob::~HubicJob()
{

}

void HubicJob::refreshToken()
{
    mActionType = PimCommon::StorageServiceAbstract::AccessToken;
    QNetworkRequest request(QUrl(mServiceUrl + QLatin1String("/oauth/token")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/x-www-form-urlencoded"));

    QUrl postData;

    postData.addQueryItem(QLatin1String("refresh_token"), mRefreshToken);
    postData.addQueryItem(QLatin1String("grant_type"), QLatin1String("refresh_token"));
    postData.addQueryItem(QLatin1String("client_id"), mClientId);
    postData.addQueryItem(QLatin1String("client_secret"), mClientSecret);
    qDebug()<<"refreshToken postData: "<<postData;

    QNetworkReply *reply = mNetworkAccessManager->post(request, postData.encodedQuery());
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(slotError(QNetworkReply::NetworkError)));
}


void HubicJob::parseAccountInfo(const QString &data)
{
    QJson::Parser parser;
    bool ok;

    const QMap<QString, QVariant> info = parser.parse(data.toUtf8(), &ok).toMap();
    PimCommon::AccountInfo accountInfo;
    if (info.contains(QLatin1String("used"))) {
        accountInfo.shared = info.value(QLatin1String("used")).toLongLong();
    }
    if (info.contains(QLatin1String("quota"))) {
        accountInfo.quota = info.value(QLatin1String("quota")).toLongLong();
    }
    Q_EMIT accountInfoDone(accountInfo);
    deleteLater();
}

