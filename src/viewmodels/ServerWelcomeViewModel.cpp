#include "ServerWelcomeViewModel.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

#include "settings_manager.h"

namespace {

QString extractResponseMessage(const QJsonObject& root)
{
    const QString message = root.value("message").toString().trimmed();
    if (!message.isEmpty()) {
        return message;
    }
    const QJsonObject data = root.value("data").toObject();
    return data.value("message").toString().trimmed();
}

}

ServerWelcomeViewModel::ServerWelcomeViewModel(QObject* parent)
    : BaseViewModel(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

QString ServerWelcomeViewModel::serverHost() const
{
    return SettingsManager::instance().serverHost();
}

int ServerWelcomeViewModel::serverPort() const
{
    return SettingsManager::instance().serverPort();
}

bool ServerWelcomeViewModel::hasWindowPos() const
{
    return SettingsManager::instance().hasServerWelcomeWindowPos();
}

QPoint ServerWelcomeViewModel::windowPos() const
{
    return SettingsManager::instance().serverWelcomeWindowPos();
}

void ServerWelcomeViewModel::setWindowPos(const QPoint& point)
{
    SettingsManager::instance().setServerWelcomeWindowPos(point);
}

bool ServerWelcomeViewModel::verifyServer(const QString& rawHost,
                                          int currentPort,
                                          QString* normalizedHost,
                                          int* normalizedPort,
                                          QString* errorMessage)
{
    int port = currentPort;
    const QString host = normalizeHostInput(rawHost, &port);
    if (host.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("请输入有效的服务器 IP 或域名。");
        }
        return false;
    }

    if (normalizedHost) {
        *normalizedHost = host;
    }
    if (normalizedPort) {
        *normalizedPort = port;
    }

    const QString baseUrl = QStringLiteral("http://%1:%2/").arg(host, QString::number(port));

    QJsonObject pingRoot;
    if (!getJson(baseUrl + QStringLiteral("client/ping"), &pingRoot, errorMessage)) {
        return false;
    }
    const int pingCode = pingRoot.value("code").toInt(0);
    if (pingCode != 0) {
        if (errorMessage) {
            const QString message = extractResponseMessage(pingRoot);
            *errorMessage = message.isEmpty()
                    ? QStringLiteral("连通性检查失败，请确认服务端网关可访问。")
                    : message;
        }
        return false;
    }
    const QJsonObject pingData = pingRoot.value("data").toObject();
    const QString pingStatus = pingData.value("status").toString().trimmed().toLower();
    if (!pingStatus.isEmpty() && pingStatus != QStringLiteral("ok")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("服务端 /client/ping 返回状态异常。");
        }
        return false;
    }

    QJsonObject bootstrapRoot;
    if (!getJson(baseUrl + QStringLiteral("client/bootstrap"), &bootstrapRoot, errorMessage)) {
        return false;
    }
    const int bootstrapCode = bootstrapRoot.value("code").toInt(0);
    if (bootstrapCode != 0) {
        if (errorMessage) {
            const QString message = extractResponseMessage(bootstrapRoot);
            *errorMessage = message.isEmpty()
                    ? QStringLiteral("服务端引导检查失败，请稍后重试。")
                    : message;
        }
        return false;
    }

    const QJsonObject bootstrapData = bootstrapRoot.value("data").toObject();
    if (bootstrapData.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("服务端引导响应缺少 data 字段。");
        }
        return false;
    }

    const bool ready = bootstrapData.value("ready").toBool(false);
    if (!ready) {
        const QJsonObject checks = bootstrapData.value("checks").toObject();
        const bool dbReady = checks.value("database").toBool(false);
        const bool redisReady = checks.value("redis").toBool(false);
        if (errorMessage) {
            *errorMessage = QStringLiteral("服务端尚未就绪（database=%1, redis=%2），请稍后重试。")
                    .arg(dbReady ? QStringLiteral("true") : QStringLiteral("false"),
                         redisReady ? QStringLiteral("true") : QStringLiteral("false"));
        }
        return false;
    }

    SettingsManager::instance().setServerEndpoint(host, port);
    return true;
}

QString ServerWelcomeViewModel::normalizeHostInput(const QString& rawHost, int* portInOut)
{
    QString host = rawHost.trimmed();
    if (host.isEmpty()) {
        return QString();
    }

    QUrl url(host);
    if (url.isValid() && !url.scheme().isEmpty()) {
        if (!url.host().isEmpty()) {
            host = url.host().trimmed();
        }
        if (portInOut && url.port() > 0) {
            *portInOut = url.port();
        }
        return host;
    }

    int slashIndex = host.indexOf('/');
    if (slashIndex >= 0) {
        host = host.left(slashIndex).trimmed();
    }

    const int colonIndex = host.lastIndexOf(':');
    const bool hasSinglePort = colonIndex > 0 && host.indexOf(':') == colonIndex;
    if (hasSinglePort) {
        bool ok = false;
        const int parsedPort = host.mid(colonIndex + 1).toInt(&ok);
        if (ok && parsedPort > 0 && parsedPort <= 65535) {
            if (portInOut) {
                *portInOut = parsedPort;
            }
            host = host.left(colonIndex).trimmed();
        }
    }
    return host.trimmed();
}

bool ServerWelcomeViewModel::getJson(const QString& fullUrl, QJsonObject* root, QString* errorMessage)
{
    QNetworkRequest request{QUrl(fullUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    QNetworkReply* reply = m_networkManager->get(request);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start(5000);
    loop.exec();
    const bool timedOut = !timer.isActive();
    timer.stop();

    const QByteArray body = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorText = reply->errorString();
    reply->deleteLater();

    if (timedOut) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("请求超时：%1").arg(fullUrl);
        }
        return false;
    }

    if (networkError != QNetworkReply::NoError) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("请求失败：%1（%2）").arg(fullUrl, networkErrorText);
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("服务端响应不是有效 JSON：%1").arg(fullUrl);
        }
        return false;
    }

    if (root) {
        *root = doc.object();
    }
    return true;
}
