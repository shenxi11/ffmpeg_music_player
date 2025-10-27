// main_qml.cpp - QML版本的主入口
#include "qml_bridge.h"
#include <QApplication>  // 改用QApplication以获得更好的窗口支持
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include "plugin_manager.h"
#include "logger.h"
#include "httprequest.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#include <string>
#endif

// 自定义日志处理函数（过滤无用日志）
void customLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (msg.contains("pa_stream_begin_write, error = 无效参数"))
    {
        // 忽略这条日志
        return;
    }

    // 继续处理其他日志
    QTextStream(stderr) << msg << endl;
}

int main(int argc, char *argv[])
{
    // 在创建 QGuiApplication 之前先设置 DLL 搜索路径
#ifdef Q_OS_WIN
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring exeDir = exePath;
    size_t pos = exeDir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        exeDir = exeDir.substr(0, pos);
    }
    std::wstring resourceDir = exeDir + L"\\resource";
    
    // 使用 SetDllDirectory 添加搜索路径
    SetDllDirectoryW(resourceDir.c_str());
#endif
    
    // 设置 Qt Quick 属性（Qt 5.14）
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    
    QApplication app(argc, argv);  // 改用QApplication
    
    // 初始化日志系统
    QString logPath = QCoreApplication::applicationDirPath() + "/debug.log";
    initLogger(logPath);
    qDebug() << "=========================================";
    qDebug() << "FFmpeg Music Player (QML Version) Starting...";
    qDebug() << "Log file:" << logPath;
    
    // 记录 DLL 搜索路径
#ifdef Q_OS_WIN
    QString resourceDirQt = QCoreApplication::applicationDirPath() + "/resource";
    qDebug() << "DLL search path (resource):" << resourceDirQt;
    qDebug() << "Resource directory exists:" << QDir(resourceDirQt).exists();
#endif
    qDebug() << "=========================================";
    
    // 设置应用程序信息
    app.setOrganizationName("MusicPlayer");
    app.setApplicationName("FFmpegMusicPlayer");
    app.setWindowIcon(QIcon("qrc:/new/prefix1/icon/netease.png"));
    
    setlocale(LC_ALL, "chs");
    
    // 安装日志过滤器
    qInstallMessageHandler(customLogHandler);
    
    // 注册类型
    qRegisterMetaType<std::map<int, std::string>>("std::map<int, std::string>");
    qRegisterMetaType<std::vector<std::pair<qint64,qint64>>>("std::vector<std::pair<qint64,qint64>>");
    QLoggingCategory::setFilterRules("qt.audio.debug=false");
    
    // 加载插件
    QString pluginDir = QCoreApplication::applicationDirPath() + "/plugin";
    qDebug() << "Loading plugins from:" << pluginDir;
    
    PluginManager& pluginManager = PluginManager::instance();
    int loadedPlugins = pluginManager.loadPlugins(pluginDir);
    qDebug() << "Loaded" << loadedPlugins << "plugins";
    
    // 创建 QML 引擎
    QQmlApplicationEngine engine;
    
    // 捕获QML警告和错误
    QObject::connect(&engine, &QQmlApplicationEngine::warnings,
                     [](const QList<QQmlError> &warnings) {
        for (int i = 0; i < warnings.size(); ++i) {
            qWarning() << "QML Warning:" << warnings.at(i).toString();
        }
    });
    
    // 创建 UI 桥接对象
    UIBridge uiBridge;
    
    // 将 UIBridge 注册到 QML 上下文
    engine.rootContext()->setContextProperty("uiBridge", &uiBridge);
    
    // 加载 QML 主文件
    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            qCritical() << "Failed to create root QML object!";
            QCoreApplication::exit(-1);
        } else if (obj) {
            qDebug() << "QML root object created successfully";
            qDebug() << "Root object type:" << obj->metaObject()->className();
        }
    }, Qt::QueuedConnection);
    
    qDebug() << "Loading QML from:" << url;
    engine.load(url);
    
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML main file!";
        qCritical() << "Possible reasons:";
        qCritical() << "  1. QML file not found in resources";
        qCritical() << "  2. QML syntax errors";
        qCritical() << "  3. Missing QML imports or components";
        return -1;
    }
    
    qDebug() << "QML Engine loaded successfully";
    qDebug() << "Root objects count:" << engine.rootObjects().size();
    
    int result = app.exec();
    
    // 确保应用退出前完成所有清理
    qDebug() << "Application exiting, ensuring cleanup...";
    QThreadPool::globalInstance()->waitForDone(5000);
    qDebug() << "Application cleanup complete";
    
    cleanupLogger();
    return result;
}
