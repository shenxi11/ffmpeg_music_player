#include "AgentSessionService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

namespace {

constexpr const char* kBaseHttpUrl = "http://127.0.0.1:8765";

QString normalizeTitle(const QString& title)
{
    const QString trimmed = title.trimmed();
    return trimmed.isEmpty() ? QStringLiteral("新建会话") : trimmed;
}

bool looksLikeCorruptedTitle(const QString& title)
{
    const QString trimmed = title.trimmed();
    if (trimmed.isEmpty()) {
        return true;
    }
    if (trimmed.contains(QChar::ReplacementCharacter)) {
        return true;
    }

    int questionCount = 0;
    int validCount = 0;
    for (QChar ch : trimmed) {
        if (ch == QChar('?')) {
            ++questionCount;
            continue;
        }
        if (!ch.isSpace()) {
            ++validCount;
        }
    }

    if (questionCount == 0) {
        return false;
    }
    if (validCount == 0) {
        return true;
    }
    const double ratio = static_cast<double>(questionCount) / static_cast<double>(questionCount + validCount);
    return ratio >= 0.35;
}

QString fallbackTitleFromPreview(const QString& preview)
{
    QString text = preview.trimmed();
    if (text.isEmpty()) {
        return QStringLiteral("新建会话");
    }

    text.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    const int maxLen = 26;
    if (text.size() > maxLen) {
        text = text.left(maxLen) + QStringLiteral("...");
    }
    return text;
}

} // namespace

AgentSessionService::AgentSessionService(QObject* parent)
    : QObject(parent)
{
}

void AgentSessionService::fetchSessions(const QString& query, int limit)
{
    const quint64 token = ++m_fetchSessionsToken;
    QMap<QString, QString> queryItems;
    const QString trimmedQuery = query.trimmed();
    if (!trimmedQuery.isEmpty()) {
        queryItems.insert(QStringLiteral("query"), trimmedQuery);
    }
    if (limit > 0) {
        queryItems.insert(QStringLiteral("limit"), QString::number(limit));
    }

    const QNetworkRequest request = buildJsonRequest(buildUrl(QStringLiteral("/sessions"), queryItems));
    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, token]() {
        const QByteArray body = reply->readAll();
        const bool ok = (reply->error() == QNetworkReply::NoError);
        reply->deleteLater();

        if (token != m_fetchSessionsToken) {
            return;
        }

        if (!ok) {
            emit requestFailed(QStringLiteral("fetch_sessions"),
                               buildReplyError(QStringLiteral("获取会话列表失败"), reply, body));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit requestFailed(QStringLiteral("fetch_sessions"),
                               QStringLiteral("会话列表响应 JSON 解析失败。"));
            return;
        }

        const QJsonArray items = doc.object().value(QStringLiteral("items")).toArray();
        QVector<ChatSessionItem> sessions;
        sessions.reserve(items.size());
        for (const QJsonValue& value : items) {
            if (!value.isObject()) {
                continue;
            }
            sessions.push_back(parseSessionItem(value.toObject()));
        }
        emit sessionsLoaded(sessions);
    });
}

void AgentSessionService::createSession(const QString& title)
{
    QJsonObject payload;
    const QString normalized = title.trimmed();
    if (!normalized.isEmpty()) {
        payload.insert(QStringLiteral("title"), normalized);
    }

    const QNetworkRequest request = buildJsonRequest(buildUrl(QStringLiteral("/sessions")));
    QNetworkReply* reply = m_networkManager.post(
        request,
        QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray body = reply->readAll();
        const bool ok = (reply->error() == QNetworkReply::NoError);
        reply->deleteLater();

        if (!ok) {
            emit requestFailed(QStringLiteral("create_session"),
                               buildReplyError(QStringLiteral("创建会话失败"), reply, body));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit requestFailed(QStringLiteral("create_session"),
                               QStringLiteral("创建会话返回内容解析失败。"));
            return;
        }

        ChatSessionItem session = parseSessionItem(doc.object());
        session.title = normalizeTitle(session.title);
        emit sessionCreated(session);
    });
}

