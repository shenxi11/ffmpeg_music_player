#include "httprequest.h"
#include "music.h"

User* User::instance = nullptr;
QMutex User::mutex;

// User类构造函数实现
User::User(QString account, QString password, QString username)
    : account(account), password(password), username(username)
{
}

// User类setter实现
void User::setUsername(QString username)
{
    this->username = username;
}

void User::setAccount(QString account)
{
    this->account = account;
}

void User::setPassword(QString password)
{
    this->password = password;
}

void User::setMusicPath(QStringList musics)
{
    this->music_path = musics;
    emit signalAddSongs();
}

// User类getter实现
QString User::getUsername()
{
    return username;
}

QString User::getAccount()
{
    return account;
}

QString User::getPassword()
{
    return password;
}

QStringList User::getMusicPath()
{
    return this->music_path;
}

HttpRequest::HttpRequest(QObject *parent)
{

}
bool HttpRequest::addMusic(const QString music_path)
{   if(!manager)
        manager = new QNetworkAccessManager();
    QUrl url = localUrl + "users/add_music";
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    auto user = User::getInstance();
    QJsonObject json;
    json["username"] = user->getUsername();
    json["music_path"] = music_path;
    QJsonDocument jsonDoc(json);
    QByteArray jsonData = jsonDoc.toJson();

    QNetworkReply *reply = manager->post(request, jsonData);
    connect(reply, &QNetworkReply::finished, [this, reply](){
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response_data = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response_data);
            QJsonObject jsonObject = jsonDoc.object();
            if (jsonObject.contains("success")){
                qDebug()<<"success"<<response_data;
            }
        }
        else
        {
            qDebug() << "Error:" << reply->errorString();
        }
        reply->deleteLater();
    });
    return true;
}
bool HttpRequest::login(const QString& account, const QString& password)
{
    if(!manager)
            manager = new QNetworkAccessManager();
    QUrl url = localUrl + "users/login";

    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["account"] = account;
    json["password"] = password;
    QJsonDocument jsonDoc(json);
    QByteArray jsonData = jsonDoc.toJson();

    QNetworkReply *reply = manager->post(request, jsonData);


    connect(reply, &QNetworkReply::finished, [this, reply, account, password]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response_data = reply->readAll();
            
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response_data);
            QJsonObject jsonObject = jsonDoc.object();
            
            qDebug() << "Login Response:" << response_data;
            
            // 检查登录是否成功（优先使用 success_bool，兼容 success 字符串）
            bool loginSuccess = false;
            if (jsonObject.contains("success_bool")) {
                loginSuccess = jsonObject.value("success_bool").toBool();
            } else if (jsonObject.contains("success")) {
                QString successStr = jsonObject.value("success").toString();
                loginSuccess = (successStr == "true");
            }
            
            if (loginSuccess) {
                auto user = User::getInstance();
                
                // 设置账号和密码
                user->setAccount(account);
                user->setPassword(password);
                
                // 设置用户名（从 username 字段获取）
                if (jsonObject.contains("username")) {
                    QString username = jsonObject.value("username").toString();
                    user->setUsername(username);
                    emit signalGetusername(username);
                    qDebug() << "Login successful, username:" << username;
                }
                
                // 设置用户收藏的歌曲列表
                if (jsonObject.contains("song_path_list")) {
                    QJsonArray songPathArray = jsonObject.value("song_path_list").toArray();
                    QStringList musics;
                    for (const auto& songPath : songPathArray) {
                        musics << songPath.toString();
                    }
                    user->setMusicPath(musics);
                    emit signalAddSongs();
                    qDebug() << "User's favorite songs:" << musics;
                }
                
                emit signalLoginFlag(true);
            } else {
                qDebug() << "Login failed: invalid credentials";
                emit signalLoginFlag(false);
            }

        } else {
            qDebug() << "Login request error:" << reply->errorString();
            emit signalLoginFlag(false);
        }
        reply->deleteLater();
    });
    return true;
}
bool HttpRequest:: Register(const QString& account, const QString& password, const QString& username)
{
    if(!manager)
            manager = new QNetworkAccessManager();
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
            emit signalRegisterFlag(true);
        } else {
            qDebug() << "Error:" << reply->errorString();
            emit signalRegisterFlag(false);
        }
        reply->deleteLater();
    });
    return true;
}

