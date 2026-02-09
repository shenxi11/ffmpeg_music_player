# PlayWidget迁移到MVVM指南

**日期**: 2026-02-01  
**状态**: ✅ Phase 1 完成 - 基础迁移已实现  
**目标**: 将PlayWidget从直接操作AudioService改为通过PlaybackViewModel

---

## ✅ 已完成的迁移工作（2026-02-01）

### Phase 1: 基础集成 ✅ 完成
- [x] 创建PlaybackViewModel实例
- [x] 添加Q_PROPERTY暴露ViewModel  
- [x] 在构造函数中初始化ViewModel
- [x] 连接ViewModel信号到UI更新
- [x] 使用条件编译支持新旧代码（`#define USE_MVVM_PLAYBACK`）

### Phase 2: 核心方法迁移 ✅ 完成
- [x] `_play_click(QString)` - 播放指定歌曲
- [x] `slot_play_click()` - 播放/暂停切换
- [x] `slot_lyric_seek(int)` - 进度跳转

### 代码变更摘要
1. **play_widget.h**: 添加MVVM配置宏和ViewModel属性
2. **play_widget.cpp**: 
   - 初始化PlaybackViewModel
   - 连接ViewModel信号到UI
   - 3个核心方法支持MVVM模式
3. **viewmodels/PlaybackViewModel.cpp**: 完善信号连接

---

## 🎯 MVVM模式使用说明

