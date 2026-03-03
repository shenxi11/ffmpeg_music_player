#ifndef RESPONSE_CACHE_H
#define RESPONSE_CACHE_H

#include <QObject>
#include <QMap>
#include <QMutex>
#include <QTimer>
#include <QCryptographicHash>
#include <optional>
#include "network_types.h"

namespace Network {

/**
 * @brief 响应缓存类 - 使用LRU策略管理HTTP响应缓存
 * 
 * 功能：
 * 1. 基于URL的缓存（仅GET请求）
 * 2. TTL（Time To Live）过期管理
 * 3. LRU（Least Recently Used）驱逐策略
 * 4. 线程安全
 */
class ResponseCache : public QObject {
    Q_OBJECT
    
public:
    explicit ResponseCache(QObject* parent = nullptr);
    ~ResponseCache();
    
    /**
     * @brief 设置缓存
     * @param key 缓存键（通常是URL）
     * @param entry 缓存条目
     */
    void set(const QString& key, const CacheEntry& entry);
    
    /**
     * @brief 获取缓存
     * @param key 缓存键
     * @return 缓存条目（如果存在且有效）
     */
    std::optional<CacheEntry> get(const QString& key);
    
    /**
     * @brief 检查缓存是否存在且有效
     * @param key 缓存键
     * @return true如果缓存存在且有效
     */
    bool has(const QString& key);
    
    /**
     * @brief 使指定缓存失效
     * @param key 缓存键
     */
    void invalidate(const QString& key);
    
    /**
     * @brief 使所有缓存失效
     */
    void clear();
    
    /**
     * @brief 设置最大缓存条目数
     * @param maxSize 最大条目数
     */
    void setMaxSize(int maxSize);
    
    /**
     * @brief 获取当前缓存大小
     * @return 缓存条目数
     */
    int size() const;
    
    /**
     * @brief 获取缓存统计信息
     */
    struct CacheStats {
        int totalEntries = 0;
        qint64 totalBytes = 0;
        int hits = 0;
        int misses = 0;
        
        double hitRate() const {
            int total = hits + misses;
            return total > 0 ? (double)hits / total * 100.0 : 0.0;
        }
    };
    
    CacheStats getStats() const;
    
    /**
     * @brief 生成缓存键
     * @param url URL
     * @param method HTTP方法
     * @param headers 请求头（可选）
     * @return 缓存键
     */
    static QString generateKey(const QString& url, HttpMethod method, 
                              const QMap<QString, QString>& headers = {});
    
signals:
    void cacheHit(const QString& key);
    void cacheMiss(const QString& key);
    void cacheEvicted(const QString& key);
    
private slots:
    void cleanupExpiredEntries();
    
private:
    struct CacheNode {
        QString key;
        CacheEntry entry;
        QDateTime accessTime;
        int accessCount = 0;
    };
    
    void evictLRU();
    void touch(const QString& key);
    
    mutable QMutex m_mutex;
    QMap<QString, CacheNode> m_cache;
    QList<QString> m_lruList;  // 最近使用列表，最旧的在前面
    int m_maxSize = 100;       // 默认最多100个缓存条目
    QTimer* m_cleanupTimer;
    
    // 统计信息
    mutable int m_hits = 0;
    mutable int m_misses = 0;
};

} // namespace Network

#endif // RESPONSE_CACHE_H