bool HttpRequest::upload(const QString &path)
{
    if(!manager)
            manager = new QNetworkAccessManager();
    QFile *file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) {
        qWarning("Failed to open file");
        return false;
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"file\"; filename=\"" + QFileInfo(path).fileName() + "\""));

    filePart.setBodyDevice(file);
    file->setParent(multiPart);

    multiPart->append(filePart);

    QNetworkRequest request(QUrl(localUrl + "upload"));
    QNetworkReply *reply = manager->post(request, multiPart);

    QObject::connect(reply, &QNetworkReply::finished, [reply, multiPart, file]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "File uploaded successfully";
        } else {
            qWarning() << "Upload failed:" << reply->errorString();
        }
        multiPart->deleteLater();
        reply->deleteLater();
    });
    QObject::connect(reply, &QNetworkReply::uploadProgress, [](qint64 bytesSent, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            double progress = (static_cast<double>(bytesSent) / bytesTotal) * 100;
            qDebug() << "Upload progress:" << progress << "%";
        }
    });

    return true;
}
QString extractWithQUrl(const QString& fullUrl) {
    QUrl url(fullUrl);
    QString path = url.path();                          // 取路径部分（如"/uploads/东风破/东风破.lrc"）
    int lastSlashPos = path.lastIndexOf('/');
    QString dirPath = (lastSlashPos == -1) ? path : path.left(lastSlashPos); // 取目录路径

    // 重组完整URL（含协议、主机、端口）
    return QString("%1://%2%3%4")
        .arg(url.scheme())          // 协议（http）
        .arg(url.host())            // 主机（192.168.1.208）
        .arg(url.port() > 0 ? ":" + QString::number(url.port()) : "") // 端口（:8080）
        .arg(dirPath);              // 目录路径（/uploads/东风破）
}
bool HttpRequest::getFile(const QString url)
{
    if(!manager)
            manager = new QNetworkAccessManager();
    qDebug()<<__FUNCTION__<<extractWithQUrl(url);
    QNetworkRequest request(extractWithQUrl(url) + "/lrc");
    QNetworkReply* reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();

            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if(jsonDoc.isArray())
            {
                QStringList lines;
                auto filesObject = jsonDoc.array();
                for (auto it = filesObject.begin(); it != filesObject.end(); ++it)
                {
                    lines<<it->toString();
                }
                emit signalLrc(lines);
            }

        } else {
            qWarning() << "Request failed:" << reply->errorString();
        }

        reply->deleteLater();
    });

    return true;
}


bool HttpRequest::download(const QString& filename, const QString download_folder, bool downloadLyrics, const QString& coverUrl)
{
    // 构建下载URL
    QString downloadUrl = localUrl + "download";
    
    // 准备POST数据
    QJsonObject jsonObject;
    jsonObject["filename"] = filename;
    QJsonDocument jsonDoc(jsonObject);
    QByteArray postData = jsonDoc.toJson();
    
    qDebug() << "[HttpRequest] Adding download to queue:" << filename;
    qDebug() << "  Download lyrics:" << downloadLyrics;
    qDebug() << "  Cover URL:" << coverUrl;
    
    // 使用下载管理器进行异步下载，传递coverUrl
    DownloadManager::instance().addDownload(downloadUrl, filename, download_folder, postData, coverUrl);
    
    // 如果需要下载歌词
    if (downloadLyrics) {
        // 从filename提取歌曲名（去掉扩展名）
        QString lrcFilename = filename;
        int lastDot = lrcFilename.lastIndexOf('.');
        if (lastDot != -1) {
            lrcFilename = lrcFilename.left(lastDot) + ".lrc";
        }
        
        QString lrcUrl = localUrl + "download";
        QJsonObject lrcJsonObject;
        lrcJsonObject["filename"] = lrcFilename;
        QJsonDocument lrcJsonDoc(lrcJsonObject);
        QByteArray lrcPostData = lrcJsonDoc.toJson();
        
        qDebug() << "[HttpRequest] Also adding lyric file to queue:" << lrcFilename;
        DownloadManager::instance().addDownload(lrcUrl, lrcFilename, download_folder, lrcPostData);
    }
    
    return true;
}

