# AudioSession 交互逻辑说明

## 概述
AudioSession 作为协调层，管理 AudioDecoder（解码器）和 AudioPlayer（播放器）之间的交互，实现智能缓冲、状态同步和播放控制。

## 核心交互流程

### 1. 数据流向
```
AudioDecoder (解码线程)
  ├─ decode() 循环读取音频帧
  ├─ FFmpeg 解码 + 重采样
  └─ emit decodedData(QByteArray, timestamp)
       ↓
AudioSession::onDecodedData()
  ├─ 检查缓冲状态
  └─ player->writeAudioData(data, timestamp)
       ↓
AudioPlayer::writeAudioData()
  ├─ buffer->write(data)
  ├─ 记录时间戳队列
  ├─ cv.notify_one() 唤醒播放线程
  └─ emit bufferStatusChanged(fillLevel)
       ↓
AudioPlayer::playbackThread()
  ├─ 等待数据 (cv.wait)
  ├─ buffer->read(chunk)
  └─ audioDevice->write(chunk) → 扬声器输出
```

### 2. 智能缓冲机制

#### 缓冲启动条件
- **触发**: 缓冲区 fillLevel < 20%
- **动作**: 
  - 暂停播放器 (`m_player->pause()`)
  - 设置 `m_isBuffering = true`
  - 发射 `bufferingStarted()` 信号

#### 缓冲恢复条件
- **触发**: 缓冲区 fillLevel >= 80%
- **动作**:
  - 恢复播放器 (`m_player->resume()`)
  - 设置 `m_isBuffering = false`
  - 发射 `bufferingFinished()` 信号

#### 缓冲区饥饿处理
- **触发**: `onBufferUnderrun()` 被调用
- **动作**: 立即暂停播放，等待数据填充

### 3. 播放控制状态机

#### play() - 开始播放
```cpp
void AudioSession::play() {
    // 1. 激活会话
    m_active = true;
    emit sessionStarted();
    
    // 2. 启动解码线程
    m_decoder->startDecode();
    
    // 3. 启动播放线程
    m_player->start();
}
```

#### pause() - 暂停播放
```cpp
void AudioSession::pause() {
    // 1. 暂停解码
    m_decoder->pauseDecode();
    
    // 2. 暂停播放
    m_player->pause();
    // sessionPaused 由播放器信号链发出
}
```

#### resume() - 恢复播放
```cpp
void AudioSession::resume() {
    // 1. 恢复解码
    m_decoder->startDecode();
    
    // 2. 恢复播放
    m_player->resume();
    // sessionResumed 由播放器信号链发出
}
```

#### stop() - 停止播放
```cpp
void AudioSession::stop() {
    // 1. 停止解码
    m_decoder->stopDecode();
    
    // 2. 停止播放
    m_player->stop();
    
    // 3. 清理状态
    m_active = false;
    emit sessionStopped();
}
```

#### seekTo() - 位置跳转
```cpp
void AudioSession::seekTo(positionMs) {
    // 1. 记录当前播放状态
    bool wasPlaying = m_player->isPlaying();
    
    // 2. 暂停解码和播放
    m_decoder->pauseDecode();
    m_player->pause();
    
    // 3. 清空播放器缓冲区（避免旧数据）
    m_player->stop();
    
    // 4. 解码器跳转到新位置
    m_decoder->seekTo(positionMs);
    
    // 5. 恢复之前的播放状态
    if (wasPlaying) {
        m_player->start();
        m_decoder->startDecode();
    }
}
```

### 4. 播放完成检测

#### 自动检测播放结束
```cpp
void AudioSession::onPositionChanged(positionMs) {
    emit positionChanged(positionMs);
    
    // 允许100ms误差，检测播放是否接近结尾
    if (m_duration > 0 && positionMs >= m_duration - 100) {
        onPlaybackFinished();
    }
}

void AudioSession::onPlaybackFinished() {
    // 1. 停止解码器
    m_decoder->stopDecode();
    
    // 2. 停止播放器
    m_player->stop();
    
    // 3. 会话结束
    m_active = false;
    emit sessionFinished();
}
```

#### 解码完成处理
```cpp
void AudioSession::onDecodeCompleted() {
    // 解码器已读取完所有帧
    emit decodeFinished();
    
    // 注意：不立即停止播放
    // 等待缓冲区数据播放完毕
    // 播放完成由 onPositionChanged 检测
}
```

### 5. 信号连接关系

#### 解码器 → Session
```cpp
AudioDecoder::decodedData        → AudioSession::onDecodedData
AudioDecoder::metadataReady      → AudioSession::onMetadataReady
AudioDecoder::albumArtReady      → AudioSession::onAlbumArtReady
AudioDecoder::decodeError        → AudioSession::onDecodeError
AudioDecoder::decodeCompleted    → AudioSession::onDecodeCompleted
AudioDecoder::decodeStarted      → AudioSession::onDecodeStarted
AudioDecoder::decodePaused       → AudioSession::onDecodePaused
AudioDecoder::decodeStopped      → AudioSession::onDecodeStopped
```

