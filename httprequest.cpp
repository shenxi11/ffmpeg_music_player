#include "httprequest.h"
#include "music.h"
User* User::instance = nullptr;
QMutex User::mutex;
HttpRequest::HttpRequest(QObject *parent)
{

}
bool HttpRequest::AddMusic(const QString music_path)
{   if(!manager)
        manager = new QNetworkAccessManager();
    QUrl url = localUrl + "users/add_music";
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    auto user = User::getInstance();
    QJsonObject json;
    json["username"] = user->get_username();
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
bool HttpRequest::Login(const QString& account, const QString& password)
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
                user->set_account(account);
                user->set_password(password);
                
                // 设置用户名（从 username 字段获取）
                if (jsonObject.contains("username")) {
                    QString username = jsonObject.value("username").toString();
                    user->set_username(username);
                    emit signal_getusername(username);
                    qDebug() << "Login successful, username:" << username;
                }
                
                // 设置用户收藏的歌曲列表
                if (jsonObject.contains("song_path_list")) {
                    QJsonArray songPathArray = jsonObject.value("song_path_list").toArray();
                    QStringList musics;
                    for (const auto& songPath : songPathArray) {
                        musics << songPath.toString();
                    }
                    user->set_music_path(musics);
                    emit signal_add_songs();
                    qDebug() << "User's favorite songs:" << musics;
                }
                
                emit signal_Loginflag(true);
            } else {
                qDebug() << "Login failed: invalid credentials";
                emit signal_Loginflag(false);
            }

        } else {
            qDebug() << "Login request error:" << reply->errorString();
            emit signal_Loginflag(false);
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
            emit signal_Registerflag(true);
        } else {
            qDebug() << "Error:" << reply->errorString();
            emit signal_Registerflag(false);
        }
        reply->deleteLater();
    });
    return true;
}

bool HttpRequest::Upload(const QString &path)
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
bool HttpRequest::get_file(const QString url)
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
                emit signal_lrc(lines);
            }

        } else {
            qWarning() << "Request failed:" << reply->errorString();
        }

        reply->deleteLater();
    });

    return true;
}


bool HttpRequest::Download(const QString& filename, const QString download_folder)
{
    if(!manager)
            manager = new QNetworkAccessManager();
    QUrl url(localUrl + "download");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject jsonObject;
    jsonObject["filename"] = filename;
    QJsonDocument jsonDoc(jsonObject);
    QByteArray requestData = jsonDoc.toJson();

    QNetworkReply* reply = manager->post(request, requestData);

    QObject::connect(reply, &QNetworkReply::finished, [reply, filename, download_folder]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();

            qDebug()<<__FUNCTION__<<"download"<<responseData.size();

            QString downloadFolder = download_folder;
            QDir dir(downloadFolder);
            if (!dir.exists()) {
                dir.mkpath(".");
            }

            QFile file(downloadFolder + "/" + filename);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(responseData);
                file.close();
                qDebug() << "File downloaded successfully:" << filename;
            } else {
                qWarning() << "Failed to open file for writing:" << filename;
            }
        } else {
            qWarning() << "Download failed:" << reply->errorString();
        }

        reply->deleteLater();
    });
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
                emit signal_addSong_list(musicList);
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


void HttpRequest::get_music_data(const QString &fileName) {
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

                emit signal_streamurl(true, streamUrl);
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
    QUrl url(localUrl + "file");
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject requestBody;
    requestBody["name"] = name;

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
                emit signal_addSong_list(musicList);
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
                emit signal_videoList(videoList);
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
                emit signal_videoStreamUrl(videoUrl);
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
