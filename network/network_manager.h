#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QMap>
#include <QMutex>
#include <QElapsedTimer>
#include "network_types.h"

namespace Network {

class NetworkManager;

} // namespace Network

/**
 * @brief 网络管理器类 - QNetworkAccessManager的单例包装器
 * 
 * 功能：
 * 1. 单例管理QNetworkAccessManager，确保连接复用
 * 2. 超时管理
 * 3. 重试机制（支持指数退避）
 * 4. 请求统计
 * 5. 线程安全
 */
class Network::NetworkManager : public QObject {
    Q_OBJECT
    
public:
    static NetworkManager& instance();
    
    /**
     * @brief 发送HTTP请求
     * @param url 请求URL
     * @param method HTTP方法
     * @param data 请求体数据（仅POST/PUT等）
     * @param options 请求选项
     * @param callback 响应回调
     * @param progressCallback 进度回调（可选）
     * @return 请求ID，用于取消请求
     */
    QString sendRequest(
        const QString& url,
        HttpMethod method,
        const QByteArray& data,
        const RequestOptions& options,
        ResponseCallback callback,
        ProgressCallback progressCallback = nullptr
    );
    
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
     * @brief 获取统计信息
     */
    RequestStats getStats() const;
    
    /**
     * @brief 重置统计信息
     */
    void resetStats();
    
    /**
     * @brief 设置全局超时时间
     * @param timeout 超时时间（毫秒）
     */
    void setGlobalTimeout(int timeout);
    
    /**
     * @brief 设置最大并发连接数
     * @param maxConnections 最大连接数
     */
    void setMaxConnections(int maxConnections);
    
    /**
     * @brief DNS预热
     * @param hostname 主机名
     */
    void prewarmDns(const QString& hostname);
    
signals:
    void requestStarted(const QString& requestId, const QString& url);
    void requestFinished(const QString& requestId, const NetworkResponse& response);
    void requestProgress(const QString& requestId, qint64 bytesReceived, qint64 bytesTotal);
    void requestFailed(const QString& requestId, const QString& error);
    void statsUpdated(const RequestStats& stats);
    
private:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();
    
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;
    
    struct RequestContext {
        QString requestId;
        QString url;
        HttpMethod method;
        QByteArray data;
        RequestOptions options;
        ResponseCallback callback;
        ProgressCallback progressCallback;
        QNetworkReply* reply = nullptr;
        QTimer* timeoutTimer = nullptr;
        QElapsedTimer* elapsedTimer = nullptr;
        int currentRetry = 0;
    };
    
    void executeRequest(std::shared_ptr<RequestContext> context);
    void handleReplyFinished(std::shared_ptr<RequestContext> context);
    void handleTimeout(std::shared_ptr<RequestContext> context);
    void retryRequest(std::shared_ptr<RequestContext> context);
    int calculateRetryDelay(int retry, const RequestOptions& options);
    
    QNetworkRequest createRequest(const QString& url, const RequestOptions& options);
    QString httpMethodToString(HttpMethod method) const;
    void updateStats(const NetworkResponse& response);
    
    QNetworkAccessManager* m_manager;
    QMap<QString, std::shared_ptr<RequestContext>> m_activeRequests;
    mutable QMutex m_mutex;
    
    // 配置
    int m_globalTimeout = 30000;    // 默认30秒
    int m_maxConnections = 6;       // 默认6个并发连接
    
    // 统计信息
    RequestStats m_stats;
};

#endif // NETWORK_MANAGER_H
