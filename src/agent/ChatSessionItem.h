#ifndef CHAT_SESSION_ITEM_H
#define CHAT_SESSION_ITEM_H

#include <QDateTime>
#include <QString>

struct ChatSessionItem
{
    QString sessionId;
    QString title;
    QDateTime createdAt;
    QDateTime updatedAt;
    QString lastPreview;
    int messageCount = 0;
    bool selected = false;
};

#endif // CHAT_SESSION_ITEM_H
