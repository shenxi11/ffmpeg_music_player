#include "network_manager.h"

#include "curl_network_transport.h"

#include <QDebug>
#include <QHostInfo>
#include <QMetaObject>
#include <QMutexLocker>
#include <QThreadPool>
#include <QtConcurrent>
#include <QUuid>

using namespace Network;

namespace {

NetworkError mapTransportError(CurlNetworkTransport::Error error) {
    switch (error) {
        case CurlNetworkTransport::Error::NoError:
            return NetworkError::NoError;
        case CurlNetworkTransport::Error::InvalidRequest:
            return NetworkError::InvalidRequest;
        case CurlNetworkTransport::Error::Timeout:
            return NetworkError::TimeoutError;
        case CurlNetworkTransport::Error::Aborted:
            return NetworkError::AbortedError;
        case CurlNetworkTransport::Error::FileError:
            return NetworkError::FileError;
        case CurlNetworkTransport::Error::CurlError:
            return NetworkError::TransportError;
    }
    return NetworkError::UnknownError;
}

bool shouldRetryResponse(const NetworkResponse& response) {
    if (response.error == NetworkError::NoError) {
        return false;
    }
    if (response.error == NetworkError::AbortedError) {
        return false;
    }
    return response.statusCode < 400 || response.statusCode >= 500;
}

} // namespace

Network::NetworkManager& Network::NetworkManager::instance() {
    static NetworkManager instance;
    return instance;
}

Network::NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {
    qDebug() << "[NetworkManager] Initialized with libcurl transport"
             << "available=" << CurlNetworkTransport::isAvailable();
}

Network::NetworkManager::~NetworkManager() {
    cancelAllRequests();
}

QString Network::NetworkManager::sendRequest(const QString& url, HttpMethod method,
                                             const QByteArray& data, const RequestOptions& options,
                                             ResponseCallback callback,
                                             ProgressCallback progressCallback) {
    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    auto context = std::make_shared<RequestContext>();
    context->requestId = requestId;
    context->url = url;
    context->method = method;
    context->data = data;
    context->options = options;
    context->callback = std::move(callback);
    context->progressCallback = std::move(progressCallback);
    context->cancelled = std::make_shared<std::atomic_bool>(false);
    context->elapsedTimer = new QElapsedTimer();
    context->elapsedTimer->start();

    {
        QMutexLocker locker(&m_mutex);
        m_activeRequests.insert(requestId, context);
    }

    qDebug() << "[NetworkManager] Request started:" << requestId << httpMethodToString(method)
             << url;
    emit requestStarted(requestId, url);

    executeRequest(context);
    return requestId;
}

void Network::NetworkManager::executeRequest(std::shared_ptr<RequestContext> context) {
    if (!context || context->completed || context->cancelled->load()) {
        return;
    }

    CurlNetworkTransport::Request request;
    request.url = context->url;
    request.method = httpMethodToString(context->method);
    request.body = context->data;
    request.headers = createHeaders(context->options);
    request.timeoutMs = context->options.timeout > 0 ? context->options.timeout : m_globalTimeout;
    request.followRedirects = context->options.followRedirects;
    request.maxRedirects = context->options.maxRedirects;
    request.enableCompression = context->options.enableCompression;
    request.shouldAbort = [cancelled = context->cancelled]() {
        return cancelled && cancelled->load();
    };

    const QString requestId = context->requestId;
    const auto progressCallback = context->progressCallback;
    request.onDownloadProgress = [this, requestId, progressCallback](qint64 received,
                                                                      qint64 total) {
        QMetaObject::invokeMethod(this, [this, requestId, progressCallback, received, total]() {
            if (progressCallback) {
                progressCallback(received, total);
            }
            emit requestProgress(requestId, received, total);
        }, Qt::QueuedConnection);
    };

    QtConcurrent::run(QThreadPool::globalInstance(), [this, requestId, request]() {
        const CurlNetworkTransport::Response curlResponse = CurlNetworkTransport::perform(request);

        NetworkResponse response;
        response.requestId = requestId;
        response.statusCode = curlResponse.statusCode;
        response.body = curlResponse.body;
        response.headers = curlResponse.headers;
        response.error = mapTransportError(curlResponse.error);
        response.errorString = curlResponse.errorString;
        response.elapsedMs = curlResponse.elapsedMs;

        QMetaObject::invokeMethod(this, [this, requestId, response]() {
            handleCurlFinished(requestId, response);
        }, Qt::QueuedConnection);
    });
}

