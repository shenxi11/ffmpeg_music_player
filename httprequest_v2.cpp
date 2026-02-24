#include "httprequest_v2.h"
#include "headers.h"
#include "download_manager.h"
#include "user.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>

namespace {
QString readArtistFromObject(const QJsonObject& obj);

QString normalizeArtist(const QString& artist)
{
    const QString trimmed = artist.trimmed();
    if (trimmed.isEmpty() || trimmed == "null" || trimmed == "NULL") {
        return QString();
    }
    return trimmed;
}

QString readArtistFromArray(const QJsonArray& array)
{
    QStringList names;
    names.reserve(array.size());
    for (const QJsonValue& value : array) {
        if (value.isString()) {
            const QString name = normalizeArtist(value.toString());
            if (!name.isEmpty()) {
                names.append(name);
            }
            continue;
        }

        if (value.isObject()) {
            const QString name = readArtistFromObject(value.toObject());
            if (!name.isEmpty()) {
                names.append(name);
            }
        }
    }
    return names.join(" / ");
}

QString readArtistValue(const QJsonObject& obj, const QString& key)
{
    if (!obj.contains(key)) {
        return QString();
    }

    const QJsonValue value = obj.value(key);
    if (value.isString()) {
        return normalizeArtist(value.toString());
    }
    if (value.isArray()) {
        return readArtistFromArray(value.toArray());
    }
    if (value.isObject()) {
        return readArtistFromObject(value.toObject());
    }
    return QString();
}

QString readArtistFromObject(const QJsonObject& obj)
{
    // Compatible with multiple server payload styles.
    static const QStringList directKeys = {
        "artist", "singer", "author", "artist_name", "name"
    };
    for (const QString& key : directKeys) {
        const QString artist = readArtistValue(obj, key);
        if (!artist.isEmpty()) {
            return artist;
        }
    }

    static const QStringList arrayKeys = {
        "artists", "ar"
    };
    for (const QString& key : arrayKeys) {
        const QString artist = readArtistValue(obj, key);
        if (!artist.isEmpty()) {
            return artist;
        }
    }

    // Some APIs put song metadata under nested objects.
    static const QStringList nestedKeys = {
        "music", "song", "track", "file"
    };
    for (const QString& key : nestedKeys) {
        if (obj.contains(key) && obj.value(key).isObject()) {
            const QString artist = readArtistFromObject(obj.value(key).toObject());
            if (!artist.isEmpty()) {
                return artist;
            }
        }
    }

    return QString();
}
} // namespace

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
                // 服务端直接返回JSON数组
                QJsonArray favoritesArray = Network::NetworkService::parseJsonArray(response);
                
                QVariantList favorites;
                for (const QJsonValue& value : favoritesArray) {
                    QJsonObject favObj = value.toObject();
                    QVariantMap favorite;
                    
                    QString path = favObj["path"].toString();
                    bool isLocal = favObj["is_local"].toBool();
                    
                    // 对于在线音乐，如果path是相对路径，需要补全为完整URL
                    if (!isLocal && !path.startsWith("http")) {
                        path = m_baseUrl + "uploads/" + path;
                        qDebug() << "[HttpRequestV2] Online music path completed:" << path;
                    }
                    
                    favorite["path"] = path;
                    favorite["title"] = favObj["title"].toString();
                    favorite["artist"] = favObj["artist"].toString();
                    favorite["duration"] = favObj["duration"].toString();
                    favorite["is_local"] = isLocal;
                    favorite["added_at"] = favObj["added_at"].toString();
                    favorite["cover_art_url"] = favObj["cover_art_url"].toString();
                    
                    favorites.append(favorite);
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
    if (userAccount.trimmed().isEmpty()) {
        qWarning() << "[HttpRequestV2] addPlayHistory skipped: userAccount is empty";
        emit signal_addHistoryResult(false);
        return;
    }

    // Keep request contract aligned with backend:
    // POST /user/history/add?user_account=xxx
    // body: music_path, music_title, artist, album, duration_sec, is_local
    QString url = "user/history/add?user_account=" + QString(QUrl::toPercentEncoding(userAccount));

    int durationSec = 0;
    const QString trimmedDuration = duration.trimmed();
    if (trimmedDuration.contains(":")) {
        const QStringList parts = trimmedDuration.split(":");
        if (parts.size() >= 2) {
            durationSec = parts[0].toInt() * 60 + parts[1].toInt();
        }
    } else if (trimmedDuration.contains(" ")) {
        durationSec = static_cast<int>(trimmedDuration.split(" ").first().toDouble());
    } else {
        durationSec = trimmedDuration.toInt();
    }

    QJsonObject json;
    json["music_path"] = path;
    json["music_title"] = title;
    json["artist"] = artist;
    json["album"] = album;
    json["duration_sec"] = durationSec;
    json["is_local"] = isLocal;

    // Compatibility fields for older backend implementations.
    json["path"] = path;
    json["title"] = title;
    json["duration"] = QString::number(durationSec);

    auto options = Network::RequestOptions::withPriority(Network::RequestPriority::Low);

    m_networkService.postJson(url, json, options,
        [this, title](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                bool success = result.value("success").toBool();
                emit signal_addHistoryResult(success);
            } else {
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
                // 服务端直接返回JSON数组
                QJsonArray historyArray = Network::NetworkService::parseJsonArray(response);
                
                QVariantList history;
                for (const QJsonValue& value : historyArray) {
                    QJsonObject histObj = value.toObject();
                    QVariantMap historyItem;
                    
                    QString path = histObj["path"].toString();
                    bool isLocal = histObj["is_local"].toBool();
                    
                    // 对于在线音乐，如果path是相对路径，需要补全为完整URL
                    if (!isLocal && !path.startsWith("http")) {
                        path = m_baseUrl + "uploads/" + path;
                        qDebug() << "[HttpRequestV2] Online music path completed:" << path;
                    }
                    
                    historyItem["path"] = path;
                    historyItem["title"] = histObj["title"].toString();
                    historyItem["artist"] = readArtistFromObject(histObj);
                    historyItem["album"] = histObj["album"].toString();
                    historyItem["duration"] = histObj["duration"].toString();
                    historyItem["is_local"] = isLocal;
                    historyItem["play_time"] = histObj["play_time"].toString();
                    historyItem["cover_art_url"] = histObj["cover_art_url"].toString();
                    
                    history.append(historyItem);
                }
                
                qDebug() << "[HttpRequestV2] History received:" << history.size()
                         << (response.isFromCache ? "(from cache)" : "");
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

void HttpRequestV2::getMusicData(const QString& fileName)
{
    QString url = m_baseUrl + "stream";
    
    QJsonObject jsonObj;
    jsonObj["filename"] = fileName;
    QJsonDocument jsonDoc(jsonObj);
    QByteArray postData = jsonDoc.toJson();
    
    using namespace Network;
    RequestOptions options = RequestOptions::withPriority(RequestPriority::High);
    
    m_networkService.post(url, postData, options, 
        [this](const NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonDocument jsonDoc = QJsonDocument::fromJson(response.body);
                if (jsonDoc.isObject()) {
                    QJsonObject obj = jsonDoc.object();
                    if (obj.contains("stream_url")) {
                        QString streamUrl = obj["stream_url"].toString();
                        qDebug() << "[HttpRequestV2] Music stream URL:" << streamUrl;
                        emit signal_streamurl(true, streamUrl);
                    } else {
                        qWarning() << "[HttpRequestV2] stream_url not found in response";
                        emit signal_streamurl(false, "");
                    }
                } else {
                    qWarning() << "[HttpRequestV2] Invalid JSON in get music data response";
                    emit signal_streamurl(false, "");
                }
            } else {
                qWarning() << "[HttpRequestV2] Get music data error:" << response.errorString;
                emit signal_streamurl(false, "");
            }
        });
}

void HttpRequestV2::addMusic(const QString& musicPath)
{
    QString url = m_baseUrl + "users/add_music";
    
    auto user = User::getInstance();
    QJsonObject jsonObj;
    jsonObj["username"] = user->get_username();
    jsonObj["music_path"] = musicPath;
    QJsonDocument jsonDoc(jsonObj);
    QByteArray postData = jsonDoc.toJson();
    
    qDebug() << "[HttpRequestV2] Adding music:" << musicPath;
    
    m_networkService.post(url, postData, Network::RequestOptions(), 
        [musicPath](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                qDebug() << "[HttpRequestV2] Add music success:" << musicPath;
            } else {
                qWarning() << "[HttpRequestV2] Add music error:" << response.errorString;
            }
        });
}

void HttpRequestV2::getMusic(const QString& keyword)
{
    QJsonObject json;
    json["keyword"] = keyword;
    
    qDebug() << "[HttpRequestV2] Searching music with keyword:" << keyword;
    
    m_networkService.postJson("music/search", json, Network::RequestOptions::withCache(),
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonArray filesArray = Network::NetworkService::parseJsonArray(response);
                
                qDebug() << "[HttpRequestV2] Search found" << filesArray.size() << "results";
                QList<Music> musicList;
                
                for (const QJsonValue& value : filesArray) {
                    if (value.isObject()) {
                        QJsonObject fileObj = value.toObject();
                        QString filePath = fileObj["path"].toString();
                        QString durationStr = fileObj["duration"].toString();
                        QString coverUrl = fileObj["cover_art_url"].toString();
                        QString artist = fileObj.contains("artist") ? fileObj["artist"].toString() : 
                                        (fileObj.contains("singer") ? fileObj["singer"].toString() : "未知艺术家");

                        // 解析时长字符串 "239.49 seconds" -> 秒数
                        double durationValue = 0.0;
                        if (durationStr != "Error") {
                            QStringList parts = durationStr.split(" ");
                            if (!parts.isEmpty()) {
                                durationValue = parts[0].toDouble();
                            }
                        }
                        
                        int totalSeconds = static_cast<int>(durationValue);
                        int minutes = totalSeconds / 60;
                        int seconds = totalSeconds % 60;
                        
                        // 使用Music的setter方法设置属性
                        Music music;
                        music.setSongPath(filePath);
                        music.setDuration(totalSeconds * 1000);  // 转换为毫秒
                        music.setPicPath(coverUrl);
                        music.setSinger(artist);

                        musicList.append(music);
                    }
                }
                
                emit signal_addSong_list(musicList);
            } else {
                qWarning() << "[HttpRequestV2] Search music error:" << response.errorString;
            }
        });
}

