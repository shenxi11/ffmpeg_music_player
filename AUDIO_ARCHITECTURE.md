# 新音频播放架构设计文档

## 架构概览

新架构采用**分层模块化设计**，将音频播放功能拆分为5个核心模块，职责清晰、低耦合、高内聚。

```
┌─────────────────────────────────────────────────────────┐
│                   AudioService.h                        │
│              (服务协调层 - 单例模式)                      │
│   • 多会话管理  • 播放列表  • 全局控制                   │
└────────────────────┬────────────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        │   AudioSession.h        │
        │   (会话管理层)           │
        │   • 单曲播放会话         │
        │   • 解码器-播放器协调    │
        └────┬───────────┬────────┘
             │           │
    ┌────────▼──────┐  ┌▼─────────────┐
    │ AudioDecoder  │  │ AudioPlayer  │
    │ (解码引擎)     │  │ (播放控制器)  │
    │ • FFmpeg解码  │  │ • Qt音频输出  │
    │ • 重采样      │  │ • 时钟同步    │
    └───────┬───────┘  └──┬───────────┘
            │              │
            └──────┬───────┘
                   │
            ┌──────▼──────┐
            │ AudioBuffer │
            │ (环形缓冲区) │
            │ • 线程安全   │
            │ • 动态扩容   │
            └─────────────┘
```

---

## 模块职责详解

### 1. AudioBuffer.h - 环形缓冲区模块
**职责**：
- 提供线程安全的音频数据缓冲
- 支持动态扩容（4KB → 10MB）
- 管理环形读写指针

**核心API**：
```cpp
int write(const char* data, int bytes);  // 写入数据
int read(char* dest, int bytes);         // 读取数据
int availableBytes() const;              // 可读字节数
void clear();                            // 清空缓冲区
```

**对应原文件**：`audio_ringbuffer.h` → 重命名即可

---

### 2. AudioDecoder.h - 解码引擎模块
**职责**：
- FFmpeg音频解封装、解码
- 音频重采样（统一输出格式）
- 提取封面图片元数据
- 独立解码线程

**核心API**：
```cpp
bool initDecoder(const QString& filePath);  // 初始化解码器
void startDecode();                         // 开始解码
void seekTo(qint64 positionMs);            // 定位跳转

// 信号
void decodedData(const QByteArray& data, qint64 timestampMs);
void metadataReady(qint64 durationMs, int sampleRate, int channels);
void albumArtReady(const QString& imagePath);
```

**迁移方案**：
- 基础来源：`take_pcm.h`
- 需重构为独立解码器，去除播放逻辑
- 保留解码核心代码，重新组织接口

---

### 3. AudioPlayer.h - 播放控制器模块
**职责**：
- Qt音频硬件输出管理
- 播放控制（播放/暂停/停止）
- 音频时钟同步（PTS管理）
- 音量控制

**核心API**：
```cpp
bool start();                              // 开始播放
void pause() / resume() / stop();         // 控制播放
void setVolume(int volume);               // 音量 0-100
void writeAudioData(const QByteArray&, qint64 ts);  // 接收解码数据
qint64 getCurrentTimestamp() const;       // 获取当前播放时间

// 信号
void positionChanged(qint64 positionMs);  // 进度更新
void bufferUnderrun();                    // 缓冲区饥饿
```

**迁移方案**：
- 融合：`worker.h` + `video_audio_player.h`
- 提取播放控制逻辑，剥离解码依赖
- 使用AudioBuffer作为数据源

---

### 4. AudioSession.h - 会话管理模块
**职责**：
- 管理单个音频播放会话
- 协调解码器和播放器
- 元数据管理（标题/艺术家/封面）
- 缓冲状态监控

**核心API**：
```cpp
bool loadSource(const QUrl& url);         // 加载音频源
void play() / pause() / stop();          // 会话控制
void seekTo(qint64 positionMs);          // 定位
QString title() / artist();               // 元数据访问

// 信号
void metadataReady(QString title, QString artist, qint64 duration);
void positionChanged(qint64 positionMs);
void bufferingProgress(int percent);
```

**对应原文件**：`audio_session.h` → 需扩展功能

---

### 5. AudioService.h - 服务协调模块
**职责**：
- 全局单例服务
- 多会话管理（支持预加载）
- 播放列表管理
- 播放模式（顺序/循环/随机）
- 全局音量控制

**核心API**：
```cpp
static AudioService& instance();          // 单例访问

// 播放控制
bool play(const QUrl& url);               // 播放URL
void playNext() / playPrevious();        // 切歌

// 播放列表
void setPlaylist(const QList<QUrl>& urls);
void setPlayMode(PlayMode mode);          // Sequential/RepeatOne/RepeatAll/Shuffle

// 会话管理
QString createSession();                  // 创建新会话
AudioSession* currentSession();           // 获取当前会话

// 信号
void currentTrackChanged(QString title, QString artist, qint64 duration);
void positionChanged(qint64 positionMs);
```

**新增模块** - 整合原 `play_widget.h` 的播放列表管理逻辑

---

