#include "network_manager.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHostInfo>
#include <QDebug>
#include <QUuid>
#include <QThread>

using namespace Network;

Network::NetworkManager& Network::NetworkManager::instance()
{
    static NetworkManager instance;
    return instance;
}

Network::NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
{
    m_manager = new QNetworkAccessManager(this);
    
    // 閰嶇疆缃戠粶绠＄悊
    m_manager->setTransferTimeout(m_globalTimeout);
    
    // 鍚敤HTTP/2锛堝鏋淨t鐗堟湰鏀寔
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    m_manager->setProperty("HTTP2Enabled", true);
#endif
    
    qDebug() << "[NetworkManager] Initialized with HTTP/2 support";
}

Network::NetworkManager::~NetworkManager()
{
    cancelAllRequests();
}

QString Network::NetworkManager::sendRequest(
    const QString& url,
    HttpMethod method,
    const QByteArray& data,
    const RequestOptions& options,
    ResponseCallback callback,
    ProgressCallback progressCallback)
{
    // 鐢熸垚鍞竴璇锋眰ID
    QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // 鍒涘缓璇锋眰涓婁笅
    auto context = std::make_shared<RequestContext>();
    context->requestId = requestId;
    context->url = url;
    context->method = method;
    context->data = data;
    context->options = options;
    context->callback = callback;
    context->progressCallback = progressCallback;
    context->currentRetry = 0;
    context->elapsedTimer = new QElapsedTimer();
    context->elapsedTimer->start();
    
    {
        QMutexLocker locker(&m_mutex);
        m_activeRequests[requestId] = context;
    }
    
    qDebug() << "[NetworkManager] Request started:" << requestId << httpMethodToString(method) << url;
    emit requestStarted(requestId, url);
    
    // 鎵ц璇锋眰
    executeRequest(context);
    
    return requestId;
}

void Network::NetworkManager::executeRequest(std::shared_ptr<RequestContext> context)
{
    QNetworkRequest request = createRequest(context->url, context->options);
    
    // 鏍规嵁HTTP鏂规硶鍙戦€佽
    QNetworkReply* reply = nullptr;
    
    switch (context->method) {
        case HttpMethod::GET:
            reply = m_manager->get(request);
            break;
            
        case HttpMethod::POST:
            reply = m_manager->post(request, context->data);
            break;
            
        case HttpMethod::PUT:
            reply = m_manager->put(request, context->data);
            break;
            
        case HttpMethod::DELETE_METHOD:
            reply = m_manager->deleteResource(request);
            break;
            
        case HttpMethod::PATCH:
            reply = m_manager->sendCustomRequest(request, "PATCH", context->data);
            break;
            
        case HttpMethod::HEAD_METHOD:
            reply = m_manager->head(request);
            break;
    }
    
    if (!reply) {
        qWarning() << "[NetworkManager] Failed to create reply for request:" << context->requestId;
        
        NetworkResponse response;
        response.requestId = context->requestId;
        response.error = QNetworkReply::UnknownNetworkError;
        response.errorString = "Failed to create network reply";
        
        if (context->callback) {
            context->callback(response);
        }
        
        QMutexLocker locker(&m_mutex);
        m_activeRequests.remove(context->requestId);
        return;
    }
    
    context->reply = reply;
    
    // 杩炴帴瀹屾垚淇″彿
    connect(reply, &QNetworkReply::finished, this, [this, context]() {
        handleReplyFinished(context);
    });
    
    // 杩炴帴杩涘害淇″彿
    if (context->progressCallback) {
        connect(reply, &QNetworkReply::downloadProgress, this, 
                [this, context](qint64 bytesReceived, qint64 bytesTotal) {
            if (context->progressCallback) {
                context->progressCallback(bytesReceived, bytesTotal);
            }
            emit requestProgress(context->requestId, bytesReceived, bytesTotal);
        });
    }
    
    // 璁剧疆瓒呮椂瀹氭椂
    int timeout = context->options.timeout > 0 ? context->options.timeout : m_globalTimeout;
    context->timeoutTimer = new QTimer();
    context->timeoutTimer->setSingleShot(true);
    connect(context->timeoutTimer, &QTimer::timeout, this, [this, context]() {
        handleTimeout(context);
    });
    context->timeoutTimer->start(timeout);
}

