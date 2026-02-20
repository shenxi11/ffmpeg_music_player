#include "response_cache.h"
#include <QDebug>

namespace Network {

ResponseCache::ResponseCache(QObject* parent)
    : QObject(parent)
{
    // 创建定时清理过期缓存的定时器（每5分钟执行一次）
    m_cleanupTimer = new QTimer(this);
    connect(m_cleanupTimer, &QTimer::timeout, this, &ResponseCache::cleanupExpiredEntries);
    m_cleanupTimer->start(5 * 60 * 1000);  // 5分钟
}

ResponseCache::~ResponseCache()
{
    clear();
}

void ResponseCache::set(const QString& key, const CacheEntry& entry)
{
    QMutexLocker locker(&m_mutex);
    
    // 检查是否已满，需要驱逐
    if (m_cache.size() >= m_maxSize && !m_cache.contains(key)) {
        evictLRU();
    }
    
    CacheNode node;
    node.key = key;
    node.entry = entry;
    node.accessTime = QDateTime::currentDateTime();
    node.accessCount = 1;
    
    m_cache[key] = node;
    
    // 更新LRU列表
    m_lruList.removeAll(key);
    m_lruList.append(key);
    
    qDebug() << "[ResponseCache] Cached:" << key 
             << "TTL:" << entry.expireTime.secsTo(QDateTime::currentDateTime()) << "s"
             << "Size:" << entry.data.size() << "bytes";
}

std::optional<CacheEntry> ResponseCache::get(const QString& key)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_cache.contains(key)) {
        m_misses++;
        emit cacheMiss(key);
        return std::nullopt;
    }
    
    CacheNode& node = m_cache[key];
    
    // 检查是否过期
    if (!node.entry.isValid()) {
        m_cache.remove(key);
        m_lruList.removeAll(key);
        m_misses++;
        emit cacheMiss(key);
        qDebug() << "[ResponseCache] Expired:" << key;
        return std::nullopt;
    }
    
    // 更新访问信息
    node.accessTime = QDateTime::currentDateTime();
    node.accessCount++;
    
    // 更新LRU列表（移到末尾）
    m_lruList.removeAll(key);
    m_lruList.append(key);
    
    m_hits++;
    emit cacheHit(key);
    qDebug() << "[ResponseCache] Hit:" << key << "Access count:" << node.accessCount;
    
    return node.entry;
}

bool ResponseCache::has(const QString& key)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_cache.contains(key)) {
        return false;
    }
    
    return m_cache[key].entry.isValid();
}

void ResponseCache::invalidate(const QString& key)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_cache.remove(key) > 0) {
        m_lruList.removeAll(key);
        qDebug() << "[ResponseCache] Invalidated:" << key;
    }
}

void ResponseCache::clear()
{
    QMutexLocker locker(&m_mutex);
    
    m_cache.clear();
    m_lruList.clear();
    m_hits = 0;
    m_misses = 0;
    
    qDebug() << "[ResponseCache] Cleared all entries";
}

void ResponseCache::setMaxSize(int maxSize)
{
    QMutexLocker locker(&m_mutex);
    
    m_maxSize = maxSize;
    
    // 如果当前大小超过新的最大值，驱逐多余的条目
    while (m_cache.size() > m_maxSize) {
        evictLRU();
    }
    
    qDebug() << "[ResponseCache] Max size set to:" << maxSize;
}

int ResponseCache::size() const
{
    QMutexLocker locker(&m_mutex);
    return m_cache.size();
}

ResponseCache::CacheStats ResponseCache::getStats() const
{
    QMutexLocker locker(&m_mutex);
    
    CacheStats stats;
    stats.totalEntries = m_cache.size();
    stats.hits = m_hits;
    stats.misses = m_misses;
    
    for (const auto& node : m_cache) {
        stats.totalBytes += node.entry.data.size();
    }
    
    return stats;
}

QString ResponseCache::generateKey(const QString& url, HttpMethod method, 
                                   const QMap<QString, QString>& headers)
{
    // 对于GET请求，仅使用URL作为键
    if (method == HttpMethod::GET) {
        return url;
    }
    
    // 对于其他方法，结合URL、方法和某些关键请求头生成键
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(url.toUtf8());
    hash.addData(QByteArray::number(static_cast<int>(method)));
    
    // 添加可能影响响应的关键请求头
    QStringList keyHeaders = {"Authorization", "Accept", "Accept-Language"};
    for (const QString& headerName : keyHeaders) {
        if (headers.contains(headerName)) {
            hash.addData(headerName.toUtf8());
            hash.addData(headers[headerName].toUtf8());
        }
    }
    
    return QString(hash.result().toHex());
}

void ResponseCache::cleanupExpiredEntries()
{
    QMutexLocker locker(&m_mutex);
    
    QStringList expiredKeys;
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (!it.value().entry.isValid()) {
            expiredKeys.append(it.key());
        }
    }
    
    for (const QString& key : expiredKeys) {
        m_cache.remove(key);
        m_lruList.removeAll(key);
        emit cacheEvicted(key);
    }
    
    if (!expiredKeys.isEmpty()) {
        qDebug() << "[ResponseCache] Cleaned up" << expiredKeys.size() << "expired entries";
    }
}

void ResponseCache::evictLRU()
{
    // 必须在已持有锁的情况下调用
    if (m_lruList.isEmpty()) {
        return;
    }
    
    // 驱逐最久未使用的条目（列表头部）
    QString keyToEvict = m_lruList.takeFirst();
    m_cache.remove(keyToEvict);
    
    emit cacheEvicted(keyToEvict);
    qDebug() << "[ResponseCache] Evicted LRU entry:" << keyToEvict;
}

void ResponseCache::touch(const QString& key)
{
    // 更新访问时间（已在get方法中处理）
}

} // namespace Network
