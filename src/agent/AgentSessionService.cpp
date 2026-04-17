#include "AgentSessionService.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>
#include <algorithm>

namespace {

QString ensureAgentDataDir()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (base.trimmed().isEmpty()) {
        base = QDir::currentPath();
    }
    const QString dirPath = QDir(base).filePath(QStringLiteral("agent"));
    QDir().mkpath(dirPath);
    return dirPath;
}

QDateTime parseIsoDateTime(const QString& value)
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

QJsonObject serializeMessage(const ChatMessageItem& item)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("id"), item.id);
    obj.insert(QStringLiteral("role"), item.role);
    obj.insert(QStringLiteral("messageType"), item.messageType);
    obj.insert(QStringLiteral("text"), item.text);
    obj.insert(QStringLiteral("rawText"), item.rawText);
    obj.insert(QStringLiteral("requestId"), item.requestId);
    obj.insert(QStringLiteral("status"), item.status);
    obj.insert(QStringLiteral("meta"), QJsonObject::fromVariantMap(item.meta));
    obj.insert(QStringLiteral("blocks"), QJsonArray::fromVariantList(item.blocks));
    obj.insert(QStringLiteral("timestamp"), item.timestamp.toUTC().toString(Qt::ISODateWithMs));
    return obj;
}

ChatMessageItem parseMessage(const QJsonObject& obj)
{
    ChatMessageItem item;
    item.id = obj.value(QStringLiteral("id")).toString().trimmed();
    item.role = obj.value(QStringLiteral("role")).toString().trimmed();
    item.messageType = obj.value(QStringLiteral("messageType")).toString().trimmed();
    item.text = obj.value(QStringLiteral("text")).toString();
    item.rawText = obj.value(QStringLiteral("rawText")).toString();
    item.requestId = obj.value(QStringLiteral("requestId")).toString().trimmed();
    item.status = obj.value(QStringLiteral("status")).toString().trimmed();
    item.meta = obj.value(QStringLiteral("meta")).toObject().toVariantMap();
    item.blocks = obj.value(QStringLiteral("blocks")).toArray().toVariantList();
    item.timestamp = parseIsoDateTime(obj.value(QStringLiteral("timestamp")).toString());
    if (!item.timestamp.isValid()) {
        item.timestamp = QDateTime::currentDateTime();
    }
    if (item.messageType.isEmpty()) {
        item.messageType = QStringLiteral("text");
    }
    if (item.rawText.isEmpty()) {
        item.rawText = item.text;
    }
    if (item.text.isEmpty()) {
        item.text = item.rawText;
    }
    if (item.id.isEmpty()) {
        item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    return item;
}

QJsonObject serializeSession(const AgentSessionService::SessionRecord& record)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("sessionId"), record.session.sessionId);
    obj.insert(QStringLiteral("title"), record.session.title);
    obj.insert(QStringLiteral("createdAt"), record.session.createdAt.toUTC().toString(Qt::ISODateWithMs));
    obj.insert(QStringLiteral("updatedAt"), record.session.updatedAt.toUTC().toString(Qt::ISODateWithMs));
    obj.insert(QStringLiteral("lastPreview"), record.session.lastPreview);
    obj.insert(QStringLiteral("messageCount"), record.session.messageCount);

    QJsonArray messages;
    for (const ChatMessageItem& item : record.messages) {
        messages.push_back(serializeMessage(item));
    }
    obj.insert(QStringLiteral("messages"), messages);
    return obj;
}

