#include "network_service.h"
#include <QDebug>
#include <QUrlQuery>
#include <QJsonParseError>

namespace Network {

NetworkService& NetworkService::instance()
{
    static NetworkService instance;
    return instance;
}

NetworkService::NetworkService(QObject* parent)
    : QObject(parent)
    , m_networkManager(NetworkManager::instance())
{
    // 连接NetworkManager的信号
    connect(&m_networkManager, &NetworkManager::requestStarted,
            this, &NetworkService::requestStarted);
    connect(&m_networkManager, &NetworkManager::requestFinished,
            this, &NetworkService::requestFinished);
    connect(&m_networkManager, &NetworkManager::requestProgress,
            this, &NetworkService::requestProgress);
    connect(&m_networkManager, &NetworkManager::requestFailed,
            this, &NetworkService::requestFailed);
    
    // 连接缓存信号
    connect(&m_cache, &ResponseCache::cacheHit, this, [this](const QString& key) {
        emit cacheHit(key);
    });
    connect(&m_cache, &ResponseCache::cacheMiss, this, [this](const QString& key) {
        emit cacheMiss(key);
    });
    
    qDebug() << "[NetworkService] Initialized";
}

NetworkService::~NetworkService()
{
}

QString NetworkService::get(const QString& url, 
                            const RequestOptions& options,
                            ResponseCallback callback)
{
    return sendRequestInternal(url, HttpMethod::GET, QByteArray(), options, callback);
}

QString NetworkService::post(const QString& url,
                             const QByteArray& data,
                             const RequestOptions& options,
                             ResponseCallback callback)
{
    return sendRequestInternal(url, HttpMethod::POST, data, options, callback);
}

QString NetworkService::postJson(const QString& url,
                                 const QJsonObject& jsonData,
                                 const RequestOptions& options,
                                 ResponseCallback callback)
{
    QJsonDocument doc(jsonData);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    RequestOptions jsonOptions = options;
    jsonOptions.headers["Content-Type"] = "application/json";
    
    return sendRequestInternal(url, HttpMethod::POST, data, jsonOptions, callback);
}

QString NetworkService::put(const QString& url,
                            const QByteArray& data,
                            const RequestOptions& options,
                            ResponseCallback callback)
{
    return sendRequestInternal(url, HttpMethod::PUT, data, options, callback);
}

QString NetworkService::del(const QString& url,
                            const RequestOptions& options,
                            ResponseCallback callback)
{
    return sendRequestInternal(url, HttpMethod::DELETE_METHOD, QByteArray(), options, callback);
}

QString NetworkService::sendRequestInternal(
    const QString& url,
    HttpMethod method,
    const QByteArray& data,
    const RequestOptions& options,
    ResponseCallback callback)
{
    QString fullUrl = buildFullUrl(url);
    RequestOptions mergedOptions = mergeOptions(options);
    
    // 生成缓存键
    QString cacheKey = ResponseCache::generateKey(fullUrl, method, mergedOptions.headers);
    
    // 检查缓存（仅GET请求且启用缓存）
    if (method == HttpMethod::GET && mergedOptions.useCache) {
        auto cachedEntry = m_cache.get(cacheKey);
        if (cachedEntry.has_value()) {
            qDebug() << "[NetworkService] Cache hit for:" << fullUrl;
            
            // 构建缓存响应
            NetworkResponse response;
            response.statusCode = cachedEntry->statusCode;
            response.body = cachedEntry->data;
            response.headers = cachedEntry->headers;
            response.error = QNetworkReply::NoError;
            response.isFromCache = true;
            response.elapsedMs = 0;
            response.requestId = "cached-" + cacheKey;
            
            // 异步调用回调（保持一致性）
            if (callback) {
                QTimer::singleShot(0, [callback, response]() {
                    callback(response);
                });
            }
            
            emit cacheHit(fullUrl);
            return response.requestId;
        } else {
            emit cacheMiss(fullUrl);
        }
    }
    
    // 发送网络请求
    QString requestId = m_networkManager.sendRequest(
        fullUrl,
        method,
        data,
        mergedOptions,
        [this, callback, cacheKey, method, mergedOptions, fullUrl](const NetworkResponse& response) {
            // 如果成功且是GET请求且启用缓存，保存到缓存
            if (response.isSuccess() && method == HttpMethod::GET && mergedOptions.useCache) {
                CacheEntry entry;
                entry.data = response.body;
                entry.headers = response.headers;
                entry.statusCode = response.statusCode;
                entry.expireTime = QDateTime::currentDateTime().addSecs(mergedOptions.cacheTtl);
                
                m_cache.set(cacheKey, entry);
                qDebug() << "[NetworkService] Cached response for:" << fullUrl;
            }
            
            // 调用用户回调
            if (callback) {
                callback(response);
            }
            
            // 清理待处理请求映射
            QMutexLocker locker(&m_pendingMutex);
            m_pendingRequests[fullUrl].removeAll(response.requestId);
            if (m_pendingRequests[fullUrl].isEmpty()) {
                m_pendingRequests.remove(fullUrl);
            }
        }
    );
    
    // 记录待处理请求
    {
        QMutexLocker locker(&m_pendingMutex);
        m_pendingRequests[fullUrl].append(requestId);
    }
    
    return requestId;
}

void NetworkService::cancelRequest(const QString& requestId)
{
    m_networkManager.cancelRequest(requestId);
}

void NetworkService::cancelAllRequests()
{
    m_networkManager.cancelAllRequests();
    
    QMutexLocker locker(&m_pendingMutex);
    m_pendingRequests.clear();
}

void NetworkService::clearCache()
{
    m_cache.clear();
    qDebug() << "[NetworkService] Cache cleared";
}

void NetworkService::invalidateCache(const QString& url)
{
    QString fullUrl = buildFullUrl(url);
    QString cacheKey = ResponseCache::generateKey(fullUrl, HttpMethod::GET);
    m_cache.invalidate(cacheKey);
    
    qDebug() << "[NetworkService] Cache invalidated for:" << fullUrl;
}

RequestStats NetworkService::getStats() const
{
    return m_networkManager.getStats();
}

ResponseCache::CacheStats NetworkService::getCacheStats() const
{
    return m_cache.getStats();
}

void NetworkService::setBaseUrl(const QString& baseUrl)
{
    m_baseUrl = baseUrl;
    if (!m_baseUrl.endsWith('/')) {
        m_baseUrl += '/';
    }
    qDebug() << "[NetworkService] Base URL set to:" << m_baseUrl;
}

QString NetworkService::getBaseUrl() const
{
    return m_baseUrl;
}

void NetworkService::setGlobalHeaders(const QMap<QString, QString>& headers)
{
    m_globalHeaders = headers;
    qDebug() << "[NetworkService] Global headers set:" << headers.size() << "headers";
}

void NetworkService::addGlobalHeader(const QString& name, const QString& value)
{
    m_globalHeaders[name] = value;
    qDebug() << "[NetworkService] Added global header:" << name << "=" << value;
}

void NetworkService::removeGlobalHeader(const QString& name)
{
    m_globalHeaders.remove(name);
    qDebug() << "[NetworkService] Removed global header:" << name;
}

void NetworkService::prewarmDns(const QString& hostname)
{
    m_networkManager.prewarmDns(hostname);
}

QJsonObject NetworkService::parseJsonObject(const NetworkResponse& response)
{
    if (!response.isSuccess()) {
        qWarning() << "[NetworkService] Cannot parse JSON from failed response";
        return QJsonObject();
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(response.body, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "[NetworkService] JSON parse error:" << error.errorString();
        return QJsonObject();
    }
    
    if (!doc.isObject()) {
        qWarning() << "[NetworkService] Response is not a JSON object";
        return QJsonObject();
    }
    
    return doc.object();
}

QJsonArray NetworkService::parseJsonArray(const NetworkResponse& response)
{
    if (!response.isSuccess()) {
        qWarning() << "[NetworkService] Cannot parse JSON from failed response";
        return QJsonArray();
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(response.body, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "[NetworkService] JSON parse error:" << error.errorString();
        return QJsonArray();
    }
    
    if (!doc.isArray()) {
        qWarning() << "[NetworkService] Response is not a JSON array";
        return QJsonArray();
    }
    
    return doc.array();
}

QString NetworkService::buildFullUrl(const QString& url) const
{
    // 如果URL已经是完整URL（包含协议），直接返回
    if (url.startsWith("http://") || url.startsWith("https://")) {
        return url;
    }
    
    // 否则，拼接基础URL
    QString fullUrl = m_baseUrl;
    
    // 移除URL开头的斜杠（避免双斜杠）
    QString cleanUrl = url;
    if (cleanUrl.startsWith('/')) {
        cleanUrl = cleanUrl.mid(1);
    }
    
    fullUrl += cleanUrl;
    return fullUrl;
}

RequestOptions NetworkService::mergeOptions(const RequestOptions& options) const
{
    RequestOptions merged = options;
    
    // 合并全局请求头（用户选项优先）
    for (auto it = m_globalHeaders.begin(); it != m_globalHeaders.end(); ++it) {
        if (!merged.headers.contains(it.key())) {
            merged.headers[it.key()] = it.value();
        }
    }
    
    return merged;
}

} // namespace Network