bool HttpRequest::getAllFiles() {
    if(!manager)
            manager = new QNetworkAccessManager();
    QUrl url = localUrl + "files";
    QNetworkRequest request(url);

    QNetworkReply* reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();

            qDebug() << "Response data from server:" << responseData;

            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (jsonDoc.isArray()) {
                QJsonArray filesArray = jsonDoc.array();

                qDebug() << "Files and durations received from server:";
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
                        
                        Music music;
                        music.setSongPath(filePath);
                        music.setSinger(artist);
                        music.setDuration(static_cast<long>(durationValue));
                        music.setPicPath(coverUrl);
                        
                        musicList.append(music);
                        
                        qDebug() << "File Path:" << filePath << ", Duration:" << durationValue << ", Cover:" << coverUrl << ", Artist:" << artist;
                    }
                }
                emit signalAddSongList(musicList);
            } else {
                qWarning() << "Failed to parse JSON array.";
            }
        } else {
            qWarning() << "Request failed:" << reply->errorString();
        }

        reply->deleteLater();
    });

    return true;
}


void HttpRequest::getMusicData(const QString &fileName) {
    if(!manager)
            manager = new QNetworkAccessManager();
    QJsonObject json;
    json["filename"] = fileName;
    QJsonDocument doc(json);
    QByteArray requestData = doc.toJson();

    QUrl url(localUrl + "stream");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "keep-alive");

    QNetworkReply *reply = manager->post(request, requestData);

    connect(reply, &QNetworkReply::readyRead, [this, reply]() {

        QByteArray responseData = reply->readAll();

        QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);
        if (jsonResponse.isObject()) {
            QJsonObject responseObject = jsonResponse.object();
            if (responseObject.contains("stream_url")) {
                QString streamUrl = responseObject["stream_url"].toString();
                QString musicId = responseObject["ID"].toString();
                std::shared_ptr<Music> music = std::make_shared<Music>();
                music->setSongPath(streamUrl);
                music->setMusicID(musicId);
                music->setDuration(responseObject["duration"].toInt());
                music->setPicPath(responseObject["album_cover_url"].toString());
                
                // 提取歌手信息
                if (responseObject.contains("artist")) {
                    music->setSinger(responseObject["artist"].toString());
                } else if (responseObject.contains("singer")) {
                    music->setSinger(responseObject["singer"].toString());
                }

                emit signalStreamurl(true, streamUrl);
            } else {
                qDebug() << "Error: 'stream_url' not found in response.";
            }
        } else {
            qDebug() << "Error: Invalid JSON response.";
        }
    });

    connect(reply, &QNetworkReply::finished, [reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Request finished successfully.";
        } else {
            qDebug() << "Request failed: " << reply->errorString();
        }
        reply->deleteLater();
    });
}



void HttpRequest::sendAcknowledgment() {
    if(!manager)
            manager = new QNetworkAccessManager();
    QJsonObject ackJson;
    ackJson["ack"] = "ACK";
    QJsonDocument ackDoc(ackJson);
    QByteArray ackData = ackDoc.toJson();

    QUrl ackUrl(localUrl + "ack");
    QNetworkRequest ackRequest(ackUrl);
    ackRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    manager->post(ackRequest, ackData);
    qDebug()<<"ack";
}

bool HttpRequest::getMusic(const QString& name) {
    if(!manager)
            manager = new QNetworkAccessManager();
    QUrl url(localUrl + "music/search");
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject requestBody;
    requestBody["keyword"] = name;  // 使用keyword参数

    qDebug() << "[HttpRequest] Searching music with keyword:" << name;
    QNetworkReply* reply = manager->post(request, QJsonDocument(requestBody).toJson());

    QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {

            QByteArray responseData = reply->readAll();

            qDebug() << "Response data from server:" << responseData;

            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (jsonDoc.isArray()) {
                QJsonArray filesArray = jsonDoc.array();

                qDebug() << "Music files and durations received from server:";
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
                        
                        Music music;
                        music.setSongPath(filePath);
                        music.setSinger(artist);
                        music.setDuration(static_cast<long>(durationValue));
                        music.setPicPath(coverUrl);
                        
                        musicList.append(music);
                        
                        qDebug() << "File Path:" << filePath << ", Duration:" << durationValue << ", Cover:" << coverUrl << ", Artist:" << artist;
                    }
                }
                emit signalAddSongList(musicList);
            } else {
                qWarning() << "Failed to parse JSON array.";
            }
        } else {
            qWarning() << "Request failed:" << reply->errorString();
        }

        reply->deleteLater();
    });

    return true;
}
void HttpRequest::setIsUsing(bool flag_){
    isUsing = flag_;
}
bool HttpRequest::getIsUsing(){
    return isUsing;
}

