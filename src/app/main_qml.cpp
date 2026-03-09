#include "qml_bridge.h"
#include "plugin_manager.h"
#include "logger.h"
#include "httprequest_v2.h"
#include "settings_manager.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QDir>
#include <QIcon>
#include <QLoggingCategory>
#include <QThreadPool>
#include <QDebug>
#include <map>
#include <string>
#include <vector>

#ifdef Q_OS_WIN
#include <Windows.h>
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
} // namespace

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring exeDir = exePath;
    const size_t pos = exeDir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        exeDir = exeDir.substr(0, pos);
    }
    const std::wstring resourceDir = exeDir + L"\\resource";
    SetDllDirectoryW(resourceDir.c_str());
#endif

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    const QString logPath = resolvePrintLogPath();
    initLogger(logPath);
    qDebug() << "=========================================";
    qDebug() << "YunMusic (QML Version) Starting...";
    qDebug() << "Log file:" << currentLogFilePath();

#ifdef Q_OS_WIN
    const QString resourceDirQt = QCoreApplication::applicationDirPath() + "/resource";
    qDebug() << "DLL search path (resource):" << resourceDirQt;
    qDebug() << "Resource directory exists:" << QDir(resourceDirQt).exists();
#endif
    qDebug() << "=========================================";

    app.setOrganizationName("MusicPlayer");
    app.setApplicationName("FFmpegMusicPlayer");
    app.setApplicationDisplayName(QStringLiteral(u"\u4e91\u97f3\u4e50"));
    app.setWindowIcon(QIcon("qrc:/new/prefix1/icon/netease.ico"));

    setlocale(LC_ALL, "chs");

    qRegisterMetaType<std::map<int, std::string>>("std::map<int, std::string>");
    qRegisterMetaType<std::vector<std::pair<qint64, qint64>>>("std::vector<std::pair<qint64,qint64>>");
    QLoggingCategory::setFilterRules("qt.audio.debug=false");

    const QString pluginDir = QCoreApplication::applicationDirPath() + "/plugin";
    qDebug() << "Loading plugins from:" << pluginDir;

    PluginManager& pluginManager = PluginManager::instance();
    pluginManager.setAllowedPermissions({
        QStringLiteral("ui.widget"),
        QStringLiteral("audio.convert"),
        QStringLiteral("speech.transcribe"),
        QStringLiteral("network.read"),
        QStringLiteral("storage.read"),
        QStringLiteral("playback.control")
    });
    pluginManager.hostContext()->setEnvironmentValue(QStringLiteral("appName"), QStringLiteral("云音乐"));
    pluginManager.hostContext()->setEnvironmentValue(QStringLiteral("appVersion"), QStringLiteral("1.0.0"));
    pluginManager.hostContext()->setEnvironmentValue(QStringLiteral("pluginDir"), pluginDir);
    pluginManager.hostContext()->setEnvironmentValue(QStringLiteral("themeName"), QStringLiteral("netease"));
    pluginManager.hostContext()->setEnvironmentValue(QStringLiteral("themeAccentColor"), QStringLiteral("#EC4141"));
    const int loadedPlugins = pluginManager.loadPlugins(pluginDir);
    qDebug() << "Loaded" << loadedPlugins << "plugins";

    QQmlApplicationEngine engine;

    QObject::connect(&engine, &QQmlApplicationEngine::warnings,
                     [](const QList<QQmlError>& warnings) {
        for (const QQmlError& warning : warnings) {
            qWarning() << "QML Warning:" << warning.toString();
        }
    });

    UIBridge uiBridge;
    engine.rootContext()->setContextProperty("uiBridge", &uiBridge);

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject* obj, const QUrl& objUrl) {
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

    const int result = app.exec();

    qDebug() << "Application exiting, ensuring cleanup...";
    QThreadPool::globalInstance()->waitForDone(5000);
    qDebug() << "Application cleanup complete";

    cleanupLogger();
    return result;
}