void Network::NetworkManager::handleReplyFinished(std::shared_ptr<RequestContext> context)
{
    if (!context->reply) {
        qWarning() << "[NetworkManager] Reply is null in handleReplyFinished";
        return;
    }
    
    // 鍋滄瓒呮椂瀹氭椂
    if (context->timeoutTimer) {
        context->timeoutTimer->stop();
        context->timeoutTimer->deleteLater();
        context->timeoutTimer = nullptr;
    }
    
    QNetworkReply* reply = context->reply;
    
    // 鏋勫缓鍝嶅簲
    NetworkResponse response;
    response.requestId = context->requestId;
    response.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    response.body = reply->readAll();
    response.error = reply->error();
    response.errorString = reply->errorString();
    response.elapsedMs = context->elapsedTimer->elapsed();
    
    // 鑾峰彇鍝嶅簲
    const auto headerList = reply->rawHeaderPairs();
    for (const auto& pair : headerList) {
        response.headers[QString::fromUtf8(pair.first)] = QString::fromUtf8(pair.second);
    }
    
    qDebug() << "[NetworkManager] Request finished:" << context->requestId 
             << "Status:" << response.statusCode
             << "Elapsed:" << response.elapsedMs << "ms"
             << "Size:" << response.body.size() << "bytes"
             << "Retry:" << context->currentRetry;
    
    // 妫€鏌ユ槸鍚﹂渶瑕侀噸
    const bool isClientError = response.statusCode >= 400 && response.statusCode < 500;
    if (response.error != QNetworkReply::NoError &&
        !isClientError &&
        context->currentRetry < context->options.maxRetries) {
        
        qDebug() << "[NetworkManager] Request failed, will retry:" << context->requestId
                 << "Error:" << response.errorString
                 << "Retry" << (context->currentRetry + 1) << "of" << context->options.maxRetries;
        
        reply->deleteLater();
        context->reply = nullptr;
        
        retryRequest(context);
        return;
    }
    
    // 鏇存柊缁熻淇℃伅
    updateStats(response);
    
    // 璋冪敤鍥炶皟
    if (context->callback) {
        context->callback(response);
    }
    
    emit requestFinished(context->requestId, response);
    
    if (response.error != QNetworkReply::NoError) {
        emit requestFailed(context->requestId, response.errorString);
    }
    
    // 娓呯悊
    reply->deleteLater();
    delete context->elapsedTimer;
    
    {
        QMutexLocker locker(&m_mutex);
        m_activeRequests.remove(context->requestId);
    }
}

void Network::NetworkManager::handleTimeout(std::shared_ptr<RequestContext> context)
{
    qWarning() << "[NetworkManager] Request timeout:" << context->requestId;
    
    if (context->reply) {
        context->reply->abort();
    }
    
    // 妫€鏌ユ槸鍚﹂渶瑕侀噸
    if (context->currentRetry < context->options.maxRetries) {
        qDebug() << "[NetworkManager] Timeout, will retry:" << context->requestId
                 << "Retry" << (context->currentRetry + 1) << "of" << context->options.maxRetries;
        
        retryRequest(context);
    } else {
        NetworkResponse response;
        response.requestId = context->requestId;
        response.error = QNetworkReply::TimeoutError;
        response.errorString = "Request timeout";
        response.elapsedMs = context->elapsedTimer->elapsed();
        
        updateStats(response);
        
        if (context->callback) {
            context->callback(response);
        }
        
        emit requestFailed(context->requestId, "Request timeout");
        
        if (context->reply) {
            context->reply->deleteLater();
        }
        delete context->elapsedTimer;
        
        QMutexLocker locker(&m_mutex);
        m_activeRequests.remove(context->requestId);
    }
}

void Network::NetworkManager::retryRequest(std::shared_ptr<RequestContext> context)
{
    context->currentRetry++;
    m_stats.totalRetries++;
    
    int delay = calculateRetryDelay(context->currentRetry, context->options);
    
    qDebug() << "[NetworkManager] Retrying request in" << delay << "ms:" << context->requestId;
    
    // 浣跨敤瀹氭椂鍣ㄥ欢杩熼噸
    QTimer::singleShot(delay, this, [this, context]() {
        context->elapsedTimer->restart();
        executeRequest(context);
    });
}

