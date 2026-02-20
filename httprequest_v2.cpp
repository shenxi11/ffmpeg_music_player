#include "httprequest_v2.h"
#include "httprequest.h"
#include "headers.h"
#include "download_manager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

HttpRequestV2::HttpRequestV2(QObject* parent)
    : QObject(parent)
    , m_networkService(Network::NetworkService::instance())
{
    // 设置基础URL
    m_networkService.setBaseUrl(m_baseUrl);
    
    // DNS预热
    m_networkService.prewarmDns("slcdut.xyz");
    
    qDebug() << "[HttpRequestV2] Initialized with new network layer";
}

void HttpRequestV2::login(const QString& account, const QString& password)
{
    QJsonObject json;
    json["account"] = account;
    json["password"] = password;
    
    // 登录使用Critical优先级，确保快速响应
    auto options = Network::RequestOptions::critical();
    
    qDebug() << "[HttpRequestV2] Login:" << account;
    
    m_networkService.postJson("users/login", json, options,
        [this, account, password](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                
                qDebug() << "[HttpRequestV2] Login response received in" << response.elapsedMs << "ms";
                
                // 检查登录是否成功
                bool loginSuccess = false;
                if (result.contains("success_bool")) {
                    loginSuccess = result.value("success_bool").toBool();
                } else if (result.contains("success")) {
                    QString successStr = result.value("success").toString();
                    loginSuccess = (successStr == "true");
                }
                
                if (loginSuccess) {
                    auto user = User::getInstance();
                    user->set_account(account);
                    user->set_password(password);
                    
                    if (result.contains("username")) {
                        QString username = result.value("username").toString();
                        user->set_username(username);
                        emit signal_getusername(username);
                        qDebug() << "[HttpRequestV2] Login successful, username:" << username;
                    }
                    
                    // 设置用户收藏的歌曲列表
                    if (result.contains("song_path_list")) {
                        QJsonArray songPathArray = result.value("song_path_list").toArray();
                        QStringList musics;
                        for (const auto& songPath : songPathArray) {
                            musics << songPath.toString();
                        }
                        user->set_music_path(musics);
                        qDebug() << "[HttpRequestV2] User's favorite songs:" << musics.size();
                    }
                    
                    emit signal_Loginflag(true);
                } else {
                    qDebug() << "[HttpRequestV2] Login failed: invalid credentials";
                    emit signal_Loginflag(false);
                }
            } else {
                qWarning() << "[HttpRequestV2] Login request error:" << response.errorString;
                emit signal_Loginflag(false);
            }
        }
    );
}

void HttpRequestV2::registerUser(const QString& account, const QString& password, const QString& username)
{
    QJsonObject json;
    json["account"] = account;
    json["password"] = password;
    json["username"] = username;
    
    auto options = Network::RequestOptions::critical();
    
    qDebug() << "[HttpRequestV2] Register:" << account;
    
    m_networkService.postJson("users/register", json, options,
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                qDebug() << "[HttpRequestV2] Registration successful";
                emit signal_Registerflag(true);
            } else {
                qWarning() << "[HttpRequestV2] Registration error:" << response.errorString;
                emit signal_Registerflag(false);
            }
        }
    );
}

void HttpRequestV2::getAllFiles(bool useCache)
{
    Network::RequestOptions options;
    if (useCache) {
        options.useCache = true;
        options.cacheTtl = 300;  // 缓存5分钟
    }
    options.priority = Network::RequestPriority::High;
    
    qDebug() << "[HttpRequestV2] Getting all files, useCache:" << useCache;
    
    m_networkService.get("files", options,
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonArray filesArray = Network::NetworkService::parseJsonArray(response);
                QList<Music> musicList;
                
                qDebug() << "[HttpRequestV2] Received" << filesArray.size() << "files"
                         << (response.isFromCache ? "(from cache)" : "(from server)")
                         << "in" << response.elapsedMs << "ms";
                
                for (const QJsonValue& value : filesArray) {
                    if (value.isObject()) {
                        QJsonObject fileObj = value.toObject();
                        QString filePath = fileObj["path"].toString();
                        QString durationStr = fileObj["duration"].toString();
                        QString coverUrl = fileObj["cover_art_url"].toString();
                        QString artist = fileObj.contains("artist") ? fileObj["artist"].toString() :
                                        (fileObj.contains("singer") ? fileObj["singer"].toString() : "未知艺术家");
                        
                        // 解析时长
                        double durationValue = 0.0;
                        if (durationStr != "Error") {
                            QStringList parts = durationStr.split(" ");
                            if (!parts.isEmpty()) {
                                durationValue = parts[0].toDouble();
                            }
                        }
                        
                        Music music;
                        music.setSongPath(filePath);
                        music.setSinger(artist);
                        music.setDuration(static_cast<long>(durationValue));
                        music.setPicPath(coverUrl);
                        
                        musicList.append(music);
                    }
                }
                
                emit signal_addSong_list(musicList);
            } else {
                qWarning() << "[HttpRequestV2] Get all files error:" << response.errorString;
            }
        }
    );
}

