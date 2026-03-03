#ifndef NETWORK_TYPES_H
#define NETWORK_TYPES_H

#include <QString>
#include <QByteArray>
#include <QMap>
#include <QNetworkReply>
#include <QDateTime>
#include <functional>
#include <memory>

namespace Network {

// 请求优先级
enum class RequestPriority {
    Critical = 0,  // 登录、注册 - 最高优先级
    High = 1,      // 播放、获取歌曲列表
    Normal = 2,    // 普通请求
    Low = 3        // 下载、预加载 - 最低优先级
};

// HTTP方法
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE_METHOD,
    PATCH,
    HEAD_METHOD
};

// 请求配置选项
struct RequestOptions {
    int timeout = 30000;                        // 超时时间（毫秒）
    int maxRetries = 3;                         // 最大重试次数
    int retryDelay = 1000;                      // 重试延迟（毫秒）
    bool exponentialBackoff = true;             // 使用指数退避
    RequestPriority priority = RequestPriority::Normal;  // 请求优先级
    bool useCache = false;                      // 是否使用缓存（仅GET）
    int cacheTtl = 300;                         // 缓存TTL（秒）
    QMap<QString, QString> headers;             // 自定义请求头
    bool enableCompression = true;              // 启用压缩
    bool followRedirects = true;                // 跟随重定向
    int maxRedirects = 5;                       // 最大重定向次数
    
    // 构造函数
    RequestOptions() = default;
    
    // 便捷构造函数
    static RequestOptions withCache(int ttl = 300) {
        RequestOptions opts;
        opts.useCache = true;
        opts.cacheTtl = ttl;
        return opts;
    }
    
    static RequestOptions withPriority(RequestPriority priority) {
        RequestOptions opts;
        opts.priority = priority;
        return opts;
    }
    
    static RequestOptions critical() {
        RequestOptions opts;
        opts.priority = RequestPriority::Critical;
        opts.timeout = 15000;  // 更短的超时时间
        return opts;
    }
};

// 网络响应
struct NetworkResponse {
    int statusCode = 0;                         // HTTP状态码
    QByteArray body;                            // 响应体
    QMap<QString, QString> headers;             // 响应头
    QNetworkReply::NetworkError error = QNetworkReply::NoError;  // 错误类型
    QString errorString;                        // 错误信息
    bool isFromCache = false;                   // 是否来自缓存
    qint64 elapsedMs = 0;                       // 请求耗时（毫秒）
    QString requestId;                          // 请求ID
    
    // 便捷方法
    bool isSuccess() const {
        return statusCode >= 200 && statusCode < 300 && error == QNetworkReply::NoError;
    }
    
    bool isClientError() const {
        return statusCode >= 400 && statusCode < 500;
    }
    
    bool isServerError() const {
        return statusCode >= 500 && statusCode < 600;
    }
    
    bool isNetworkError() const {
        return error != QNetworkReply::NoError;
    }
    
    QString getHeader(const QString& name, const QString& defaultValue = QString()) const {
        return headers.value(name, defaultValue);
    }
};

// 请求回调
using ResponseCallback = std::function<void(const NetworkResponse&)>;
using ProgressCallback = std::function<void(qint64 bytesReceived, qint64 bytesTotal)>;
using UploadProgressCallback = std::function<void(qint64 bytesSent, qint64 bytesTotal)>;

// 请求取消令牌
class CancellationToken {
public:
    CancellationToken() : m_cancelled(std::make_shared<bool>(false)) {}
    
    void cancel() {
        *m_cancelled = true;
    }
    
    bool isCancelled() const {
        return *m_cancelled;
    }
    
private:
    std::shared_ptr<bool> m_cancelled;
};

// 缓存条目
struct CacheEntry {
    QByteArray data;
    QMap<QString, QString> headers;
    QDateTime expireTime;
    int statusCode;
    
    bool isValid() const {
        return QDateTime::currentDateTime() < expireTime;
    }
};

// 请求统计信息
struct RequestStats {
    qint64 totalRequests = 0;
    qint64 successfulRequests = 0;
    qint64 failedRequests = 0;
    qint64 cachedRequests = 0;
    qint64 totalRetries = 0;
    qint64 totalBytes = 0;
    qint64 avgResponseTime = 0;
    
    double successRate() const {
        return totalRequests > 0 ? (double)successfulRequests / totalRequests * 100.0 : 0.0;
    }
    
    double cacheHitRate() const {
        return totalRequests > 0 ? (double)cachedRequests / totalRequests * 100.0 : 0.0;
    }
};

} // namespace Network

#endif // NETWORK_TYPES_H
