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
#include <QList>
#include <memory>
#include <mutex>
#include "headers.h"
class User{
public:
    static User* getInstance()
    {
        if(instance == nullptr)
        {
            QMutexLocker locker(&mutex);
            if(instance == nullptr)
            {
                instance = new User();
            }
        }
        return instance;
    }

    void set_username(QString username){this->username = username;};
    void set_account(QString account){this->account = account;};
    void set_password(QString password){this->password = password;};
    void set_music_path(QStringList musics){this->music_path = musics;};

    QString get_username(){return username;};
    QString get_account(){return account;};
    QString get_password(){return password;};
    QStringList get_music_path(){return this->music_path;};
private:
    User(QString account = "", QString password = "", QString username = ""){};
    User& operator=(const User a) = delete;
    User(const User&) = delete;

    static User* instance;
    static QMutex mutex;
    QString username;
    QString password;
    QString account;
    QStringList music_path;
};

class HttpRequest:public QObject
{
    Q_OBJECT
public:


    static HttpRequest* getInstance()
    {
        if(instance == nullptr)
        {
            QMutexLocker locker(&mutex);
            if(instance == nullptr)
            {
                instance = new HttpRequest();
            }
        }
        return instance;
    }


    bool Login(const QString& account, const QString& password);
    bool Register(const QString& account, const QString& password, const QString& username);
    bool Upload(const QString& path);
    bool Download(const QString& filename, const QString download_folder);
    bool AddMusic(const QString username);
    bool getAllFiles();
    void get_music_data(const QString &fileName);
    void sendAcknowledgment();
    bool getMusic(const QString& name);
    bool get_file(const QString url);
signals:
    void signal_Loginflag(bool flag);
    void signal_Registerflag(bool flag);
    void signal_getusername(QString username);
    void signal_send_Packet(QByteArray chunk);
    void signal_addSong_list(const QStringList songName_list, const QList<double> duration);
    void signal_streamurl(bool flag, QString path);
    void signal_add_songs();
    void signal_lrc(QStringList content);
private:
    explicit  HttpRequest(QObject *parent = nullptr);
    HttpRequest(const HttpRequest&) = delete;
    HttpRequest& operator=(const HttpRequest&) = delete;

    const QString localUrl = "http://localhost:5000/";
    QNetworkAccessManager* manager;
    static HttpRequest* instance;
    static QMutex mutex;
};


#endif // HTTPREQUEST_H
