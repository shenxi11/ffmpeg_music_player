#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "network_types.h"

#include <QElapsedTimer>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QTimer>

#include <atomic>

namespace Network {

class NetworkManager;

} // namespace Network

/**
 * @brief 网络管理器类 - libcurl 统一入口。
 *
 * 功能：
 * 1. 统一调度 libcurl 请求；
 * 2. 保持原有 NetworkService 对外接口；
 * 3. 支持超时、重试、取消、进度和统计；
 * 4. 回调统一回到 NetworkManager 所在线程，避免 UI 线程安全问题。
 */
class Network::NetworkManager : public QObject {
    Q_OBJECT

  public:
    static NetworkManager& instance();

    QString sendRequest(const QString& url, HttpMethod method, const QByteArray& data,
                        const RequestOptions& options, ResponseCallback callback,
                        ProgressCallback progressCallback = nullptr);

    void cancelRequest(const QString& requestId);
    void cancelAllRequests();

    RequestStats getStats() const;
    void resetStats();

    void setGlobalTimeout(int timeout);
    void setMaxConnections(int maxConnections);
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
        QTimer* retryTimer = nullptr;
        QElapsedTimer* elapsedTimer = nullptr;
        std::shared_ptr<std::atomic_bool> cancelled;
        int currentRetry = 0;
        bool completed = false;
    };

  private slots:
    void onRetryTimerTimeout();

  private:
    void executeRequest(std::shared_ptr<RequestContext> context);
    void handleCurlFinished(const QString& requestId, const NetworkResponse& response);
    void retryRequest(std::shared_ptr<RequestContext> context);
    int calculateRetryDelay(int retry, const RequestOptions& options);
    void cleanupTimers(RequestContext* context);
    void cleanupElapsedTimer(RequestContext* context);
    void removeActiveRequest(const std::shared_ptr<RequestContext>& context);

    QMap<QString, QString> createHeaders(const RequestOptions& options) const;
    QString httpMethodToString(HttpMethod method) const;
    NetworkResponse makeCancelledResponse(const RequestContext& context) const;
    void updateStats(const NetworkResponse& response);

    QMap<QString, std::shared_ptr<RequestContext>> m_activeRequests;
    mutable QMutex m_mutex;

    int m_globalTimeout = 30000;
    int m_maxConnections = 6;
    RequestStats m_stats;
};

#endif // NETWORK_MANAGER_H
