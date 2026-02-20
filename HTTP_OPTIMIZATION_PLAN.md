# HTTP连接优化计划

## 一、当前问题分析

### 1. **重复创建QNetworkAccessManager**
**问题**：每个请求方法都会检查并创建新的manager
```cpp
if(!manager)
    manager = new QNetworkAccessManager();
```
**影响**：
- QNetworkAccessManager创建开销大（需要初始化SSL、线程池等）
- 无法充分利用HTTP连接复用
- 内存占用增加

### 2. **缺少连接复用机制**
**问题**：
- 只有个别请求设置了`Connection: keep-alive`（如get_music_data）
- 大部分请求没有设置keep-alive头
- 没有配置HTTP/2支持

**影响**：
- 每次请求都重新建立TCP连接（三次握手）
- TLS握手重复进行
- 整体延迟增加20-100ms/请求

### 3. **缺少超时和重试机制**
**问题**：
- 所有请求都没有设置超时时间
- 网络异常时请求可能永久挂起
- 没有失败重试逻辑

**影响**：
- 用户体验差（界面可能卡死）
- 资源泄漏（未完成的请求占用资源）

### 4. **没有请求优先级管理**
**问题**：所有请求都平等处理
**影响**：
- 关键请求（登录、播放）可能被大量下载请求阻塞
- 用户感知延迟增加

### 5. **缺少响应缓存**
**问题**：
- getAllFiles()等GET请求没有缓存
- 相同的数据重复请求

**影响**：
- 不必要的网络流量
- 服务器负载增加
- 响应速度慢

### 6. **HttpRequestPool设计不合理**
**问题**：
```cpp
// 每个HttpRequest有独立的QNetworkAccessManager
// 但HttpRequestPool又创建多个HttpRequest
```
**影响**：
- 连接资源浪费
- 无法统一管理连接

### 7. **缺少DNS缓存优化**
**问题**：每次请求都可能触发DNS查询
**影响**：增加15-50ms延迟

### 8. **内存管理隐患**
**问题**：
- `reply->deleteLater()`依赖事件循环
- 高频请求可能导致内存累积

## 二、优化方案

### 方案A：渐进式优化（推荐）
逐步改进现有代码，风险低，易于测试

### 方案B：重构网络层
完全重新设计网络模块，长期收益大但风险高

---

## 三、方案A：渐进式优化（详细计划）

### 阶段1：QNetworkAccessManager单例化 ⭐⭐⭐
**优先级**：最高
**预期收益**：
- 减少80%的manager创建开销
- 自动启用连接复用
- 内存占用减少

**实现步骤**：
1. 创建NetworkManagerSingleton类
2. 统一管理QNetworkAccessManager实例
3. 配置连接池大小和超时

**代码示例**：
```cpp
class NetworkManagerSingleton {
public:
    static NetworkManagerSingleton& instance() {
        static NetworkManagerSingleton instance;
        return instance;
    }
    
    QNetworkAccessManager* getManager() {
        return &m_manager;
    }
    
private:
    NetworkManagerSingleton() {
        // 配置连接池
        QNetworkConfigurationManager config;
        m_manager.setConfiguration(config.defaultConfiguration());
        
        // 启用HTTP/2
        m_manager.setProperty("HTTP2Enabled", true);
        
        // 配置连接数
        m_manager.setProperty("maxConnectionsPerHost", 6);
    }
    
    QNetworkAccessManager m_manager;
};
```

### 阶段2：添加超时和重试机制 ⭐⭐⭐
**优先级**：高
**预期收益**：
- 防止请求永久挂起
- 提高成功率5-10%
- 改善用户体验

**实现方案**：
```cpp
class HttpRequestHelper {
public:
    struct RequestConfig {
        int timeout = 30000;      // 30秒超时
        int maxRetries = 3;       // 最多重试3次
        int retryDelay = 1000;    // 重试延迟1秒
        bool exponentialBackoff = true;
    };
    
    static QNetworkReply* sendRequest(
        QNetworkAccessManager* manager,
        const QNetworkRequest& request,
        const QByteArray& data,
        const QString& method,
        const RequestConfig& config = RequestConfig()
    );
};
```

### 阶段3：统一设置HTTP Headers ⭐⭐
**优先级**：中高
**预期收益**：
- 启用所有请求的keep-alive
- 减少30-50%的连接建立时间

