#include "online_presence_manager.h"

#include <QDateTime>
#include <QEventLoop>
#include <QJsonObject>
#include <QSysInfo>
#include <QTimer>
#include <QUrlQuery>

namespace {
constexpr int kDefaultHeartbeatSec = 30;
constexpr int kDefaultTtlSec = 600;
constexpr int kMinHeartbeatSec = 5;
constexpr int kMaxHeartbeatSec = 300;
} // namespace

OnlinePresenceManager& OnlinePresenceManager::instance() {
    static OnlinePresenceManager manager;
    return manager;
}

OnlinePresenceManager::OnlinePresenceManager(QObject* parent)
    : QObject(parent), m_networkService(Network::NetworkService::instance()) {
    m_deviceId = buildDeviceId();

    m_heartbeatTimer.setSingleShot(false);
    connect(&m_heartbeatTimer, &QTimer::timeout, this,
            &OnlinePresenceManager::onHeartbeatTimerTimeout);
}

void OnlinePresenceManager::onLoginSucceeded(const QString& account, const QString& username,
                                             const QString& sessionTokenFromLogin,
                                             int heartbeatIntervalSec, int ttlSec) {
    m_account = account.trimmed();
    m_username = username.trimmed();
    m_heartbeatIntervalSec = normalizeIntervalSec(heartbeatIntervalSec);
    m_onlineTtlSec = normalizeTtlSec(ttlSec);

    const QString token = sessionTokenFromLogin.trimmed();
    if (!token.isEmpty()) {
        m_sessionToken = token;
        m_online = true;
        m_statusMessage = QStringLiteral("在线会话已建立");
        restartHeartbeatTimer();
        emitSnapshot(m_statusMessage);
        QTimer::singleShot(0, this, &OnlinePresenceManager::triggerImmediateHeartbeat);
        return;
    }

    startSessionIfNeeded();
}

void OnlinePresenceManager::ensureSessionForUser(const QString& account, const QString& username) {
    const QString normalizedAccount = account.trimmed();
    const QString normalizedUsername = username.trimmed();
    if (normalizedAccount.isEmpty() && normalizedUsername.isEmpty()) {
        return;
    }

    if (!normalizedAccount.isEmpty()) {
        m_account = normalizedAccount;
    }
    if (!normalizedUsername.isEmpty()) {
        m_username = normalizedUsername;
    }

    if (m_sessionToken.trimmed().isEmpty()) {
        m_statusMessage = QStringLiteral("补偿创建在线会话中");
        emitSnapshot(m_statusMessage);
        startSessionIfNeeded();
        return;
    }

    if (!m_heartbeatTimer.isActive()) {
        restartHeartbeatTimer();
    }
    requestStatusRefresh();
}

void OnlinePresenceManager::updateCurrentUsername(const QString& username) {
    const QString normalizedUsername = username.trimmed();
    if (normalizedUsername.isEmpty() || m_username == normalizedUsername) {
        return;
    }

    m_username = normalizedUsername;
    emitSnapshot(m_statusMessage);
}