void AgentSessionService::fetchSession(const QString& sessionId)
{
    const QString sid = sessionId.trimmed();
    if (sid.isEmpty()) {
        emit requestFailed(QStringLiteral("fetch_session"),
                           QStringLiteral("会话 ID 不能为空。"));
        return;
    }

    const QNetworkRequest request = buildJsonRequest(
        buildUrl(QStringLiteral("/sessions/%1").arg(sid)));
    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray body = reply->readAll();
        const bool ok = (reply->error() == QNetworkReply::NoError);
        reply->deleteLater();

        if (!ok) {
            emit requestFailed(QStringLiteral("fetch_session"),
                               buildReplyError(QStringLiteral("获取会话详情失败"), reply, body));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit requestFailed(QStringLiteral("fetch_session"),
                               QStringLiteral("会话详情响应解析失败。"));
            return;
        }

        emit sessionLoaded(parseSessionItem(doc.object()));
    });
}

void AgentSessionService::fetchSessionMessages(const QString& sessionId)
{
    const QString sid = sessionId.trimmed();
    if (sid.isEmpty()) {
        emit requestFailed(QStringLiteral("fetch_messages"),
                           QStringLiteral("会话 ID 不能为空。"));
        return;
    }

    const quint64 token = ++m_fetchMessagesToken;
    const QNetworkRequest request = buildJsonRequest(
        buildUrl(QStringLiteral("/sessions/%1/messages").arg(sid)));
    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, token]() {
        const QByteArray body = reply->readAll();
        const bool ok = (reply->error() == QNetworkReply::NoError);
        reply->deleteLater();

        if (token != m_fetchMessagesToken) {
            return;
        }

        if (!ok) {
            emit requestFailed(QStringLiteral("fetch_messages"),
                               buildReplyError(QStringLiteral("获取历史消息失败"), reply, body));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit requestFailed(QStringLiteral("fetch_messages"),
                               QStringLiteral("历史消息响应解析失败。"));
            return;
        }

        const QJsonObject root = doc.object();
        ChatSessionItem session = parseSessionItem(root.value(QStringLiteral("session")).toObject());
        QVector<ChatMessageItem> messages;
        const QJsonArray items = root.value(QStringLiteral("items")).toArray();
        messages.reserve(items.size());
        for (const QJsonValue& value : items) {
            if (!value.isObject()) {
                continue;
            }
            messages.push_back(parseMessageItem(value.toObject()));
        }
        emit sessionMessagesLoaded(session, messages);
    });
}

void AgentSessionService::renameSession(const QString& sessionId, const QString& title)
{
    const QString sid = sessionId.trimmed();
    const QString trimmedTitle = title.trimmed();
    if (sid.isEmpty() || trimmedTitle.isEmpty()) {
        emit requestFailed(QStringLiteral("rename_session"),
                           QStringLiteral("会话 ID 和标题不能为空。"));
        return;
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("title"), trimmedTitle);

    const QNetworkRequest request = buildJsonRequest(
        buildUrl(QStringLiteral("/sessions/%1").arg(sid)));
    QNetworkReply* reply = m_networkManager.sendCustomRequest(
        request,
        QByteArrayLiteral("PATCH"),
        QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray body = reply->readAll();
        const bool ok = (reply->error() == QNetworkReply::NoError);
        reply->deleteLater();

        if (!ok) {
            emit requestFailed(QStringLiteral("rename_session"),
                               buildReplyError(QStringLiteral("重命名会话失败"), reply, body));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit requestFailed(QStringLiteral("rename_session"),
                               QStringLiteral("重命名响应解析失败。"));
            return;
        }

        emit sessionUpdated(parseSessionItem(doc.object()));
    });
}