### 启用MVVM模式
在 [play_widget.h](play_widget.h#L4-L6) 中：
```cpp
#define USE_MVVM_PLAYBACK  // 已启用
```

### 禁用MVVM模式（回退到旧代码）
注释掉宏定义：
```cpp
// #define USE_MVVM_PLAYBACK  // 禁用，使用旧代码
```

---

## 📋 迁移策略

### 原则
1. **双轨运行**: 旧代码和新代码并存,逐步切换
2. **逐个方法迁移**: 每次只迁移一个方法,保持可编译
3. **使用条件编译**: 通过宏控制新旧代码
4. **保持向后兼容**: 旧信号槽仍可正常工作

---

## 🔄 迁移步骤

### Step 1: 初始化ViewModel（✅ 已完成）

**已完成的工作**:
```cpp
// play_widget.h
Q_PROPERTY(PlaybackViewModel* playbackViewModel READ playbackViewModel CONSTANT)
PlaybackViewModel* playbackViewModel() const { return m_playbackViewModel; }

// play_widget.cpp - 构造函数
PlayWidget::PlayWidget(QWidget *parent, QWidget *mainWidget)
    : QWidget(parent),
      m_playbackViewModel(new PlaybackViewModel(this)),  // 新增
      audioService(&AudioService::instance()),  // 保留（兼容旧代码）
      ...
```

---

### Step 2: 迁移播放控制方法

#### 方法1: `slot_play_click()` - 播放/暂停切换

**当前实现（旧代码）**:
```cpp
void PlayWidget::slot_play_click(){
    ProcessSliderQml::State currentState = controlBar->getState();
    
    if(currentState == ProcessSliderQml::Pause || currentState == ProcessSliderQml::Stop) {
        emit signal_playState(ProcessSliderQml::Play);
    } else {
        emit signal_playState(ProcessSliderQml::Pause);
    }
    emit signal_worker_play();
}
```

**新实现（MVVM）**:
```cpp
void PlayWidget::slot_play_click(){
    qDebug() << "[MVVM] slot_play_click using ViewModel";
    
    // 使用条件编译支持新旧两种模式
    #ifdef USE_MVVM_PLAYBACK  // 新模式
        m_playbackViewModel->togglePlayPause();
        
        // 同步UI状态（临时方案，最终UI应直接绑定ViewModel）
        if (m_playbackViewModel->isPlaying()) {
            emit signal_playState(ProcessSliderQml::Play);
        } else {
            emit signal_playState(ProcessSliderQml::Pause);
        }
    #else  // 旧模式（保留）
        ProcessSliderQml::State currentState = controlBar->getState();
        
        if(currentState == ProcessSliderQml::Pause || currentState == ProcessSliderQml::Stop) {
            emit signal_playState(ProcessSliderQml::Play);
        } else {
            emit signal_playState(ProcessSliderQml::Pause);
        }
        emit signal_worker_play();
    #endif
}
```

**启用新模式**: 在`play_widget.h`顶部添加:
```cpp
#define USE_MVVM_PLAYBACK  // 启用MVVM播放控制
```

---

#### 方法2: `_play_click(QString songName)` - 播放指定歌曲

**当前实现（旧代码）**:
```cpp
void PlayWidget::_play_click(QString songName){
    // ... 验证路径 ...
    filePath = songName;
    fileName = QFileInfo(filePath).fileName();
    
    // 直接操作AudioService
    audioService->play(QUrl::fromLocalFile(filePath));
    
    // 发射信号
    emit signal_begin_to_play(songName);
}
```

**新实现（MVVM）**:
```cpp
void PlayWidget::_play_click(QString songName){
    if (!checkAndWarnIfPathNotExists(songName)) {
        return;
    }
    
    filePath = songName;
    fileName = QFileInfo(filePath).fileName();
    
    #ifdef USE_MVVM_PLAYBACK
        // 通过ViewModel播放
        m_playbackViewModel->play(QUrl::fromLocalFile(filePath));
        
        // ViewModel会自动更新状态,这里只需要处理UI层特有的逻辑
        emit signal_begin_to_play(songName);
        
        qDebug() << "[MVVM] Playing via ViewModel:" << songName;
    #else
        // 旧方式（保留）
        audioService->play(QUrl::fromLocalFile(filePath));
        emit signal_begin_to_play(songName);
    #endif
}
```

---

#### 方法3: 进度跳转 `slot_lyric_seek(int timeMs)`

**当前实现**:
```cpp
void PlayWidget::slot_lyric_seek(int timeMs){
    audioService->seekTo(timeMs);
    lastSeekPosition = timeMs;
}
```

**新实现（MVVM）**:
```cpp
void PlayWidget::slot_lyric_seek(int timeMs){
    #ifdef USE_MVVM_PLAYBACK
        m_playbackViewModel->seekTo(timeMs);
    #else
        audioService->seekTo(timeMs);
    #endif
    
    lastSeekPosition = timeMs;
}
```

---

### Step 3: 连接ViewModel信号到UI

**目标**: 让UI自动响应ViewModel的状态变化

在`PlayWidget`构造函数中添加:

```cpp
PlayWidget::PlayWidget(...) {
    // ... 现有初始化代码 ...
    
    // ========== 连接ViewModel信号到UI ==========
    
    // 播放状态变化 -> 更新UI
    connect(m_playbackViewModel, &PlaybackViewModel::isPlayingChanged, this, [this]() {
        bool playing = m_playbackViewModel->isPlaying();
        qDebug() << "[MVVM] isPlaying changed:" << playing;
        
        // 更新控制栏状态
        if (playing) {
            emit signal_playState(ProcessSliderQml::Play);
            rotatingCircle->startRotation();
        } else {
            emit signal_playState(ProcessSliderQml::Pause);
            rotatingCircle->stopRotation();
        }
    });
    
    // 进度变化 -> 更新进度条
    connect(m_playbackViewModel, &PlaybackViewModel::positionChanged, this, [this]() {
        qint64 position = m_playbackViewModel->position();
        
        // 更新进度条（如果不是用户拖拽）
        if (controlBar && !controlBar->isSliderPressed()) {
            emit signal_process_Change(position, false);
        }
    });
    
    // 时长变化 -> 更新进度条范围
    connect(m_playbackViewModel, &PlaybackViewModel::durationChanged, this, [this]() {
        qint64 duration = m_playbackViewModel->duration();
        this->duration = duration;
        
        if (controlBar) {
            controlBar->setMaxSeconds(duration / 1000);
        }
    });
    
    // 专辑封面变化 -> 更新旋转图片
    connect(m_playbackViewModel, &PlaybackViewModel::currentAlbumArtChanged, this, [this]() {
        QString albumArt = m_playbackViewModel->currentAlbumArt();
        
        if (!albumArt.isEmpty()) {
            rotatingCircle->setImage(albumArt);
            slot_updateBackground(albumArt);  // 更新背景
        }
    });
    
    // 曲目信息变化 -> 更新标题
    connect(m_playbackViewModel, &PlaybackViewModel::currentTitleChanged, this, [this]() {
        currentSongTitle = m_playbackViewModel->currentTitle();
        // 更新UI显示
        if (nameLabel) {
            QString displayText = currentSongTitle;
            if (!m_playbackViewModel->currentArtist().isEmpty()) {
                displayText += " - " + m_playbackViewModel->currentArtist();
            }
            nameLabel->setText(displayText);
        }
    });
}
```

---

### Step 4: QML层迁移（未来）

**最终目标**: QML直接绑定ViewModel属性

```qml
// controlbar.qml (未来版本)
Item {
    // 从C++注入ViewModel
    property var viewModel: playWidget.playbackViewModel
    
    Button {
        text: viewModel.isPlaying ? "暂停" : "播放"
        enabled: viewModel.duration > 0
        
        onClicked: viewModel.togglePlayPause()
    }
    
    Slider {
        from: 0
        to: viewModel.duration
        value: viewModel.position
        
        onMoved: viewModel.seekTo(value)
    }
    
    Text {
        text: viewModel.positionText + " / " + viewModel.durationText
    }
    
    Text {
        text: viewModel.currentTitle + " - " + viewModel.currentArtist
    }
}
```

---

## ✅ 迁移检查清单

### Phase 2.1: 基础集成（当前阶段）
- [x] 创建PlaybackViewModel实例
- [x] 添加Q_PROPERTY暴露ViewModel
- [x] 在构造函数中初始化ViewModel
- [ ] 连接ViewModel信号到UI更新
- [ ] 使用条件编译支持新旧代码

### Phase 2.2: 逐个方法迁移
- [ ] `slot_play_click()` - 播放/暂停
- [ ] `_play_click(QString)` - 播放指定歌曲
- [ ] `slot_lyric_seek(int)` - 进度跳转
- [ ] `slot_work_play()` - 播放控制
- [ ] `slot_work_stop()` - 停止播放

### Phase 2.3: 清理旧代码
- [ ] 移除条件编译宏
- [ ] 删除直接调用audioService的代码
- [ ] 删除重复的状态变量
- [ ] 简化信号槽连接

---

## 🚧 注意事项

### 1. AudioService信号缺失
当前AudioService可能缺少以下信号:
```cpp
// 需要在AudioService中添加
signals:
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void metadataChanged(QString title, QString artist, QString album, QString albumArt);
```

**临时方案**: 在AudioSession中添加这些信号,然后转发到AudioService

### 2. 状态同步问题
在迁移期间,状态可能存在于两个地方:
- PlayWidget的成员变量（旧）
- PlaybackViewModel的属性（新）

**解决方案**: 使用ViewModel作为单一数据源,Widget只读取不修改

### 3. 进度更新频率
播放进度更新频率高(每100ms一次),确保:
- 使用`QMetaObject::invokeMethod(..., Qt::QueuedConnection)`避免阻塞
- 进度条拖拽时暂停自动更新
- ViewModel中的`positionChanged`信号不要触发太多UI重绘

---

## 📊 迁移进度追踪

| 功能模块 | 状态 | 负责人 | 备注 |
|---------|------|--------|------|
| ViewModel基础设施 | ✅ 完成 | - | BaseViewModel + PlaybackViewModel |
| PlayWidget集成 | 🔄 进行中 | - | 已添加ViewModel实例 |
| 播放控制迁移 | ⏳ 待开始 | - | slot_play_click等方法 |
| 信号连接 | ⏳ 待开始 | - | ViewModel -> UI |
| QML绑定 | ⏳ 待开始 | - | 最后阶段 |

---

## 🎯 下一步行动

1. **添加AudioService缺失信号** (高优先级)
   - 在AudioSession中添加position/duration/metadata信号
   - 在AudioService中转发这些信号

2. **连接ViewModel信号到UI** (高优先级)
   - 实现上面Step 3的代码
   - 测试播放时UI是否自动更新

3. **迁移第一个播放方法** (中优先级)
   - 选择`_play_click(QString)`作为第一个迁移目标
   - 添加条件编译支持
   - 测试新旧两种模式

4. **编写单元测试** (中优先级)
   - 测试PlaybackViewModel的各个方法
   - 确保状态变化正确触发信号

---

**最后更新**: 2026-02-01  
**下次审查**: 完成Step 3后