AgentSessionService::SessionRecord parseSession(const QJsonObject& obj)
{
    AgentSessionService::SessionRecord record;
    record.session.sessionId = obj.value(QStringLiteral("sessionId")).toString().trimmed();
    record.session.title = AgentSessionService::normalizeTitle(obj.value(QStringLiteral("title")).toString());
    record.session.createdAt = parseIsoDateTime(obj.value(QStringLiteral("createdAt")).toString());
    record.session.updatedAt = parseIsoDateTime(obj.value(QStringLiteral("updatedAt")).toString());
    record.session.lastPreview = obj.value(QStringLiteral("lastPreview")).toString();
    record.session.messageCount = obj.value(QStringLiteral("messageCount")).toInt();

    const QJsonArray messages = obj.value(QStringLiteral("messages")).toArray();
    record.messages.reserve(messages.size());
    for (const QJsonValue& value : messages) {
        if (value.isObject()) {
            record.messages.push_back(parseMessage(value.toObject()));
        }
    }
    AgentSessionService::refreshSessionMeta(&record);
    return record;
}

bool titleMatches(const AgentSessionService::SessionRecord& record, const QString& query)
{
    if (query.isEmpty()) {
        return true;
    }
    return record.session.title.contains(query, Qt::CaseInsensitive)
           || record.session.lastPreview.contains(query, Qt::CaseInsensitive);
}

}

AgentSessionService::AgentSessionService(QObject* parent)
    : QObject(parent)
{
}

void AgentSessionService::fetchSessions(const QString& query, int limit)
{
    QVector<SessionRecord> records = loadRecords();
    const QString trimmedQuery = query.trimmed();
    records.erase(std::remove_if(records.begin(),
                                 records.end(),
                                 [&](const SessionRecord& record) {
                                     return !titleMatches(record, trimmedQuery);
                                 }),
                  records.end());

    std::sort(records.begin(), records.end(), [](const SessionRecord& left, const SessionRecord& right) {
        return left.session.updatedAt > right.session.updatedAt;
    });

    if (limit > 0 && records.size() > limit) {
        records.resize(limit);
    }

    QVector<ChatSessionItem> sessions;
    sessions.reserve(records.size());
    for (const SessionRecord& record : records) {
        sessions.push_back(record.session);
    }
    emit sessionsLoaded(sessions);
}

void AgentSessionService::createSession(const QString& title)
{
    QVector<SessionRecord> records = loadRecords();
    SessionRecord record;
    record.session.sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    record.session.title = normalizeTitle(title);
    record.session.createdAt = QDateTime::currentDateTime();
    record.session.updatedAt = record.session.createdAt;
    record.session.lastPreview.clear();
    record.session.messageCount = 0;
    record.session.selected = false;

    records.push_front(record);
    if (!saveRecords(records)) {
        emit requestFailed(QStringLiteral("create_session"), QStringLiteral("写入本地会话文件失败。"));
        return;
    }

    emit sessionCreated(record.session);
}

void AgentSessionService::fetchSession(const QString& sessionId)
{
    const QVector<SessionRecord> records = loadRecords();
    const int index = indexOfSession(records, sessionId);
    if (index < 0) {
        emit requestFailed(QStringLiteral("fetch_session"), QStringLiteral("未找到指定会话。"));
        return;
    }
    emit sessionLoaded(records.at(index).session);
}

void AgentSessionService::fetchSessionMessages(const QString& sessionId)
{
    const QVector<SessionRecord> records = loadRecords();
    const int index = indexOfSession(records, sessionId);
    if (index < 0) {
        emit requestFailed(QStringLiteral("fetch_messages"), QStringLiteral("未找到指定会话。"));
        return;
    }
    emit sessionMessagesLoaded(records.at(index).session, records.at(index).messages);
}

void AgentSessionService::renameSession(const QString& sessionId, const QString& title)
{
    QVector<SessionRecord> records = loadRecords();
    const int index = indexOfSession(records, sessionId);
    if (index < 0) {
        emit requestFailed(QStringLiteral("rename_session"), QStringLiteral("未找到指定会话。"));
        return;
    }

    records[index].session.title = normalizeTitle(title);
    records[index].session.updatedAt = QDateTime::currentDateTime();
    if (!saveRecords(records)) {
        emit requestFailed(QStringLiteral("rename_session"), QStringLiteral("保存会话标题失败。"));
        return;
    }
    emit sessionUpdated(records.at(index).session);
}

