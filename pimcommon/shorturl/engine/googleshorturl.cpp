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

#include "googleshorturl.h"

#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>
#include <QJsonDocument>

using namespace PimCommon;
GoogleShortUrl::GoogleShortUrl(QObject *parent)
    : PimCommon::AbstractShortUrl(parent)
{
    connect(mNetworkAccessManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(slotSslErrors(QNetworkReply*,QList<QSslError>)));
}

GoogleShortUrl::~GoogleShortUrl()
{

}

void GoogleShortUrl::start()
{
    QNetworkRequest request(QUrl(QLatin1String("https://www.googleapis.com/urlshortener/v1/url")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));

    const QString data = QStringLiteral("{\"longUrl\": \"%1/\"}").arg(mOriginalUrl);

    QNetworkReply *reply = mNetworkAccessManager->post(request, data.toUtf8());
    connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &GoogleShortUrl::slotErrorFound);
}

QString GoogleShortUrl::shortUrlName() const
{
    return QLatin1String("Google");
}

void GoogleShortUrl::slotSslErrors(QNetworkReply *reply, const QList<QSslError> &error)
{
    reply->ignoreSslErrors(error);
}

void GoogleShortUrl::slotShortUrlFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    if (mErrorFound) {
        return;
    }

    qDebug() << "void GoogleShortUrl::slotShortUrlFinished(QNetworkReply *reply) " << reply->readAll();
    QJsonParseError error;
    const QJsonDocument json = QJsonDocument::fromJson(reply->readAll(), &error);
    if (error.error != QJsonParseError::NoError || json.isNull()) {
        qDebug() << " Error during parsing" << error.errorString();
        return;
    }
    const QMap<QString, QVariant> map = json.toVariant().toMap();

    if (map.contains(QLatin1String("id")) && map.contains(QLatin1String("kind"))) {
        Q_EMIT shortUrlDone(map.value(QLatin1String("id")).toString());
    }
}

