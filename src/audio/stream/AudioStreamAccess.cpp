#include "AudioStreamAccess.h"

#include <QtDebug>

AudioStreamAccess::AudioStreamAccess() {
    m_network.setInterruptCallback([this]() {
        return shouldAbort();
    });
}

void AudioStreamAccess::setInterruptCallback(InterruptCallback callback) {
    m_interruptCallback = std::move(callback);
}

bool AudioStreamAccess::open(const QUrl& url) {
    m_lastError.clear();
    m_abortRequested = false;
    if (!m_network.open(url)) {
        m_lastError = m_network.lastError();
        return false;
    }

    qDebug() << "[STREAM_EVT][AudioStreamAccess] opened"
             << "url=" << url.toString() << "length=" << contentLength()
             << "canSeek=" << canSeek();
    return true;
}

AudioStreamAccess::ReadResult AudioStreamAccess::readAt(qint64 offset, qint64 maxBytes,
                                                        int timeoutMs, qint64 minReturnBytes) {
    ReadResult result;
    if (offset < 0 || maxBytes <= 0) {
        result.error = QStringLiteral("Invalid stream read range");
        return result;
    }

    m_abortRequested = false;
    AudioNetworkAccess::RangeResult range =
        m_network.fetchRange(offset, maxBytes, timeoutMs, qMin(maxBytes, minReturnBytes));

    result.ok = range.ok;
    result.eof = range.eof;
    result.interrupted = range.interrupted;
    result.statusCode = range.statusCode;
    result.responseStart = range.responseStart;
    result.totalLength = range.totalLength;
    result.data = std::move(range.data);
    result.error = std::move(range.error);
    if (!result.ok && !result.interrupted) {
        m_lastError = result.error;
    }
    return result;
}

void AudioStreamAccess::abortCurrentRequest() {
    m_abortRequested = true;
}

bool AudioStreamAccess::shouldAbort() const {
    return m_abortRequested.load() || (m_interruptCallback && m_interruptCallback());
}
