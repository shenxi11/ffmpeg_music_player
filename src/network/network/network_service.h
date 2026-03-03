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
 * @brief 缃戠粶鏈嶅姟绫?- 缁熶竴鐨勭綉缁滆姹傞棬闈?
 * 
 * 鍔熻兘锛?
 * 1. 鎻愪緵绠€娲佺殑API鎺ュ彛
 * 2. 闆嗘垚缂撳瓨銆侀噸璇曘€佽秴鏃剁瓑鍔熻兘
 * 3. 鏀寔JSON搴忓垪鍖?鍙嶅簭鍒楀寲
 * 4. 璇锋眰鍘婚噸
 * 5. 缁熻鍜岀洃鎺?
 */
class NetworkService : public QObject {
    Q_OBJECT
    
public:
    static NetworkService& instance();
    
    /**
     * @brief GET璇锋眰
     * @param url 璇锋眰URL
     * @param options 璇锋眰閫夐」
     * @param callback 鍝嶅簲鍥炶皟
     * @return 璇锋眰ID
     */
    QString get(const QString& url, 
                const RequestOptions& options = RequestOptions(),
                ResponseCallback callback = nullptr);
    
    /**
     * @brief POST璇锋眰
     * @param url 璇锋眰URL
     * @param data 璇锋眰浣撴暟鎹?
     * @param options 璇锋眰閫夐」
     * @param callback 鍝嶅簲鍥炶皟
     * @return 璇锋眰ID
     */
    QString post(const QString& url, 
                 const QByteArray& data,
                 const RequestOptions& options = RequestOptions(),
                 ResponseCallback callback = nullptr);
    
    /**
     * @brief POST JSON璇锋眰
     * @param url 璇锋眰URL
     * @param jsonData JSON瀵硅薄
     * @param options 璇锋眰閫夐」
     * @param callback 鍝嶅簲鍥炶皟
     * @return 璇锋眰ID
     */
    QString postJson(const QString& url,
                     const QJsonObject& jsonData,
                     const RequestOptions& options = RequestOptions(),
                     ResponseCallback callback = nullptr);
    
    /**
     * @brief PUT璇锋眰
     * @param url 璇锋眰URL
     * @param data 璇锋眰浣撴暟鎹?
     * @param options 璇锋眰閫夐」
     * @param callback 鍝嶅簲鍥炶皟
     * @return 璇锋眰ID
     */
    QString put(const QString& url,
                const QByteArray& data,
                const RequestOptions& options = RequestOptions(),
                ResponseCallback callback = nullptr);
    
    /**
     * @brief DELETE璇锋眰
     * @param url 璇锋眰URL
     * @param options 璇锋眰閫夐」
     * @param callback 鍝嶅簲鍥炶皟
     * @return 璇锋眰ID
     */
    QString del(const QString& url,
                const RequestOptions& options = RequestOptions(),
                ResponseCallback callback = nullptr);
    
    /**
     * @brief 鍙栨秷璇锋眰
     * @param requestId 璇锋眰ID
     */
    void cancelRequest(const QString& requestId);
    
    /**
     * @brief 鍙栨秷鎵€鏈夎姹?
     */
    void cancelAllRequests();
    
    /**
     * @brief 娓呴櫎缂撳瓨
     */
    void clearCache();
    
    /**
     * @brief 浣挎寚瀹歎RL鐨勭紦瀛樺け鏁?
     * @param url URL
     */
    void invalidateCache(const QString& url);
    
    /**
     * @brief 鑾峰彇缁熻淇℃伅
     */
    RequestStats getStats() const;
    
    /**
     * @brief 鑾峰彇缂撳瓨缁熻淇℃伅
     */
    ResponseCache::CacheStats getCacheStats() const;
    
    /**
     * @brief 璁剧疆鍩虹URL
     * @param baseUrl 鍩虹URL锛堝 http://192.168.1.208:8080/锛?
     */
    void setBaseUrl(const QString& baseUrl);
    
    /**
     * @brief 鑾峰彇鍩虹URL
     */
    QString getBaseUrl() const;
    
    /**
     * @brief 璁剧疆鍏ㄥ眬璇锋眰澶?
     * @param headers 璇锋眰澶存槧灏?
     */
    void setGlobalHeaders(const QMap<QString, QString>& headers);
    
    /**
     * @brief 娣诲姞鍏ㄥ眬璇锋眰澶?
     * @param name 璇锋眰澶村悕绉?
     * @param value 璇锋眰澶村€?
     */
    void addGlobalHeader(const QString& name, const QString& value);
    
    /**
     * @brief 绉婚櫎鍏ㄥ眬璇锋眰澶?
     * @param name 璇锋眰澶村悕绉?
     */
    void removeGlobalHeader(const QString& name);
    
    /**
     * @brief DNS棰勭儹
     * @param hostname 涓绘満鍚?
     */
    void prewarmDns(const QString& hostname);
    
    /**
     * @brief 瑙ｆ瀽JSON鍝嶅簲
     * @param response 缃戠粶鍝嶅簲
     * @return JSON瀵硅薄锛堣В鏋愬け璐ヨ繑鍥炵┖瀵硅薄锛?
     */
    static QJsonObject parseJsonObject(const NetworkResponse& response);
    
    /**
     * @brief 瑙ｆ瀽JSON鏁扮粍鍝嶅簲
     * @param response 缃戠粶鍝嶅簲
     * @return JSON鏁扮粍锛堣В鏋愬け璐ヨ繑鍥炵┖鏁扮粍锛?
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
    
    // 璇锋眰鍘婚噸鏄犲皠锛圲RL -> 璇锋眰ID鍒楄〃锛?
    QMap<QString, QStringList> m_pendingRequests;
    QMutex m_pendingMutex;
};

} // namespace Network

#endif // NETWORK_SERVICE_H

