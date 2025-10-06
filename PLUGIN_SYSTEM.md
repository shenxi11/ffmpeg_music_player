# FFmpeg Music Player 插件系统

## 概述

FFmpeg Music Player 现已支持插件系统，允许通过动态加载 DLL/SO 的方式扩展功能。

## 架构说明

### 核心组件

1. **插件接口** (`plugin_interface.h`)
   - 定义所有插件必须实现的标准接口
   - 确保插件与主程序的兼容性

2. **插件管理器** (`plugin_manager.h/cpp`)
   - 自动扫描和加载插件
   - 管理插件生命周期
   - 提供插件查询功能

3. **主菜单** (`main_menu.h/cpp`)
   - 动态显示所有已加载的插件
   - 提供统一的插件访问入口

### 目录结构

```
ffmpeg_music_player/
├── plugin/                           # 插件输出目录
│   ├── audio_converter_plugin.dll   # 音频转换插件
│   └── README.md
├── plugins/                          # 插件源码目录
│   ├── audio_converter_plugin/      # 音频转换插件源码
│   │   ├── audio_converter_plugin.pro
│   │   ├── audio_converter_plugin.h
│   │   ├── audio_converter_plugin.cpp
│   │   └── audio_converter_plugin.json
│   └── PLUGIN_DEVELOPMENT.md        # 插件开发指南
├── plugin_interface.h                # 插件接口定义
├── plugin_manager.h/cpp             # 插件管理器
├── build_plugin.bat                 # Windows 插件编译脚本
└── build_plugin.sh                  # Linux/Mac 插件编译脚本
```

## 工作流程

### 1. 程序启动

```
MainWidget 构造函数
  ↓
PluginManager::loadPlugins("./plugin")
  ↓
扫描 plugin 目录
  ↓
加载所有 DLL/SO 文件
  ↓
验证插件接口
  ↓
调用 plugin->initialize()
  ↓
插件就绪
```

### 2. 用户使用插件

```
用户点击主菜单按钮
  ↓
MainMenu 显示
  ↓
显示所有已加载的插件列表
  ↓
用户点击插件菜单项
  ↓
发出 pluginRequested(pluginName) 信号
  ↓
MainWidget 接收信号
  ↓
从 PluginManager 获取插件实例
  ↓
调用 plugin->createWidget()
  ↓
显示插件界面
```

## 现有插件

### 音频转换插件 (Audio Converter Plugin)

- **功能**：支持多种音频格式之间的转换
- **支持格式**：MP3, WAV, FLAC, AAC, OGG
- **特性**：
  - 拖放文件支持
  - 批量转换
  - 自定义编码参数
  - 转换进度跟踪

## 编译说明

### 编译主程序

```bash
cd ffmpeg_music_player
qmake untitled.pro
make
```

### 编译插件

#### Windows
```batch
build_plugin.bat
```

#### Linux/Mac
```bash
chmod +x build_plugin.sh
./build_plugin.sh
```

或手动编译单个插件：
```bash
cd plugins/audio_converter_plugin
qmake audio_converter_plugin.pro
make
```

## 开发新插件

详细的插件开发指南请参阅：[plugins/PLUGIN_DEVELOPMENT.md](plugins/PLUGIN_DEVELOPMENT.md)

### 快速开始

1. 在 `plugins/` 目录下创建新的插件文件夹
2. 实现 `PluginInterface` 接口
3. 创建 `.pro` 文件，设置 `DESTDIR = ../../plugin`
4. 编译插件
5. 重启主程序，插件会自动加载

## 插件配置

插件在 `plugin` 目录下，可以：
- 添加新插件：将编译好的 DLL/SO 复制到此目录
- 禁用插件：将插件文件移出此目录
- 更新插件：替换旧的 DLL/SO 文件

## 调试

### 查看插件加载日志

主程序会输出详细的插件加载信息：
```
Loading plugins from: /path/to/plugin
Found 1 plugin files
Attempting to load plugin: audio_converter_plugin.dll
AudioConverterPlugin initialized
Plugin loaded successfully: 音频转换器 Version: 1.0.0
Loaded 1 plugins
```

### 常见问题

1. **插件无法加载**
   - 检查 Qt 版本是否匹配
   - 检查编译器版本是否一致
   - 查看错误日志

2. **插件不显示**
   - 确认插件在 `plugin` 目录
   - 确认 `initialize()` 返回 true
   - 重启主程序

## 技术细节

### Qt 插件系统

使用 Qt 的插件机制：
- `Q_PLUGIN_METADATA` 宏声明插件元数据
- `Q_INTERFACES` 宏声明实现的接口
- `QPluginLoader` 动态加载插件

### 内存管理

- 插件实例由 `QPluginLoader` 管理
- 插件卸载时自动调用 `cleanup()`
- 插件创建的窗口由 Qt 的父子关系管理

### 线程安全

- `PluginManager` 使用单例模式
- 所有插件操作在主线程执行
- 插件内部需要自行处理多线程

## 未来扩展

可以添加更多类型的插件：
- 视频转换工具
- 音频效果器
- 播放列表管理器
- 在线音乐搜索
- 歌词编辑器
- 等等...

## 许可证

与主程序相同的许可证。

## 贡献

欢迎贡献新插件！请遵循插件开发指南，并提交 Pull Request。
