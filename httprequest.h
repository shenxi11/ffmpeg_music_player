#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QWaitCondition>
#include <QMutex>
#include <QByteArray>
#include <QQueue>


class HttpRequest:public QObject
{
    Q_OBJECT
public:
    explicit  HttpRequest(QObject *parent = nullptr);

    bool Login(const QString& account, const QString& password);

    bool Register(const QString& account, const QString& password, const QString& username);

    bool Upload(const QString& path);

    bool Download(const QString& filename);

    bool getAllFiles();

    void sendRequestAndForwardData();

    void sendAcknowledgment();

    bool getMusic(const QString& name);
signals:
    void signal_Loginflag(bool flag);
    void signal_Registerflag(bool flag);
    void signal_getusername(QString username);
    void signal_send_Packet(QByteArray chunk);
    void signal_addSong(const QString songName,const QString songPath);
private:

    const QString localUrl = "http://localhost:5000/";

    QNetworkAccessManager* manager;
};

#endif // HTTPREQUEST_H