## 数据流向

### 播放流程
```
1. UI调用 → AudioService::play(url)
2. AudioService创建AudioSession
3. AudioSession初始化AudioDecoder + AudioPlayer
4. AudioDecoder解码 → decodedData信号
5. AudioSession转发数据 → AudioPlayer::writeAudioData()
6. AudioPlayer写入AudioBuffer → 播放线程读取输出
7. 定时器触发 → positionChanged信号 → UI更新进度
```

### 信号传递链
```
AudioDecoder::decodedData 
    → AudioSession::onDecodedData 
    → AudioPlayer::writeAudioData 
    → AudioPlayer::positionChanged 
    → AudioSession::onPositionChanged 
    → AudioService::onPositionChanged 
    → UI更新
```

---

## 迁移步骤

### 阶段1：重命名和整理
```bash
# 1. 环形缓冲区（已完成）
audio_ringbuffer.h → AudioBuffer.h ✅

# 2. 会话管理（需扩展）
audio_session.h → AudioSession.h（新版本）
```

### 阶段2：拆分解码器
```cpp
// 从 take_pcm.h 提取核心解码逻辑
// 移除：
// - 播放控制逻辑（移至AudioPlayer）
// - UI交互代码（移至AudioSession）
// 保留：
// - FFmpeg解封装/解码
// - SwrContext重采样
// - 封面提取
```

### 阶段3：整合播放器
```cpp
// 融合 worker.h + video_audio_player.h
// 统一接口：
// - QAudioOutput管理（来自video_audio_player.h）
// - 播放控制逻辑（来自worker.h）
// - 使用AudioBuffer替代原有队列
```

### 阶段4：构建服务层
```cpp
// 新建 AudioService
// 接管 play_widget.h 的播放列表管理
// 实现：
// - 单例模式
// - 播放队列
// - 播放模式切换
```

---

## 优势对比

### 旧架构问题
- ❌ 模块耦合严重（take_pcm既解码又管理播放）
- ❌ 职责不清（worker既播放又管理UI状态）
- ❌ 难以测试（UI和业务逻辑混杂）
- ❌ 扩展困难（添加新功能需要修改多个类）

### 新架构优势
- ✅ **单一职责**：每个模块只做一件事
- ✅ **低耦合**：通过信号槽解耦，易于替换组件
- ✅ **可测试**：核心模块独立，可单元测试
- ✅ **易扩展**：新增播放源只需实现新的AudioDecoder子类
- ✅ **高复用**：AudioBuffer/AudioPlayer可用于视频、网络流等场景

---

## 使用示例

### 简单播放
```cpp
// 方式1：通过服务层
AudioService::instance().play(QUrl::fromLocalFile("/path/to/music.mp3"));

// 方式2：手动创建会话
AudioSession* session = new AudioSession("session_1");
session->loadSource(QUrl::fromLocalFile("/path/to/music.mp3"));
session->play();
```

### 播放列表
```cpp
AudioService& service = AudioService::instance();

// 设置播放列表
QList<QUrl> playlist = {
    QUrl::fromLocalFile("song1.mp3"),
    QUrl::fromLocalFile("song2.mp3"),
    QUrl::fromLocalFile("song3.mp3")
};
service.setPlaylist(playlist);

// 设置循环模式
service.setPlayMode(AudioService::RepeatAll);

// 播放第一首
service.playAtIndex(0);

// 自动播放下一首
connect(&service, &AudioService::currentSessionChanged, [&](const QString& id) {
    qDebug() << "Now playing:" << service.currentSession()->title();
});
```

### 监听进度
```cpp
AudioSession* session = AudioService::instance().currentSession();

connect(session, &AudioSession::positionChanged, [](qint64 ms) {
    qDebug() << "Position:" << ms << "ms";
});

connect(session, &AudioSession::metadataReady, 
        [](const QString& title, const QString& artist, qint64 duration) {
    qDebug() << "Now playing:" << title << "by" << artist;
});
```

---

## 下一步工作

### 必须实现
1. [ ] 完成AudioDecoder.cpp实现（基于take_pcm.cpp重构）
2. [ ] 完成AudioPlayer.cpp实现（融合worker.cpp + video_audio_player.cpp）
3. [ ] 完成AudioSession.cpp实现（扩展audio_session.cpp）
4. [ ] 完成AudioService.cpp实现（新建）

### 可选优化
5. [ ] 添加网络流支持（HttpAudioDecoder继承AudioDecoder）
6. [ ] 实现音频均衡器插件
7. [ ] 支持无损音频格式（FLAC/APE/DSD）
8. [ ] 添加音频可视化接口（频谱/波形）

---

## 注意事项

1. **线程安全**：AudioBuffer必须使用互斥锁保护
2. **内存管理**：FFmpeg资源必须在析构时释放
3. **信号槽**：跨线程信号连接使用Qt::QueuedConnection
4. **错误处理**：所有公共接口需捕获异常并发出错误信号

---

**创建时间**：2026年1月20日  
**版本**：v1.0  
**状态**：架构设计完成，待实现
