#include "ChatSessionListModel.h"

ChatSessionListModel::ChatSessionListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ChatSessionListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_items.size();
}

QVariant ChatSessionListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return QVariant();
    }

    const ChatSessionItem& item = m_items.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return item.title;
    case SessionIdRole:
        return item.sessionId;
    case TitleRole:
        return item.title;
    case CreatedAtRole:
        return item.createdAt;
    case UpdatedAtRole:
        return item.updatedAt;
    case LastPreviewRole:
        return item.lastPreview;
    case MessageCountRole:
        return item.messageCount;
    case SelectedRole:
        return item.selected;
    case UpdatedAtTextRole:
        return formatUpdatedAt(item.updatedAt);
    default:
        break;
    }
    return QVariant();
}

QHash<int, QByteArray> ChatSessionListModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names.insert(SessionIdRole, "sessionId");
    names.insert(TitleRole, "title");
    names.insert(CreatedAtRole, "createdAt");
    names.insert(UpdatedAtRole, "updatedAt");
    names.insert(LastPreviewRole, "lastPreview");
    names.insert(MessageCountRole, "messageCount");
    names.insert(SelectedRole, "selected");
    names.insert(UpdatedAtTextRole, "updatedAtText");
    return names;
}

void ChatSessionListModel::clear()
{
    if (m_items.isEmpty()) {
        return;
    }
    beginResetModel();
    m_items.clear();
    endResetModel();
}

void ChatSessionListModel::setSessions(const QVector<ChatSessionItem>& sessions)
{
    beginResetModel();
    m_items = sessions;
    endResetModel();
}

void ChatSessionListModel::upsertSession(const ChatSessionItem& session, bool prependIfMissing)
{
    const int row = rowOfSession(session.sessionId);
    if (row >= 0) {
        ChatSessionItem merged = session;
        merged.selected = m_items.at(row).selected || session.selected;
        m_items[row] = merged;
        const QModelIndex idx = index(row);
        emit dataChanged(idx, idx);
        return;
    }

    const int insertRow = prependIfMissing ? 0 : m_items.size();
    beginInsertRows(QModelIndex(), insertRow, insertRow);
    m_items.insert(insertRow, session);
    endInsertRows();
}

bool ChatSessionListModel::removeSession(const QString& sessionId)
{
    const int row = rowOfSession(sessionId);
    if (row < 0) {
        return false;
    }
    beginRemoveRows(QModelIndex(), row, row);
    m_items.removeAt(row);
    endRemoveRows();
    return true;
}

bool ChatSessionListModel::containsSession(const QString& sessionId) const
{
    return rowOfSession(sessionId) >= 0;
}

int ChatSessionListModel::rowOfSession(const QString& sessionId) const
{
    const QString sid = sessionId.trimmed();
    if (sid.isEmpty()) {
        return -1;
    }

    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).sessionId == sid) {
            return i;
        }
    }
    return -1;
}

ChatSessionItem ChatSessionListModel::sessionAt(int row) const
{
    if (row < 0 || row >= m_items.size()) {
        return ChatSessionItem{};
    }
    return m_items.at(row);
}

ChatSessionItem ChatSessionListModel::sessionById(const QString& sessionId) const
{
    const int row = rowOfSession(sessionId);
    if (row < 0) {
        return ChatSessionItem{};
    }
    return m_items.at(row);
}

void ChatSessionListModel::setSelectedSession(const QString& sessionId)
{
    const QString sid = sessionId.trimmed();
    QVector<int> changedRows;
    changedRows.reserve(m_items.size());

    for (int i = 0; i < m_items.size(); ++i) {
        ChatSessionItem& item = m_items[i];
        const bool expected = (!sid.isEmpty() && item.sessionId == sid);
        if (item.selected == expected) {
            continue;
        }
        item.selected = expected;
        changedRows.append(i);
    }

    for (int row : changedRows) {
        const QModelIndex idx = index(row);
        emit dataChanged(idx, idx, {SelectedRole});
    }
}

QString ChatSessionListModel::selectedSessionId() const
{
    for (const ChatSessionItem& item : m_items) {
        if (item.selected) {
            return item.sessionId;
        }
    }
    return QString();
}

QString ChatSessionListModel::formatUpdatedAt(const QDateTime& dt) const
{
    if (!dt.isValid()) {
        return QString();
    }
    return dt.toLocalTime().toString(QStringLiteral("MM-dd HH:mm"));
}
