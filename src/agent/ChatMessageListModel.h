#ifndef CHAT_MESSAGE_LIST_MODEL_H
#define CHAT_MESSAGE_LIST_MODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QVector>
#include <QtGlobal>

#include "ChatMessageItem.h"

/**
 * @brief 聊天消息列表模型，供 QWidget/QML 统一复用。
 */
class ChatMessageListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        RoleRole,
        MessageTypeRole,
        MetaRole,
        TextRole,
        RawTextRole,
        BlocksRole,
        RequestIdRole,
        TimestampRole,
        StatusRole
    };
    Q_ENUM(Roles)

public:
    explicit ChatMessageListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void appendMessage(const ChatMessageItem& item);
    void clear();
    QVariantList dumpMessages() const;
    void loadMessages(const QVariantList& messages);
    void updateMessageStatusByRequestId(const QString& requestId, const QString& status);
    bool setRawTextAndReparse(const QString& requestId, const QString& rawText, const QString& status);
    bool appendRawDeltaAndReparse(const QString& requestId, const QString& delta);
    bool markErrorKeepRendered(const QString& requestId, const QString& message);
    bool beginAssistantMessage(const QString& requestId);
    bool appendAssistantChunk(const QString& requestId, const QString& delta);
    bool finalizeAssistantMessage(const QString& requestId, const QString& content);
    bool markAssistantMessageError(const QString& requestId, const QString& message);

private:
    static QString roleText(const QString& role);
    int findLastAssistantRowByRequestId(const QString& requestId) const;
    void notifyRowChanged(int row, const QVector<int>& roles);
    void scheduleAsyncReparse(int row);
    void dispatchNextAsyncReparse();
    void dispatchAsyncReparse(int row, quint64 parseRevision, const QString& rawTextSnapshot);
    void applyParsedBlocks(int row,
                           quint64 parseRevision,
                           const QString& rawTextSnapshot,
                           const QVariantList& blocks);

private:
    QVector<ChatMessageItem> m_items;
    QList<int> m_parseQueue;
    bool m_parseDispatching = false;
};

#endif // CHAT_MESSAGE_LIST_MODEL_H
