#include "ChatMessageListModel.h"
#include "markdown/MarkdownMessageParser.h"

#include <QDateTime>
#include <QMetaObject>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>

ChatMessageListModel::ChatMessageListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ChatMessageListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_items.size();
}

QVariant ChatMessageListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return QVariant();
    }

    const ChatMessageItem& item = m_items.at(index.row());
    switch (role) {
    case Qt::DisplayRole: {
        const QString timeText = item.timestamp.isValid()
            ? item.timestamp.toString(QStringLiteral("HH:mm:ss"))
            : QStringLiteral("--:--:--");
        return QStringLiteral("[%1] %2\n%3")
            .arg(timeText, roleText(item.role), item.text);
    }
    case IdRole:
        return item.id;
    case RoleRole:
        return item.role;
    case MessageTypeRole:
        return item.messageType;
    case MetaRole:
        return item.meta;
    case TextRole:
        return item.text;
    case RawTextRole:
        return item.rawText;
    case BlocksRole:
        return item.blocks;
    case RequestIdRole:
        return item.requestId;
    case TimestampRole:
        return item.timestamp;
    case StatusRole:
        return item.status;
    default:
        break;
    }

    return QVariant();
}

QHash<int, QByteArray> ChatMessageListModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names.insert(IdRole, "id");
    names.insert(RoleRole, "role");
    names.insert(MessageTypeRole, "messageType");
    names.insert(MetaRole, "meta");
    names.insert(TextRole, "text");
    names.insert(RawTextRole, "rawText");
    names.insert(BlocksRole, "blocks");
    names.insert(RequestIdRole, "requestId");
    names.insert(TimestampRole, "timestamp");
    names.insert(StatusRole, "status");
    return names;
}

void ChatMessageListModel::appendMessage(const ChatMessageItem& item)
{
    ChatMessageItem normalized = item;
    if (normalized.timestamp.isValid() == false) {
        normalized.timestamp = QDateTime::currentDateTime();
    }
    if (normalized.messageType.trimmed().isEmpty()) {
        normalized.messageType = QStringLiteral("text");
    }
    if (normalized.rawText.isEmpty()) {
        normalized.rawText = normalized.text;
    }
    if (normalized.text.isEmpty()) {
        normalized.text = normalized.rawText;
    }

    const int row = m_items.size();
    beginInsertRows(QModelIndex(), row, row);
    m_items.append(normalized);
    endInsertRows();

    if (m_items[row].blocks.isEmpty()
        && !m_items[row].rawText.trimmed().isEmpty()
        && m_items[row].role != QStringLiteral("user")) {
        scheduleAsyncReparse(row);
    }
}

void ChatMessageListModel::clear()
{
    if (m_items.isEmpty()) {
        return;
    }
    beginResetModel();
    m_items.clear();
    m_parseQueue.clear();
    m_parseDispatching = false;
    endResetModel();
}

QVariantList ChatMessageListModel::dumpMessages() const
{
    QVariantList result;
    result.reserve(m_items.size());

    for (const ChatMessageItem& item : m_items) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), item.id);
        map.insert(QStringLiteral("role"), item.role);
        map.insert(QStringLiteral("messageType"), item.messageType);
        map.insert(QStringLiteral("meta"), item.meta);
        map.insert(QStringLiteral("text"), item.text);
        map.insert(QStringLiteral("rawText"), item.rawText);
        map.insert(QStringLiteral("blocks"), item.blocks);
        map.insert(QStringLiteral("requestId"), item.requestId);
        map.insert(QStringLiteral("status"), item.status);
        map.insert(QStringLiteral("timestampMs"),
                   item.timestamp.isValid() ? item.timestamp.toMSecsSinceEpoch() : 0);
        result.append(map);
    }

    return result;
}

void ChatMessageListModel::loadMessages(const QVariantList& messages)
{
    beginResetModel();
    m_items.clear();
    m_parseQueue.clear();
    m_parseDispatching = false;

    for (const QVariant& value : messages) {
        const QVariantMap map = value.toMap();
        if (map.isEmpty()) {
            continue;
        }

        ChatMessageItem item;
        item.id = map.value(QStringLiteral("id")).toString();
        item.role = map.value(QStringLiteral("role")).toString();
        item.messageType = map.value(QStringLiteral("messageType")).toString();
        item.meta = map.value(QStringLiteral("meta")).toMap();
        item.text = map.value(QStringLiteral("text")).toString();
        item.rawText = map.value(QStringLiteral("rawText")).toString();
        item.blocks = map.value(QStringLiteral("blocks")).toList();
        item.requestId = map.value(QStringLiteral("requestId")).toString();
        item.status = map.value(QStringLiteral("status")).toString();
        const qint64 ts = map.value(QStringLiteral("timestampMs")).toLongLong();
        if (ts > 0) {
            item.timestamp = QDateTime::fromMSecsSinceEpoch(ts);
        } else {
            item.timestamp = QDateTime::currentDateTime();
        }
        if (item.rawText.isEmpty()) {
            item.rawText = item.text;
        }
        if (item.text.isEmpty()) {
            item.text = item.rawText;
        }
        if (item.messageType.trimmed().isEmpty()) {
            item.messageType = QStringLiteral("text");
        }

        m_items.append(item);
    }

    endResetModel();

    const int startRow = qMax(0, m_items.size() - 24);
    for (int i = startRow; i < m_items.size(); ++i) {
        const ChatMessageItem& item = m_items[i];
        if (item.role == QStringLiteral("user")) {
            continue;
        }
        if (!item.blocks.isEmpty()) {
            continue;
        }
        if (item.rawText.trimmed().isEmpty()) {
            continue;
        }
        scheduleAsyncReparse(i);
    }
}