void AgentSessionService::deleteSession(const QString& sessionId)
{
    QVector<SessionRecord> records = loadRecords();
    const int index = indexOfSession(records, sessionId);
    if (index < 0) {
        emit requestFailed(QStringLiteral("delete_session"), QStringLiteral("未找到指定会话。"));
        return;
    }

    records.removeAt(index);
    if (!saveRecords(records)) {
        emit requestFailed(QStringLiteral("delete_session"), QStringLiteral("删除会话失败。"));
        return;
    }
    emit sessionDeleted(sessionId.trimmed());
}

bool AgentSessionService::saveSessionMessages(const QString& sessionId, const QVector<ChatMessageItem>& messages)
{
    QVector<SessionRecord> records = loadRecords();
    const int index = indexOfSession(records, sessionId);
    if (index < 0) {
        return false;
    }

    records[index].messages = messages;
    refreshSessionMeta(&records[index]);
    records[index].session.updatedAt = QDateTime::currentDateTime();
    const bool ok = saveRecords(records);
    if (ok) {
        emit sessionUpdated(records.at(index).session);
    }
    return ok;
}

QString AgentSessionService::storeFilePath() const
{
    return QDir(ensureAgentDataDir()).filePath(QStringLiteral("chat_sessions.json"));
}

QVector<AgentSessionService::SessionRecord> AgentSessionService::loadRecords() const
{
    const QString path = storeFilePath();
    QFile file(path);
    if (!file.exists()) {
        return {};
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QByteArray raw = file.readAll();
    file.close();
    if (raw.trimmed().isEmpty()) {
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return {};
    }

    QVector<SessionRecord> records;
    const QJsonArray sessions = doc.object().value(QStringLiteral("sessions")).toArray();
    records.reserve(sessions.size());
    for (const QJsonValue& value : sessions) {
        if (value.isObject()) {
            records.push_back(parseSession(value.toObject()));
        }
    }
    return records;
}

bool AgentSessionService::saveRecords(const QVector<SessionRecord>& records) const
{
    QJsonArray sessions;
    for (const SessionRecord& record : records) {
        sessions.push_back(serializeSession(record));
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("sessions"), sessions);

    QFile file(storeFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

int AgentSessionService::indexOfSession(const QVector<SessionRecord>& records, const QString& sessionId) const
{
    const QString target = sessionId.trimmed();
    if (target.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < records.size(); ++i) {
        if (records.at(i).session.sessionId == target) {
            return i;
        }
    }
    return -1;
}

QString AgentSessionService::normalizeTitle(const QString& title)
{
    const QString trimmed = title.trimmed();
    return trimmed.isEmpty() ? QStringLiteral("新建会话") : trimmed;
}

QString AgentSessionService::normalizePreview(const QString& preview)
{
    QString text = preview.trimmed();
    if (text.isEmpty()) {
        return QString();
    }
    text.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    if (text.size() > 48) {
        text = text.left(48) + QStringLiteral("...");
    }
    return text;
}

QString AgentSessionService::buildPreviewFromMessages(const QVector<ChatMessageItem>& messages)
{
    for (int i = messages.size() - 1; i >= 0; --i) {
        const ChatMessageItem& item = messages.at(i);
        const QString candidate = normalizePreview(item.rawText.isEmpty() ? item.text : item.rawText);
        if (!candidate.isEmpty()) {
            return candidate;
        }
    }
    return QString();
}

void AgentSessionService::refreshSessionMeta(SessionRecord* record)
{
    if (!record) {
        return;
    }
    if (!record->session.createdAt.isValid()) {
        record->session.createdAt = QDateTime::currentDateTime();
    }
    if (!record->session.updatedAt.isValid()) {
        record->session.updatedAt = record->session.createdAt;
    }
    record->session.title = normalizeTitle(record->session.title);
    record->session.lastPreview = buildPreviewFromMessages(record->messages);
    record->session.messageCount = record->messages.size();
}
