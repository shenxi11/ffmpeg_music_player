#include "curl_network_transport.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QUrl>
#include <QtDebug>

#ifdef CLOUDMUSIC_AUDIO_USE_LIBCURL
#include <curl/curl.h>
#endif

namespace {

#ifdef CLOUDMUSIC_AUDIO_USE_LIBCURL
struct CurlGlobalInitializer {
    CurlGlobalInitializer() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobalInitializer() { curl_global_cleanup(); }
};

CurlGlobalInitializer& curlGlobalInitializer() {
    static CurlGlobalInitializer initializer;
    return initializer;
}

struct CurlRequestState {
    const CurlNetworkTransport::Request* request = nullptr;
    CurlNetworkTransport::Response* response = nullptr;
    bool headersDelivered = false;
};

QByteArray encodedUrlForCurl(const QString& urlText) {
    const QUrl url(urlText);
    if (url.isValid() && !url.isEmpty()) {
        return url.toString(QUrl::FullyEncoded).toUtf8();
    }
    return urlText.toUtf8();
}

size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* state = static_cast<CurlRequestState*>(userdata);
    const size_t byteCount = size * nmemb;
    if (!state || !state->request || !state->response || byteCount == 0) {
        return 0;
    }
    if (state->request->shouldAbort && state->request->shouldAbort()) {
        return 0;
    }
    if (!state->headersDelivered && state->request->onHeaders) {
        state->headersDelivered = true;
        state->request->onHeaders(state->response->statusCode, state->response->headers);
    }

    const QByteArray chunk(ptr, static_cast<int>(byteCount));
    if (state->request->onData) {
        return state->request->onData(chunk) ? byteCount : 0;
    }

    state->response->body.append(chunk);
    return byteCount;
}

size_t headerCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* state = static_cast<CurlRequestState*>(userdata);
    const size_t byteCount = size * nmemb;
    if (!state || !state->response || byteCount == 0) {
        return 0;
    }

    const QByteArray line(ptr, static_cast<int>(byteCount));
    if (line.startsWith("HTTP/")) {
        state->response->headers.clear();
        state->headersDelivered = false;
        const QList<QByteArray> parts = line.trimmed().split(' ');
        if (parts.size() >= 2) {
            bool ok = false;
            const int statusCode = parts.at(1).toInt(&ok);
            if (ok) {
                state->response->statusCode = statusCode;
            }
        }
    }
    state->response->rawHeaders.append(line);

    const int colon = line.indexOf(':');
    if (colon > 0) {
        const QString key = QString::fromLatin1(line.left(colon)).trimmed();
        const QString value = QString::fromLatin1(line.mid(colon + 1)).trimmed();
        if (!key.isEmpty()) {
            state->response->headers.insert(key, value);
        }
    }
    return byteCount;
}

int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
                     curl_off_t ulnow) {
    auto* state = static_cast<CurlRequestState*>(clientp);
    if (!state || !state->request) {
        return 0;
    }
    if (state->request->shouldAbort && state->request->shouldAbort()) {
        return 1;
    }
    if (state->request->onDownloadProgress) {
        state->request->onDownloadProgress(static_cast<qint64>(dlnow),
                                           static_cast<qint64>(dltotal));
    }
    if (state->request->onUploadProgress) {
        state->request->onUploadProgress(static_cast<qint64>(ulnow),
                                         static_cast<qint64>(ultotal));
    }
    return 0;
}

void applyMethod(CURL* easy, const CurlNetworkTransport::Request& request) {
    const QString method = request.method.trimmed().toUpper();
    if (!request.multipartParts.isEmpty()) {
        curl_easy_setopt(easy, CURLOPT_POST, 1L);
        return;
    }
    if (method == QStringLiteral("GET")) {
        curl_easy_setopt(easy, CURLOPT_HTTPGET, 1L);
        return;
    }
    if (method == QStringLiteral("HEAD")) {
        curl_easy_setopt(easy, CURLOPT_NOBODY, 1L);
        return;
    }
    if (method == QStringLiteral("POST")) {
        curl_easy_setopt(easy, CURLOPT_POST, 1L);
        curl_easy_setopt(easy, CURLOPT_POSTFIELDS, request.body.constData());
        curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE, static_cast<long>(request.body.size()));
        return;
    }

    const QByteArray customMethod = method.toLatin1();
    curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, customMethod.constData());
    if (!request.body.isEmpty()) {
        curl_easy_setopt(easy, CURLOPT_POSTFIELDS, request.body.constData());
        curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE, static_cast<long>(request.body.size()));
    }
}

