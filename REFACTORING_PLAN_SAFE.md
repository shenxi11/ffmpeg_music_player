# 安全重构计划 - FFmpeg音乐播放器

**制定日期**: 2026年2月1日  
**目标**: 渐进式重构为MVVM架构，确保每一步都可编译可运行

---

## 🚨 重构原则（最重要）

1. **每改一个文件立即测试编译** - 不积累错误
2. **保留旧代码** - 使用条件编译或注释，不删除
3. **双轨运行** - 新旧代码可以共存，逐步切换
4. **小步快跑** - 每次只改一个类，改完测试
5. **先统一再重构** - 解决架构冲突后再做MVVM

---

## 📊 当前架构诊断

### ❌ 严重问题

1. **Session重复定义**
   - `AudioSession.h/cpp` (新架构)
   - `audio_session.h/cpp` (旧架构)
   - **风险**: 符号冲突、链接错误

2. **Service重复定义**
   - `AudioService.h/cpp` (协调器)
   - `MediaService.h/cpp` (似乎功能重叠)
   - **风险**: 职责不清

3. **大量QML包装类** (18个 `*_qml.h`)
   - `loginwidget_qml.h`, `userwidget_qml.h`, `music_item_qml.h`...
   - **问题**: 增加维护成本，功能重复

4. **PlayWidget职责过重**
   - 包含UI、业务逻辑、网络请求、歌词解析
   - 代码行数可能超过1000行

### ✅ 良好设计

- AudioPlayer单例模式设计合理
- AudioDecoder/AudioBuffer分层清晰

---

## 🎯 分阶段重构路线图

### Phase 0: 清理冲突（第一优先级）⚠️

**目标**: 消除重复定义，建立统一基础架构

#### 0.1 分析Session冲突
```bash
# 检查两个Session的使用情况
grep -r "AudioSession" --include="*.cpp" --include="*.h"
grep -r "audio_session" --include="*.cpp" --include="*.h"
```

#### 0.2 决定保留哪个
**建议**: 保留 `AudioSession.h/cpp`（新架构）
- 理由: 与AudioService配套，职责更清晰
- 操作: 
  1. 检查 `audio_session.h/cpp` 的独特功能
  2. 迁移独特功能到 `AudioSession`
  3. 重命名 `audio_session.*` 为 `audio_session_old.*`
  4. 添加弃用警告宏

#### 0.3 统一Service层
**目标**: 明确AudioService和MediaService的职责

建议职责划分:
```
AudioService
├── 音频播放控制
├── 音频会话管理
└── 音频播放列表

MediaService  
├── 媒体元数据管理
├── 网络资源获取
└── 缓存管理
```

---

### Phase 1: 建立ViewModel基础设施

#### 1.1 创建BaseViewModel抽象类
**文件**: `BaseViewModel.h`

```cpp
#ifndef BASEVIEWMODEL_H
#define BASEVIEWMODEL_H

#include <QObject>

/**
 * @brief ViewModel基类
 * 提供MVVM模式的基础设施
 */
class BaseViewModel : public QObject
{
    Q_OBJECT
    
    // 通用属性
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    
public:
    explicit BaseViewModel(QObject *parent = nullptr)
        : QObject(parent), m_isBusy(false) {}
    
    virtual ~BaseViewModel() = default;
    
    // 属性访问器
    bool isBusy() const { return m_isBusy; }
    QString errorMessage() const { return m_errorMessage; }
    
protected:
    // 属性设置器（子类使用）
    void setIsBusy(bool busy) {
        if (m_isBusy != busy) {
            m_isBusy = busy;
            emit isBusyChanged();
        }
    }
    
    void setErrorMessage(const QString& error) {
        if (m_errorMessage != error) {
            m_errorMessage = error;
            emit errorMessageChanged();
        }
    }
    
signals:
    void isBusyChanged();
    void errorMessageChanged();
    
private:
    bool m_isBusy;
    QString m_errorMessage;
};

#endif // BASEVIEWMODEL_H
```

#### 1.2 创建PlaybackViewModel
**职责**: 管理播放器核心状态（替代部分PlayWidget功能）

**文件**: `PlaybackViewModel.h`

