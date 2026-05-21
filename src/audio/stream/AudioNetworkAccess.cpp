#include "AudioNetworkAccess.h"

#include "network/curl_network_transport.h"

#include <QRegularExpression>
#include <QtDebug>

namespace {
constexpr int kProbeTimeoutMs = 5000;
constexpr int kRetryCount = 2;
constexpr qint64 kMinPartialReturnBytes = 128LL * 1024;

QString headerValue(const QMap<QString, QString>& headers, const QString& name) {
    for (auto it = headers.cbegin(); it != headers.cend(); ++it) {
        if (it.key().compare(name, Qt::CaseInsensitive) == 0) {
            return it.value();
        }
    }
    return QString();
}

qint64 headerContentLength(const QMap<QString, QString>& headers) {
    const QString valueText = headerValue(headers, QStringLiteral("Content-Length"));
    if (valueText.isEmpty()) {
        return -1;
    }
    bool ok = false;
    const qint64 value = valueText.toLongLong(&ok);
    return ok ? value : -1;
}
} // namespace

void AudioNetworkAccess::setInterruptCallback(InterruptCallback callback) {
    m_interruptCallback = std::move(callback);
}

bool AudioNetworkAccess::open(const QUrl& url) {
    m_url = url;
    m_contentLength = -1;
    m_canSeek = false;
    m_lastError.clear();

    if (!m_url.isValid() || m_url.isEmpty()) {
        m_lastError = QStringLiteral("Invalid audio URL");
        return false;
    }

    Response head = sendRequest("HEAD", QByteArray(), kProbeTimeoutMs);
    if (head.ok) {
        m_contentLength = head.contentLength;
        m_canSeek = QString::fromLatin1(head.acceptRanges).toLower().contains(QStringLiteral("bytes"));
        if (m_contentLength > 0) {
            qDebug() << "[AudioNetworkAccess] Opened stream from HEAD:"
                     << "url=" << m_url.toString() << "length=" << m_contentLength
                     << "canSeek=" << m_canSeek;
            return true;
        }
    }

    RangeResult probe = fetchRange(0, 1, kProbeTimeoutMs);
    if (probe.ok) {
        if (probe.totalLength > 0) {
            m_contentLength = probe.totalLength;
        } else if (m_contentLength < 0 && probe.data.size() == 1) {
            m_contentLength = qMax<qint64>(m_contentLength, 1);
        }
        m_canSeek = probe.statusCode == 206 || m_canSeek;
        qDebug() << "[AudioNetworkAccess] Opened range stream:"
                 << "url=" << m_url.toString() << "length=" << m_contentLength
                 << "canSeek=" << m_canSeek;
        return true;
    }

    if (head.ok && m_contentLength > 0) {
        qWarning() << "[AudioNetworkAccess] Range probe failed, sequential open only:"
                   << "url=" << m_url.toString() << "error=" << probe.error;
        return true;
    }

    m_lastError = probe.error.isEmpty() ? head.error : probe.error;
    return false;
}

AudioNetworkAccess::RangeResult AudioNetworkAccess::fetchRange(qint64 offset, qint64 maxBytes,
                                                               int timeoutMs,
                                                               qint64 minReturnBytes) {
    RangeResult result;
    if (offset < 0) {
        result.error = QStringLiteral("Negative range offset");
        return result;
    }
    if (m_contentLength >= 0 && offset >= m_contentLength) {
        result.ok = true;
        result.eof = true;
        result.totalLength = m_contentLength;
        result.responseStart = offset;
        return result;
    }

    QByteArray range = "bytes=" + QByteArray::number(offset) + "-";
    if (maxBytes > 0) {
        range += QByteArray::number(offset + maxBytes - 1);
    }

    for (int attempt = 0; attempt <= kRetryCount; ++attempt) {
        const qint64 effectiveMinReturnBytes =
            minReturnBytes > 0
                ? qMin(minReturnBytes, maxBytes > 0 ? maxBytes : minReturnBytes)
                : (maxBytes > 0 ? qMin(maxBytes, kMinPartialReturnBytes)
                                : kMinPartialReturnBytes);
        Response response =
            sendRequest("GET", range, timeoutMs + attempt * 500, effectiveMinReturnBytes);
        result.statusCode = response.statusCode;
        result.interrupted = response.interrupted;
        result.partial = response.partial;
        result.timedOut = response.timedOut;
        if (response.interrupted) {
            result.error = QStringLiteral("Interrupted");
            return result;
        }
        if (!response.ok) {
            result.error = response.error;
            if (attempt < kRetryCount &&
                (response.statusCode == 408 || response.statusCode == 429 ||
                 response.statusCode == 500 || response.statusCode == 502 ||
                 response.statusCode == 503 || response.statusCode == 504 ||
                 response.statusCode == 0)) {
                qWarning() << "[AudioNetworkAccess] Range retry:"
                           << "attempt=" << attempt + 1 << "range=" << range
                           << "status=" << response.statusCode << "error=" << response.error;
                continue;
            }
            return result;
        }

        qint64 rangeStart = -1;
        qint64 rangeEnd = -1;
        qint64 rangeTotal = -1;
        const bool hasContentRange =
            parseContentRange(response.contentRange, &rangeStart, &rangeEnd, &rangeTotal);
        if (response.statusCode == 206) {
            if (!hasContentRange || rangeStart != offset) {
                result.error = QStringLiteral("Content-Range mismatch");
                qWarning() << "[AudioNetworkAccess] range_validated=false"
                           << "requested=" << offset << "contentRange=" << response.contentRange;
                return result;
            }
            m_canSeek = true;
            if (rangeTotal > 0) {
                m_contentLength = rangeTotal;
            }
            result.responseStart = rangeStart;
        } else if (response.statusCode == 200) {
            if (offset != 0) {
                result.error = QStringLiteral("Server ignored Range request");
                qWarning() << "[AudioNetworkAccess] range_validated=false"
                           << "requested=" << offset << "status=200";
                return result;
            }
            result.responseStart = 0;
            if (response.contentLength > 0) {
                m_contentLength = response.contentLength;
            }
        } else if (response.statusCode == 416) {
            result.ok = true;
            result.eof = true;
            result.responseStart = offset;
            result.totalLength = m_contentLength;
            return result;
        }

        result.ok = true;
        result.data = response.body;
        result.totalLength = m_contentLength;
        qDebug() << "[AudioNetworkAccess] range_validated=true"
                 << "offset=" << offset << "bytes=" << result.data.size()
                 << "status=" << response.statusCode << "total=" << m_contentLength
                 << "partial=" << result.partial << "timedOut=" << result.timedOut;
        return result;
    }

    result.error = QStringLiteral("Range request retry budget exhausted");
    return result;
}

