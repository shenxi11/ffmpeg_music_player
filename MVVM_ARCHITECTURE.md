# MVVM 架构重构方案（渐进式）- 已更新

## ✅ 最新进展（2026-02-01）

**Phase 0: 清理冲突** ✅ 完成
- ✅ 发现并处理重复的Session定义（audio_session vs AudioSession）
- ✅ 重命名旧文件为.deprecated而非删除
- ✅ 确认CMakeLists.txt使用正确的AudioSession

**Phase 1: 建立ViewModel基础设施** ✅ 完成
- ✅ 创建 `viewmodels/BaseViewModel.h` - 提供通用MVVM基础设施
- ✅ 创建 `viewmodels/PlaybackViewModel.h/cpp` - 播放器核心ViewModel
- ✅ 添加到CMakeLists.txt编译配置
- ✅ 完整的Q_PROPERTY和Q_INVOKABLE设计
- ✅ 完善AudioService信号连接

**Phase 2: 改造PlayWidget集成ViewModel** ✅ 完成
- ✅ PlayWidget添加ViewModel实例和Q_PROPERTY
- ✅ 连接ViewModel信号到UI更新
- ✅ 迁移3个核心播放方法（支持条件编译）
  - `_play_click(QString)` - 播放指定歌曲
  - `slot_play_click()` - 播放/暂停切换  
  - `slot_lyric_seek(int)` - 进度跳转
- ✅ 使用 `#define USE_MVVM_PLAYBACK` 控制新旧模式

**当前状态**: 🎉 基础MVVM架构已完成，可以开始使用或继续扩展

**下一步（可选）**: 
- 测试编译和运行
- 迁移更多方法
- 清理QML包装类

**详细文档**: 查看 [MVVM_IMPLEMENTATION_SUMMARY.md](MVVM_IMPLEMENTATION_SUMMARY.md)

---

## 重构原则
✅ **保持现有文件结构**，不大规模移动文件  
✅ **渐进式重构**，新旧代码可以共存  
✅ **每次只改一个模块**，保持项目可编译  
✅ **优先使用 Q_PROPERTY**，让 QML 可以直接绑定  

## 当前架构问题分析

```
❌ 问题 1: Widget 直接操作 Service
PlayWidget → AudioService::play()
应改为：PlayWidget → 发射信号 → Controller → AudioService

❌ 问题 2: 业务逻辑在 Widget 中
PlayWidget 包含歌词加载、网络请求等逻辑
应改为：Widget 只负责 UI，业务逻辑移到 Service/Controller

❌ 问题 3: QML Wrapper 类功能重复
*_qml.h 类只是简单包装 QML 组件
应改为：使用 Q_PROPERTY 直接暴露数据
```

## 标准 MVVM 模式（适用于 Qt）

```
┌─────────────────────────────────────┐
│         View (QML + QWidget)         │
│  - 只负责 UI 渲染                     │
│  - 通过属性绑定读取数据                │
│  - 调用 ViewModel 的方法              │
│  - 不包含业务逻辑                     │
└──────────┬──────────────────────────┘
           │ Q_PROPERTY + signals/slots
┌──────────▼──────────────────────────┐
│      ViewModel (C++ Widget)          │
│  - 使用 Q_PROPERTY 暴露属性           │
│  - Q_INVOKABLE 方法供调用             │
│  - 处理 UI 交互逻辑                   │
│  - 协调 Model 层调用                  │
└──────────┬──────────────────────────┘
           │ 方法调用
┌──────────▼──────────────────────────┐
│         Model (Service层)            │
│  - AudioService (单例)                │
│  - MediaController (单例)             │
│  - 纯业务逻辑，不依赖 UI               │
└─────────────────────────────────────┘
```

## 实施步骤

### Phase 1: 改造 Service 层（已完成）
- ✅ AudioService 单例模式
- ✅ MediaController 单例模式
- ✅ Service 不依赖 Widget

### Phase 2: 改造现有 Widget 为 ViewModel
不创建新文件，直接在现有类中添加 MVVM 特性：

#### 2.1 ProcessSliderQml（进度条）
```cpp
// 添加 Q_PROPERTY
Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
Q_PROPERTY(qint64 duration READ duration WRITE setDuration NOTIFY durationChanged)
Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)

// 添加 Q_INVOKABLE 方法
Q_INVOKABLE void seek(qint64 pos);
```

#### 2.2 RotatingCircleImage（旋转封面）
```cpp
Q_PROPERTY(QString imageUrl READ imageUrl WRITE setImageUrl NOTIFY imageUrlChanged)
Q_PROPERTY(bool rotating READ isRotating WRITE setRotating NOTIFY rotatingChanged)
```

#### 2.3 PlayWidget（主播放器）
```cpp
// 暴露播放状态
Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
Q_PROPERTY(QString currentTrack READ currentTrack NOTIFY currentTrackChanged)
Q_PROPERTY(QString albumArt READ albumArt NOTIFY albumArtChanged)

// 暴露命令方法
Q_INVOKABLE void play(const QString& filePath);
Q_INVOKABLE void pause();
Q_INVOKABLE void togglePlayPause();
```

### Phase 3: 清理冗余代码
- 移除重复的 getter/setter
- 统一信号命名（使用 xxxChanged）
- 移除不必要的 wrapper 类

### Phase 4: QML 数据绑定
将 QML 中的命令式调用改为声明式绑定：
```qml
// ❌ 旧方式：命令式
onClicked: { playWidget.slot_play_click() }

// ✅ 新方式：声明式
enabled: playWidget.isPlaying
onClicked: playWidget.togglePlayPause()
```

## 改造示例

### 示例 1: ProcessSliderQml

**改造前（部分代码）：**
```cpp
class ProcessSliderQml : public QQuickWidget {
    Q_OBJECT
public:
    void setPosition(qint64 pos);  // 只是普通方法
    void setDuration(qint64 dur);
};
```

**改造后：**
```cpp
class ProcessSliderQml : public QQuickWidget {
    Q_OBJECT
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration WRITE setDuration NOTIFY durationChanged)
    
public:
    qint64 position() const { return m_position; }
    qint64 duration() const { return m_duration; }
    
    void setPosition(qint64 pos) {
        if (m_position != pos) {
            m_position = pos;
            emit positionChanged();  // 自动通知 QML
        }
    }
    
signals:
    void positionChanged();
    void durationChanged();
    
private:
    qint64 m_position = 0;
    qint64 m_duration = 0;
};
```

## 迁移检查清单

每改造一个类，确保：
- [ ] 添加 Q_PROPERTY 暴露状态
- [ ] 使用 xxxChanged 信号命名
- [ ] Q_INVOKABLE 标记公共方法
- [ ] 移除业务逻辑到 Service 层
- [ ] QML 使用属性绑定而非直接调用
- [ ] 编译通过，功能正常

## 当前状态
- ✅ Service 层已解耦
- ⏳ Widget 层待改造为 ViewModel
- ⏳ QML 层待改为数据绑定
