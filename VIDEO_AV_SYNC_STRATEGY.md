# 视频音画同步策略说明（当前实现）

## 1. 总体策略

当前实现采用**音频主时钟（Audio Master Clock）**策略：

- 音频播放位置作为全局时间基准。
- 视频渲染线程根据音频时钟决定“等待渲染”或“丢帧追赶”。
- `MediaSession` 负责解复用、seek 后时钟重建、状态协同。
- `VideoRendererGL` 负责逐帧与音频时钟对齐。

---

## 2. 核心组件与职责

### `MediaSession`

文件：`MediaSession.cpp` / `MediaSession.h`

- 维护会话级时钟：`m_masterClock`
- 控制播放状态：`play/pause/stop/seekTo`
- 解复用线程输出：
  - 音频帧 -> `AudioPlayer::writeAudioData(...)`
  - 视频包 -> `VideoSession::pushPacket(...)`
- seek 后通过首个有效音频帧 PTS 同步音频时钟（`m_seekPending`）。

### `AudioPlayer`

文件：`AudioPlayer.cpp` / `AudioPlayer.h`

- 提供主时钟来源：`getPlaybackPosition()`
- 支持 owner 机制，避免音频/视频会话互相写入污染
- `setCurrentTimestamp(...)` 用于 seek / owner 切换后的时钟重锚定

### `VideoSession`

文件：`VideoSession.cpp` / `VideoSession.h`

- 视频解码与缓冲管理（`VideoDecoder + VideoBuffer`）
- 通过渲染器回读当前视频 PTS：`getCurrentPTS()`

### `VideoRendererGL`

文件：`VideoRendererGL.cpp` / `VideoRendererGL.h`

- 在 `renderNextFrame()` 中执行实际 A/V 对齐策略
- 基于 `AudioPlayer::getPlaybackPosition()` 决定视频帧处理方式

---

## 3. 播放时序（简化）

1. `MediaSession::play()` 启动/恢复音频与视频链路。  
2. Demux 线程持续输出：
   - 音频帧（携带 PTS）进入 `AudioPlayer`
   - 视频包进入 `VideoSession/VideoBuffer`
3. `VideoRendererGL::renderNextFrame()` 定时取视频帧，对比音频时钟并执行同步策略。  
4. UI 进度通过 `MediaSession::updatePosition()` 获取当前位置（优先音频时钟）。

---

## 4. 当前同步判定规则

实现位置：`VideoRendererGL::renderNextFrame()`

- **视频超前**：`frame->pts > audioClock + 100ms`  
  -> 暂不渲染该帧，等待音频追上。

- **视频落后**：`audioClock > frame->pts + 200ms`  
  -> 丢弃旧帧追赶音频（循环丢帧，最多一次 100 帧）。

- **可播放窗口内**：  
  -> 渲染当前帧并更新 `m_lastPTS`。

这是一种常见的“音频稳定优先，视频柔性追赶”策略。

---

## 5. Seek / 恢复的同步处理

实现位置：`MediaSession::seekTo()` + demux 音频解码段

- seek 时执行：
  - 停 demux、`av_seek_frame(...)`
  - flush 音频解码器与视频缓冲
  - `AudioPlayer::resetBuffer()`
  - `m_seekPending = true`
  - `m_masterClock = targetPos`
- seek 后首个音频帧到达时：
  - `AudioPlayer::setCurrentTimestamp(inputPtsMs)`
  - `m_seekPending = false`

这样可避免 seek 后沿用旧时钟导致的音画漂移。

---

## 6. 主时钟选择逻辑

实现位置：`MediaSession::getCurrentPosition()`

优先级：

1. 若音频链路有效且 owner 匹配 -> 返回 `AudioPlayer::getPlaybackPosition()`
2. 否则若有视频 -> 返回 `VideoSession::getCurrentPTS()`
3. 否则返回 `m_masterClock`

---

## 7. 当前限制与现状

`MediaSession::syncAudioVideo()` 中的“主动校正”逻辑仍是占位：

- `videoPos` 目前未接入真实视频 PTS 比较（代码中有 TODO）
- `holdFrame()/skipNonKeyFrames()` 只在接口层保留，主同步仍依赖渲染线程的等待/丢帧策略

结论：当前真正生效的同步核心在 `VideoRendererGL::renderNextFrame()`。

---

## 8. 调参建议（现有框架下）

- `+100ms`（视频超前等待阈值）可下调到 `60~80ms`，减少“画面慢半拍”感。
- `+200ms`（视频落后丢帧阈值）可按内容类型调节：
  - MV/演唱会：`120~160ms`（更追求口型）
  - 普通内容：`180~250ms`（更平滑）
- 首次起播可考虑“最小音频缓冲阈值 + 首帧门限”联动，降低开播抖动。

---

## 9. 排障日志关键点

建议重点关注以下日志：

- `[MediaSession] Seek completed - clock synced to actual PTS: ...`
- `AudioPlayer: Timestamp set to ...`
- `AudioPlayer: Position update: ...`
- `VideoRendererGL::renderNextFrame` 中的等待/丢帧行为（可按需补调试日志）

用于判断是“时钟锚点错误”还是“渲染追帧策略阈值不合适”。

