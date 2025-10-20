# 全局状态机 + 双缓冲架构设计文档

## 🎯 架构优化目标

### 当前问题
1. **状态管理分散**: 播放状态散布在Worker、ControlBar、PlayWidget中
2. **缓冲区单一**: 跳转时清空唯一缓冲区导致播放中断  
3. **信号时序混乱**: 异步信号没有统一协调
4. **用户体验差**: 跳转延迟、播放延迟、状态不同步

### 目标效果
- ✅ **统一状态管理**: 所有播放状态由全局状态机统一管理
- ✅ **无缝跳转**: 双缓冲实现跳转时不中断播放
- ✅ **响应迅速**: 状态变化立即反映到UI
- ✅ **逻辑清晰**: 组件职责明确，易于维护

## 🏗️ 架构设计

### 1. 全局播放状态机 (PlaybackStateManager)

```
           ┌─────────────┐
           │   Stopped   │◄──────────┐
           └──────┬──────┘           │
                  │ play             │ stop
                  ▼                  │
           ┌─────────────┐           │
           │   Loading   │           │
           └──────┬──────┘           │
                  │ loadComplete     │
                  ▼                  │
┌─────────┐   ┌─────────────┐   ┌───────────┐
│ Seeking │◄──┤   Playing   ├──►│  Paused   │
└─────┬───┘   └─────────────┘   └─────┬─────┘
      │              │                │
      │ seekComplete │ seek           │ play
      └──────────────┼────────────────┘
                     ▼
              ┌─────────────┐
              │    Error    │
              └─────────────┘
```

**职责**:
- 统一管理所有播放状态
- 协调各组件的状态转换
- 提供状态查询接口
- 发送控制指令给各组件

### 2. 双缓冲音频管理器 (DualBufferManager)

```
┌─────────────────┐    跳转时    ┌─────────────────┐
│   活动缓冲区    │◄──────────────┤   跳转缓冲区    │
│  (正在播放)     │    交换       │  (准备数据)     │
│                 │              │                 │
│ PCM Data 1      │              │ New PCM Data 1  │
│ PCM Data 2      │              │ New PCM Data 2  │
│ PCM Data 3      │              │ New PCM Data 3  │
│ ...             │              │ ...             │
└─────────────────┘              └─────────────────┘
        │                                 ▲
        │ 音频输出                         │
        ▼                                 │ TakePcm解码
┌─────────────────┐                      │
│   音频设备      │──────────────────────┘
└─────────────────┘
```

**职责**:
- 管理两个PCM缓冲区
- 跳转时数据预加载到备用缓冲区
- 跳转完成时瞬间交换缓冲区
- 提供缓冲区状态查询

## 🔧 组件重构方案

### 1. Worker重构
```cpp
class Worker : public QObject {
    // 移除状态管理职责，专注音频播放
private:
    DualBufferManager* m_bufferManager;  // 使用双缓冲管理器
    PlaybackStateManager& m_stateManager; // 引用全局状态机
    
public slots:
    void onPlayRequested();    // 响应状态机的播放指令
    void onPauseRequested();   // 响应状态机的暂停指令  
    void onSeekRequested(qint64 pos); // 响应状态机的跳转指令
};
```

### 2. PlayWidget重构  
```cpp
class PlayWidget : public QWidget {
    // 移除复杂的信号连接，通过状态机协调
private:
    PlaybackStateManager& m_stateManager;
    
private slots:
    void onStateChanged(PlaybackState newState, PlaybackState oldState);
    void slot_play_click(); // 简化为调用状态机
};
```

### 3. TakePcm重构
```cpp
class TakePcm : public QObject {
    // 专注解码，通过双缓冲管理器输出数据
private:
    DualBufferManager* m_bufferManager;
    
public slots:
    void seekToPosition(qint64 position); // 配合双缓冲实现跳转
};
```

## 🚀 实施步骤

### Phase 1: 基础架构 (不影响现有功能)
1. ✅ 实现PlaybackStateManager类
2. ✅ 实现DualBufferManager类  
3. ✅ 编写单元测试验证逻辑

### Phase 2: 渐进式集成
1. 🔄 在PlayWidget中集成状态机 (保持原有逻辑作为备份)
2. 🔄 在Worker中集成双缓冲 (并行运行，逐步切换)
3. 🔄 验证基本播放功能正常

### Phase 3: 深度集成
1. 🔄 重构信号连接，统一使用状态机协调
2. 🔄 实现无缝跳转功能
3. 🔄 移除旧的状态管理代码

### Phase 4: 优化完善
1. 🔄 性能优化和内存管理
2. 🔄 错误处理和异常恢复
3. 🔄 用户体验细节打磨

## 📊 预期效果

### 跳转体验改善
```
用户拖动进度条
    ↓
状态机: Playing → Seeking
    ↓
双缓冲: 准备跳转缓冲区
    ↓
TakePcm: 解码新位置数据到跳转缓冲区
    ↓
双缓冲: 瞬间交换缓冲区
    ↓
状态机: Seeking → Playing
    ↓
用户听到: 无缝跳转，无停顿
```

### 播放控制改善
```
用户点击播放按钮
    ↓
状态机: 立即UI反馈 + 发送播放指令
    ↓
Worker: 响应播放指令
    ↓
状态机: 确认播放状态
    ↓
用户体验: 立即响应，状态一致
```

## 🎯 关键优势

1. **职责分离**: 每个组件职责明确，易于维护
2. **状态一致**: 全局状态机确保状态同步
3. **无缝跳转**: 双缓冲实现真正的无缝跳转
4. **易于扩展**: 添加新功能只需扩展状态机
5. **错误处理**: 统一的错误状态管理

## ⚠️ 实施注意事项

1. **渐进式迁移**: 不要一次性替换所有代码
2. **保持兼容**: 迁移过程中保持现有功能正常
3. **充分测试**: 每个阶段都要验证功能正常
4. **性能监控**: 关注内存使用和CPU占用

---
*设计时间: 2025年10月19日*
*架构类型: 状态机 + 双缓冲*  
*目标: 解决播放控制和跳转问题*