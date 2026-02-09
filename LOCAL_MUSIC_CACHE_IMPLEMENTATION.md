# 本地音乐缓存功能实现

## 概述
为本地音乐tab添加了缓存功能和添加按钮，使得本地音乐列表能够持久化保存，并在下次启动应用时自动加载。

## 新增文件

### 1. local_music_cache.h
- **作用**: 单例类，负责本地音乐信息的持久化存储
- **核心功能**:
  - 存储音乐信息: 文件路径、文件名、封面URL、时长、艺术家
  - 使用 QSettings 保存数据（JSON格式）
  - 提供增删改查接口
  - 发射 musicListChanged() 信号通知UI更新

### 2. local_music_model.h/cpp
- **作用**: QAbstractListModel，作为LocalMusicCache和QML之间的桥梁
- **核心功能**:
  - 6个角色: FilePath, FileName, CoverUrl, Duration, Artist, IsPlaying
  - Q_PROPERTY currentPlayingPath 用于播放状态同步
  - addMusicRequested() 信号触发文件选择对话框
  - 自动连接LocalMusicCache的信号，实时刷新数据

## 修改文件

### 1. LocalAndDownloadWidget
- **local_and_download_widget.h**:
  - 添加 `LocalMusicModel* m_localMusicModel` 成员
  - 添加 `addLocalMusicRequested()` 信号
  
- **local_and_download_widget.cpp**:
  - 初始化 LocalMusicModel
  - 通过 setContextProperty 将 localMusicModel 暴露给QML
  - 连接 LocalMusicModel::addMusicRequested 到 addLocalMusicRequested
  - 更新 setCurrentPlayingPath() 同时设置两个模型（下载和本地）

### 2. MainWidget
- **main_widget.h**:
  - 添加 `#include "local_music_cache.h"`
  
- **main_widget.cpp**:
  - 连接 `addLocalMusicRequested` 到 `PlayWidget::openfile`（打开文件选择对话框）
  - 连接 `signal_add_song` 到 LocalMusicCache::addMusic（保存添加的音乐）
  - 连接 `signal_metadata_updated` 到 LocalMusicCache::updateMetadata（更新封面、时长等）
  - 在删除音乐时调用 LocalMusicCache::removeMusic

### 3. LocalMusicList.qml
- 已经包含完整的UI实现：
  - "+" 添加音乐" 按钮在标题栏
  - ListView显示音乐列表（序号、封面、歌曲名、时长、艺术家、操作）
  - 播放/删除按钮
  - 空状态提示

### 4. CMakeLists.txt
- 添加新文件到构建系统：
  ```cmake
  local_music_cache.h
  local_music_model.cpp
  local_music_model.h
  ```

## 数据流程

### 添加音乐流程
1. 用户点击QML中的"+ 添加音乐"按钮
2. LocalMusicList.qml 调用 `localMusicModel.addMusic("")`
3. LocalMusicModel 发射 `addMusicRequested` 信号
4. LocalAndDownloadWidget 转发到 MainWidget
5. MainWidget 调用 `PlayWidget::openfile()` 打开文件选择对话框
6. 用户选择文件后，PlayWidget 发射 `signal_add_song(filename, path)`
7. MainWidget 接收信号，调用 `LocalMusicCache::addMusic(path, filename)`
8. LocalMusicCache 保存到 QSettings 并发射 `musicListChanged()`
9. LocalMusicModel 接收信号，调用 `refresh()` 更新模型
10. QML ListView 自动更新显示

### 元数据更新流程
1. 用户播放音乐
2. PlayWidget 解析音频文件获取元数据（封面、时长、艺术家）
3. PlayWidget 发射 `signal_metadata_updated(filePath, coverUrl, duration)`
4. MainWidget 接收信号，调用 `LocalMusicCache::updateMetadata(...)`
5. LocalMusicCache 更新对应音乐的元数据并保存
6. LocalMusicModel 自动刷新，UI显示更新后的信息

### 播放状态同步
1. PlayWidget 发射 `signal_play_button_click(playing, filename)`
2. MainWidget 调用 `localAndDownloadWidget->setCurrentPlayingPath(...)`
3. LocalAndDownloadWidget 设置 `localMusicModel->setCurrentPlayingPath(...)`
4. LocalMusicModel 更新所有项的 IsPlaying 角色
5. QML ListView 根据 isPlaying 显示播放图标

### 删除音乐流程
1. 用户点击QML中的删除按钮
2. LocalMusicList.qml 发射 `deleteMusic(filename)` 信号
3. LocalAndDownloadWidget 转发到 MainWidget
4. MainWidget 调用 `LocalMusicCache::removeMusic(filename)` 删除缓存记录
5. MainWidget 删除实际文件
6. LocalMusicCache 发射 `musicListChanged()`
7. LocalMusicModel 自动刷新，UI移除该项

## 数据持久化

### 存储位置
使用 QSettings("FFmpegMusicPlayer", "LocalMusic") 存储，具体位置：
- Windows: `HKEY_CURRENT_USER\Software\FFmpegMusicPlayer\LocalMusic`
- Linux/Mac: `~/.config/FFmpegMusicPlayer/LocalMusic.conf`

### 存储格式
```json
{
  "musicList": [
    {
      "filePath": "C:/Music/song.mp3",
      "fileName": "song.mp3",
      "coverUrl": "file:///C:/temp/cover.jpg",
      "duration": "3:45",
      "artist": "Artist Name"
    }
  ]
}
```

## 特性

✅ 本地音乐列表持久化存储  
✅ 自动保存元数据（封面、时长、艺术家）  
✅ 播放状态实时同步  
✅ 支持删除音乐及缓存  
✅ QML + C++ Model架构，UI响应流畅  
✅ 单例模式，全局统一管理  

## 测试要点

1. **添加音乐**:
   - 点击"+ 添加音乐"按钮
   - 选择一个音频文件
   - 验证文件出现在列表中

2. **播放音乐**:
   - 点击播放按钮
   - 验证播放图标正确显示
   - 验证播放状态在不同tab间同步

3. **元数据更新**:
   - 播放一首音乐
   - 等待封面加载
   - 验证封面、时长显示正确

4. **持久化**:
   - 添加几首音乐
   - 关闭应用
   - 重新打开应用
   - 验证音乐列表自动加载

5. **删除音乐**:
   - 点击删除按钮
   - 验证音乐从列表移除
   - 验证文件被删除（如果需要）