// 获取视频列表 GET /videos
bool HttpRequest::getVideoList() {
    if (!manager) {
        manager = new QNetworkAccessManager(this);
    }
    
    QString url = localUrl + "videos";
    qDebug() << "[HttpRequest] Fetching video list from:" << url;
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qDebug() << "[HttpRequest] Video list response:" << responseData;
            
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (jsonDoc.isArray()) {
                QJsonArray jsonArray = jsonDoc.array();
                QVariantList videoList;
                
                for (const QJsonValue& value : jsonArray) {
                    QJsonObject obj = value.toObject();
                    QVariantMap videoInfo;
                    videoInfo["name"] = obj["name"].toString();
                    videoInfo["path"] = obj["path"].toString();
                    videoInfo["size"] = obj["size"].toVariant().toLongLong();
                    videoList.append(videoInfo);
                }
                
                qDebug() << "[HttpRequest] Parsed" << videoList.size() << "videos";
                emit signalVideoList(videoList);
            } else {
                qWarning() << "[HttpRequest] Failed to parse video list JSON";
            }
        } else {
            qWarning() << "[HttpRequest] Video list request failed:" << reply->errorString();
        }
        
        reply->deleteLater();
    });
    
    return true;
}

// 获取视频流URL POST /video/stream
bool HttpRequest::getVideoStreamUrl(const QString& videoPath) {
    if (!manager) {
        manager = new QNetworkAccessManager(this);
    }
    
    QString url = localUrl + "video/stream";
    qDebug() << "[HttpRequest] Fetching video stream URL for:" << videoPath;
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject jsonObj;
    jsonObj["path"] = videoPath;
    QJsonDocument jsonDoc(jsonObj);
    QByteArray postData = jsonDoc.toJson();
    
    QNetworkReply* reply = manager->post(request, postData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qDebug() << "[HttpRequest] Video stream response:" << responseData;
            
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                QString videoUrl = obj["url"].toString();
                qDebug() << "[HttpRequest] Video stream URL:" << videoUrl;
                emit signalVideoStreamUrl(videoUrl);
            } else {
                qWarning() << "[HttpRequest] Failed to parse video stream URL JSON";
            }
        } else {
            qWarning() << "[HttpRequest] Video stream URL request failed:" << reply->errorString();
        }
        
        reply->deleteLater();
    });
    
    return true;
}

// 搜索歌手是否存在 POST /artist/search
void HttpRequest::searchArtist(const QString& artist) {
    if (!manager) {
        manager = new QNetworkAccessManager(this);
    }
    
    QString url = localUrl + "artist/search";
    qDebug() << "[HttpRequest] Searching artist:" << artist;
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject jsonObj;
    jsonObj["artist"] = artist;
    QJsonDocument jsonDoc(jsonObj);
    QByteArray postData = jsonDoc.toJson();
    
    QNetworkReply* reply = manager->post(request, postData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qDebug() << "[HttpRequest] Artist search response:" << responseData;
            
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                bool exists = obj["exists"].toBool();
                qDebug() << "[HttpRequest] Artist exists:" << exists;
                emit signalArtistExists(exists, artist);
            } else {
                qWarning() << "[HttpRequest] Failed to parse artist search JSON";
                emit signalArtistExists(false, artist);
            }
        } else {
            qWarning() << "[HttpRequest] Artist search request failed:" << reply->errorString();
            emit signalArtistExists(false, artist);
        }
        
        reply->deleteLater();
    });
}

