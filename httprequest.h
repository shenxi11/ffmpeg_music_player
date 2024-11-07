#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

class HttpRequest:public QObject
{
    Q_OBJECT
public:
    explicit  HttpRequest(QObject *parent = nullptr);

    bool Login(const QString& account, const QString& password);

    bool Register(const QString& account, const QString& password, const QString& username);
signals:
    void Loginflag(bool flag);
    void Registerflag(bool flag);

private:

    const QString localUrl = "http://localhost:8080/";

    QNetworkAccessManager* manager;
};

#endif // HTTPREQUEST_H
