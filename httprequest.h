#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
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
#include "music.h"
#include "download_manager.h"

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

    void set_username(QString username);
    void set_account(QString account);
    void set_password(QString password);
    void set_music_path(QStringList musics);

    QString get_username();
    QString get_account();
    QString get_password();
    QStringList get_music_path();
signals:
    void signal_add_songs();
private:
    User(QString account = "", QString password = "", QString username = "");
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
    bool Download(const QString& filename, const QString download_folder, bool downloadLyrics = true, const QString& coverUrl = QString());
    bool AddMusic(const QString username);
    bool getAllFiles();
    void get_music_data(const QString &fileName);
    void sendAcknowledgment();
    bool getMusic(const QString& name);
    bool get_file(const QString url);
    
    // 视频相关接口
    bool getVideoList();  // 获取视频列表 GET /videos
    bool getVideoStreamUrl(const QString& videoPath);  // 获取视频流URL POST /video/stream
    
    // 歌手相关接口
    void searchArtist(const QString& artist);  // 搜索歌手是否存在 POST /artist/search
    void getMusicByArtist(const QString& artist);  // 根据歌手查询音乐 POST /music/artist
    
    // 喜欢音乐相关接口
    void addFavorite(const QString& userAccount, const QString& path, const QString& title, 
                    const QString& artist, const QString& duration, bool isLocal);  // 添加喜欢音乐 POST /user/favorites/add
    void removeFavorite(const QString& userAccount, const QStringList& paths);  // 移除喜欢音乐 DELETE /user/favorites/remove
    void getFavorites(const QString& userAccount);  // 获取喜欢音乐列表 GET /user/favorites
    
    // 播放历史相关接口
    void addPlayHistory(const QString& userAccount, const QString& path, const QString& title,
                       const QString& artist, const QString& album, const QString& duration, bool isLocal);  // 添加播放历史 POST /user/history/add
    void getPlayHistory(const QString& userAccount, int limit = 50);  // 获取播放历史 GET /user/history
    void removePlayHistory(const QString& userAccount, const QStringList& paths);  // 删除播放历史 DELETE /user/history/remove

    void setIsUsing(bool flag_);
    bool getIsUsing();
signals:
    void signal_Loginflag(bool flag);
    void signal_Registerflag(bool flag);
    void signal_getusername(QString username);
    void signal_send_Packet(QByteArray chunk);
    void signal_addSong_list(const QList<Music>& musicList);
    void signal_streamurl(bool flag, QString path);
    void signal_add_songs();
    void signal_lrc(QStringList content);
    
    // 视频相关信号
    void signal_videoList(const QVariantList& videoList);  // 视频列表信号
    void signal_videoStreamUrl(const QString& videoUrl);    // 视频流URL信号
    
    // 歌手相关信号
    void signal_artistExists(bool exists, const QString& artist);  // 歌手是否存在信号
    void signal_artistMusicList(const QList<Music>& musicList, const QString& artist);  // 歌手音乐列表信号
    
    // 喜欢音乐相关信号
    void signal_addFavoriteResult(bool success);  // 添加喜欢音乐结果信号
    void signal_removeFavoriteResult(bool success);  // 移除喜欢音乐结果信号
    void signal_favoritesList(const QVariantList& favorites);  // 喜欢音乐列表信号
    
    // 播放历史相关信号
    void signal_addHistoryResult(bool success);  // 添加播放历史结果信号
    void signal_historyList(const QVariantList& history);  // 播放历史列表信号
    void signal_removeHistoryResult(bool success);  // 删除播放历史结果信号
    
private:
    HttpRequest(const HttpRequest&) = delete;
    HttpRequest& operator=(const HttpRequest&) = delete;

    const QString localUrl = "http://slcdut.xyz:8080/";
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
