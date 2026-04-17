#ifndef AGENT_SESSION_SERVICE_H
#define AGENT_SESSION_SERVICE_H

#include <QObject>
#include <QDateTime>
#include <QVector>

#include "ChatMessageItem.h"
#include "ChatSessionItem.h"

class AgentSessionService : public QObject
{
    Q_OBJECT

public:
    explicit AgentSessionService(QObject* parent = nullptr);

    struct SessionRecord
    {
        ChatSessionItem session;
        QVector<ChatMessageItem> messages;
    };

    void fetchSessions(const QString& query = QString(), int limit = 100);
    void createSession(const QString& title = QString());
    void fetchSession(const QString& sessionId);
    void fetchSessionMessages(const QString& sessionId);
    void renameSession(const QString& sessionId, const QString& title);
    void deleteSession(const QString& sessionId);
    bool saveSessionMessages(const QString& sessionId, const QVector<ChatMessageItem>& messages);
    static QString normalizeTitle(const QString& title);
    static QString normalizePreview(const QString& preview);
    static QString buildPreviewFromMessages(const QVector<ChatMessageItem>& messages);
    static void refreshSessionMeta(SessionRecord* record);

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
    QString storeFilePath() const;
    QVector<SessionRecord> loadRecords() const;
    bool saveRecords(const QVector<SessionRecord>& records) const;
    int indexOfSession(const QVector<SessionRecord>& records, const QString& sessionId) const;
};

#endif // AGENT_SESSION_SERVICE_H
