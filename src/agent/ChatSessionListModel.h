#ifndef CHAT_SESSION_LIST_MODEL_H
#define CHAT_SESSION_LIST_MODEL_H

#include <QAbstractListModel>
#include <QVector>

#include "ChatSessionItem.h"

class ChatSessionListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        SessionIdRole = Qt::UserRole + 1,
        TitleRole,
        CreatedAtRole,
        UpdatedAtRole,
        LastPreviewRole,
        MessageCountRole,
        SelectedRole,
        UpdatedAtTextRole
    };
    Q_ENUM(Roles)

public:
    explicit ChatSessionListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void clear();
    void setSessions(const QVector<ChatSessionItem>& sessions);
    void upsertSession(const ChatSessionItem& session, bool prependIfMissing = false);
    bool removeSession(const QString& sessionId);
    bool containsSession(const QString& sessionId) const;
    int rowOfSession(const QString& sessionId) const;
    ChatSessionItem sessionAt(int row) const;
    ChatSessionItem sessionById(const QString& sessionId) const;
    void setSelectedSession(const QString& sessionId);
    QString selectedSessionId() const;

private:
    QString formatUpdatedAt(const QDateTime& dt) const;

private:
    QVector<ChatSessionItem> m_items;
};

#endif // CHAT_SESSION_LIST_MODEL_H