void ChatMessageListModel::updateMessageStatusByRequestId(const QString& requestId, const QString& status)
{
    const QString rid = requestId.trimmed();
    if (rid.isEmpty()) {
        return;
    }

    bool changed = false;
    for (int i = 0; i < m_items.size(); ++i) {
        ChatMessageItem& item = m_items[i];
        if (item.requestId != rid) {
            continue;
        }

        if (item.status == status) {
            continue;
        }

        item.status = status;
        const QModelIndex rowIndex = index(i);
        emit dataChanged(rowIndex, rowIndex, {StatusRole, Qt::DisplayRole});
        changed = true;
    }

    Q_UNUSED(changed);
}

bool ChatMessageListModel::setRawTextAndReparse(const QString& requestId,
                                                const QString& rawText,
                                                const QString& status)
{
    const QString rid = requestId.trimmed();
    if (rid.isEmpty()) {
        return false;
    }

    int row = findLastAssistantRowByRequestId(rid);
    if (row < 0) {
        if (!beginAssistantMessage(rid)) {
            return false;
        }
        row = findLastAssistantRowByRequestId(rid);
    }
    if (row < 0 || row >= m_items.size()) {
        return false;
    }

    ChatMessageItem& item = m_items[row];
    item.rawText = rawText;
    item.text = rawText;
    if (!status.trimmed().isEmpty()) {
        item.status = status.trimmed();
    }
    scheduleAsyncReparse(row);
    notifyRowChanged(row, {TextRole, RawTextRole, StatusRole, Qt::DisplayRole});
    return true;
}

bool ChatMessageListModel::appendRawDeltaAndReparse(const QString& requestId, const QString& delta)
{
    const QString rid = requestId.trimmed();
    if (rid.isEmpty()) {
        return false;
    }

    int row = findLastAssistantRowByRequestId(rid);
    if (row < 0) {
        if (!beginAssistantMessage(rid)) {
            return false;
        }
        row = findLastAssistantRowByRequestId(rid);
    }
    if (row < 0 || row >= m_items.size()) {
        return false;
    }

    ChatMessageItem& item = m_items[row];
    item.rawText += delta;
    item.text = item.rawText;
    item.status = QStringLiteral("streaming");
    scheduleAsyncReparse(row);
    notifyRowChanged(row, {TextRole, RawTextRole, StatusRole, Qt::DisplayRole});
    return true;
}

bool ChatMessageListModel::markErrorKeepRendered(const QString& requestId, const QString& message)
{
    const QString rid = requestId.trimmed();
    if (rid.isEmpty()) {
        return false;
    }

    int row = findLastAssistantRowByRequestId(rid);
    if (row < 0 || row >= m_items.size()) {
        return false;
    }

    ChatMessageItem& item = m_items[row];
    if (item.rawText.trimmed().isEmpty() && !message.trimmed().isEmpty()) {
        item.rawText = message;
        item.text = message;
        scheduleAsyncReparse(row);
    }
    item.status = QStringLiteral("error");
    notifyRowChanged(row, {TextRole, RawTextRole, StatusRole, Qt::DisplayRole});
    return true;
}

bool ChatMessageListModel::beginAssistantMessage(const QString& requestId)
{
    const QString rid = requestId.trimmed();
    if (rid.isEmpty()) {
        return false;
    }

    if (findLastAssistantRowByRequestId(rid) >= 0) {
        return true;
    }

    ChatMessageItem item;
    item.id = QStringLiteral("assistant-%1").arg(QDateTime::currentMSecsSinceEpoch());
    item.role = QStringLiteral("assistant");
    item.messageType = QStringLiteral("text");
    item.requestId = rid;
    item.text = QString();
    item.rawText = QString();
    item.blocks.clear();
    item.status = QStringLiteral("streaming");
    item.timestamp = QDateTime::currentDateTime();
    appendMessage(item);
    return true;
}

bool ChatMessageListModel::appendAssistantChunk(const QString& requestId, const QString& delta)
{
    return appendRawDeltaAndReparse(requestId, delta);
}