AudioNetworkAccess::Response AudioNetworkAccess::sendRequest(const QByteArray& method,
                                                             const QByteArray& rangeHeader,
                                                             int timeoutMs,
                                                             qint64 minReturnBytes) {
    Response response;
    if (shouldAbort()) {
        response.interrupted = true;
        return response;
    }

    CurlNetworkTransport::Request request;
    request.url = m_url.toString(QUrl::FullyEncoded);
    request.method = QString::fromLatin1(method);
    request.timeoutMs = qMax(500, timeoutMs);
    request.userAgent = QStringLiteral("FFmpegMusicPlayer/2.0");
    request.headers.insert(QStringLiteral("Accept"), QStringLiteral("*/*"));
    request.headers.insert(QStringLiteral("Connection"), QStringLiteral("keep-alive"));
    if (!rangeHeader.isEmpty()) {
        request.headers.insert(QStringLiteral("Range"), QString::fromLatin1(rangeHeader));
    }
    request.shouldAbort = [this]() {
        return shouldAbort();
    };
    if (minReturnBytes > 0 && method != "HEAD") {
        request.onData = [&response, minReturnBytes](const QByteArray& chunk) {
            response.body.append(chunk);
            return response.body.size() < minReturnBytes;
        };
    }

    const CurlNetworkTransport::Response curlResponse = CurlNetworkTransport::perform(request);
    response.statusCode = curlResponse.statusCode;
    response.acceptRanges =
        headerValue(curlResponse.headers, QStringLiteral("Accept-Ranges")).toLatin1();
    response.contentRange =
        headerValue(curlResponse.headers, QStringLiteral("Content-Range")).toLatin1();
    response.contentLength = headerContentLength(curlResponse.headers);
    response.body = curlResponse.body;
    response.interrupted = curlResponse.aborted || shouldAbort();
    response.timedOut = curlResponse.timedOut;

    if (!response.interrupted && curlResponse.error == CurlNetworkTransport::Error::FileError &&
        minReturnBytes > 0 && response.body.size() >= minReturnBytes) {
        response.partial = true;
        response.ok = response.statusCode == 200 || response.statusCode == 206 ||
                      response.statusCode == 0;
        qDebug() << "[AudioNetworkAccess] range_partial=true"
                 << "range=" << rangeHeader << "bytes=" << response.body.size()
                 << "status=" << response.statusCode;
    } else if (!response.interrupted &&
               curlResponse.error == CurlNetworkTransport::Error::NoError) {
        response.ok = response.statusCode == 200 || response.statusCode == 206 ||
                      response.statusCode == 416 || response.statusCode == 0;
    } else if (!response.interrupted) {
        response.error = curlResponse.errorString;
        if (response.error.isEmpty()) {
            response.error = response.timedOut ? QStringLiteral("Range request timed out")
                                               : QStringLiteral("HTTP request failed");
        }
    }

    if (!response.ok && response.error.isEmpty() && !response.interrupted) {
        response.error = QStringLiteral("HTTP status %1").arg(response.statusCode);
    }

    return response;
}

bool AudioNetworkAccess::shouldAbort() const {
    return m_interruptCallback && m_interruptCallback();
}

bool AudioNetworkAccess::parseContentRange(const QByteArray& value, qint64* start, qint64* end,
                                           qint64* total) const {
    if (!start || !end || !total || value.isEmpty()) {
        return false;
    }

    static const QRegularExpression re(
        QStringLiteral(R"(^\s*bytes\s+(\d+)-(\d+)/(\d+|\*)\s*$)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = re.match(QString::fromLatin1(value));
    if (!match.hasMatch()) {
        return false;
    }

    bool okStart = false;
    bool okEnd = false;
    *start = match.captured(1).toLongLong(&okStart);
    *end = match.captured(2).toLongLong(&okEnd);
    if (match.captured(3) == QStringLiteral("*")) {
        *total = -1;
    } else {
        bool okTotal = false;
        *total = match.captured(3).toLongLong(&okTotal);
        if (!okTotal) {
            return false;
        }
    }
    return okStart && okEnd;
}
