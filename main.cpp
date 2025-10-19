#include "play_widget.h"
#include <QFontMetrics>
#include <QDebug>
#include <QIcon>
#include <QApplication>
#include <QThread>
#include <QDir>
#include <QStandardPaths>
#include "main_widget.h"
#include "headers.h"
#include "plugin_manager.h"
#include "logger.h"

#ifdef Q_OS_WIN
#include <Windows.h>  // For SetDllDirectoryW, GetModuleFileNameW
#include <string>     // For std::wstring
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
    // 在创建 QApplication 之前先设置 DLL 搜索路径
#ifdef Q_OS_WIN
    // 将 resource 目录添加到 DLL 搜索路径
    // 注意：这里直接使用相对路径，因为 QCoreApplication 还未创建
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
    
    // 或者使用 AddDllDirectory（Windows 8+）
    // AddDllDirectory(resourceDir.c_str());
#endif
    
    QApplication a(argc, argv);

    // 初始化日志系统 - 日志文件保存在程序目录
    QString logPath = QCoreApplication::applicationDirPath() + "/debug.log";
    initLogger(logPath);
    qDebug() << "=========================================";
    qDebug() << "FFmpeg Music Player Starting...";
    qDebug() << "Log file:" << logPath;
    
    // 记录 DLL 搜索路径
#ifdef Q_OS_WIN
    QString resourceDirQt = QCoreApplication::applicationDirPath() + "/resource";
    qDebug() << "DLL search path (resource):" << resourceDirQt;
    qDebug() << "Resource directory exists:" << QDir(resourceDirQt).exists();
#endif
    qDebug() << "=========================================";

    // 设置应用程序信息，用于QSettings
    a.setOrganizationName("MusicPlayer");
    a.setApplicationName("FFmpegMusicPlayer");

    a.setWindowIcon(QIcon("qrc:/new/prefix1/icon/netease.png"));

    setlocale(LC_ALL, "chs");

    qInstallMessageHandler(customLogHandler);
    qRegisterMetaType<std::map<int, std::string>>("std::map<int, std::string>");
    QLoggingCategory::setFilterRules("qt.audio.debug=false");
    qRegisterMetaType<std::vector<std::pair<qint64,qint64>>>("std::vector<std::pair<qint64,qint64> >");

    auto user = User::getInstance();

    // 加载插件
    QString pluginDir = QCoreApplication::applicationDirPath() + "/plugin";
    qDebug() << "Loading plugins from:" << pluginDir;
    
    PluginManager& pluginManager = PluginManager::instance();
    int loadedPlugins = pluginManager.loadPlugins(pluginDir);
    qDebug() << "Loaded" << loadedPlugins << "plugins";

    MainWidget w;

    w.show();
    w.Update_paint();

    //qDebug()<<"启动";
    int result = a.exec();
    
    // 确保应用退出前完成所有清理
    qDebug() << "Application exiting, ensuring cleanup...";
    QThreadPool::globalInstance()->waitForDone(5000);  // 等待所有线程池任务完成
    qDebug() << "Application cleanup complete";
    
    cleanupLogger();  // 关闭日志文件
    return result;
}