bool ChatMessageListModel::finalizeAssistantMessage(const QString& requestId, const QString& content)
{
    const QString rid = requestId.trimmed();
    if (rid.isEmpty()) {
        return false;
    }

    int row = findLastAssistantRowByRequestId(rid);
    if (row < 0) {
        ChatMessageItem item;
        item.id = QStringLiteral("assistant-%1").arg(QDateTime::currentMSecsSinceEpoch());
        item.role = QStringLiteral("assistant");
        item.messageType = QStringLiteral("text");
        item.requestId = rid;
        item.text = content;
        item.rawText = content;
        item.blocks.clear();
        item.status = QStringLiteral("done");
        item.timestamp = QDateTime::currentDateTime();
        appendMessage(item);
        return true;
    }

    return setRawTextAndReparse(rid, content, QStringLiteral("done"));
}

bool ChatMessageListModel::markAssistantMessageError(const QString& requestId, const QString& message)
{
    return markErrorKeepRendered(requestId, message);
}

int ChatMessageListModel::findLastAssistantRowByRequestId(const QString& requestId) const
{
    const QString rid = requestId.trimmed();
    if (rid.isEmpty()) {
        return -1;
    }

    for (int i = m_items.size() - 1; i >= 0; --i) {
        const ChatMessageItem& item = m_items[i];
        if (item.role == QStringLiteral("assistant") && item.requestId == rid) {
            return i;
        }
    }
    return -1;
}

void ChatMessageListModel::notifyRowChanged(int row, const QVector<int>& roles)
{
    if (row < 0 || row >= m_items.size()) {
        return;
    }
    const QModelIndex rowIndex = index(row);
    emit dataChanged(rowIndex, rowIndex, roles);
}

void ChatMessageListModel::scheduleAsyncReparse(int row)
{
    if (row < 0 || row >= m_items.size()) {
        return;
    }

    ChatMessageItem& item = m_items[row];
    if (item.role == QStringLiteral("user")) {
        return;
    }

    if (item.rawText.trimmed().isEmpty()) {
        if (!item.blocks.isEmpty()) {
            item.blocks.clear();
            notifyRowChanged(row, {BlocksRole});
        }
        return;
    }

    item.parseRevision += 1;
    if (item.parseInFlight) {
        return;
    }
    if (!m_parseQueue.contains(row)) {
        m_parseQueue.append(row);
    }
    dispatchNextAsyncReparse();
}

void ChatMessageListModel::dispatchNextAsyncReparse()
{
    if (m_parseDispatching) {
        return;
    }

    while (!m_parseQueue.isEmpty()) {
        const int row = m_parseQueue.takeFirst();
        if (row < 0 || row >= m_items.size()) {
            continue;
        }
        ChatMessageItem& item = m_items[row];
        if (item.role == QStringLiteral("user")) {
            continue;
        }
        if (item.rawText.trimmed().isEmpty()) {
            continue;
        }
        if (item.parseInFlight) {
            continue;
        }

        m_parseDispatching = true;
        dispatchAsyncReparse(row, item.parseRevision, item.rawText);
        return;
    }
}

void ChatMessageListModel::applyParsedBlocks(int row,
                                             quint64 parseRevision,
                                             const QString& rawTextSnapshot,
                                             const QVariantList& blocks)
{
    if (row < 0 || row >= m_items.size()) {
        return;
    }

    ChatMessageItem& item = m_items[row];
    if (item.parseInFlight && item.parseInFlightRevision == parseRevision) {
        item.parseInFlight = false;
    }
    m_parseDispatching = false;

    const bool isLatest = (item.parseRevision == parseRevision) && (item.rawText == rawTextSnapshot);
    if (isLatest) {
        item.blocks = blocks;
        notifyRowChanged(row, {BlocksRole});
    }

    if (!item.parseInFlight && item.parseRevision > parseRevision && !item.rawText.trimmed().isEmpty()) {
        if (!m_parseQueue.contains(row)) {
            m_parseQueue.append(row);
        }
    }
    dispatchNextAsyncReparse();
}

void ChatMessageListModel::dispatchAsyncReparse(int row,
                                                quint64 parseRevision,
                                                const QString& rawTextSnapshot)
{
    if (row < 0 || row >= m_items.size()) {
        return;
    }

    ChatMessageItem& item = m_items[row];
    item.parseInFlight = true;
    item.parseInFlightRevision = parseRevision;
    QPointer<ChatMessageListModel> self(this);

    QtConcurrent::run([self, row, parseRevision, rawTextSnapshot]() {
        MarkdownMessageParser parser;
        const QVariantList blocks = parser.parse(rawTextSnapshot);
        if (!self) {
            return;
        }

        QMetaObject::invokeMethod(self.data(),
                                  [self, row, parseRevision, rawTextSnapshot, blocks]() {
            if (!self) {
                return;
            }
            self->applyParsedBlocks(row, parseRevision, rawTextSnapshot, blocks);
        },
                                  Qt::QueuedConnection);
    });
}

QString ChatMessageListModel::roleText(const QString& role)
{
    if (role == QStringLiteral("user")) {
        return QStringLiteral("我");
    }
    if (role == QStringLiteral("assistant")) {
        return QStringLiteral("AI 助手");
    }
    if (role == QStringLiteral("error")) {
        return QStringLiteral("错误");
    }
    if (role == QStringLiteral("system")) {
        return QStringLiteral("系统");
    }
    return QStringLiteral("消息");
}

