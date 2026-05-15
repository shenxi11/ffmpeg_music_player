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
    
    // 配置网络管理器。
    m_manager->setTransferTimeout(m_globalTimeout);
    
    // 启用 HTTP/2（若 Qt 版本支持）
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
    // 生成唯一请求 ID。
    QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // 创建请求上下文。
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
    
    // 执行请求。
    executeRequest(context);
    
    return requestId;
}

void Network::NetworkManager::executeRequest(std::shared_ptr<RequestContext> context)
{
    if (!context || context->completed) {
        return;
    }

    QNetworkRequest request = createRequest(context->url, context->options);
    
    // 根据 HTTP 方法发送请求
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
        
        context->completed = true;
        cleanupElapsedTimer(context.get());
        removeActiveRequest(context);
        return;
    }
    
    context->reply = reply;
    reply->setProperty("requestId", context->requestId);
    
    // 连接完成信号。
    connect(reply, &QNetworkReply::finished, this, &NetworkManager::onReplyFinished);
    
    // 连接进度信号。
    if (context->progressCallback) {
        connect(reply, &QNetworkReply::downloadProgress,
                this, &NetworkManager::onReplyDownloadProgress);
    }
    
    // 设置超时定时器
    int timeout = context->options.timeout > 0 ? context->options.timeout : m_globalTimeout;
    context->timeoutTimer = new QTimer(this);
    context->timeoutTimer->setSingleShot(true);
    context->timeoutTimer->setProperty("requestId", context->requestId);
    connect(context->timeoutTimer, &QTimer::timeout,
            this, &NetworkManager::onRequestTimeout);
    context->timeoutTimer->start(timeout);
}

void Network::NetworkManager::onReplyFinished()
{
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    const QString requestId = reply->property("requestId").toString();
    if (requestId.isEmpty()) {
        qWarning() << "[NetworkManager] Missing requestId on reply";
        return;
    }

    std::shared_ptr<RequestContext> context;
    {
        QMutexLocker locker(&m_mutex);
        context = m_activeRequests.value(requestId);
    }
    if (!context) {
        qWarning() << "[NetworkManager] Context not found for finished reply:" << requestId;
        return;
    }

    handleReplyFinished(context);
}

void Network::NetworkManager::onReplyDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    const QString requestId = reply->property("requestId").toString();
    if (requestId.isEmpty()) {
        return;
    }

    std::shared_ptr<RequestContext> context;
    {
        QMutexLocker locker(&m_mutex);
        context = m_activeRequests.value(requestId);
    }
    if (!context) {
        return;
    }

    if (context->progressCallback) {
        context->progressCallback(bytesReceived, bytesTotal);
    }
    emit requestProgress(context->requestId, bytesReceived, bytesTotal);
}

void Network::NetworkManager::onRequestTimeout()
{
    auto* timer = qobject_cast<QTimer*>(sender());
    if (!timer) {
        return;
    }

    const QString requestId = timer->property("requestId").toString();
    if (requestId.isEmpty()) {
        return;
    }

    std::shared_ptr<RequestContext> context;
    {
        QMutexLocker locker(&m_mutex);
        context = m_activeRequests.value(requestId);
    }
    if (!context) {
        return;
    }

    handleTimeout(context);
}

void Network::NetworkManager::handleReplyFinished(std::shared_ptr<RequestContext> context)
{
    if (!context || context->completed) {
        return;
    }

    QNetworkReply* reply = detachReply(context.get());
    if (!reply) {
        qWarning() << "[NetworkManager] Reply is null in handleReplyFinished";
        return;
    }

    cleanupTimers(context.get());
    
    // 构建响应对象。
    NetworkResponse response;
    response.requestId = context->requestId;
    response.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    response.body = reply->readAll();
    response.error = reply->error();
    response.errorString = reply->errorString();
    response.elapsedMs = context->elapsedTimer ? context->elapsedTimer->elapsed() : 0;
    
    // 读取响应头与状态信息
    const auto headerList = reply->rawHeaderPairs();
    for (const auto& pair : headerList) {
        response.headers[QString::fromUtf8(pair.first)] = QString::fromUtf8(pair.second);
    }
    
    qDebug() << "[NetworkManager] Request finished:" << context->requestId 
             << "Status:" << response.statusCode
             << "Elapsed:" << response.elapsedMs << "ms"
             << "Size:" << response.body.size() << "bytes"
             << "Retry:" << context->currentRetry;
    
    // 检查是否需要重试
    const bool isClientError = response.statusCode >= 400 && response.statusCode < 500;
    if (response.error != QNetworkReply::NoError &&
        !isClientError &&
        context->currentRetry < context->options.maxRetries) {
        
        qDebug() << "[NetworkManager] Request failed, will retry:" << context->requestId
                 << "Error:" << response.errorString
                 << "Retry" << (context->currentRetry + 1) << "of" << context->options.maxRetries;
        
        disposeReply(reply, false);
        
        retryRequest(context);
        return;
    }
    
    // 更新统计信息
    updateStats(response);
    
    // 调用回调
    if (context->callback) {
        context->callback(response);
    }
    
    emit requestFinished(context->requestId, response);
    
    if (response.error != QNetworkReply::NoError) {
        emit requestFailed(context->requestId, response.errorString);
    }
    
    // 清理请求上下文
    context->completed = true;
    disposeReply(reply, false);
    cleanupElapsedTimer(context.get());
    removeActiveRequest(context);
}