void AgentSessionService::deleteSession(const QString& sessionId)
{
    const QString sid = sessionId.trimmed();
    if (sid.isEmpty()) {
        emit requestFailed(QStringLiteral("delete_session"),
                           QStringLiteral("会话 ID 不能为空。"));
        return;
    }

    const QNetworkRequest request = buildJsonRequest(
        buildUrl(QStringLiteral("/sessions/%1").arg(sid)));
    QNetworkReply* reply = m_networkManager.deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, sid]() {
        const QByteArray body = reply->readAll();
        const bool ok = (reply->error() == QNetworkReply::NoError);
        reply->deleteLater();

        if (!ok) {
            emit requestFailed(QStringLiteral("delete_session"),
                               buildReplyError(QStringLiteral("删除会话失败"), reply, body));
            return;
        }

        emit sessionDeleted(sid);
    });
}

QDateTime AgentSessionService::parseIsoDateTime(const QString& value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return QDateTime();
    }

    QDateTime dt = QDateTime::fromString(trimmed, Qt::ISODateWithMs);
    if (!dt.isValid()) {
        dt = QDateTime::fromString(trimmed, Qt::ISODate);
    }
    return dt;
}

ChatSessionItem AgentSessionService::parseSessionItem(const QJsonObject& obj)
{
    ChatSessionItem item;
    item.sessionId = obj.value(QStringLiteral("sessionId")).toString().trimmed();
    item.title = normalizeTitle(obj.value(QStringLiteral("title")).toString());
    item.createdAt = parseIsoDateTime(obj.value(QStringLiteral("createdAt")).toString());
    item.updatedAt = parseIsoDateTime(obj.value(QStringLiteral("updatedAt")).toString());
    item.lastPreview = obj.value(QStringLiteral("lastPreview")).toString();
    item.messageCount = obj.value(QStringLiteral("messageCount")).toInt();
    item.selected = false;
    if (looksLikeCorruptedTitle(item.title)) {
        item.title = fallbackTitleFromPreview(item.lastPreview);
    }
    return item;
}

ChatMessageItem AgentSessionService::parseMessageItem(const QJsonObject& obj)
{
    ChatMessageItem item;
    item.id = obj.value(QStringLiteral("messageId")).toString().trimmed();
    item.role = obj.value(QStringLiteral("role")).toString().trimmed();
    item.messageType = QStringLiteral("text");
    item.rawText = obj.value(QStringLiteral("content")).toString();
    item.text = item.rawText;
    item.meta.clear();
    item.requestId = QString();
    item.timestamp = parseIsoDateTime(obj.value(QStringLiteral("createdAt")).toString());
    item.status = QStringLiteral("done");
    item.blocks.clear();
    return item;
}

QString AgentSessionService::buildReplyError(const QString& fallbackOperation,
                                             QNetworkReply* reply,
                                             const QByteArray& body)
{
    QString errorText = fallbackOperation;
    if (reply) {
        const QString netError = reply->errorString().trimmed();
        if (!netError.isEmpty()) {
            errorText = QStringLiteral("%1：%2").arg(errorText, netError);
        }
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        const QJsonObject obj = doc.object();
        const QString message = obj.value(QStringLiteral("message")).toString().trimmed();
        const QString detail = obj.value(QStringLiteral("detail")).toString().trimmed();
        if (!message.isEmpty()) {
            return QStringLiteral("%1：%2").arg(fallbackOperation, message);
        }
        if (!detail.isEmpty()) {
            return QStringLiteral("%1：%2").arg(fallbackOperation, detail);
        }
    }
    return errorText;
}

QUrl AgentSessionService::buildUrl(const QString& path, const QMap<QString, QString>& queryItems) const
{
    QUrl url(QString::fromLatin1(kBaseHttpUrl) + path);
    if (!queryItems.isEmpty()) {
        QUrlQuery query(url);
        for (auto it = queryItems.constBegin(); it != queryItems.constEnd(); ++it) {
            if (it.value().trimmed().isEmpty()) {
                continue;
            }
            query.addQueryItem(it.key(), it.value());
        }
        url.setQuery(query);
    }
    return url;
}

QNetworkRequest AgentSessionService::buildJsonRequest(const QUrl& url) const
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(5000);
#endif
    return request;
}