// 根据歌手查询音乐 POST /music/artist
void HttpRequest::getMusicByArtist(const QString& artist) {
    if (!manager) {
        manager = new QNetworkAccessManager(this);
    }
    
    QString url = localUrl + "music/artist";
    qDebug() << "[HttpRequest] Fetching music by artist:" << artist;
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject jsonObj;
    jsonObj["artist"] = artist;
    QJsonDocument jsonDoc(jsonObj);
    QByteArray postData = jsonDoc.toJson();
    
    QNetworkReply* reply = manager->post(request, postData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qDebug() << "[HttpRequest] Artist music list response size:" << responseData.size();
            
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (jsonDoc.isArray()) {
                QJsonArray jsonArray = jsonDoc.array();
                QList<Music> musicList;
                
                qDebug() << "[HttpRequest] Parsing" << jsonArray.size() << "songs for artist:" << artist;
                
                for (const QJsonValue& value : jsonArray) {
                    QJsonObject obj = value.toObject();
                    
                    QString path = obj["path"].toString();
                    QString durationStr = obj["duration"].toString();
                    QString artistName = obj["artist"].toString();
                    QString coverUrl = obj["cover_art_url"].toString();
                    
                    qDebug() << "[HttpRequest] Parsing song - path:" << path 
                             << ", artist:" << artistName 
                             << ", duration:" << durationStr
                             << ", cover:" << coverUrl;
                    
                    // 解析时长
                    double duration = 0.0;
                    if (durationStr.contains("seconds")) {
                        durationStr.replace(" seconds", "");
                        duration = durationStr.toDouble();
                    }
                    
                    // 构建完整URL
                    QString fullPath = localUrl + "uploads/" + path;
                    
                    qDebug() << "[HttpRequest] Full path built:" << fullPath;
                    
                    Music music;
                    music.setSongPath(fullPath);
                    music.setDuration(duration);
                    music.setSinger(artistName);
                    music.setPicPath(coverUrl);
                    
                    qDebug() << "[HttpRequest] Music created - songName:" << music.getSongName() 
                             << ", singer:" << music.getSinger();
                    
                    musicList.append(music);
                }
                
                qDebug() << "[HttpRequest] Successfully parsed" << musicList.size() << "songs for artist:" << artist;
                emit signalArtistMusicList(musicList, artist);
            } else {
                qWarning() << "[HttpRequest] Failed to parse artist music list JSON";
                emit signalArtistMusicList(QList<Music>(), artist);
            }
        } else {
            qWarning() << "[HttpRequest] Artist music list request failed:" << reply->errorString();
            emit signalArtistMusicList(QList<Music>(), artist);
        }
        
        reply->deleteLater();
    });
}

// 添加喜欢音乐 POST /user/favorites/add
void HttpRequest::addFavorite(const QString& userAccount, const QString& path, const QString& title,
                              const QString& artist, const QString& duration, bool isLocal) {
    if (!manager) {
        manager = new QNetworkAccessManager(this);
    }
    
    QString url = localUrl + "user/favorites/add?user_account=" + QUrl::toPercentEncoding(userAccount);
    qDebug() << "[HttpRequest] Adding favorite:" << title << "for user:" << userAccount;
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // 将duration字符串转换为秒数
    int durationSec = 0;
    if (duration.contains(":")) {
        QStringList parts = duration.split(":");
        if (parts.size() >= 2) {
            durationSec = parts[0].toInt() * 60 + parts[1].toInt();
        }
    } else {
        durationSec = duration.toInt();
    }
    
    QJsonObject jsonObj;
    jsonObj["music_path"] = path;
    jsonObj["music_title"] = title;
    jsonObj["artist"] = artist;
    jsonObj["duration_sec"] = durationSec;
    jsonObj["is_local"] = isLocal;
    
    QJsonDocument jsonDoc(jsonObj);
    QByteArray postData = jsonDoc.toJson();
    
    QNetworkReply* reply = manager->post(request, postData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        bool success = false;
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qDebug() << "[HttpRequest] Add favorite response:" << responseData;
            
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                success = obj["success"].toBool();
                qDebug() << "[HttpRequest] Add favorite success:" << success;
            }
        } else {
            qWarning() << "[HttpRequest] Add favorite request failed:" << reply->errorString();
        }
        
        emit signalAddFavoriteResult(success);
        reply->deleteLater();
    });
}

