#include "play_widget.h"
#include <QFontMetrics>
#include <QDebug>
#include <QIcon>
#include <QApplication>
#include <QThread>
#include <QTimer>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include "main_widget.h"
#include "server_welcome_dialog.h"
#include "headers.h"
#include "plugin_manager.h"
#include "logger.h"
#include "settings_manager.h"
#include "user.h"
#include "AudioPlayer.h"
#include "AudioService.h"
#include "MediaSession.h"
#include "network/online_presence_manager.h"
#include "network/network/network_service.h"
#include <functional>
#include <cstdio>

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

void finalizeRuntimeForProcessExit(bool runtimeInitialized)
{
    qDebug() << "[AppShutdown] Main event loop exited, begin runtime finalization";

    if (runtimeInitialized) {
        qDebug() << "[AppShutdown] Runtime singletons cleanup begin";

        OnlinePresenceManager::instance().logoutAndClear(false, 400);
        Network::NetworkService::instance().cancelAllRequests();

        AudioService::instance().stop();
        AudioPlayer::instance().stop();
        AudioPlayer::instance().clearWriteOwner();
        AudioPlayer::instance().resetBuffer();

        qDebug() << "[AppShutdown] Runtime singletons cleanup end";
    } else {
        qDebug() << "[AppShutdown] Runtime cleanup skipped because main window was never entered";
    }

    qDebug() << "[AppShutdown] Unloading plugins before logger shutdown";
    PluginManager::instance().unloadAllPlugins();

    qDebug() << "[AppShutdown] Waiting for global thread pool...";
    QThreadPool::globalInstance()->waitForDone(5000);
    qDebug() << "[AppShutdown] Global thread pool wait complete";
}
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

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
    a.setQuitOnLastWindowClosed(false);

    // Initialize asynchronous file logger.
    const QString logPath = resolvePrintLogPath();
    initLogger(logPath);
    qDebug() << "=========================================";
    qDebug() << "YunMusic Starting...";
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
    a.setApplicationDisplayName(QStringLiteral(u"\u4e91\u97f3\u4e50"));

    QFile themeFile(":/styles/netease.qss");
    if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        a.setStyleSheet(QString::fromUtf8(themeFile.readAll()));
        qDebug() << "Loaded NetEase theme stylesheet";
    } else {
        qWarning() << "Failed to load NetEase theme stylesheet";
    }

    a.setWindowIcon(QIcon("qrc:/new/prefix1/icon/netease.ico"));

    setlocale(LC_ALL, "chs");

    qRegisterMetaType<std::map<int, std::string>>("std::map<int, std::string>");
    qRegisterMetaType<MediaSession::PlaybackState>("MediaSession::PlaybackState");
    qRegisterMetaType<MediaSession::PlaybackState>("PlaybackState");
    QLoggingCategory::setFilterRules("qt.audio.debug=false");
    qRegisterMetaType<std::vector<std::pair<qint64,qint64>>>("std::vector<std::pair<qint64,qint64> >");

    auto user = User::getInstance();

    // 从应用目录加载插件
    QString pluginDir = QCoreApplication::applicationDirPath() + "/plugin";
    qDebug() << "Loading plugins from:" << pluginDir;
    
    PluginManager& pluginManager = PluginManager::instance();
    pluginManager.setAllowedPermissions({
        QStringLiteral("ui.widget"),
        QStringLiteral("audio.convert"),
        QStringLiteral("speech.transcribe"),
        QStringLiteral("agent.host.read"),
        QStringLiteral("agent.host.control"),
        QStringLiteral("network.read"),
        QStringLiteral("storage.read"),
        QStringLiteral("playback.control")
    });
    pluginManager.hostContext()->setEnvironmentValue(QStringLiteral("appName"), QStringLiteral("云音乐"));
    pluginManager.hostContext()->setEnvironmentValue(QStringLiteral("appVersion"), QStringLiteral("1.0.0"));
    pluginManager.hostContext()->setEnvironmentValue(QStringLiteral("pluginDir"), pluginDir);
    pluginManager.hostContext()->setEnvironmentValue(QStringLiteral("themeName"), QStringLiteral("netease"));
    pluginManager.hostContext()->setEnvironmentValue(QStringLiteral("themeAccentColor"), QStringLiteral("#EC4141"));
    int loadedPlugins = pluginManager.loadPlugins(pluginDir);
    qDebug() << "Loaded" << loadedPlugins << "plugins";

    MainWidget* mainWindow = nullptr;
    bool pendingReturnToWelcome = false;
    bool runtimeInitialized = false;

    std::function<void(bool)> enterMainWindow;
    std::function<void(bool)> launchWelcomeFlow;

    enterMainWindow = [&](bool localOnlyMode) {
        if (mainWindow) {
            return;
        }

        runtimeInitialized = true;

        if (localOnlyMode) {
            qDebug() << "Entering main window in local-only mode";
        } else {
            qDebug() << "Server verified, using base URL:"
                     << SettingsManager::instance().serverBaseUrl();
        }

        mainWindow = new MainWidget(localOnlyMode);
        mainWindow->setAttribute(Qt::WA_DeleteOnClose, true);
        QObject::connect(mainWindow, &MainWidget::requestReturnToWelcome, &a, [&]() {
            pendingReturnToWelcome = true;
        });
        QObject::connect(mainWindow, &QObject::destroyed, &a, [&]() {
            mainWindow = nullptr;
            if (pendingReturnToWelcome) {
                pendingReturnToWelcome = false;
                QTimer::singleShot(0, &a, [&]() {
                    launchWelcomeFlow(false);
                });
                return;
            }
            QTimer::singleShot(0, &a, &QCoreApplication::quit);
        });

        mainWindow->show();
        mainWindow->updatePaint();
    };

    launchWelcomeFlow = [&](bool autoVerifyOnShow) {
        ServerWelcomeDialog welcomeDialog(autoVerifyOnShow);
        if (welcomeDialog.exec() != QDialog::Accepted) {
            qDebug() << "Server verification canceled or failed, application exits.";
            QTimer::singleShot(0, &a, &QCoreApplication::quit);
            return;
        }

        enterMainWindow(welcomeDialog.enterLocalOnly());
    };

    QTimer::singleShot(0, &a, [&]() {
        launchWelcomeFlow(true);
    });

    const int result = a.exec();

    finalizeRuntimeForProcessExit(runtimeInitialized);

    qDebug() << "Application cleanup complete";
    qDebug() << "[AppShutdown] Logger cleanup begin";
    cleanupLogger();  // 释放日志资源并刷新缓冲

#ifdef Q_OS_WIN
    std::fflush(nullptr);
    ExitProcess(static_cast<UINT>(result));
#endif

    return result;
}



