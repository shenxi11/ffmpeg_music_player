#include "httprequest.h"

HttpRequest::HttpRequest(QObject *parent)
{
    manager = new QNetworkAccessManager(this);
}
bool HttpRequest::Login(const QString& account, const QString& password)
{
    QUrl url = localUrl + "users/login";

    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["account"] = account;
    json["password"] = password;
    QJsonDocument jsonDoc(json);
    QByteArray jsonData = jsonDoc.toJson();

    QNetworkReply *reply = manager->post(request, jsonData);


    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response_data = reply->readAll();
            qDebug() << "Response:" << response_data;
            emit Loginflag(true);
        } else {
            qDebug() << "Error:" << reply->errorString();
            emit Loginflag(false);
        }
        reply->deleteLater();
    });
}
bool HttpRequest:: Register(const QString& account, const QString& password, const QString& username)
{
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
            emit Registerflag(true);
        } else {
            qDebug() << "Error:" << reply->errorString();
            emit Registerflag(false);
        }
        reply->deleteLater();
    });
}

bool HttpRequest::Upload(const QString &path)
{

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

bool HttpRequest::Download(const QString& filename)
{
    // 创建请求并设置目标URL
    QUrl url(localUrl + "download");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 创建 JSON 请求体
    QJsonObject jsonObject;
    jsonObject["filename"] = filename;
    QJsonDocument jsonDoc(jsonObject);
    QByteArray requestData = jsonDoc.toJson();

    // 发送 POST 请求
    QNetworkReply* reply = manager->post(request, requestData);

    // 等待请求完成
    QObject::connect(reply, &QNetworkReply::finished, [reply, filename]() {
        if (reply->error() == QNetworkReply::NoError) {
            // 读取响应数据
            QByteArray responseData = reply->readAll();

            // 设定文件保存路径到 download 文件夹
            QString downloadFolder = "./download";  // 确保该文件夹存在
            QDir dir(downloadFolder);
            if (!dir.exists()) {
                dir.mkpath(".");  // 如果 download 文件夹不存在，创建它
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

        // 清理
        reply->deleteLater();
    });
}

bool HttpRequest::getAllFiles()
{
    QUrl url = localUrl + "files";
    QNetworkRequest request(url);

    // 发送 GET 请求并接收 QNetworkReply
    QNetworkReply* reply = manager->get(request);

    // 等待请求完成
    QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {
        // 检查请求是否成功
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();

            // 打印原始响应数据，确保它符合预期
            qDebug() << "Response data from server:" << responseData;

            // 解析 JSON 响应数据
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (jsonDoc.isArray()) {
                QJsonArray filesArray = jsonDoc.array();

                // 输出所有文件名
                qDebug() << "Files received from server:";
                for (const QJsonValue& value : filesArray) {
                    //qDebug() << value.toString();
                    emit addSong(value.toString(), value.toString());
                }


            } else {
                qWarning() << "Failed to parse JSON array.";
            }
        } else {
            qWarning() << "Request failed:" << reply->errorString();
        }

        // 清理回复对象
        reply->deleteLater();
    });


}

void HttpRequest::sendRequestAndForwardData() {
    const QString &fileName = "周杰伦 - 七里香_EM.flac";

    // 创建请求数据
    QJsonObject json;
    json["filename"] = fileName;
    QJsonDocument doc(json);
    QByteArray requestData = doc.toJson();

    // 创建请求
    QUrl url(localUrl + "stream");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "keep-alive");

    // 发送 POST 请求
    QNetworkReply *reply = manager->post(request, requestData);

    // 连接 readyRead 信号来持续接收数据
    connect(reply, &QNetworkReply::readyRead, [this, reply]() {
           // 逐块读取数据
           while (reply->bytesAvailable()) {
               QByteArray chunk = reply->read(4096);
               emit send_Packet(chunk);
           }
       });

    // 连接finished信号来处理响应的结束
    connect(reply, &QNetworkReply::finished, [reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Request finished successfully.";
        } else {
            qDebug() << "Request failed: " << reply->errorString();
        }
        reply->deleteLater();  // 清理回复对象
    });
}


void HttpRequest::sendAcknowledgment() {
    QJsonObject ackJson;
    ackJson["ack"] = "ACK";  // 表示确认接收当前数据块
    QJsonDocument ackDoc(ackJson);
    QByteArray ackData = ackDoc.toJson();

    QUrl ackUrl(localUrl + "ack");  // 假设服务端有一个处理 ACK 的路由
    QNetworkRequest ackRequest(ackUrl);
    ackRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 发送确认请求
    manager->post(ackRequest, ackData);
    qDebug()<<"ack";
}

bool HttpRequest::getMusic(const QString& name) {
    QUrl url (localUrl + "file"); // 拼接 URL
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

                qDebug() << "Music files received from server:";
                for (const QJsonValue& value : filesArray) {
                    qDebug() << value.toString();
                    emit addSong(value.toString(), value.toString());
                }
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