// 移除喜欢音乐 DELETE /user/favorites/remove
void HttpRequest::removeFavorite(const QString& userAccount, const QStringList& paths) {
    if (!manager) {
        manager = new QNetworkAccessManager(this);
    }
    
    QString url = localUrl + "user/favorites/remove";
    qDebug() << "[HttpRequest] Removing favorites for user:" << userAccount << "paths count:" << paths.size();
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject jsonObj;
    jsonObj["user_account"] = userAccount;
    jsonObj["music_path"] = paths.size() > 0 ? paths[0] : "";  // 服务端接口只支持单个删除
    
    
    QJsonDocument jsonDoc(jsonObj);
    QByteArray postData = jsonDoc.toJson();
    
    qDebug() << "[HttpRequest] Remove favorite request body:" << postData;
    
    QNetworkReply* reply = manager->sendCustomRequest(request, "DELETE", postData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        bool success = false;
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qDebug() << "[HttpRequest] Remove favorite response:" << responseData;
            
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                success = obj["success"].toBool();
                qDebug() << "[HttpRequest] Remove favorite success:" << success;
            }
        } else {
            qWarning() << "[HttpRequest] Remove favorite request failed:" << reply->errorString();
        }
        
        emit signalRemoveFavoriteResult(success);
        reply->deleteLater();
    });
}

// 获取喜欢音乐列表 GET /user/favorites
void HttpRequest::getFavorites(const QString& userAccount) {
    if (!manager) {
        manager = new QNetworkAccessManager(this);
    }
    
    QString url = localUrl + "user/favorites?user_account=" + QUrl::toPercentEncoding(userAccount);
    qDebug() << "[HttpRequest] Fetching favorites for user:" << userAccount;
    
    QNetworkRequest request(url);
    QNetworkReply* reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        QVariantList favoritesList;
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qDebug() << "[HttpRequest] Favorites response:" << responseData;
            
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            
            // 服务端直接返回数组，不是对象
            if (jsonDoc.isArray()) {
                QJsonArray favoritesArray = jsonDoc.array();
                
                for (const QJsonValue& value : favoritesArray) {
                    QJsonObject favObj = value.toObject();
                    QVariantMap favorite;
                    
                    QString path = favObj["path"].toString();
                    bool isLocal = favObj["is_local"].toBool();
                    
                    // 对于在线音乐，如果path是相对路径，需要补全为完整URL
                    if (!isLocal && !path.startsWith("http")) {
                        path = localUrl + "uploads/" + path;
                        qDebug() << "[HttpRequest] Online music path completed:" << path;
                    }
                    
                    favorite["path"] = path;
                    favorite["title"] = favObj["title"].toString();
                    favorite["artist"] = favObj["artist"].toString();
                    favorite["duration"] = favObj["duration"].toString();
                    favorite["is_local"] = isLocal;
                    favorite["added_at"] = favObj["added_at"].toString();
                    favorite["cover_art_url"] = favObj["cover_art_url"].toString();
                    
                    favoritesList.append(favorite);
                }
                
                qDebug() << "[HttpRequest] Successfully parsed" << favoritesList.size() << "favorites";
            } else {
                qWarning() << "[HttpRequest] Favorites response is not a JSON array!";
            }
        } else {
            qWarning() << "[HttpRequest] Favorites request failed:" << reply->errorString();
        }
        
        emit signalFavoritesList(favoritesList);
        reply->deleteLater();
    });
}

// 添加播放历史 POST /user/history/add
void HttpRequest::addPlayHistory(const QString& userAccount, const QString& path, const QString& title,
                                 const QString& artist, const QString& album, const QString& duration, bool isLocal) {
    if (!manager) {
        manager = new QNetworkAccessManager(this);
    }
    
    QString url = localUrl + "user/history/add?user_account=" + QUrl::toPercentEncoding(userAccount);
    qDebug() << "[HttpRequest] Adding play history:" << title << "for user:" << userAccount;
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // 将duration字符串转换为秒数
    int durationSec = 0;
    if (duration.contains(":")) {
        QStringList parts = duration.split(":");
        if (parts.size() >= 2) {
            durationSec = parts[0].toInt() * 60 + parts[1].toInt();
        }
    } else {
        durationSec = duration.toInt();
    }
    
    QJsonObject jsonObj;
    jsonObj["music_path"] = path;
    jsonObj["music_title"] = title;
    jsonObj["artist"] = artist;
    jsonObj["album"] = album;
    jsonObj["duration_sec"] = durationSec;
    jsonObj["is_local"] = isLocal;
    
    QJsonDocument jsonDoc(jsonObj);
    QByteArray postData = jsonDoc.toJson();
    
    qDebug() << "[HttpRequest] ========== ADD HISTORY REQUEST ==========";
    qDebug() << "[HttpRequest] URL:" << url;
    qDebug() << "[HttpRequest] POST Data:" << QString::fromUtf8(postData);
    qDebug() << "[HttpRequest] music_path:" << path;
    qDebug() << "[HttpRequest] music_title:" << title;
    qDebug() << "[HttpRequest] artist:" << artist;
    qDebug() << "[HttpRequest] is_local:" << isLocal;
    qDebug() << "[HttpRequest] =============================================="; 
    
    QNetworkReply* reply = manager->post(request, postData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        bool success = false;
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qDebug() << "[HttpRequest] Add history response:" << responseData;
            
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                success = obj["success"].toBool();
                qDebug() << "[HttpRequest] Add history success:" << success;
            }
        } else {
            qWarning() << "[HttpRequest] Add history request failed:" << reply->errorString();
        }
        
        emit signalAddHistoryResult(success);
        reply->deleteLater();
    });
}