void HttpRequestV2::getLyrics(const QString& url)
{
    // 解析URL获取目录
    QUrl qurl(url);
    QString path = qurl.path();
    int lastSlashPos = path.lastIndexOf('/');
    QString dirPath = (lastSlashPos == -1) ? path : path.left(lastSlashPos);
    
    QString lrcUrl = QString("%1://%2%3%4/lrc")
        .arg(qurl.scheme())
        .arg(qurl.host())
        .arg(qurl.port() > 0 ? ":" + QString::number(qurl.port()) : "")
        .arg(dirPath);
    
    qDebug() << "[HttpRequestV2] Getting lyrics from:" << lrcUrl;
    
    // 使用缓存，歌词一般不会变化
    auto options = Network::RequestOptions::withCache(3600);  // 缓存1小时
    
    m_networkService.get(lrcUrl, options,
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonArray filesArray = Network::NetworkService::parseJsonArray(response);
                QStringList lines;
                
                for (const auto& item : filesArray) {
                    lines << item.toString();
                }
                
                qDebug() << "[HttpRequestV2] Lyrics received:" << lines.size() << "lines";
                emit signal_lrc(lines);
            } else {
                qWarning() << "[HttpRequestV2] Get lyrics error:" << response.errorString;
            }
        }
    );
}

void HttpRequestV2::downloadFile(const QString& filename, const QString& downloadFolder,
                                bool downloadLyrics, const QString& coverUrl)
{
    // 下载仍使用DownloadManager（异步下载管理）
    QString downloadUrl = m_baseUrl + "download";
    
    QJsonObject jsonObject;
    jsonObject["filename"] = filename;
    QJsonDocument jsonDoc(jsonObject);
    QByteArray postData = jsonDoc.toJson();
    
    qDebug() << "[HttpRequestV2] Adding download to queue:" << filename;
    
    DownloadManager::instance().addDownload(downloadUrl, filename, downloadFolder, postData, coverUrl);
    
    if (downloadLyrics) {
        QString lrcFilename = filename;
        int lastDot = lrcFilename.lastIndexOf('.');
        if (lastDot != -1) {
            lrcFilename = lrcFilename.left(lastDot) + ".lrc";
        }
        
        QJsonObject lrcJsonObject;
        lrcJsonObject["filename"] = lrcFilename;
        QJsonDocument lrcJsonDoc(lrcJsonObject);
        QByteArray lrcPostData = lrcJsonDoc.toJson();
        
        DownloadManager::instance().addDownload(downloadUrl, lrcFilename, downloadFolder, lrcPostData);
    }
}

void HttpRequestV2::addFavorite(const QString& userAccount, const QString& path, const QString& title,
                               const QString& artist, const QString& duration, bool isLocal)
{
    QJsonObject json;
    json["user_account"] = userAccount;
    json["path"] = path;
    json["title"] = title;
    json["artist"] = artist;
    json["duration"] = duration;
    json["is_local"] = isLocal;
    
    qDebug() << "[HttpRequestV2] Adding favorite:" << title;
    
    m_networkService.postJson("user/favorites/add", json, {},
        [this, title](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                bool success = result.value("success").toBool();
                
                qDebug() << "[HttpRequestV2] Add favorite" << (success ? "success" : "failed") << title;
                emit signal_addFavoriteResult(success);
            } else {
                qWarning() << "[HttpRequestV2] Add favorite error:" << response.errorString;
                emit signal_addFavoriteResult(false);
            }
        }
    );
}

void HttpRequestV2::getFavorites(const QString& userAccount)
{
    QString url = QString("user/favorites?user_account=%1").arg(userAccount);
    
    // 使用缓存，减少服务器负载
    auto options = Network::RequestOptions::withCache(60);  // 缓存1分钟
    
    qDebug() << "[HttpRequestV2] Getting favorites for:" << userAccount;
    
    m_networkService.get(url, options,
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                QJsonArray favoritesArray = result.value("favorites").toArray();
                
                QVariantList favorites;
                for (const auto& item : favoritesArray) {
                    favorites.append(item.toVariant());
                }
                
                qDebug() << "[HttpRequestV2] Favorites received:" << favorites.size()
                         << (response.isFromCache ? "(from cache)" : "");
                emit signal_favoritesList(favorites);
            } else {
                qWarning() << "[HttpRequestV2] Get favorites error:" << response.errorString;
            }
        }
    );
}