curl_mime* buildMultipart(CURL* easy, const CurlNetworkTransport::Request& request) {
    if (request.multipartParts.isEmpty()) {
        return nullptr;
    }

    curl_mime* mime = curl_mime_init(easy);
    for (const CurlNetworkTransport::MultipartPart& item : request.multipartParts) {
        curl_mimepart* part = curl_mime_addpart(mime);
        curl_mime_name(part, item.name.toUtf8().constData());
        if (!item.contentType.isEmpty()) {
            curl_mime_type(part, item.contentType.toUtf8().constData());
        }
        if (item.isFile) {
            curl_mime_filedata(part, item.filePath.toUtf8().constData());
            const QString fileName =
                item.fileName.isEmpty() ? QFileInfo(item.filePath).fileName() : item.fileName;
            if (!fileName.isEmpty()) {
                curl_mime_filename(part, fileName.toUtf8().constData());
            }
        } else {
            curl_mime_data(part, item.data.constData(), static_cast<size_t>(item.data.size()));
        }
    }
    return mime;
}

CurlNetworkTransport::Error mapCurlError(CURLcode code) {
    if (code == CURLE_OK) {
        return CurlNetworkTransport::Error::NoError;
    }
    if (code == CURLE_OPERATION_TIMEDOUT) {
        return CurlNetworkTransport::Error::Timeout;
    }
    if (code == CURLE_ABORTED_BY_CALLBACK) {
        return CurlNetworkTransport::Error::Aborted;
    }
    if (code == CURLE_READ_ERROR || code == CURLE_WRITE_ERROR) {
        return CurlNetworkTransport::Error::FileError;
    }
    return CurlNetworkTransport::Error::CurlError;
}
#endif

} // namespace

bool CurlNetworkTransport::isAvailable() {
#ifdef CLOUDMUSIC_AUDIO_USE_LIBCURL
    return true;
#else
    return false;
#endif
}

CurlNetworkTransport::Response CurlNetworkTransport::perform(const Request& request) {
    Response response;
    if (request.url.trimmed().isEmpty()) {
        response.error = Error::InvalidRequest;
        response.errorString = QStringLiteral("Empty URL");
        return response;
    }
    if (request.shouldAbort && request.shouldAbort()) {
        response.error = Error::Aborted;
        response.aborted = true;
        response.errorString = QStringLiteral("Request aborted");
        return response;
    }

#ifndef CLOUDMUSIC_AUDIO_USE_LIBCURL
    response.error = Error::CurlError;
    response.errorString = QStringLiteral("libcurl backend is not compiled");
    return response;
#else
    curlGlobalInitializer();
    CURL* easy = curl_easy_init();
    if (!easy) {
        response.error = Error::CurlError;
        response.errorString = QStringLiteral("Failed to initialize libcurl");
        return response;
    }

    CurlRequestState state{&request, &response};
    const QByteArray url = encodedUrlForCurl(request.url);
    const QByteArray userAgent = request.userAgent.toUtf8();
    struct curl_slist* headerList = nullptr;
    curl_mime* mime = nullptr;

    curl_easy_setopt(easy, CURLOPT_URL, url.constData());
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, &writeCallback);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &state);
    curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, &headerCallback);
    curl_easy_setopt(easy, CURLOPT_HEADERDATA, &state);
    curl_easy_setopt(easy, CURLOPT_XFERINFOFUNCTION, &progressCallback);
    curl_easy_setopt(easy, CURLOPT_XFERINFODATA, &state);
    curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(easy, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(easy, CURLOPT_TCP_NODELAY, 1L);
    curl_easy_setopt(easy, CURLOPT_USERAGENT, userAgent.constData());
    curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, qMax(1000, request.timeoutMs / 3));
    curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, qMax(1000, request.timeoutMs));
    curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    if (request.followRedirects) {
        curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(easy, CURLOPT_MAXREDIRS, qMax(0, request.maxRedirects));
    }
    if (request.enableCompression) {
        curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "");
    }

    for (auto it = request.headers.cbegin(); it != request.headers.cend(); ++it) {
        const QByteArray header = it.key().toUtf8() + ": " + it.value().toUtf8();
        headerList = curl_slist_append(headerList, header.constData());
    }
    if (headerList) {
        curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headerList);
    }

    mime = buildMultipart(easy, request);
    if (mime) {
        curl_easy_setopt(easy, CURLOPT_MIMEPOST, mime);
    }
    applyMethod(easy, request);

    QElapsedTimer timer;
    timer.start();
    const CURLcode code = curl_easy_perform(easy);
    response.elapsedMs = timer.elapsed();

    long status = 0;
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
    response.statusCode = static_cast<int>(status);
    response.error = mapCurlError(code);
    response.timedOut = response.error == Error::Timeout;
    response.aborted = response.error == Error::Aborted ||
                       (request.shouldAbort && request.shouldAbort());
    if (response.error != Error::NoError) {
        response.errorString = response.aborted ? QStringLiteral("Request aborted")
                                                : QString::fromLatin1(curl_easy_strerror(code));
    }

    if (mime) {
        curl_mime_free(mime);
    }
    if (headerList) {
        curl_slist_free_all(headerList);
    }
    curl_easy_cleanup(easy);

    return response;
#endif
}