void OnlinePresenceManager::logoutAndClear(bool blocking, int timeoutMs) {
    const QString token = m_sessionToken.trimmed();
    if (token.isEmpty()) {
        clearSession();
        return;
    }

    QJsonObject json;
    if (!m_account.trimmed().isEmpty()) {
        json["account"] = m_account.trimmed();
    } else if (!m_username.trimmed().isEmpty()) {
        json["username"] = m_username.trimmed();
    }
    json["session_token"] = token;

    auto options = Network::RequestOptions::critical();
    options.useCache = false;
    options.maxRetries = 0;
    options.timeout = qBound(400, timeoutMs, 3000);

    if (!blocking) {
        m_networkService.postJson(
            "users/online/logout", json, options, [](const Network::NetworkResponse& response) {
                qDebug() << "[OnlinePresence] logout finished:" << response.statusCode
                         << response.errorString;
            });
        clearSession();
        return;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    bool done = false;

    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_networkService.postJson("users/online/logout", json, options,
                              [&](const Network::NetworkResponse& response) {
                                  Q_UNUSED(response);
                                  done = true;
                                  loop.quit();
                              });

    timer.start(qBound(400, timeoutMs, 3000));
    loop.exec();

    qDebug() << "[OnlinePresence] logout blocking finished, done =" << done;
    clearSession();
}

void OnlinePresenceManager::clearSession() {
    stopHeartbeatTimer();
    m_heartbeatInFlight = false;
    m_online = false;
    m_lastSeenAt = 0;
    m_ttlRemainingSec = 0;
    m_statusMessage = QStringLiteral("未登录");
    m_sessionToken.clear();
    m_account.clear();
    m_username.clear();
    emitSnapshot(m_statusMessage);
}

void OnlinePresenceManager::requestStatusRefresh() {
    if (m_sessionToken.trimmed().isEmpty()) {
        m_online = false;
        m_ttlRemainingSec = 0;
        m_statusMessage = QStringLiteral("无在线会话");
        emitSnapshot(m_statusMessage);
        return;
    }

    QUrlQuery query;
    if (!m_account.trimmed().isEmpty()) {
        query.addQueryItem("account", m_account.trimmed());
    } else if (!m_username.trimmed().isEmpty()) {
        query.addQueryItem("username", m_username.trimmed());
    }
    query.addQueryItem("session_token", m_sessionToken.trimmed());

    QString path = QStringLiteral("users/online/status");
    const QString encoded = query.toString(QUrl::FullyEncoded);
    if (!encoded.isEmpty()) {
        path += "?" + encoded;
    }

    auto options = Network::RequestOptions::critical();
    options.useCache = false;
    options.maxRetries = 0;
    options.timeout = 8000;

    m_networkService.get(path, options, [this](const Network::NetworkResponse& response) {
        QJsonObject data;
        int code = -1;
        QString message;
        if (!parseEnvelope(response, &data, &code, &message)) {
            const bool unauthorized = (response.statusCode == 401 || code == 401);
            if (unauthorized) {
                stopHeartbeatTimer();
                m_sessionToken.clear();
                m_online = false;
                m_ttlRemainingSec = 0;
                m_statusMessage = QStringLiteral("在线会话已失效");
                emitSnapshot(m_statusMessage);
                emit sessionExpired();
                return;
            }
            m_statusMessage = message.isEmpty() ? QStringLiteral("状态刷新失败") : message;
            emitSnapshot(m_statusMessage);
            return;
        }

        m_online = data.value("online").toBool(false);
        m_lastSeenAt = data.value("last_seen_at").toVariant().toLongLong();
        m_ttlRemainingSec = data.value("ttl_remaining_sec").toInt(0);
        m_heartbeatIntervalSec = normalizeIntervalSec(
            data.value("heartbeat_interval_sec").toInt(m_heartbeatIntervalSec));
        m_onlineTtlSec = normalizeTtlSec(data.value("online_ttl_sec").toInt(m_onlineTtlSec));
        m_statusMessage = m_online ? QStringLiteral("在线") : QStringLiteral("离线");
        emitSnapshot(m_statusMessage);
    });
}

bool OnlinePresenceManager::hasSession() const {
    return !m_sessionToken.trimmed().isEmpty();
}

QString OnlinePresenceManager::currentToken() const {
    return m_sessionToken;
}

void OnlinePresenceManager::onHeartbeatTimerTimeout() {
    sendHeartbeat();
}

void OnlinePresenceManager::triggerImmediateHeartbeat() {
    sendHeartbeat();
}

void OnlinePresenceManager::startSessionIfNeeded() {
    if (!m_sessionToken.trimmed().isEmpty()) {
        restartHeartbeatTimer();
        return;
    }
    if (m_account.trimmed().isEmpty() && m_username.trimmed().isEmpty()) {
        return;
    }

    QJsonObject json;
    if (!m_account.trimmed().isEmpty()) {
        json["account"] = m_account.trimmed();
    } else {
        json["username"] = m_username.trimmed();
    }
    json["device_id"] = m_deviceId;

    auto options = Network::RequestOptions::critical();
    options.useCache = false;
    options.maxRetries = 1;
    options.timeout = 8000;

    m_networkService.postJson(
        "users/online/session/start", json, options,
        [this](const Network::NetworkResponse& response) {
            QJsonObject data;
            int code = -1;
            QString message;
            if (!parseEnvelope(response, &data, &code, &message)) {
                qWarning() << "[OnlinePresence] start session failed:" << response.statusCode
                           << response.errorString << message;
                return;
            }

            const QString token = data.value("session_token").toString().trimmed();
            if (token.isEmpty()) {
                qWarning() << "[OnlinePresence] start session response missing session_token";
                return;
            }

            m_sessionToken = token;
            m_heartbeatIntervalSec = normalizeIntervalSec(
                data.value("heartbeat_interval_sec").toInt(m_heartbeatIntervalSec));
            m_onlineTtlSec = normalizeTtlSec(data.value("online_ttl_sec").toInt(m_onlineTtlSec));
            m_online = true;
            m_lastSeenAt = data.value("last_seen_at").toVariant().toLongLong();
            const qint64 expireAt = data.value("expire_at").toVariant().toLongLong();
            if (expireAt > 0) {
                const qint64 nowSec = QDateTime::currentSecsSinceEpoch();
                m_ttlRemainingSec = static_cast<int>(qMax<qint64>(0, expireAt - nowSec));
            }
            m_statusMessage = QStringLiteral("在线会话已创建");
            restartHeartbeatTimer();
            emitSnapshot(m_statusMessage);
            sendHeartbeat();
        });
}

void OnlinePresenceManager::sendHeartbeat() {
    if (m_heartbeatInFlight) {
        return;
    }
    if (m_sessionToken.trimmed().isEmpty()) {
        return;
    }
    if (m_account.trimmed().isEmpty() && m_username.trimmed().isEmpty()) {
        return;
    }

    QJsonObject json;
    if (!m_account.trimmed().isEmpty()) {
        json["account"] = m_account.trimmed();
    } else {
        json["username"] = m_username.trimmed();
    }
    json["session_token"] = m_sessionToken.trimmed();
    json["device_id"] = m_deviceId;

    auto options = Network::RequestOptions::critical();
    options.useCache = false;
    options.maxRetries = 0;
    options.timeout = 8000;

    m_heartbeatInFlight = true;
    m_networkService.postJson(
        "users/online/heartbeat", json, options, [this](const Network::NetworkResponse& response) {
            m_heartbeatInFlight = false;

            QJsonObject data;
            int code = -1;
            QString message;
            if (!parseEnvelope(response, &data, &code, &message)) {
                const bool unauthorized = (response.statusCode == 401 || code == 401);
                qWarning() << "[OnlinePresence] heartbeat failed:" << response.statusCode
                           << response.errorString << message;
                if (unauthorized) {
                    stopHeartbeatTimer();
                    m_sessionToken.clear();
                    m_online = false;
                    m_ttlRemainingSec = 0;
                    m_statusMessage = QStringLiteral("在线会话已失效");
                    emitSnapshot(m_statusMessage);
                    emit sessionExpired();
                }
                return;
            }

            const QString renewedToken = data.value("session_token").toString().trimmed();
            if (!renewedToken.isEmpty()) {
                m_sessionToken = renewedToken;
            }
            const int heartbeatSec =
                data.value("heartbeat_interval_sec").toInt(m_heartbeatIntervalSec);
            const int ttlSec = data.value("online_ttl_sec").toInt(m_onlineTtlSec);
            const int normalizedHeartbeat = normalizeIntervalSec(heartbeatSec);
            const int normalizedTtl = normalizeTtlSec(ttlSec);
            m_online = true;
            m_lastSeenAt = data.value("last_seen_at").toVariant().toLongLong();
            const qint64 expireAt = data.value("expire_at").toVariant().toLongLong();
            if (expireAt > 0) {
                const qint64 nowSec = QDateTime::currentSecsSinceEpoch();
                m_ttlRemainingSec = static_cast<int>(qMax<qint64>(0, expireAt - nowSec));
            } else {
                m_ttlRemainingSec = normalizedTtl;
            }
            m_statusMessage = QStringLiteral("在线");
            if (normalizedHeartbeat != m_heartbeatIntervalSec || normalizedTtl != m_onlineTtlSec) {
                m_heartbeatIntervalSec = normalizedHeartbeat;
                m_onlineTtlSec = normalizedTtl;
                restartHeartbeatTimer();
            }
            emitSnapshot(m_statusMessage);
        });
}

void OnlinePresenceManager::restartHeartbeatTimer() {
    const int intervalMs = normalizeIntervalSec(m_heartbeatIntervalSec) * 1000;
    m_heartbeatTimer.start(intervalMs);
}

void OnlinePresenceManager::stopHeartbeatTimer() {
    if (m_heartbeatTimer.isActive()) {
        m_heartbeatTimer.stop();
    }
}

QString OnlinePresenceManager::buildDeviceId() const {
    QString host = QSysInfo::machineHostName().trimmed();
    if (host.isEmpty()) {
        host = QStringLiteral("desktop");
    }
    return QStringLiteral("pc-") + host;
}

int OnlinePresenceManager::normalizeIntervalSec(int value) {
    if (value <= 0) {
        value = kDefaultHeartbeatSec;
    }
    return qBound(kMinHeartbeatSec, value, kMaxHeartbeatSec);
}

int OnlinePresenceManager::normalizeTtlSec(int value) {
    if (value <= 0) {
        return kDefaultTtlSec;
    }
    return value;
}

bool OnlinePresenceManager::parseEnvelope(const Network::NetworkResponse& response,
                                          QJsonObject* dataOut, int* codeOut,
                                          QString* messageOut) const {
    if (!response.isSuccess()) {
        if (messageOut) {
            *messageOut = response.errorString;
        }
        if (codeOut) {
            *codeOut = response.statusCode;
        }
        return false;
    }

    const QJsonObject root = Network::NetworkService::parseJsonObject(response);
    if (root.isEmpty()) {
        if (messageOut) {
            *messageOut = QStringLiteral("empty response");
        }
        return false;
    }

    const int code = root.value("code").toInt(0);
    const QString message = root.value("message").toString().trimmed();

    if (codeOut) {
        *codeOut = code;
    }
    if (messageOut) {
        *messageOut = message;
    }

    if (code != 0) {
        return false;
    }

    if (dataOut) {
        *dataOut = root.value("data").toObject();
    }
    return true;
}

void OnlinePresenceManager::emitSnapshot(const QString& statusMessage) {
    if (!statusMessage.trimmed().isEmpty()) {
        m_statusMessage = statusMessage.trimmed();
    }

    emit presenceSnapshotChanged(m_account, m_sessionToken, m_online, m_heartbeatIntervalSec,
                                 m_onlineTtlSec, m_ttlRemainingSec, m_statusMessage, m_lastSeenAt);
}
