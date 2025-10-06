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

            emit signal_Loginflag(true);
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response_data);

            QJsonObject jsonObject = jsonDoc.object();
            auto user = User::getInstance();
            if (jsonObject.contains("success")) {
                QJsonValue successValue = jsonObject.value("success");
                qDebug() << "Response:" << response_data;

                user->set_account(account);
                user->set_password(password);
                user->set_username(successValue.toString());

                emit signal_getusername(successValue.toString());
            }
            if(jsonObject.contains("song_path_list"))
            {
                QJsonArray successValue = jsonObject.value("song_path_list").toArray();
                QStringList musics;
                for(auto i: successValue)
                {
                    musics<<i.toString();
                }
                user->set_music_path(musics);
                emit signal_add_songs();
                qDebug()<<__FUNCTION__<<successValue;
            }

        } else {
            qDebug() << "Error:" << reply->errorString();
            emit signal_Loginflag(false);
        }
        reply->deleteLater();
    });
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
bool HttpRequest::get_file(const QString url)
{
    if(!manager)
            manager = new QNetworkAccessManager();
    QNetworkRequest request(url + "/lrc");
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
            if (jsonDoc.isObject()) {
                QJsonObject filesObject = jsonDoc.object();

                qDebug() << "Files and durations received from server:";
                QStringList songList;
                QList<double> durations;
                for (auto it = filesObject.begin(); it != filesObject.end(); ++it) {
                    QString fileName = it.key();
                    QString duration = it.value().toString();

                    qDebug() << "File Name:" << fileName << ", Duration:" << duration;

                    songList<<fileName;
                    durations<<it.value().toDouble();
                }
                emit signal_addSong_list(songList, durations);
            } else {
                qWarning() << "Failed to parse JSON object.";
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
            if (jsonDoc.isObject()) {
                QJsonObject filesObject = jsonDoc.object();

                qDebug() << "Music files and durations received from server:";
                QStringList songList;
                QList<double> durations;
                for (auto it = filesObject.begin(); it != filesObject.end(); ++it) {
                    QString fileName = it.key();
                    QString duration = it.value().toString();

                    qDebug() << "File Name:" << fileName << ", Duration:" << duration;

                    // Emit signal with file name and duration
                    songList << fileName;
                    durations << it.value().toDouble();
                }
                emit signal_addSong_list(songList, durations);
            } else {
                qWarning() << "Failed to parse JSON object.";
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

