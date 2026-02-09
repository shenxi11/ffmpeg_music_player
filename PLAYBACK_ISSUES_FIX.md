# 播放问题修复记录

**日期**: 2026年2月1日  
**问题**: 拖动进度条有问题，歌词没有显示

---

## 🔍 问题分析

### 问题1: 歌词完全不显示

**症状**：
- 点击网络音乐播放后，没有歌词显示
- 日志中没有任何歌词加载相关的输出：
  - ❌ 没有 `begin_take_lrc`
  - ❌ 没有 `Lrc_send_lrc`
  - ❌ 没有歌词文件路径

**根本原因**：
在MVVM模式下，`_play_click()`方法中缺少`signal_filepath`信号发射：

```cpp
// ❌ MVVM模式下原来的代码
m_playbackViewModel->play(url);
emit signal_begin_to_play(songPath);
// 缺少：emit signal_filepath(songPath);  ← 这个触发歌词加载！
```

歌词加载依赖于信号连接：
```cpp
connect(this, &PlayWidget::signal_filepath, this, &PlayWidget::_begin_take_lrc);
```

### 问题2: 拖动进度条后位置更新异常

**症状**：
- 拖动进度条后，位置更新可能不稳定
- 日志显示断开和重连，但可能存在时序问题

**原因分析**：
1. 拖动时断开连接 ✅
2. 释放时重连 ✅
3. **但重连后可能立即收到旧的位置更新**，导致跳回原位置

---

## ✅ 修复方案

### 修复1: 添加歌词加载信号

**文件**: [play_widget.cpp](play_widget.cpp#L948)

```cpp
#ifdef USE_MVVM_PLAYBACK
    m_playbackViewModel->play(url);
    emit signal_begin_to_play(songPath);
    
    // ✅ 新增：触发歌词加载
    emit signal_filepath(songPath);
#else
    emit signal_filepath(songPath);
#endif
```

### 修复2: 优化进度条位置更新逻辑

**文件**: [play_widget.cpp](play_widget.cpp#L653)

**改进点**：
1. 断开连接时清空连接句柄
2. 重连时添加二次检查，确保不在拖动状态

```cpp
connect(process_slider, &ProcessSliderQml::signal_sliderPressed, [this](){
    qDebug() << "拖动进度条 - 断开位置更新";
    if (positionUpdateConnection) {
        disconnect(positionUpdateConnection);
        positionUpdateConnection = QMetaObject::Connection();  // ✅ 清空句柄
    }
});

connect(process_slider, &ProcessSliderQml::signal_sliderReleased, [this](){
    qDebug() << "进度条释放 - 重新连接位置更新";
    positionUpdateConnection = connect(audioService, &AudioService::positionChanged,
            this, [this](qint64 positionMs) {
        // ✅ 二次检查：确保不在拖动状态
        if (!process_slider->isSliderPressed()) {
            int seconds = static_cast<int>(positionMs / 1000);
            process_slider->setCurrentSeconds(seconds);
        }
    });
});
```

---

## 🧪 测试验证

### 预期行为

#### 歌词显示
1. 点击网络音乐播放
2. 日志应显示：
   ```
   [MVVM] _play_click: Playing via ViewModel: http://...
   // 新增日志：
   LrcAnalyze: begin_take_lrc called with path: http://...
   LrcAnalyze: Downloading lyrics from: http://.../xxx.lrc
   ```
3. 歌词面板应显示歌词内容

#### 进度条拖动
1. 拖动进度条到任意位置
2. 释放鼠标
3. 进度条应：
   - ✅ 跳转到拖动位置
   - ✅ 继续平滑更新，不跳回原位置
   - ✅ 歌词同步到新位置

---

## 📊 修改文件清单

| 文件 | 修改内容 | 行数 |
|------|---------|------|
| [AudioSession.cpp](AudioSession.cpp#L47) | 支持网络URL播放 | +6 |
| [play_widget.cpp](play_widget.cpp#L948) | 添加歌词加载信号 | +3 |
| [play_widget.cpp](play_widget.cpp#L653) | 优化进度条位置更新 | +5 |
| [PlaybackViewModel.cpp](viewmodels/PlaybackViewModel.cpp) | 移除重复槽函数定义 | -8 |

**总计**: 4个文件，净增加约6行

---

## 🎯 后续建议

### 可选优化

1. **统一进度更新到ViewModel**
   - 当前：AudioService直接连接到UI
   - 未来：所有UI更新通过ViewModel

2. **歌词加载迁移到ViewModel**
   - 创建 `LyricViewModel`
   - 封装歌词下载、解析、同步逻辑

3. **进度条组件优化**
   - 使用QML的属性绑定替代信号槽
   - 减少C++与QML之间的频繁调用

---

**最后更新**: 2026年2月1日  
**状态**: ✅ 已修复，待测试
