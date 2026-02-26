#include "play_widget.h"
#include <QFontMetrics>
#include <QDebug>
#include <QIcon>
#include <QApplication>
#include <QThread>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include "main_widget.h"
#include "headers.h"
#include "plugin_manager.h"
#include "logger.h"
#include "settings_manager.h"
#include "user.h"

#ifdef Q_OS_WIN
#include <Windows.h>  // For SetDllDirectoryW, GetModuleFileNameW
#include <string>     // For std::wstring
#endif

namespace {
QString resolvePrintLogPath()
{
    const QString envPath = qEnvironmentVariable("PRINT_LOG_PATH").trimmed();
    if (!envPath.isEmpty()) {
        return QDir::cleanPath(QDir::fromNativeSeparators(envPath));
    }

    const QString settingsPath = SettingsManager::instance().logPath().trimmed();
    if (!settingsPath.isEmpty()) {
        return QDir::cleanPath(settingsPath);
    }

    return QDir::current().absoluteFilePath(QStringLiteral("打印日志.txt"));
}
}

int main(int argc, char *argv[])
{
    // Windows 下先设置 DLL 搜索目录，避免运行期找不到依赖库
#ifdef Q_OS_WIN
    // 获取当前可执行文件所在目录
    // 资源与第三方 DLL 统一放在 exe 同级的 resource 目录
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring exeDir = exePath;
    size_t pos = exeDir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        exeDir = exeDir.substr(0, pos);
    }
    std::wstring resourceDir = exeDir + L"\\resource";
    
    // 把 resource 目录加入系统 DLL 搜索路径
    SetDllDirectoryW(resourceDir.c_str());
    
    // 如果需要更精细的目录管理，可改用 AddDllDirectory
    // AddDllDirectory(resourceDir.c_str());
#endif
    
    QApplication a(argc, argv);

    // Initialize asynchronous file logger.
    const QString logPath = resolvePrintLogPath();
    initLogger(logPath);
    qDebug() << "=========================================";
    qDebug() << "FFmpeg Music Player Starting...";
    qDebug() << "Log file:" << currentLogFilePath();
    
    // 打印运行时 DLL 搜索信息，便于排查依赖加载失败
#ifdef Q_OS_WIN
    QString resourceDirQt = QCoreApplication::applicationDirPath() + "/resource";
    qDebug() << "DLL search path (resource):" << resourceDirQt;
    qDebug() << "Resource directory exists:" << QDir(resourceDirQt).exists();
#endif
    qDebug() << "=========================================";

    // 设置组织与应用名（影响配置文件路径等 Qt 行为）
    a.setOrganizationName("MusicPlayer");
    a.setApplicationName("FFmpegMusicPlayer");

    QFile themeFile(":/styles/netease.qss");
    if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        a.setStyleSheet(QString::fromUtf8(themeFile.readAll()));
        qDebug() << "Loaded NetEase theme stylesheet";
    } else {
        qWarning() << "Failed to load NetEase theme stylesheet";
    }

    a.setWindowIcon(QIcon("qrc:/new/prefix1/icon/netease.png"));

    setlocale(LC_ALL, "chs");

    qRegisterMetaType<std::map<int, std::string>>("std::map<int, std::string>");
    QLoggingCategory::setFilterRules("qt.audio.debug=false");
    qRegisterMetaType<std::vector<std::pair<qint64,qint64>>>("std::vector<std::pair<qint64,qint64> >");

    auto user = User::getInstance();

    // 从应用目录加载插件
    QString pluginDir = QCoreApplication::applicationDirPath() + "/plugin";
    qDebug() << "Loading plugins from:" << pluginDir;
    
    PluginManager& pluginManager = PluginManager::instance();
    int loadedPlugins = pluginManager.loadPlugins(pluginDir);
    qDebug() << "Loaded" << loadedPlugins << "plugins";

    int result = 0;
    {
        MainWidget w;

        w.show();
        w.Update_paint();

        // qDebug() << "started";
        result = a.exec();
    }
    
    // 应用退出前等待线程池任务收敛，减少资源竞争与崩溃概率
    qDebug() << "Application exiting, ensuring cleanup...";
    QThreadPool::globalInstance()->waitForDone(5000);  // 最多等待 5 秒
    qDebug() << "Application cleanup complete";
    
    cleanupLogger();  // 释放日志资源并刷新缓冲
    return result;
}