void Network::NetworkManager::handleCurlFinished(const QString& requestId,
                                                 const NetworkResponse& response) {
    std::shared_ptr<RequestContext> context;
    {
        QMutexLocker locker(&m_mutex);
        context = m_activeRequests.value(requestId);
    }
    if (!context || context->completed) {
        return;
    }

    cleanupTimers(context.get());
    NetworkResponse finalResponse = response;
    finalResponse.requestId = requestId;
    if (context->cancelled->load()) {
        finalResponse = makeCancelledResponse(*context);
    }

    qDebug() << "[NetworkManager] Request finished:" << requestId
             << "Status:" << finalResponse.statusCode << "Elapsed:" << finalResponse.elapsedMs
             << "ms Size:" << finalResponse.body.size() << "bytes"
             << "Retry:" << context->currentRetry
             << "Error:" << static_cast<int>(finalResponse.error)
             << finalResponse.errorString;

    if (shouldRetryResponse(finalResponse) &&
        context->currentRetry < context->options.maxRetries) {
        qDebug() << "[NetworkManager] Request failed, will retry:" << requestId
                 << "Error:" << finalResponse.errorString << "Retry"
                 << (context->currentRetry + 1) << "of" << context->options.maxRetries;
        retryRequest(context);
        return;
    }

    updateStats(finalResponse);

    if (context->callback) {
        context->callback(finalResponse);
    }
    emit requestFinished(requestId, finalResponse);
    if (finalResponse.error != NetworkError::NoError) {
        emit requestFailed(requestId, finalResponse.errorString);
    }

    context->completed = true;
    cleanupElapsedTimer(context.get());
    removeActiveRequest(context);
}

void Network::NetworkManager::retryRequest(std::shared_ptr<RequestContext> context) {
    if (!context || context->completed || context->cancelled->load()) {
        return;
    }

    context->currentRetry++;
    m_stats.totalRetries++;

    const int delay = calculateRetryDelay(context->currentRetry, context->options);
    qDebug() << "[NetworkManager] Retrying request in" << delay << "ms:" << context->requestId;

    if (context->retryTimer) {
        context->retryTimer->stop();
        context->retryTimer->deleteLater();
        context->retryTimer = nullptr;
    }

    context->retryTimer = new QTimer(this);
    context->retryTimer->setSingleShot(true);
    context->retryTimer->setProperty("requestId", context->requestId);
    connect(context->retryTimer, &QTimer::timeout, this, &NetworkManager::onRetryTimerTimeout);
    context->retryTimer->start(delay);
}

void Network::NetworkManager::onRetryTimerTimeout() {
    auto* retryTimer = qobject_cast<QTimer*>(sender());
    if (!retryTimer) {
        return;
    }

    const QString requestId = retryTimer->property("requestId").toString();
    std::shared_ptr<RequestContext> context;
    {
        QMutexLocker locker(&m_mutex);
        context = m_activeRequests.value(requestId);
    }
    if (!context) {
        retryTimer->deleteLater();
        return;
    }

    if (context->retryTimer == retryTimer) {
        context->retryTimer = nullptr;
    }
    retryTimer->deleteLater();

    if (context->elapsedTimer) {
        context->elapsedTimer->restart();
    }
    executeRequest(context);
}

int Network::NetworkManager::calculateRetryDelay(int retry, const RequestOptions& options) {
    if (!options.exponentialBackoff) {
        return options.retryDelay;
    }
    return qMin(options.retryDelay * (1 << retry), 30000);
}

void Network::NetworkManager::cancelRequest(const QString& requestId) {
    std::shared_ptr<RequestContext> context;
    {
        QMutexLocker locker(&m_mutex);
        auto it = m_activeRequests.find(requestId);
        if (it != m_activeRequests.end()) {
            context = it.value();
            m_activeRequests.erase(it);
        }
    }
    if (!context) {
        return;
    }

    context->completed = true;
    context->cancelled->store(true);
    cleanupTimers(context.get());
    cleanupElapsedTimer(context.get());
    qDebug() << "[NetworkManager] Request cancelled:" << requestId;
}

void Network::NetworkManager::cancelAllRequests() {
    QList<std::shared_ptr<RequestContext>> contexts;
    {
        QMutexLocker locker(&m_mutex);
        contexts = m_activeRequests.values();
        m_activeRequests.clear();
    }

    for (auto& context : contexts) {
        if (!context) {
            continue;
        }
        context->completed = true;
        context->cancelled->store(true);
        cleanupTimers(context.get());
        cleanupElapsedTimer(context.get());
    }
    qDebug() << "[NetworkManager] All requests cancelled";
}