void Network::NetworkManager::handleTimeout(std::shared_ptr<RequestContext> context)
{
    if (!context || context->completed) {
        return;
    }

    qWarning() << "[NetworkManager] Request timeout:" << context->requestId;

    // abort() 会触发 finished；先断开并脱离上下文，避免 finished 路径重复处理同一 reply。
    QNetworkReply* reply = detachReply(context.get());
    disposeReply(reply, true);
    cleanupTimers(context.get());
    
    // 检查超时后是否需要重试
    if (context->currentRetry < context->options.maxRetries) {
        qDebug() << "[NetworkManager] Timeout, will retry:" << context->requestId
                 << "Retry" << (context->currentRetry + 1) << "of" << context->options.maxRetries;
        
        retryRequest(context);
    } else {
        NetworkResponse response;
        response.requestId = context->requestId;
        response.error = QNetworkReply::TimeoutError;
        response.errorString = "Request timeout";
        response.elapsedMs = context->elapsedTimer ? context->elapsedTimer->elapsed() : 0;
        
        updateStats(response);
        
        if (context->callback) {
            context->callback(response);
        }
        
        emit requestFailed(context->requestId, "Request timeout");
        
        context->completed = true;
        cleanupElapsedTimer(context.get());
        removeActiveRequest(context);
    }
}

void Network::NetworkManager::retryRequest(std::shared_ptr<RequestContext> context)
{
    context->currentRetry++;
    m_stats.totalRetries++;
    
    int delay = calculateRetryDelay(context->currentRetry, context->options);
    
    qDebug() << "[NetworkManager] Retrying request in" << delay << "ms:" << context->requestId;

    if (context->retryTimer) {
        context->retryTimer->stop();
        context->retryTimer->deleteLater();
        context->retryTimer = nullptr;
    }

    context->retryTimer = new QTimer(this);
    context->retryTimer->setSingleShot(true);
    context->retryTimer->setProperty("requestId", context->requestId);
    connect(context->retryTimer, &QTimer::timeout,
            this, &NetworkManager::onRetryTimerTimeout);
    context->retryTimer->start(delay);
}

void Network::NetworkManager::onRetryTimerTimeout()
{
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

int Network::NetworkManager::calculateRetryDelay(int retry, const RequestOptions& options)
{
    if (!options.exponentialBackoff) {
        return options.retryDelay;
    }
    
    // 指数退避：delay * (2 ^ retry)。
    int delay = options.retryDelay * (1 << retry);
    
    // 限制最大延迟为 30 秒。
    return qMin(delay, 30000);
}

void Network::NetworkManager::cancelRequest(const QString& requestId)
{
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
    disposeReply(detachReply(context.get()), true);
    cleanupTimers(context.get());
    cleanupElapsedTimer(context.get());

    qDebug() << "[NetworkManager] Request cancelled:" << requestId;
}

void Network::NetworkManager::cancelAllRequests()
{
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
        disposeReply(detachReply(context.get()), true);
        cleanupTimers(context.get());
        cleanupElapsedTimer(context.get());
    }
    
    qDebug() << "[NetworkManager] All requests cancelled";
}

QNetworkReply* Network::NetworkManager::detachReply(RequestContext* context)
{
    if (!context) {
        return nullptr;
    }

    QNetworkReply* reply = context->reply;
    context->reply = nullptr;
    if (reply) {
        disconnect(reply, nullptr, this, nullptr);
    }
    return reply;
}

void Network::NetworkManager::disposeReply(QNetworkReply* reply, bool abortReply)
{
    if (!reply) {
        return;
    }

    disconnect(reply, nullptr, this, nullptr);
    if (abortReply && reply->isRunning()) {
        reply->abort();
    }
    reply->deleteLater();
}

void Network::NetworkManager::cleanupTimers(RequestContext* context)
{
    if (!context) {
        return;
    }

    if (context->timeoutTimer) {
        context->timeoutTimer->stop();
        context->timeoutTimer->deleteLater();
        context->timeoutTimer = nullptr;
    }
    if (context->retryTimer) {
        context->retryTimer->stop();
        context->retryTimer->deleteLater();
        context->retryTimer = nullptr;
    }
}

void Network::NetworkManager::cleanupElapsedTimer(RequestContext* context)
{
    if (!context || !context->elapsedTimer) {
        return;
    }

    delete context->elapsedTimer;
    context->elapsedTimer = nullptr;
}

void Network::NetworkManager::removeActiveRequest(const std::shared_ptr<RequestContext>& context)
{
    if (!context) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_activeRequests.remove(context->requestId);
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
    // Qt 的 QNetworkAccessManager 会自动管理连接池。
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
    
    // 设置默认请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Keep-Alive", "timeout=300, max=1000");
    request.setRawHeader("User-Agent", "FFmpeg-Music-Player/1.0");
    
    // 启用压缩。
    if (options.enableCompression) {
        request.setRawHeader("Accept-Encoding", "gzip, deflate");
    }
    
    // 设置自定义请求头
    for (auto it = options.headers.begin(); it != options.headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
    
    // 重定向策略。
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
    
    // 更新平均响应时间
    if (m_stats.totalRequests > 0) {
        m_stats.avgResponseTime = (m_stats.avgResponseTime * (m_stats.totalRequests - 1) + response.elapsedMs) 
                                 / m_stats.totalRequests;
    }
    
    emit statsUpdated(m_stats);
}




