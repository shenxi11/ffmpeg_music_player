#ifndef AGENT_SESSION_SERVICE_H
#define AGENT_SESSION_SERVICE_H

#include <QObject>
#include <QDateTime>
#include <QJsonObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QPointer>
#include <QUrl>
#include <QVector>

#include "ChatMessageItem.h"
#include "ChatSessionItem.h"

class QNetworkReply;

class AgentSessionService : public QObject
{
    Q_OBJECT

public:
    explicit AgentSessionService(QObject* parent = nullptr);

    void fetchSessions(const QString& query = QString(), int limit = 100);
    void createSession(const QString& title = QString());
    void fetchSession(const QString& sessionId);
    void fetchSessionMessages(const QString& sessionId);
    void renameSession(const QString& sessionId, const QString& title);
    void deleteSession(const QString& sessionId);

signals:
    void sessionsLoaded(const QVector<ChatSessionItem>& sessions);
    void sessionCreated(const ChatSessionItem& session);
    void sessionLoaded(const ChatSessionItem& session);
    void sessionUpdated(const ChatSessionItem& session);
    void sessionDeleted(const QString& sessionId);
    void sessionMessagesLoaded(const ChatSessionItem& session,
                               const QVector<ChatMessageItem>& messages);
    void requestFailed(const QString& operation, const QString& errorMessage);

private:
    static QDateTime parseIsoDateTime(const QString& value);
    static ChatSessionItem parseSessionItem(const QJsonObject& obj);
    static ChatMessageItem parseMessageItem(const QJsonObject& obj);
    static QString buildReplyError(const QString& fallbackOperation,
                                   QNetworkReply* reply,
                                   const QByteArray& body);
    QUrl buildUrl(const QString& path, const QMap<QString, QString>& queryItems = {}) const;
    QNetworkRequest buildJsonRequest(const QUrl& url) const;

private:
    QNetworkAccessManager m_networkManager;
    quint64 m_fetchSessionsToken = 0;
    quint64 m_fetchMessagesToken = 0;
};

#endif // AGENT_SESSION_SERVICE_H
