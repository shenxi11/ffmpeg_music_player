#ifndef NETWORK_SERVICE_H
#define NETWORK_SERVICE_H

#include <QObject>
#include <QFuture>
#include <QFutureInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "network_types.h"
#include "network_manager.h"
#include "response_cache.h"

namespace Network {

/**
 * @brief 网络服务类 - 统一的网络请求门面
 * 
 * 功能：
 * 1. 提供简洁的API接口
 * 2. 集成缓存、重试、超时等功能
 * 3. 支持JSON序列化/反序列化
 * 4. 请求去重
 * 5. 统计和监控
 */
class NetworkService : public QObject {
    Q_OBJECT
    
public:
    static NetworkService& instance();
    
    /**
     * @brief GET请求
     * @param url 请求URL
     * @param options 请求选项
     * @param callback 响应回调
     * @return 请求ID
     */
    QString get(const QString& url, 
                const RequestOptions& options = RequestOptions(),
                ResponseCallback callback = nullptr);
    
    /**
     * @brief POST请求
     * @param url 请求URL
     * @param data 请求体数据
     * @param options 请求选项
     * @param callback 响应回调
     * @return 请求ID
     */
    QString post(const QString& url, 
                 const QByteArray& data,
                 const RequestOptions& options = RequestOptions(),
                 ResponseCallback callback = nullptr);
    
    /**
     * @brief POST JSON请求
     * @param url 请求URL
     * @param jsonData JSON对象
     * @param options 请求选项
     * @param callback 响应回调
     * @return 请求ID
     */
    QString postJson(const QString& url,
                     const QJsonObject& jsonData,
                     const RequestOptions& options = RequestOptions(),
                     ResponseCallback callback = nullptr);
    
    /**
     * @brief PUT请求
     * @param url 请求URL
     * @param data 请求体数据
     * @param options 请求选项
     * @param callback 响应回调
     * @return 请求ID
     */
    QString put(const QString& url,
                const QByteArray& data,
                const RequestOptions& options = RequestOptions(),
                ResponseCallback callback = nullptr);
    
    /**
     * @brief DELETE请求
     * @param url 请求URL
     * @param options 请求选项
     * @param callback 响应回调
     * @return 请求ID
     */
    QString del(const QString& url,
                const RequestOptions& options = RequestOptions(),
                ResponseCallback callback = nullptr);
    
    /**
     * @brief 取消请求
     * @param requestId 请求ID
     */
    void cancelRequest(const QString& requestId);
    
    /**
     * @brief 取消所有请求
     */
    void cancelAllRequests();
    
    /**
     * @brief 清除缓存
     */
    void clearCache();
    
    /**
     * @brief 使指定URL的缓存失效
     * @param url URL
     */
    void invalidateCache(const QString& url);
    
    /**
     * @brief 获取统计信息
     */
    RequestStats getStats() const;
    
    /**
     * @brief 获取缓存统计信息
     */
    ResponseCache::CacheStats getCacheStats() const;
    
    /**
     * @brief 设置基础URL
     * @param baseUrl 基础URL（如 http://slcdut.xyz:8080/）
     */
    void setBaseUrl(const QString& baseUrl);
    
    /**
     * @brief 获取基础URL
     */
    QString getBaseUrl() const;
    
    /**
     * @brief 设置全局请求头
     * @param headers 请求头映射
     */
    void setGlobalHeaders(const QMap<QString, QString>& headers);
    
    /**
     * @brief 添加全局请求头
     * @param name 请求头名称
     * @param value 请求头值
     */
    void addGlobalHeader(const QString& name, const QString& value);
    
    /**
     * @brief 移除全局请求头
     * @param name 请求头名称
     */
    void removeGlobalHeader(const QString& name);
    
    /**
     * @brief DNS预热
     * @param hostname 主机名
     */
    void prewarmDns(const QString& hostname);
    
    /**
     * @brief 解析JSON响应
     * @param response 网络响应
     * @return JSON对象（解析失败返回空对象）
     */
    static QJsonObject parseJsonObject(const NetworkResponse& response);
    
    /**
     * @brief 解析JSON数组响应
     * @param response 网络响应
     * @return JSON数组（解析失败返回空数组）
     */
    static QJsonArray parseJsonArray(const NetworkResponse& response);
    
signals:
    void requestStarted(const QString& requestId, const QString& url);
    void requestFinished(const QString& requestId, const NetworkResponse& response);
    void requestProgress(const QString& requestId, qint64 bytesReceived, qint64 bytesTotal);
    void requestFailed(const QString& requestId, const QString& error);
    void cacheHit(const QString& url);
    void cacheMiss(const QString& url);
    
private:
    explicit NetworkService(QObject* parent = nullptr);
    ~NetworkService();
    
    NetworkService(const NetworkService&) = delete;
    NetworkService& operator=(const NetworkService&) = delete;
    
    QString sendRequestInternal(
        const QString& url,
        HttpMethod method,
        const QByteArray& data,
        const RequestOptions& options,
        ResponseCallback callback
    );
    
    QString buildFullUrl(const QString& url) const;
    RequestOptions mergeOptions(const RequestOptions& options) const;
    
    NetworkManager& m_networkManager;
    ResponseCache m_cache;
    
    QString m_baseUrl;
    QMap<QString, QString> m_globalHeaders;
    
    // 请求去重映射（URL -> 请求ID列表）
    QMap<QString, QStringList> m_pendingRequests;
    QMutex m_pendingMutex;
};

} // namespace Network

#endif // NETWORK_SERVICE_H
