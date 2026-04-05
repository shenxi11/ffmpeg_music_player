#ifndef AUDIO_SESSION_H
#define AUDIO_SESSION_H

#include "headers.h"
#include "audio_ringbuffer.h"

class AudioSession : public QObject
{
    Q_OBJECT
public:
    explicit AudioSession(const QString& sessionId, QObject* parent = nullptr)
        : QObject(parent),
        m_sessionId(sessionId),
        m_buffer(new AudioRingBuffer(64 * 1024, 10 * 1024 * 1024)){}

    QString id() const;
    bool isActive() const;

private:
    QString m_sessionId;
    QUrl m_url;
    bool m_active = false;

    AudioRingBuffer* m_buffer;
};

#endif // AUDIO_SESSION_H