#### 播放器 → Session
```cpp
AudioPlayer::positionChanged     → AudioSession::onPositionChanged
AudioPlayer::bufferStatusChanged → AudioSession::onBufferStatusChanged
AudioPlayer::bufferUnderrun      → AudioSession::onBufferUnderrun
AudioPlayer::playbackError       → AudioSession::onPlaybackError
AudioPlayer::playbackStarted     → AudioSession::onPlaybackStarted
AudioPlayer::playbackPaused      → AudioSession::onPlaybackPaused
AudioPlayer::playbackResumed     → AudioSession::onPlaybackResumed
AudioPlayer::playbackStopped     → AudioSession::onPlaybackStopped
```

#### Session → 外部（UI层）
```cpp
sessionStarted()       // 会话开始
sessionPaused()        // 播放暂停
sessionResumed()       // 播放恢复
sessionStopped()       // 播放停止
sessionFinished()      // 播放完成
sessionError(error)    // 错误发生

positionChanged(ms)    // 播放进度更新
durationChanged(ms)    // 时长就绪

bufferingStarted()     // 缓冲开始
bufferingFinished()    // 缓冲完成
decodeFinished()       // 解码完成

metadataReady(...)     // 元数据就绪
albumArtReady(path)    // 封面图片就绪
```

## 关键设计要点

### 1. 线程安全
- AudioDecoder 运行在独立解码线程
- AudioPlayer 运行在独立播放线程
- 通过 Qt 信号槽机制跨线程通信（自动排队连接）
- AudioBuffer 使用 QMutex 保护读写操作

### 2. 智能缓冲
- 双阈值设计：20% 暂停，80% 恢复
- 避免频繁暂停/恢复（滞后效应）
- 缓冲区饥饿时立即响应

### 3. 状态同步
- seekTo 时清空缓冲区，避免播放旧数据
- pause/resume 同时控制解码器和播放器
- 播放完成由时间戳检测，而非解码完成标志

### 4. 错误处理
- 解码错误和播放错误统一通过 `sessionError` 信号上报
- 每个模块独立处理内部错误，Session 负责协调

### 5. 生命周期管理
- Session 拥有 AudioDecoder 和 AudioPlayer 的所有权
- 析构时自动清理资源
- stop() 可安全重复调用

## 使用示例

### 基本播放
```cpp
auto session = new AudioSession("session_001");

// 连接信号
connect(session, &AudioSession::sessionStarted, []() {
    qDebug() << "播放开始";
});
connect(session, &AudioSession::positionChanged, [](qint64 ms) {
    qDebug() << "当前位置:" << ms / 1000 << "秒";
});
connect(session, &AudioSession::sessionFinished, []() {
    qDebug() << "播放完成";
});

// 加载并播放
session->loadSource(QUrl::fromLocalFile("/path/to/music.mp3"));
session->play();
```

### 处理缓冲状态
```cpp
connect(session, &AudioSession::bufferingStarted, [ui]() {
    ui->showBufferingIndicator();
});
connect(session, &AudioSession::bufferingFinished, [ui]() {
    ui->hideBufferingIndicator();
});
```

### 跳转控制
```cpp
// 跳转到30秒位置
session->seekTo(30000);

// 跳转后会自动恢复之前的播放状态
// 如果之前在播放，跳转后继续播放
// 如果之前暂停，跳转后保持暂停
```

## 性能优化

1. **减少信号发射频率**
   - `positionChanged` 由定时器控制（默认100ms间隔）
   - `bufferStatusChanged` 仅在数据写入时发射

2. **内存管理**
   - AudioBuffer 动态扩展（4KB → 10MB）
   - 解码数据通过 QByteArray 自动引用计数

3. **线程调度**
   - 解码线程使用条件变量等待
   - 播放线程使用条件变量等待
   - 避免忙等待（busy-waiting）

## 待优化项

1. **网络流支持**
   - 当前仅支持本地文件
   - 需要扩展 `loadSource()` 支持 HTTP/RTSP

2. **预加载机制**
   - 播放列表中的下一首歌曲可预解码

3. **动态缓冲策略**
   - 根据网络速度动态调整缓冲阈值

4. **A/V 同步**
   - 当前仅处理音频
   - 可扩展支持视频同步

---

# AudioService 使用指南

## 概述
AudioService 是顶层服务单例，管理播放列表、播放模式和全局状态。

## 功能特性

### 1. 单例访问
```cpp
AudioService& service = AudioService::instance();
```

### 2. 播放列表管理
```cpp
// 设置播放列表
QList<QUrl> playlist;
playlist << QUrl::fromLocalFile("/music/song1.mp3");
playlist << QUrl::fromLocalFile("/music/song2.mp3");
playlist << QUrl::fromLocalFile("/music/song3.flac");
service.setPlaylist(playlist);

// 添加单曲
service.addToPlaylist(QUrl::fromLocalFile("/music/song4.mp3"));

// 插入到指定位置
service.insertToPlaylist(1, QUrl::fromLocalFile("/music/insert.mp3"));

// 移动歌曲位置
service.moveInPlaylist(0, 2);  // 将第0首移到第2位

// 删除歌曲
service.removeFromPlaylist(1);

// 清空列表
service.clearPlaylist();
```

