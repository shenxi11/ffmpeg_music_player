#ifndef ONLINE_PRESENCE_MANAGER_H
#define ONLINE_PRESENCE_MANAGER_H

#include <QObject>
#include <QTimer>

#include "network/network_service.h"

class OnlinePresenceManager : public QObject
{
    Q_OBJECT

public:
    static OnlinePresenceManager& instance();

    void onLoginSucceeded(const QString& account,
                          const QString& username,
                          const QString& sessionTokenFromLogin,
                          int heartbeatIntervalSec,
                          int ttlSec);
    void ensureSessionForUser(const QString& account, const QString& username);

    void logoutAndClear(bool blocking = false, int timeoutMs = 1200);
    void clearSession();
    void requestStatusRefresh();

    bool hasSession() const;
    QString currentToken() const;
    QString currentAccount() const { return m_account; }
    bool currentOnline() const { return m_online; }
    int currentHeartbeatIntervalSec() const { return m_heartbeatIntervalSec; }
    int currentOnlineTtlSec() const { return m_onlineTtlSec; }
    int currentTtlRemainingSec() const { return m_ttlRemainingSec; }
    QString currentStatusMessage() const { return m_statusMessage; }
    qint64 currentLastSeenAt() const { return m_lastSeenAt; }

signals:
    void sessionExpired();
    void presenceSnapshotChanged(const QString& account,
                                 const QString& sessionToken,
                                 bool online,
                                 int heartbeatIntervalSec,
                                 int onlineTtlSec,
                                 int ttlRemainingSec,
                                 const QString& statusMessage,
                                 qint64 lastSeenAt);

private:
    explicit OnlinePresenceManager(QObject* parent = nullptr);
    ~OnlinePresenceManager() override = default;

    OnlinePresenceManager(const OnlinePresenceManager&) = delete;
    OnlinePresenceManager& operator=(const OnlinePresenceManager&) = delete;

    void startSessionIfNeeded();
    void sendHeartbeat();
    void emitSnapshot(const QString& statusMessage = QString());
    void restartHeartbeatTimer();
    void stopHeartbeatTimer();
    QString buildDeviceId() const;
    static int normalizeIntervalSec(int value);
    static int normalizeTtlSec(int value);
    bool parseEnvelope(const Network::NetworkResponse& response,
                       QJsonObject* dataOut,
                       int* codeOut,
                       QString* messageOut) const;

private:
    Network::NetworkService& m_networkService;
    QTimer m_heartbeatTimer;

    QString m_account;
    QString m_username;
    QString m_sessionToken;
    QString m_deviceId;
    int m_heartbeatIntervalSec = 30;
    int m_onlineTtlSec = 600;
    int m_ttlRemainingSec = 0;
    qint64 m_lastSeenAt = 0;
    bool m_online = false;
    QString m_statusMessage;
    bool m_heartbeatInFlight = false;
};

#endif // ONLINE_PRESENCE_MANAGER_H
