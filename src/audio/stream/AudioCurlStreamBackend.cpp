#include "AudioCurlStreamBackend.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QtDebug>

#ifdef CLOUDMUSIC_AUDIO_USE_LIBCURL
#include <curl/curl.h>
#endif

namespace {
#ifdef CLOUDMUSIC_AUDIO_USE_LIBCURL
struct CurlGlobalInit {
    CurlGlobalInit() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobalInit() { curl_global_cleanup(); }
};

CurlGlobalInit& curlGlobalInit() {
    static CurlGlobalInit init;
    return init;
}
#endif

bool parseContentRangeHeader(const QByteArray& headers, qint64* start, qint64* total) {
    if (!start || !total || headers.isEmpty()) {
        return false;
    }

    static const QRegularExpression re(
        QStringLiteral(R"((?im)^\s*Content-Range:\s*bytes\s+(\d+)-(\d+)/(\d+|\*)\s*$)"));
    const QRegularExpressionMatch match = re.match(QString::fromLatin1(headers));
    if (!match.hasMatch()) {
        return false;
    }

    bool okStart = false;
    *start = match.captured(1).toLongLong(&okStart);
    if (match.captured(3) == QStringLiteral("*")) {
        *total = -1;
        return okStart;
    }

    bool okTotal = false;
    *total = match.captured(3).toLongLong(&okTotal);
    return okStart && okTotal;
}

qint64 parseContentLengthHeader(const QByteArray& headers) {
    static const QRegularExpression re(QStringLiteral(R"((?im)^\s*Content-Length:\s*(\d+)\s*$)"));
    const QRegularExpressionMatch match = re.match(QString::fromLatin1(headers));
    if (!match.hasMatch()) {
        return -1;
    }

    bool ok = false;
    const qint64 value = match.captured(1).toLongLong(&ok);
    return ok ? value : -1;
}

bool parseAcceptRangesHeader(const QByteArray& headers) {
    static const QRegularExpression re(
        QStringLiteral(R"((?im)^\s*Accept-Ranges:\s*([^\r\n]+)\s*$)"));
    const QRegularExpressionMatch match = re.match(QString::fromLatin1(headers));
    return match.hasMatch() &&
           match.captured(1).contains(QStringLiteral("bytes"), Qt::CaseInsensitive);
}
} // namespace

AudioCurlStreamBackend::AudioCurlStreamBackend() = default;

AudioCurlStreamBackend::~AudioCurlStreamBackend() {
#ifdef CLOUDMUSIC_AUDIO_USE_LIBCURL
    if (m_easy) {
        curl_easy_cleanup(static_cast<CURL*>(m_easy));
        m_easy = nullptr;
    }
#endif
}

bool AudioCurlStreamBackend::isAvailable() {
#ifdef CLOUDMUSIC_AUDIO_USE_LIBCURL
    return true;
#else
    return false;
#endif
}

void AudioCurlStreamBackend::setInterruptCallback(InterruptCallback callback) {
    m_interruptCallback = std::move(callback);
}

bool AudioCurlStreamBackend::open(const QUrl& url) {
    m_url = url;
    m_contentLength = -1;
    m_canSeek = true;
    m_lastError.clear();
    m_abortRequested = false;

    if (!m_url.isValid() || m_url.isEmpty()) {
        m_lastError = QStringLiteral("Invalid audio URL");
        return false;
    }

#ifdef CLOUDMUSIC_AUDIO_USE_LIBCURL
    curlGlobalInit();
    if (!m_easy) {
        m_easy = curl_easy_init();
    }
    if (!m_easy) {
        m_lastError = QStringLiteral("Failed to initialize libcurl");
        return false;
    }
    if (!configureCommonOptions()) {
        return false;
    }
    qDebug() << "[STREAM_EVT][AudioCurlStreamBackend] opened"
             << "url=" << m_url.toString();
    return true;
#else
    m_lastError = QStringLiteral("libcurl backend is not compiled");
    return false;
#endif
}

AudioCurlStreamBackend::Response AudioCurlStreamBackend::fetchFrom(qint64 offset,
                                                                   const DataCallback& onData) {
    Response response;
    if (offset < 0) {
        response.error = QStringLiteral("Negative range offset");
        return response;
    }
    if (m_contentLength > 0 && offset >= m_contentLength) {
        response.ok = true;
        response.eof = true;
        response.responseStart = offset;
        response.totalLength = m_contentLength;
        return response;
    }
    if (shouldAbort()) {
        response.interrupted = true;
        response.error = QStringLiteral("Interrupted");
        return response;
    }

#ifdef CLOUDMUSIC_AUDIO_USE_LIBCURL
    CURL* easy = static_cast<CURL*>(m_easy);
    if (!easy && !open(m_url)) {
        response.error = m_lastError;
        return response;
    }
    easy = static_cast<CURL*>(m_easy);

    m_abortRequested = false;
    m_headerBuffer.clear();
    m_requestedOffset = offset;
    m_activeResponse = &response;
    m_activeDataCallback = onData;

    m_rangeSpec = QByteArray::number(offset) + "-";
    curl_easy_setopt(easy, CURLOPT_RANGE, m_rangeSpec.constData());
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, &AudioCurlStreamBackend::writeCallback);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, &AudioCurlStreamBackend::headerCallback);
    curl_easy_setopt(easy, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(easy, CURLOPT_XFERINFOFUNCTION, &AudioCurlStreamBackend::progressCallback);
    curl_easy_setopt(easy, CURLOPT_XFERINFODATA, this);

    const qint64 startedAt = QDateTime::currentMSecsSinceEpoch();
    const CURLcode code = curl_easy_perform(easy);
    long status = 0;
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
    response.statusCode = static_cast<int>(status);
    response.interrupted = shouldAbort() || code == CURLE_ABORTED_BY_CALLBACK;

    if (!parseHeaders(&response)) {
        response.ok = false;
    }

    if (response.interrupted) {
        response.error = QStringLiteral("Interrupted");
    } else if (code != CURLE_OK) {
        response.error = QString::fromLatin1(curl_easy_strerror(code));
        response.ok = false;
    }

    m_activeResponse = nullptr;
    m_activeDataCallback = nullptr;

    if (!response.ok && !response.interrupted) {
        m_lastError = response.error;
    }
    qDebug() << "[STREAM_EVT][AudioCurlStreamBackend] fetch_done"
             << "offset=" << offset << "status=" << response.statusCode
             << "elapsedMs=" << (QDateTime::currentMSecsSinceEpoch() - startedAt)
             << "ok=" << response.ok << "interrupted=" << response.interrupted
             << "total=" << response.totalLength;
    return response;
#else
    Q_UNUSED(onData);
    response.error = QStringLiteral("libcurl backend is not compiled");
    return response;
#endif
}