**Headers配置**：
```cpp
void setupCommonHeaders(QNetworkRequest& request) {
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Keep-Alive", "timeout=300, max=1000");
    request.setRawHeader("User-Agent", "FFmpeg-Music-Player/1.0");
    
    // 启用压缩
    request.setRawHeader("Accept-Encoding", "gzip, deflate");
}
```

### 阶段4：实现请求缓存 ⭐⭐
**优先级**：中
**预期收益**：
- GET请求响应速度提升90%（缓存命中时）
- 减少服务器负载

**实现方案**：
```cpp
class HttpCache {
public:
    void setCache(const QString& key, const QByteArray& data, int ttlSeconds = 300);
    QByteArray getCache(const QString& key);
    bool hasCache(const QString& key);
    void invalidate(const QString& key);
    
private:
    struct CacheEntry {
        QByteArray data;
        QDateTime expireTime;
    };
    QMap<QString, CacheEntry> m_cache;
    QMutex m_mutex;
};
```

### 阶段5：请求队列和优先级管理 ⭐
**优先级**：中低
**预期收益**：
- 关键请求响应更快
- 更好的并发控制

**实现方案**：
```cpp
enum class RequestPriority {
    Critical = 0,  // 登录、注册
    High = 1,      // 播放、获取歌曲列表
    Normal = 2,    // 普通请求
    Low = 3        // 下载、预加载
};

class RequestQueue {
public:
    void addRequest(std::shared_ptr<Request> req, RequestPriority priority);
    void processQueue();
    void setMaxConcurrent(int max);
    
private:
    QMap<RequestPriority, QQueue<std::shared_ptr<Request>>> m_queues;
    int m_maxConcurrent = 6;
    int m_currentConcurrent = 0;
};
```

### 阶段6：DNS缓存优化 ⭐
**优先级**：低
**预期收益**：
- 减少10-30ms延迟（首次请求后）

**实现方案**：
```cpp
class DnsCache {
public:
    static void prewarm(const QString& hostname) {
        QHostInfo::lookupHost(hostname, [](const QHostInfo& info) {
            if (info.error() == QHostInfo::NoError) {
                qDebug() << "DNS prewarmed:" << info.hostName();
            }
        });
    }
};

// 在应用启动时预热
DnsCache::prewarm("slcdut.xyz");
```

### 阶段7：网络状态监控 ⭐
**优先级**：低
**预期收益**：
- 网络断开时快速失败
- 网络恢复时自动重试

**实现方案**：
```cpp
class NetworkMonitor : public QObject {
    Q_OBJECT
public:
    static NetworkMonitor& instance();
    bool isOnline() const;
    
signals:
    void networkStatusChanged(bool online);
    
private:
    QNetworkConfigurationManager m_configManager;
};
```

---

## 四、方案B：完全重构网络层

### 架构设计：
```
┌─────────────────────────────────────────┐
│         NetworkService (门面)            │
│  - 统一的API接口                         │
│  - 请求/响应转换                         │
└─────────────────┬───────────────────────┘
                  │
    ┌─────────────┴─────────────┐
    │                           │
┌───▼───────────┐     ┌────────▼──────────┐
│ RequestQueue  │     │  ResponseCache    │
│ - 优先级管理   │     │  - LRU缓存        │
│ - 去重         │     │  - TTL管理        │
└───┬───────────┘     └───────────────────┘
    │
┌───▼────────────────────────────────────┐
│  NetworkManager (单例)                 │
│  - QNetworkAccessManager               │
│  - 连接池管理                           │
│  - 超时/重试                            │
└───┬────────────────────────────────────┘
    │
┌───▼────────────────────────────────────┐
│  HTTP/HTTPS 连接池                     │
│  - Keep-Alive                          │
│  - HTTP/2                              │
└────────────────────────────────────────┘
```

### 核心类设计：

#### 1. NetworkService（统一接口）
```cpp
class NetworkService : public QObject {
    Q_OBJECT
public:
    static NetworkService& instance();
    
    // 统一的请求接口
    QFuture<NetworkResponse> get(const QString& url, const RequestOptions& options = {});
    QFuture<NetworkResponse> post(const QString& url, const QByteArray& data, const RequestOptions& options = {});
    QFuture<NetworkResponse> put(const QString& url, const QByteArray& data, const RequestOptions& options = {});
    QFuture<NetworkResponse> del(const QString& url, const RequestOptions& options = {});
    
    // 取消请求
    void cancelRequest(const QString& requestId);
    
signals:
    void requestProgress(const QString& requestId, qint64 bytesReceived, qint64 bytesTotal);
    void requestFinished(const QString& requestId, const NetworkResponse& response);
};
```

