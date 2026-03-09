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
        
        QMutexLocker locker(&m_mutex);
        m_activeRequests.remove(context->requestId);
        return;
    }
    
    context->reply = reply;
    
    // 连接完成信号。
    connect(reply, &QNetworkReply::finished, this, [this, context]() {
        handleReplyFinished(context);
    });
    
    // 连接进度信号。
    if (context->progressCallback) {
        connect(reply, &QNetworkReply::downloadProgress, this, 
                [this, context](qint64 bytesReceived, qint64 bytesTotal) {
            if (context->progressCallback) {
                context->progressCallback(bytesReceived, bytesTotal);
            }
            emit requestProgress(context->requestId, bytesReceived, bytesTotal);
        });
    }
    
    // 设置超时定时器
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
    
    // 停止超时计时器。
    if (context->timeoutTimer) {
        context->timeoutTimer->stop();
        context->timeoutTimer->deleteLater();
        context->timeoutTimer = nullptr;
    }
    
    QNetworkReply* reply = context->reply;
    
    // 构建响应对象。
    NetworkResponse response;
    response.requestId = context->requestId;
    response.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    response.body = reply->readAll();
    response.error = reply->error();
    response.errorString = reply->errorString();
    response.elapsedMs = context->elapsedTimer->elapsed();
    
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
        
        reply->deleteLater();
        context->reply = nullptr;
        
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
    
    // 使用定时器延迟重试
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
    
    // 指数退避：delay * (2 ^ retry)。
    int delay = options.retryDelay * (1 << retry);
    
    // 限制最大延迟为 30 秒。
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