void HttpRequestV2::removeFavorite(const QString& userAccount, const QStringList& paths)
{
    if (paths.isEmpty()) {
        qWarning() << "[HttpRequestV2] removeFavorite called with empty paths";
        emit signal_removeFavoriteResult(false);
        return;
    }
    
    QJsonObject json;
    json["user_account"] = userAccount;
    json["music_path"] = paths[0];  // 服务端接口只支持单个删除
    
    qDebug() << "[HttpRequestV2] Removing favorite for user:" << userAccount << "path:" << paths[0];
    
    // 使用POST请求删除收藏（服务端通过路由区分操作）
    m_networkService.postJson("user/favorites/remove", json, Network::RequestOptions(),
        [this](const Network::NetworkResponse& response) {
            bool success = false;
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                success = result["success"].toBool();
                qDebug() << "[HttpRequestV2] Remove favorite success:" << success;
            } else {
                qWarning() << "[HttpRequestV2] Remove favorite error:" << response.errorString;
            }
            emit signal_removeFavoriteResult(success);
        });
}

void HttpRequestV2::removePlayHistory(const QString& userAccount, const QStringList& paths)
{
    if (paths.isEmpty()) {
        qWarning() << "[HttpRequestV2] removePlayHistory called with empty paths";
        emit signal_removeHistoryResult(false);
        return;
    }
    
    QJsonObject json;
    QJsonArray pathArray;
    for (const QString& path : paths) {
        pathArray.append(path);
    }
    json["music_paths"] = pathArray;
    
    QString url = "user/history/delete?user_account=" + QString(QUrl::toPercentEncoding(userAccount));
    
    qDebug() << "[HttpRequestV2] Deleting play history for user:" << userAccount << "paths count:" << paths.size();
    
    m_networkService.postJson(url, json, Network::RequestOptions(),
        [this](const Network::NetworkResponse& response) {
            bool success = false;
            int deletedCount = 0;
            
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                success = result["success"].toBool();
                deletedCount = result["deleted_count"].toInt();
                qDebug() << "[HttpRequestV2] Delete history success:" << success << "deleted count:" << deletedCount;
            } else {
                qWarning() << "[HttpRequestV2] Delete history error:" << response.errorString;
            }
            
            emit signal_removeHistoryResult(success);
        });
}

