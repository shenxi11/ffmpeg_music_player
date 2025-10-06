# FFmpeg Music Player 插件开发指南

## 概述

本项目支持通过插件系统扩展功能。插件以动态链接库（DLL/SO）形式存在，在程序启动时自动加载。

## 插件架构

### 目录结构

```
ffmpeg_music_player/
├── plugin/                          # 插件输出目录（编译后的 DLL/SO 文件）
├── plugins/                         # 插件源码目录
│   ├── audio_converter_plugin/     # 音频转换插件
│   │   ├── audio_converter_plugin.pro
│   │   ├── audio_converter_plugin.h
│   │   ├── audio_converter_plugin.cpp
│   │   └── audio_converter_plugin.json
│   └── your_plugin/                # 你的插件
│       ├── your_plugin.pro
│       ├── your_plugin.h
│       ├── your_plugin.cpp
│       └── your_plugin.json
├── plugin_interface.h               # 插件接口定义
└── plugin_manager.h/cpp            # 插件管理器
```

## 创建新插件

### 1. 实现插件接口

所有插件必须实现 `PluginInterface` 接口：

```cpp
class PluginInterface {
public:
    virtual ~PluginInterface() {}
    virtual QString pluginName() const = 0;
    virtual QString pluginDescription() const = 0;
    virtual QString pluginVersion() const = 0;
    virtual QIcon pluginIcon() const = 0;
    virtual QWidget* createWidget(QWidget* parent = nullptr) = 0;
    virtual bool initialize() = 0;
    virtual void cleanup() = 0;
};
```

### 2. 创建插件类

```cpp
// your_plugin.h
#include "plugin_interface.h"
#include <QObject>
#include <QtPlugin>

class YourPlugin : public QObject, public PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "your_plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    QString pluginName() const override { return "你的插件名称"; }
    QString pluginDescription() const override { return "插件描述"; }
    QString pluginVersion() const override { return "1.0.0"; }
    QIcon pluginIcon() const override { return QIcon(":/path/to/icon.png"); }
    QWidget* createWidget(QWidget* parent) override;
    bool initialize() override;
    void cleanup() override;
};
```

### 3. 创建 .pro 文件

```qmake
QT += widgets
CONFIG += plugin c++17
TEMPLATE = lib
TARGET = your_plugin

# 输出到主程序的plugin目录
DESTDIR = ../../plugin

# 包含主程序的头文件路径
INCLUDEPATH += ../../

HEADERS += your_plugin.h
SOURCES += your_plugin.cpp

# 插件元数据
OTHER_FILES += your_plugin.json
```

### 4. 创建 JSON 元数据文件

```json
{
    "name": "your_plugin",
    "version": "1.0.0",
    "description": "Your plugin description",
    "author": "Your Name"
}
```

## 编译插件

### Windows

```batch
cd plugins\your_plugin
qmake your_plugin.pro
nmake
```

或使用提供的批处理脚本：
```batch
build_plugin.bat
```

### Linux/Mac

```bash
cd plugins/your_plugin
qmake your_plugin.pro
make
```

或使用提供的 Shell 脚本：
```bash
./build_plugin.sh
```

## 插件加载流程

1. 主程序启动时，`PluginManager` 扫描 `plugin` 目录
2. 加载所有 DLL/SO 文件
3. 验证是否实现了 `PluginInterface` 接口
4. 调用插件的 `initialize()` 方法
5. 在主菜单中显示插件入口
6. 用户点击菜单项时，调用 `createWidget()` 创建插件界面

## 调试插件

### 启用调试输出

在插件代码中使用 `qDebug()` 输出调试信息：

```cpp
qDebug() << "YourPlugin: Initializing...";
```

### 检查插件加载状态

主程序会输出插件加载信息：
- 扫描到的插件文件
- 加载成功/失败的插件
- 插件初始化状态

## 示例：音频转换插件

参考 `plugins/audio_converter_plugin` 目录下的完整实现。

### 关键代码

```cpp
QWidget* AudioConverterPlugin::createWidget(QWidget* parent)
{
    AudioConverter* converter = new AudioConverter(parent);
    return converter;
}

bool AudioConverterPlugin::initialize()
{
    qDebug() << "AudioConverterPlugin initialized";
    return true;
}

void AudioConverterPlugin::cleanup()
{
    qDebug() << "AudioConverterPlugin cleaned up";
}
```

## 最佳实践

1. **资源管理**：在 `cleanup()` 中释放所有资源
2. **错误处理**：`initialize()` 返回 false 时插件不会被加载
3. **独立窗口**：插件窗口应设置为独立窗口（`Qt::Window`）
4. **版本兼容**：注意 Qt 版本和编译器版本需要与主程序一致
5. **依赖管理**：将插件依赖的库放在 plugin 目录或系统路径

## 常见问题

### Q: 插件无法加载？
A: 检查以下几点：
- 确保 Qt 版本一致
- 确保编译器版本一致（MSVC/MinGW）
- 检查是否实现了所有接口方法
- 查看控制台输出的错误信息

### Q: 插件在菜单中不显示？
A: 确保：
- 插件文件在 `plugin` 目录下
- `initialize()` 返回 true
- `pluginName()` 返回有效字符串

### Q: 如何依赖主程序的类？
A: 在插件 .pro 文件中添加：
```qmake
INCLUDEPATH += ../../
HEADERS += ../../your_class.h
SOURCES += ../../your_class.cpp
```

## 联系支持

如有问题，请提交 Issue 或联系开发团队。