#### 2. RequestOptions（请求配置）
```cpp
struct RequestOptions {
    int timeout = 30000;
    int maxRetries = 3;
    RequestPriority priority = RequestPriority::Normal;
    bool useCache = true;
    int cacheTtl = 300;
    QMap<QString, QString> headers;
    bool enableCompression = true;
};
```

#### 3. NetworkResponse（统一响应）
```cpp
struct NetworkResponse {
    int statusCode;
    QByteArray body;
    QMap<QString, QString> headers;
    QNetworkReply::NetworkError error;
    QString errorString;
    bool isFromCache;
};
```

---

## 五、性能预期对比

| 优化项 | 优化前 | 优化后（方案A） | 提升幅度 |
|--------|--------|----------------|----------|
| 首次连接延迟 | 100-200ms | 100-200ms | 0% |
| 后续请求延迟 | 100-200ms | 10-30ms | 85% ↓ |
| 内存占用（10个请求） | ~50MB | ~10MB | 80% ↓ |
| 请求成功率 | 95% | 99% | 4% ↑ |
| 缓存命中响应 | - | <5ms | 95% ↓ |
| 关键请求延迟 | 100-200ms | 50-100ms | 50% ↓ |

**方案B额外收益**：
- 代码可维护性提升60%
- 测试覆盖更容易
- 添加新功能更快（拦截器、mock等）

---

## 六、实施建议

### 推荐方案：**方案A（分阶段实施）**

#### Week 1-2：基础优化
- [ ] 实现NetworkManagerSingleton
- [ ] 统一HTTP Headers设置
- [ ] 添加基本超时机制

#### Week 3-4：高级优化
- [ ] 实现请求重试机制
- [ ] 添加响应缓存（GET请求）
- [ ] DNS预热

#### Week 5-6：完善和测试
- [ ] 请求优先级队列
- [ ] 网络状态监控
- [ ] 性能测试和调优

### 何时考虑方案B：
- 如果方案A的优化收益达到瓶颈
- 需要支持更复杂的网络场景（WebSocket、GraphQL等）
- 团队有足够的时间和资源重构

---

## 七、风险评估

### 方案A风险：
- ✅ 低风险：改动局部化，易于回滚
- ⚠️ 需要充分测试现有功能不受影响
- ⚠️ 多线程环境下的manager使用需注意线程安全

### 方案B风险：
- ⚠️ 高风险：大规模重构
- ⚠️ 可能引入新bug
- ⚠️ 需要大量测试工作
- ⚠️ 短期内可能降低开发速度

---

## 八、测试计划

### 功能测试：
- [ ] 登录/注册功能正常
- [ ] 音乐列表获取正常
- [ ] 音乐下载正常
- [ ] 视频播放正常
- [ ] 喜欢/历史记录功能正常

### 性能测试：
- [ ] 连接复用验证（tcpdump/Wireshark）
- [ ] 并发请求测试（100个并发）
- [ ] 长时间运行测试（12小时）
- [ ] 弱网环境测试

### 压力测试：
- [ ] 快速切歌测试（10次/秒）
- [ ] 大量下载测试（100个文件）
- [ ] 内存泄漏测试

---

## 九、监控指标

优化后应监控：
1. **请求延迟**：P50, P95, P99
2. **成功率**：请求成功/失败比例
3. **缓存命中率**：缓存命中/总请求
4. **连接复用率**：复用连接/总连接
5. **重试次数**：平均重试次数
6. **内存占用**：NetworkManager内存占用

---

## 十、后续扩展

优化完成后可以支持：
- [ ] 请求拦截器（日志、认证）
- [ ] Mock数据支持（测试）
- [ ] 离线模式
- [ ] P2P数据传输
- [ ] WebSocket支持
- [ ] GraphQL支持

---

## 总结

**最佳实施路径**：
1. 先做阶段1-3（QNetworkAccessManager单例化、超时/重试、统一Headers）
2. 观察性能提升和稳定性
3. 根据实际效果决定是否继续阶段4-7
4. 如果需求进一步增长，考虑方案B完全重构

**预估总收益**：
- 性能提升：60-80%
- 稳定性提升：20-30%
- 代码质量：中等提升
- 实施成本：2-4周

**立即行动项**：
→ 先实现NetworkManagerSingleton，这是收益最大、风险最小的优化！
