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
class User:public QObject{
    Q_OBJECT
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
    void set_music_path(QStringList musics){this->music_path = musics; emit signal_add_songs();};

    QString get_username(){return username;};
    QString get_account(){return account;};
    QString get_password(){return password;};
    QStringList get_music_path(){return this->music_path;};
signals:
    void signal_add_songs();
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
    explicit  HttpRequest(QObject *parent = nullptr);
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

    void setIsUsing(bool flag_);
    bool getIsUsing();
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
    HttpRequest(const HttpRequest&) = delete;
    HttpRequest& operator=(const HttpRequest&) = delete;

    const QString localUrl = "http://192.168.110.53:8080/";
    QNetworkAccessManager* manager = nullptr;

    bool isUsing = false;
};

class HttpRequestPool{
public:
    static HttpRequestPool& getInstance(){
        static HttpRequestPool pool(1);
        return pool;
    }
    HttpRequest* getRequest(){
        for(int i = 0; i < requestList.size(); i++){
            if(!requestList[i]->getIsUsing()){
                HttpRequest* request = requestList[i];
                requestList[i]->setIsUsing(true);
                return request;
            }
        }
        HttpRequest* request = new HttpRequest();
        requestList.emplace_back(request);
        request->setIsUsing(true);
        request_cnt ++;
        return request;
    }
    void setRequestNum(int n){
        request_cnt = n;
    }
    int getRequestNum(){
        return request_cnt;
    }
    ~HttpRequestPool(){
        for(int i = 0; i < request_cnt; i++){
            auto request = requestList.back();
            requestList.pop_back();
            delete request;
            request = nullptr;
        }
    }
private:
    HttpRequestPool(int cnt = 1):request_cnt(cnt){
        for(int i = 0; i < request_cnt; i++){
            HttpRequest *request = new HttpRequest();
            requestList.emplace_back(request);
        }
    };
    HttpRequestPool(const HttpRequestPool& httpRequestPool) = delete ;
    HttpRequestPool& operator = (const HttpRequestPool& httpRequestPool) = delete ;

    int request_cnt = 0;
    std::vector<HttpRequest*> requestList;
};

#endif // HTTPREQUEST_H