int Network::NetworkManager::calculateRetryDelay(int retry, const RequestOptions& options)
{
    if (!options.exponentialBackoff) {
        return options.retryDelay;
    }
    
    // 鎸囨暟閫€閬匡細delay * (2 ^ retry)
    int delay = options.retryDelay * (1 << retry);
    
    // 闄愬埗鏈€澶у欢杩熶负30
    return qMin(delay, 30000);
}

void Network::NetworkManager::cancelRequest(const QString& requestId)
{
    QMutexLocker locker(&m_mutex);
    
    auto it = m_activeRequests.find(requestId);
    if (it != m_activeRequests.end()) {
        auto context = it.value();
        
        if (context->reply) {
            context->reply->abort();
            context->reply->deleteLater();
        }
        
        if (context->timeoutTimer) {
            context->timeoutTimer->stop();
            context->timeoutTimer->deleteLater();
        }
        
        delete context->elapsedTimer;
        
        m_activeRequests.erase(it);
        
        qDebug() << "[NetworkManager] Request cancelled:" << requestId;
    }
}

void Network::NetworkManager::cancelAllRequests()
{
    QMutexLocker locker(&m_mutex);
    
    for (auto& context : m_activeRequests) {
        if (context->reply) {
            context->reply->abort();
            context->reply->deleteLater();
        }
        
        if (context->timeoutTimer) {
            context->timeoutTimer->stop();
            context->timeoutTimer->deleteLater();
        }
        
        delete context->elapsedTimer;
    }
    
    m_activeRequests.clear();
    
    qDebug() << "[NetworkManager] All requests cancelled";
}

RequestStats Network::NetworkManager::getStats() const
{
    QMutexLocker locker(&m_mutex);
    return m_stats;
}

void Network::NetworkManager::resetStats()
{
    QMutexLocker locker(&m_mutex);
    m_stats = RequestStats();
    qDebug() << "[NetworkManager] Stats reset";
}

void Network::NetworkManager::setGlobalTimeout(int timeout)
{
    m_globalTimeout = timeout;
    m_manager->setTransferTimeout(timeout);
    qDebug() << "[NetworkManager] Global timeout set to:" << timeout << "ms";
}

void Network::NetworkManager::setMaxConnections(int maxConnections)
{
    m_maxConnections = maxConnections;
    // Qt鐨凲NetworkAccessManager浼氳嚜鍔ㄧ鐞嗚繛鎺ユ睜
    qDebug() << "[NetworkManager] Max connections set to:" << maxConnections;
}

void Network::NetworkManager::prewarmDns(const QString& hostname)
{
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

QNetworkRequest Network::NetworkManager::createRequest(const QString& url, const RequestOptions& options)
{
    QUrl qurl(url);
    QNetworkRequest request(qurl);
    
    // 璁剧疆榛樿璇锋眰
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Keep-Alive", "timeout=300, max=1000");
    request.setRawHeader("User-Agent", "FFmpeg-Music-Player/1.0");
    
    // 鍚敤鍘嬬缉
    if (options.enableCompression) {
        request.setRawHeader("Accept-Encoding", "gzip, deflate");
    }
    
    // 璁剧疆鑷畾涔夎姹傚ご
    for (auto it = options.headers.begin(); it != options.headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
    
    // 閲嶅畾鍚戠瓥
    if (options.followRedirects) {
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, 
                           QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setMaximumRedirectsAllowed(options.maxRedirects);
    }
    
    return request;
}

QString Network::NetworkManager::httpMethodToString(HttpMethod method) const
{
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE_METHOD: return "DELETE";
        case HttpMethod::PATCH: return "PATCH";
        case HttpMethod::HEAD_METHOD: return "HEAD";
        default: return "UNKNOWN";
    }
}

void Network::NetworkManager::updateStats(const NetworkResponse& response)
{
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
    
    // 鏇存柊骞冲潎鍝嶅簲鏃堕棿
    if (m_stats.totalRequests > 0) {
        m_stats.avgResponseTime = (m_stats.avgResponseTime * (m_stats.totalRequests - 1) + response.elapsedMs) 
                                 / m_stats.totalRequests;
    }
    
    emit statsUpdated(m_stats);
}