void Network::NetworkManager::cleanupTimers(RequestContext* context) {
    if (!context) {
        return;
    }
    if (context->retryTimer) {
        context->retryTimer->stop();
        context->retryTimer->deleteLater();
        context->retryTimer = nullptr;
    }
}

void Network::NetworkManager::cleanupElapsedTimer(RequestContext* context) {
    if (!context || !context->elapsedTimer) {
        return;
    }
    delete context->elapsedTimer;
    context->elapsedTimer = nullptr;
}

void Network::NetworkManager::removeActiveRequest(
    const std::shared_ptr<RequestContext>& context) {
    if (!context) {
        return;
    }
    QMutexLocker locker(&m_mutex);
    m_activeRequests.remove(context->requestId);
}

RequestStats Network::NetworkManager::getStats() const {
    QMutexLocker locker(&m_mutex);
    return m_stats;
}

void Network::NetworkManager::resetStats() {
    QMutexLocker locker(&m_mutex);
    m_stats = RequestStats();
    qDebug() << "[NetworkManager] Stats reset";
}

void Network::NetworkManager::setGlobalTimeout(int timeout) {
    m_globalTimeout = timeout;
    qDebug() << "[NetworkManager] Global timeout set to:" << timeout << "ms";
}

void Network::NetworkManager::setMaxConnections(int maxConnections) {
    m_maxConnections = maxConnections;
    QThreadPool::globalInstance()->setMaxThreadCount(qMax(1, maxConnections));
    qDebug() << "[NetworkManager] Max connections set to:" << maxConnections;
}

void Network::NetworkManager::prewarmDns(const QString& hostname) {
    QHostInfo::lookupHost(hostname, this, [hostname](const QHostInfo& info) {
        if (info.error() == QHostInfo::NoError) {
            qDebug() << "[NetworkManager] DNS prewarmed for:" << hostname
                     << "Addresses:" << info.addresses();
        } else {
            qWarning() << "[NetworkManager] DNS prewarm failed for:" << hostname
                       << "Error:" << info.errorString();
        }
    });
}

QMap<QString, QString> Network::NetworkManager::createHeaders(
    const RequestOptions& options) const {
    QMap<QString, QString> headers = options.headers;
    if (!headers.contains(QStringLiteral("Content-Type")) &&
        !headers.contains(QStringLiteral("content-type"))) {
        headers.insert(QStringLiteral("Content-Type"), QStringLiteral("application/json"));
    }
    headers.insert(QStringLiteral("Connection"), QStringLiteral("keep-alive"));
    headers.insert(QStringLiteral("Keep-Alive"), QStringLiteral("timeout=300, max=1000"));
    if (options.enableCompression) {
        headers.insert(QStringLiteral("Accept-Encoding"), QStringLiteral("gzip, deflate"));
    }
    return headers;
}

QString Network::NetworkManager::httpMethodToString(HttpMethod method) const {
    switch (method) {
        case HttpMethod::GET:
            return QStringLiteral("GET");
        case HttpMethod::POST:
            return QStringLiteral("POST");
        case HttpMethod::PUT:
            return QStringLiteral("PUT");
        case HttpMethod::DELETE_METHOD:
            return QStringLiteral("DELETE");
        case HttpMethod::PATCH:
            return QStringLiteral("PATCH");
        case HttpMethod::HEAD_METHOD:
            return QStringLiteral("HEAD");
    }
    return QStringLiteral("GET");
}

NetworkResponse Network::NetworkManager::makeCancelledResponse(
    const RequestContext& context) const {
    NetworkResponse response;
    response.requestId = context.requestId;
    response.error = NetworkError::AbortedError;
    response.errorString = QStringLiteral("Request cancelled");
    response.elapsedMs = context.elapsedTimer ? context.elapsedTimer->elapsed() : 0;
    return response;
}

void Network::NetworkManager::updateStats(const NetworkResponse& response) {
    QMutexLocker locker(&m_mutex);

    m_stats.totalRequests++;
    if (response.isSuccess()) {
        m_stats.successfulRequests++;
    } else {
        m_stats.failedRequests++;
    }
    if (response.isFromCache) {
        m_stats.cachedRequests++;
    }
    m_stats.totalBytes += response.body.size();
    if (m_stats.totalRequests > 0) {
        m_stats.avgResponseTime =
            (m_stats.avgResponseTime * (m_stats.totalRequests - 1) + response.elapsedMs) /
            m_stats.totalRequests;
    }

    emit statsUpdated(m_stats);
}