// 获取播放历史 GET /user/history
void HttpRequest::getPlayHistory(const QString& userAccount, int limit) {
    if (!manager) {
        manager = new QNetworkAccessManager(this);
    }
    
    QString url = localUrl + "user/history?user_account=" + QUrl::toPercentEncoding(userAccount) 
                  + "&limit=" + QString::number(limit);
    qDebug() << "[HttpRequest] Fetching play history for user:" << userAccount << "limit:" << limit;
    
    QNetworkRequest request(url);
    QNetworkReply* reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        QVariantList historyList;
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qDebug() << "[HttpRequest] History response:" << responseData;
            
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            
            // 服务端直接返回数组，不是对象
            if (jsonDoc.isArray()) {
                QJsonArray historyArray = jsonDoc.array();
                
                for (const QJsonValue& value : historyArray) {
                    QJsonObject histObj = value.toObject();
                    QVariantMap history;
                    
                    QString path = histObj["path"].toString();
                    bool isLocal = histObj["is_local"].toBool();
                    
                    // 对于在线音乐，如果path是相对路径，需要补全为完整URL
                    if (!isLocal && !path.startsWith("http")) {
                        path = localUrl + "uploads/" + path;
                        qDebug() << "[HttpRequest] Online music path completed:" << path;
                    }
                    
                    history["path"] = path;
                    history["title"] = histObj["title"].toString();
                    history["artist"] = histObj["artist"].toString();
                    history["album"] = histObj["album"].toString();
                    history["duration"] = histObj["duration"].toString();
                    history["is_local"] = isLocal;
                    history["play_time"] = histObj["play_time"].toString();
                    history["cover_art_url"] = histObj["cover_art_url"].toString();
                    
                    historyList.append(history);
                }
                
                qDebug() << "[HttpRequest] Successfully parsed" << historyList.size() << "history items";
            } else {
                qWarning() << "[HttpRequest] History response is not a JSON array!";
            }
        } else {
            qWarning() << "[HttpRequest] History request failed:" << reply->errorString();
        }
        
        emit signalHistoryList(historyList);
        reply->deleteLater();
    });
}

// 删除播放历史 POST /user/history/delete
void HttpRequest::removePlayHistory(const QString& userAccount, const QStringList& paths) {
    if (!manager) {
        manager = new QNetworkAccessManager(this);
    }
    
    QString url = localUrl + "user/history/delete?user_account=" + QUrl::toPercentEncoding(userAccount);
    qDebug() << "[HttpRequest] Deleting play history for user:" << userAccount << "paths count:" << paths.size();
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // 构建请求体，使用 music_paths 数组
    QJsonObject jsonObj;
    QJsonArray pathArray;
    for (const QString& path : paths) {
        pathArray.append(path);
    }
    jsonObj["music_paths"] = pathArray;
    
    QJsonDocument jsonDoc(jsonObj);
    QByteArray postData = jsonDoc.toJson();
    
    qDebug() << "[HttpRequest] Delete history request body:" << postData;
    
    QNetworkReply* reply = manager->post(request, postData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        bool success = false;
        int deletedCount = 0;
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qDebug() << "[HttpRequest] Delete history response:" << responseData;
            
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                success = obj["success"].toBool();
                deletedCount = obj["deleted_count"].toInt();
                qDebug() << "[HttpRequest] Delete history success:" << success << "deleted count:" << deletedCount;
            }
        } else {
            qWarning() << "[HttpRequest] Delete history request failed:" << reply->errorString();
        }
        
        emit signalRemoveHistoryResult(success);
        reply->deleteLater();
    });
}