### 3. 播放模式
```cpp
// 顺序播放（默认）
service.setPlayMode(AudioService::Sequential);

// 单曲循环
service.setPlayMode(AudioService::RepeatOne);

// 列表循环
service.setPlayMode(AudioService::RepeatAll);

// 随机播放（智能避免重复）
service.setPlayMode(AudioService::Shuffle);
```

### 4. 播放控制
```cpp
// 播放整个列表（从第一首开始）
service.playPlaylist();

// 播放指定索引
service.playAtIndex(2);

// 下一首
service.playNext();

// 上一首
service.playPrevious();

// 暂停
service.pause();

// 恢复
service.resume();

// 停止
service.stop();

// 跳转
service.seekTo(30000);  // 跳转到30秒
```

### 5. 音量控制
```cpp
// 设置全局音量（0-100）
service.setVolume(75);

// 获取音量
int vol = service.volume();
```

### 6. 状态查询
```cpp
// 是否正在播放
bool playing = service.isPlaying();

// 是否暂停
bool paused = service.isPaused();

// 当前索引
int index = service.currentIndex();

// 当前URL
QUrl url = service.currentUrl();

// 列表大小
int size = service.playlistSize();
```

### 7. 信号连接
```cpp
// 播放状态
connect(&service, &AudioService::playbackStarted, [](const QString& sessionId) {
    qDebug() << "开始播放，会话ID:" << sessionId;
});

connect(&service, &AudioService::playbackPaused, []() {
    qDebug() << "播放暂停";
});

connect(&service, &AudioService::playbackResumed, []() {
    qDebug() << "播放恢复";
});

connect(&service, &AudioService::playbackStopped, []() {
    qDebug() << "播放停止";
});

// 当前歌曲信息
connect(&service, &AudioService::currentTrackChanged, 
        [](const QString& title, const QString& artist, qint64 duration) {
    qDebug() << "当前歌曲:" << title << "-" << artist;
    qDebug() << "时长:" << duration / 1000 << "秒";
});

// 封面图片
connect(&service, &AudioService::albumArtChanged, [](const QString& path) {
    qDebug() << "封面图片:" << path;
    // 加载并显示图片
});

// 播放进度
connect(&service, &AudioService::positionChanged, [](qint64 ms) {
    qDebug() << "进度:" << ms / 1000 << "秒";
});

// 播放列表变化
connect(&service, &AudioService::playlistChanged, []() {
    qDebug() << "播放列表已更新";
});

// 当前索引变化
connect(&service, &AudioService::currentIndexChanged, [](int index) {
    qDebug() << "当前播放第" << index + 1 << "首";
});

// 错误处理
connect(&service, &AudioService::serviceError, [](const QString& error) {
    qWarning() << "服务错误:" << error;
});
```

## 完整示例

### 音乐播放器应用
```cpp
#include "AudioService.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 获取服务实例
    AudioService& service = AudioService::instance();
    
    // 构建播放列表
    QList<QUrl> playlist;
    playlist << QUrl::fromLocalFile("E:/Music/周杰伦 - 晴天.mp3");
    playlist << QUrl::fromLocalFile("E:/Music/林俊杰 - 江南.flac");
    playlist << QUrl::fromLocalFile("E:/Music/陈奕迅 - 十年.mp3");
    
    service.setPlaylist(playlist);
    service.setPlayMode(AudioService::RepeatAll);
    service.setVolume(80);
    
    // 连接信号
    QObject::connect(&service, &AudioService::currentTrackChanged,
                     [](const QString& title, const QString& artist, qint64 duration) {
        qDebug() << "正在播放:" << title << "-" << artist;
        qDebug() << "总时长:" << duration / 1000 << "秒";
    });
    
    QObject::connect(&service, &AudioService::positionChanged, [](qint64 ms) {
        int seconds = ms / 1000;
        qDebug() << QString("进度: %1:%2").arg(seconds / 60).arg(seconds % 60, 2, 10, QChar('0'));
    });
    
    // 开始播放
    service.playPlaylist();
    
    return app.exec();
}
```

## 随机播放逻辑

AudioService 实现了智能随机播放：

1. **避免立即重复**: 维护播放历史（最近播放列表的一半）
2. **历史回退**: 点击"上一首"可返回历史播放的歌曲
3. **随机种子**: 使用 QRandomGenerator 保证随机性

```cpp
service.setPlayMode(AudioService::Shuffle);
service.playPlaylist();  // 随机开始

// 每次 playNext() 都会智能选择未播放或很久没播放的歌曲
service.playNext();

// playPrevious() 会返回历史播放的歌曲
service.playPrevious();
```

## 内存管理

AudioService 自动管理会话生命周期：
- 自动清理旧会话（保留当前和最近2个）
- 避免内存泄漏
- 线程安全的析构

## 最佳实践

1. **始终使用单例**: `AudioService::instance()`
2. **播放前设置列表**: 先 `setPlaylist()` 再 `playPlaylist()`
3. **处理错误信号**: 连接 `serviceError` 信号
4. **优雅退出**: 应用退出前调用 `stop()` 停止播放