```cpp
#ifndef PLAYBACKVIEWMODEL_H
#define PLAYBACKVIEWMODEL_H

#include "BaseViewModel.h"
#include "AudioService.h"
#include <QUrl>

/**
 * @brief 播放器ViewModel
 * 暴露播放状态给UI层
 */
class PlaybackViewModel : public BaseViewModel
{
    Q_OBJECT
    
    // 播放状态
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY isPausedChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    
    // 当前曲目信息
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY currentTitleChanged)
    Q_PROPERTY(QString currentArtist READ currentArtist NOTIFY currentArtistChanged)
    Q_PROPERTY(QString currentAlbumArt READ currentAlbumArt NOTIFY currentAlbumArtChanged)
    
public:
    explicit PlaybackViewModel(QObject *parent = nullptr);
    
    // 属性访问器
    bool isPlaying() const;
    bool isPaused() const;
    qint64 position() const { return m_position; }
    qint64 duration() const { return m_duration; }
    int volume() const;
    QString currentTitle() const { return m_currentTitle; }
    QString currentArtist() const { return m_currentArtist; }
    QString currentAlbumArt() const { return m_currentAlbumArt; }
    
    // 播放控制命令
    Q_INVOKABLE void play(const QUrl& url);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seekTo(qint64 positionMs);
    Q_INVOKABLE void togglePlayPause();
    
    // 音量控制
    void setVolume(int volume);
    
signals:
    void isPlayingChanged();
    void isPausedChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void currentTitleChanged();
    void currentArtistChanged();
    void currentAlbumArtChanged();
    
private slots:
    void onAudioServiceStateChanged();
    void onPositionChanged(qint64 pos);
    void onDurationChanged(qint64 dur);
    
private:
    AudioService* m_audioService;
    qint64 m_position = 0;
    qint64 m_duration = 0;
    QString m_currentTitle;
    QString m_currentArtist;
    QString m_currentAlbumArt;
};

#endif // PLAYBACKVIEWMODEL_H
```

---

### Phase 2: 渐进式改造Widget

#### 2.1 改造PlayWidget（分3步）

**Step 1: 添加ViewModel层（保留原有功能）**
```cpp
// play_widget.h
class PlayWidget : public QWidget
{
    Q_OBJECT
public:
    PlayWidget(QWidget *parent = nullptr);
    
    // ========== 旧接口（保留） ==========
    void _play_click(QString songName);  // 保留
    void slot_play_click();              // 保留
    
    // ========== 新接口（MVVM） ==========
    PlaybackViewModel* viewModel() const { return m_viewModel; }
    
private:
    // 旧架构（保留）
    AudioService* audioService;
    
    // 新架构（逐步使用）
    PlaybackViewModel* m_viewModel;  // ViewModel层
};
```

**Step 2: 迁移业务逻辑到ViewModel**
```cpp
// 将这些逻辑移到PlaybackViewModel
void PlayWidget::_play_click(QString songName) {
    // ❌ 旧方式：直接操作Service
    // audioService->play(url);
    
    // ✅ 新方式：通过ViewModel
    m_viewModel->play(QUrl::fromLocalFile(songName));
}
```

**Step 3: 移除冗余代码**
- 删除重复的状态变量
- 统一信号槽连接

#### 2.2 改造控制栏（ControlBar）

**目标**: 控制栏只负责UI展示，不包含逻辑

```cpp
// controlbar_qml.h
class ControlBarQml : public QQuickWidget
{
    Q_OBJECT
    
    // 绑定外部ViewModel（不自己管理状态）
    Q_PROPERTY(PlaybackViewModel* viewModel READ viewModel WRITE setViewModel)
    
public:
    PlaybackViewModel* viewModel() const { return m_viewModel; }
    void setViewModel(PlaybackViewModel* vm);
    
private:
    PlaybackViewModel* m_viewModel = nullptr;  // 外部注入
};
```

---

### Phase 3: 清理QML包装类

#### 3.1 评估每个`*_qml.h`的必要性

**分类处理**:
```
A类（可删除）: 只是简单包装QML组件
├── music_item_qml.h
├── playlist_history_qml.h
└── rotatingcircle_qml.h

B类（保留并改造）: 有独特业务逻辑
├── lyricdisplay_qml.h
├── process_slider_qml.h
└── controlbar_qml.h

C类（合并）: 功能重复
├── loginwidget_qml.h + loginwidget.h → LoginViewModel
├── userwidget_qml.h + user_widget_qml.h → UserViewModel
└── music_list_widget_qml.h + music_list_widget.h → PlaylistViewModel
```

#### 3.2 处理策略

**A类（删除）**:
1. 检查QML是否直接使用该类
2. 替换为直接使用QML组件或其他ViewModel
3. 移除头文件和源文件

**B类（改造）**:
```cpp
// 改造前
class LyricDisplayQml : public QQuickWidget {
    void showLyric(QString text);  // 命令式
};

// 改造后
class LyricDisplayQml : public QQuickWidget {
    Q_PROPERTY(QString currentLyric READ currentLyric NOTIFY currentLyricChanged)
    Q_PROPERTY(QStringList lyricLines READ lyricLines NOTIFY lyricLinesChanged)
    
    QString currentLyric() const;
    QStringList lyricLines() const;
    
signals:
    void currentLyricChanged();
    void lyricLinesChanged();
};
```

**C类（合并）**:
```cpp
// 新建文件 LoginViewModel.h
class LoginViewModel : public BaseViewModel {
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY isLoggedInChanged)
    Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
    
    Q_INVOKABLE void login(const QString& user, const QString& pwd);
    Q_INVOKABLE void logout();
};

// 删除: loginwidget_qml.h, loginwidget.h
// 合并功能到: LoginViewModel
```

---

### Phase 4: QML层改造

#### 4.1 改为声明式绑定

**改造前（命令式）**:
```qml
// controlbar.qml
Button {
    onClicked: {
        playWidget.slot_play_click()  // 直接调用C++槽函数
    }
}
```