void HttpRequestV2::addPlayHistory(const QString& userAccount, const QString& path, const QString& title,
                                  const QString& artist, const QString& album, const QString& duration, bool isLocal)
{
    QJsonObject json;
    json["user_account"] = userAccount;
    json["path"] = path;
    json["title"] = title;
    json["artist"] = artist;
    json["album"] = album;
    json["duration"] = duration;
    json["is_local"] = isLocal;
    
    // 播放历史使用低优先级（不影响关键功能）
    auto options = Network::RequestOptions::withPriority(Network::RequestPriority::Low);
    
    m_networkService.postJson("user/history/add", json, options,
        [this, title](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                bool success = result.value("success").toBool();
                emit signal_addHistoryResult(success);
            } else {
                // 播放历史失败不影响用户体验，只记录日志
                qDebug() << "[HttpRequestV2] Add history failed (not critical):" << response.errorString;
                emit signal_addHistoryResult(false);
            }
        }
    );
}

void HttpRequestV2::getPlayHistory(const QString& userAccount, int limit)
{
    QString url = QString("user/history?user_account=%1&limit=%2").arg(userAccount).arg(limit);
    
    auto options = Network::RequestOptions::withCache(30);  // 缓存30秒
    
    m_networkService.get(url, options,
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                QJsonArray historyArray = result.value("history").toArray();
                
                QVariantList history;
                for (const auto& item : historyArray) {
                    history.append(item.toVariant());
                }
                
                emit signal_historyList(history);
            } else {
                qWarning() << "[HttpRequestV2] Get history error:" << response.errorString;
            }
        }
    );
}

void HttpRequestV2::getVideoList()
{
    auto options = Network::RequestOptions::withCache(300);  // 缓存5分钟
    
    m_networkService.get("videos", options,
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonArray jsonArray = Network::NetworkService::parseJsonArray(response);
                QVariantList videoList;
                
                for (const QJsonValue& value : jsonArray) {
                    QJsonObject obj = value.toObject();
                    QVariantMap videoInfo;
                    videoInfo["name"] = obj["name"].toString();
                    videoInfo["path"] = obj["path"].toString();
                    videoInfo["size"] = obj["size"].toVariant().toLongLong();
                    videoList.append(videoInfo);
                }
                
                qDebug() << "[HttpRequestV2] Video list:" << videoList.size();
                emit signal_videoList(videoList);
            } else {
                qWarning() << "[HttpRequestV2] Get video list error:" << response.errorString;
            }
        }
    );
}

void HttpRequestV2::getVideoStreamUrl(const QString& videoPath)
{
    QJsonObject json;
    json["path"] = videoPath;
    
    m_networkService.postJson("video/stream", json, {},
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                QString videoUrl = result["url"].toString();
                
                qDebug() << "[HttpRequestV2] Video stream URL:" << videoUrl;
                emit signal_videoStreamUrl(videoUrl);
            } else {
                qWarning() << "[HttpRequestV2] Get video stream error:" << response.errorString;
            }
        }
    );
}

void HttpRequestV2::searchArtist(const QString& artist)
{
    QJsonObject json;
    json["artist"] = artist;
    
    // 搜索使用高优先级
    auto options = Network::RequestOptions::withPriority(Network::RequestPriority::High);
    
    m_networkService.postJson("artist/search", json, options,
        [this, artist](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                bool exists = result["exists"].toBool();
                
                qDebug() << "[HttpRequestV2] Artist" << artist << "exists:" << exists;
                emit signal_artistExists(exists, artist);
            } else {
                qWarning() << "[HttpRequestV2] Search artist error:" << response.errorString;
                emit signal_artistExists(false, artist);
            }
        }
    );
}

void HttpRequestV2::getMusicByArtist(const QString& artist)
{
    QJsonObject json;
    json["artist"] = artist;
    
    // 缓存艺术家音乐列表
    auto options = Network::RequestOptions::withCache(600);  // 缓存10分钟
    
    m_networkService.postJson("music/artist", json, options,
        [this, artist](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                QJsonArray musicArray = result.value("music_list").toArray();
                
                QList<Music> musicList;
                for (const auto& item : musicArray) {
                    QJsonObject obj = item.toObject();
                    
                    Music music;
                    music.setSongPath(obj["path"].toString());
                    music.setSinger(obj["artist"].toString());
                    music.setDuration(obj["duration"].toInt());
                    music.setPicPath(obj["cover_url"].toString());
                    
                    musicList.append(music);
                }
                
                qDebug() << "[HttpRequestV2] Music by artist" << artist << ":" << musicList.size();
                emit signal_artistMusicList(musicList, artist);
            } else {
                qWarning() << "[HttpRequestV2] Get music by artist error:" << response.errorString;
            }
        }
    );
}
