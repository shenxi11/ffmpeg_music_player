#include "httprequest.h"

HttpRequest::HttpRequest(QObject *parent)
{
    manager = new QNetworkAccessManager(this);
}
bool HttpRequest::Login(const QString& account, const QString& password)
{
    QUrl url = localUrl + "users/login";

    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["account"] = account;
    json["password"] = password;
    QJsonDocument jsonDoc(json);
    QByteArray jsonData = jsonDoc.toJson();

    QNetworkReply *reply = manager->post(request, jsonData);


    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response_data = reply->readAll();
            qDebug() << "Response:" << response_data;
            emit Loginflag(true);
        } else {
            qDebug() << "Error:" << reply->errorString();
            emit Loginflag(false);
        }
        reply->deleteLater();
    });
}
bool HttpRequest:: Register(const QString& account, const QString& password, const QString& username)
{
    QUrl url = localUrl + "users/register";

    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["account"] = account;
    json["password"] = password;
    json["username"] = username;
    QJsonDocument jsonDoc(json);
    QByteArray jsonData = jsonDoc.toJson();

    QNetworkReply *reply = manager->post(request, jsonData);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response_data = reply->readAll();
            qDebug() << "Response:" << response_data;
            emit Registerflag(true);
        } else {
            qDebug() << "Error:" << reply->errorString();
            emit Registerflag(false);
        }
        reply->deleteLater();
    });
}