**改造后（声明式）**:
```qml
// controlbar.qml
import QtQuick 2.15

Item {
    // 注入ViewModel
    property var viewModel: null
    
    Button {
        text: viewModel.isPlaying ? "暂停" : "播放"
        enabled: viewModel.duration > 0
        
        onClicked: viewModel.togglePlayPause()  // 调用ViewModel命令
    }
    
    Slider {
        from: 0
        to: viewModel.duration
        value: viewModel.position
        
        onMoved: viewModel.seekTo(value)
    }
}
```

#### 4.2 使用Binding绑定属性
```qml
Binding {
    target: rotatingImage
    property: "rotation"
    value: viewModel.isPlaying ? 360 : 0
    
    Behavior on rotation {
        RotationAnimation { duration: 3000; loops: Animation.Infinite }
    }
}
```

---

## 🛠️ 实施时间表

### 第1天: Phase 0 - 清理冲突
- [ ] 分析Session冲突
- [ ] 重命名旧文件
- [ ] 确保编译通过

### 第2天: Phase 1 - 建立基础
- [ ] 创建BaseViewModel
- [ ] 创建PlaybackViewModel
- [ ] 编写单元测试

### 第3-4天: Phase 2 - 改造Widget
- [ ] PlayWidget集成ViewModel
- [ ] ControlBar改造
- [ ] 测试播放功能

### 第5-6天: Phase 3 - 清理包装类
- [ ] 删除A类包装类
- [ ] 改造B类包装类
- [ ] 合并C类包装类

### 第7天: Phase 4 - QML改造
- [ ] 改为声明式绑定
- [ ] 移除命令式调用
- [ ] 全量测试

---

## ✅ 每个阶段的验收标准

### Phase 0
- ✅ 无编译错误
- ✅ 无符号冲突警告
- ✅ 原有功能正常运行

### Phase 1
- ✅ ViewModel可以独立编译
- ✅ 属性绑定正常工作
- ✅ 信号正确触发

### Phase 2
- ✅ PlayWidget通过ViewModel控制播放
- ✅ 播放/暂停/进度条功能正常
- ✅ 旧代码路径仍可用（向后兼容）

### Phase 3
- ✅ 删除的类无引用
- ✅ 合并后功能无缺失
- ✅ 代码量减少20%+

### Phase 4
- ✅ QML无C++槽函数直接调用
- ✅ 所有UI通过属性绑定更新
- ✅ 性能无明显下降

---

## 🚧 风险控制

### 风险1: Session冲突导致链接失败
**缓解**: 先重命名后迁移，保留两份代码

### 风险2: QML找不到C++对象
**缓解**: 使用qmlRegisterType注册所有ViewModel

### 风险3: 性能下降
**缓解**: 使用QML Profiler监控，避免不必要的属性通知

### 风险4: 功能丢失
**缓解**: 每个Phase后进行完整回归测试

---

## 📝 代码审查检查清单

每次提交前确认:
- [ ] 代码可编译
- [ ] 无编译警告
- [ ] 添加了Q_PROPERTY
- [ ] 信号命名为xxxChanged
- [ ] Q_INVOKABLE方法已标记
- [ ] 业务逻辑在Service/ViewModel，不在Widget
- [ ] QML使用属性绑定
- [ ] 添加了注释说明改动原因

---

## 🎯 最终目标架构

```
┌─────────────────────────────────────────────┐
│              QML View Layer                  │
│  - controlbar.qml, lyricdisplay.qml         │
│  - 纯UI渲染，通过属性绑定读取状态             │
└─────────────┬───────────────────────────────┘
              │ Q_PROPERTY Binding
┌─────────────▼───────────────────────────────┐
│         ViewModel Layer (C++)                │
│  - PlaybackViewModel (播放状态)              │
│  - PlaylistViewModel (列表管理)              │
│  - LoginViewModel (用户认证)                 │
│  - 使用Q_PROPERTY暴露状态                     │
│  - Q_INVOKABLE方法处理UI命令                 │
└─────────────┬───────────────────────────────┘
              │ Method Calls
┌─────────────▼───────────────────────────────┐
│           Service Layer (单例)               │
│  - AudioService (音频播放协调)                │
│  - MediaService (媒体元数据)                  │
│  - 纯业务逻辑，不依赖UI                        │
└─────────────┬───────────────────────────────┘
              │ 
┌─────────────▼───────────────────────────────┐
│         Core Layer (单例)                    │
│  - AudioPlayer (硬件控制)                     │
│  - AudioDecoder (解码)                        │
│  - AudioSession (会话管理)                    │
└─────────────────────────────────────────────┘
```

---

## 📚 参考资料

- [Qt MVVM Best Practices](https://doc.qt.io/qt-5/qtquick-bestpractices.html)
- [Q_PROPERTY Documentation](https://doc.qt.io/qt-5/properties.html)
- [QML与C++集成](https://doc.qt.io/qt-5/qtqml-cppintegration-overview.html)

---

**备注**: 本计划遵循"安全第一"原则，每一步都确保项目可编译可运行。如遇问题立即停止，回退到上一个稳定版本。