void AudioCurlStreamBackend::abort() {
    m_abortRequested = true;
}

void AudioCurlStreamBackend::clearAbort() {
    m_abortRequested = false;
}

bool AudioCurlStreamBackend::shouldAbort() const {
    return m_abortRequested.load() || (m_interruptCallback && m_interruptCallback());
}

#ifdef CLOUDMUSIC_AUDIO_USE_LIBCURL
size_t AudioCurlStreamBackend::writeCallback(char* ptr, size_t size, size_t nmemb,
                                             void* userdata) {
    auto* self = static_cast<AudioCurlStreamBackend*>(userdata);
    const size_t byteCount = size * nmemb;
    if (!self || byteCount == 0) {
        return 0;
    }
    if (self->shouldAbort()) {
        return 0;
    }

    QByteArray chunk(ptr, static_cast<int>(byteCount));
    if (self->m_activeDataCallback && !self->m_activeDataCallback(chunk)) {
        return 0;
    }
    return byteCount;
}

size_t AudioCurlStreamBackend::headerCallback(char* ptr, size_t size, size_t nmemb,
                                              void* userdata) {
    auto* self = static_cast<AudioCurlStreamBackend*>(userdata);
    const size_t byteCount = size * nmemb;
    if (!self || byteCount == 0) {
        return 0;
    }
    self->m_headerBuffer.append(ptr, static_cast<int>(byteCount));
    return byteCount;
}

int AudioCurlStreamBackend::progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                             curl_off_t ultotal, curl_off_t ulnow) {
    Q_UNUSED(dltotal);
    Q_UNUSED(dlnow);
    Q_UNUSED(ultotal);
    Q_UNUSED(ulnow);
    auto* self = static_cast<AudioCurlStreamBackend*>(clientp);
    return self && self->shouldAbort() ? 1 : 0;
}

bool AudioCurlStreamBackend::configureCommonOptions() {
    CURL* easy = static_cast<CURL*>(m_easy);
    if (!easy) {
        m_lastError = QStringLiteral("Invalid libcurl handle");
        return false;
    }

    m_encodedUrl = m_url.toString(QUrl::FullyEncoded).toUtf8();
    curl_easy_setopt(easy, CURLOPT_URL, m_encodedUrl.constData());
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy, CURLOPT_USERAGENT, "FFmpegMusicPlayer/2.0");
    curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(easy, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(easy, CURLOPT_TCP_NODELAY, 1L);
    curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, 3500L);
    curl_easy_setopt(easy, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(easy, CURLOPT_LOW_SPEED_TIME, 10L);
    curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    return true;
}

bool AudioCurlStreamBackend::parseHeaders(Response* response) {
    if (!response) {
        return false;
    }

    qint64 rangeStart = -1;
    qint64 totalLength = -1;
    const bool hasContentRange =
        parseContentRangeHeader(m_headerBuffer, &rangeStart, &totalLength);
    if (response->statusCode == 206) {
        if (!hasContentRange || rangeStart != m_requestedOffset) {
            response->error = QStringLiteral("Content-Range mismatch");
            qWarning() << "[STREAM_EVT][AudioCurlStreamBackend] header_invalid"
                       << "requested=" << m_requestedOffset
                       << "headers=" << m_headerBuffer.left(200);
            return false;
        }
        response->ok = true;
        response->responseStart = rangeStart;
        response->totalLength = totalLength;
        if (totalLength > 0) {
            m_contentLength = totalLength;
            m_canSeek = true;
        }
        return true;
    }

    if (response->statusCode == 200) {
        if (m_requestedOffset != 0) {
            response->error = QStringLiteral("Server ignored Range request");
            return false;
        }
        response->ok = true;
        response->responseStart = 0;
        m_canSeek = parseAcceptRangesHeader(m_headerBuffer);
        const qint64 contentLength = parseContentLengthHeader(m_headerBuffer);
        if (contentLength > 0) {
            m_contentLength = contentLength;
            response->totalLength = contentLength;
        }
        return true;
    }

    if (response->statusCode == 416) {
        response->ok = true;
        response->eof = true;
        response->responseStart = m_requestedOffset;
        response->totalLength = m_contentLength;
        return true;
    }

    response->error = QStringLiteral("HTTP status %1").arg(response->statusCode);
    return false;
}
#endif
