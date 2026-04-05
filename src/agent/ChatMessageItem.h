#ifndef CHAT_MESSAGE_ITEM_H
#define CHAT_MESSAGE_ITEM_H

#include <QDateTime>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QtGlobal>

/**
 * @brief 聊天消息实体。
 */
struct ChatMessageItem
{
    QString id;
    QString role;       // user / assistant / system / error
    QString messageType; // text / plan_preview / approval_request / clarification / progress / final_result
    QString text;
    QString rawText;    // 服务端原始 Markdown
    QVariantList blocks; // 解析后的渲染块
    QVariantMap meta;
    QString requestId;
    QDateTime timestamp;
    QString status;     // pending / sent / done / error
    quint64 parseRevision = 0;
    bool parseInFlight = false;
    quint64 parseInFlightRevision = 0;
};

#endif // CHAT_MESSAGE_ITEM_H
