#include "settings_widget.h"

#include <QDateTime>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QQuickWidget(parent)
{
    qDebug() << "[SettingsWidget] Initializing...";

    engine()->addImportPath("qrc:/");
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/components/settings/Settings.qml"));

    if (status() == QQuickWidget::Error) {
        qWarning() << "[SettingsWidget] Failed to load Settings.qml";
        return;
    }

    qDebug() << "[SettingsWidget] QML loaded successfully";

    QQuickItem* root = rootObject();
    if (root) {
        SettingsManager& settings = SettingsManager::instance();
        QMetaObject::invokeMethod(root, "setDownloadPath", Q_ARG(QVariant, settings.downloadPath()));
        QMetaObject::invokeMethod(root, "setDownloadLyrics", Q_ARG(QVariant, settings.downloadLyrics()));
        QMetaObject::invokeMethod(root, "setDownloadCover", Q_ARG(QVariant, settings.downloadCover()));
        QMetaObject::invokeMethod(root, "setAudioCachePath", Q_ARG(QVariant, settings.audioCachePath()));
        QMetaObject::invokeMethod(root, "setLogPath", Q_ARG(QVariant, settings.logPath()));

        connect(root, SIGNAL(chooseDownloadPath()), this, SLOT(onChooseDownloadPath()));
        connect(root, SIGNAL(chooseAudioCachePath()), this, SLOT(onChooseAudioCachePath()));
        connect(root, SIGNAL(chooseLogPath()), this, SLOT(onChooseLogPath()));
        connect(root, SIGNAL(settingsClosed()), this, SLOT(close()));

        connect(root, SIGNAL(downloadPathChanged()), this, SLOT(onDownloadPathChanged()));
        connect(root, SIGNAL(downloadLyricsChanged()), this, SLOT(onDownloadLyricsChanged()));
        connect(root, SIGNAL(downloadCoverChanged()), this, SLOT(onDownloadCoverChanged()));
        connect(root, SIGNAL(audioCachePathChanged()), this, SLOT(onAudioCachePathChanged()));
        connect(root, SIGNAL(logPathChanged()), this, SLOT(onLogPathChanged()));
        connect(root, SIGNAL(refreshPresenceRequested()), this, SLOT(onRefreshPresenceRequested()));

        qDebug() << "[SettingsWidget] Signals connected";
    }

    connect(&OnlinePresenceManager::instance(),
            &OnlinePresenceManager::presenceSnapshotChanged,
            this,
            &SettingsWidget::onPresenceSnapshotChanged);

    m_presenceRefreshTimer.setInterval(5000);
    m_presenceRefreshTimer.setSingleShot(false);
    connect(&m_presenceRefreshTimer, &QTimer::timeout, this, &SettingsWidget::onRefreshPresenceRequested);
    m_presenceRefreshTimer.start();

    onPresenceSnapshotChanged(OnlinePresenceManager::instance().currentAccount(),
                              OnlinePresenceManager::instance().currentToken(),
                              OnlinePresenceManager::instance().currentOnline(),
                              OnlinePresenceManager::instance().currentHeartbeatIntervalSec(),
                              OnlinePresenceManager::instance().currentOnlineTtlSec(),
                              OnlinePresenceManager::instance().currentTtlRemainingSec(),
                              OnlinePresenceManager::instance().currentStatusMessage(),
                              OnlinePresenceManager::instance().currentLastSeenAt());
    QTimer::singleShot(0, this, &SettingsWidget::onRefreshPresenceRequested);

    setWindowTitle("设置");
    setFixedSize(600, 560);
    // Force top-level window behavior, prevent mixed rendering inside main window.
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, false);

    qDebug() << "[SettingsWidget] Initialization complete";
}

void SettingsWidget::onChooseDownloadPath()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        "选择下载保存路径",
        SettingsManager::instance().downloadPath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dir.isEmpty()) {
        qDebug() << "[SettingsWidget] Download path selected:" << dir;

        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setDownloadPath", Q_ARG(QVariant, dir));
        }

        SettingsManager::instance().setDownloadPath(dir);
    }
}

void SettingsWidget::onChooseAudioCachePath()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        "选择音频缓存目录",
        SettingsManager::instance().audioCachePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (dir.isEmpty()) {
        return;
    }

    qDebug() << "[SettingsWidget] Audio cache path selected:" << dir;

    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "setAudioCachePath", Q_ARG(QVariant, dir));
    }

    SettingsManager::instance().setAudioCachePath(dir);
}

void SettingsWidget::onChooseLogPath()
{
    const QString currentPath = SettingsManager::instance().logPath();
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "选择日志文件路径",
        currentPath,
        "Text File (*.txt);;All Files (*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    QFileInfo info(filePath);
    if (info.suffix().isEmpty()) {
        filePath += ".txt";
    }

    qDebug() << "[SettingsWidget] Log path selected:" << filePath;

    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "setLogPath", Q_ARG(QVariant, filePath));
    }

    SettingsManager::instance().setLogPath(filePath);
}

void SettingsWidget::onDownloadPathChanged()
{
    QQuickItem* root = rootObject();
    if (root) {
        const QString path = root->property("downloadPath").toString();
        SettingsManager::instance().setDownloadPath(path);
        qDebug() << "[SettingsWidget] Download path changed:" << path;
    }
}

void SettingsWidget::onDownloadLyricsChanged()
{
    QQuickItem* root = rootObject();
    if (root) {
        const bool enable = root->property("downloadLyrics").toBool();
        SettingsManager::instance().setDownloadLyrics(enable);
        qDebug() << "[SettingsWidget] Download lyrics changed:" << enable;
    }
}

void SettingsWidget::onDownloadCoverChanged()
{
    QQuickItem* root = rootObject();
    if (root) {
        const bool enable = root->property("downloadCover").toBool();
        SettingsManager::instance().setDownloadCover(enable);
        qDebug() << "[SettingsWidget] Download cover changed:" << enable;
    }
}

void SettingsWidget::onAudioCachePathChanged()
{
    QQuickItem* root = rootObject();
    if (root) {
        const QString path = root->property("audioCachePath").toString();
        SettingsManager::instance().setAudioCachePath(path);
        qDebug() << "[SettingsWidget] Audio cache path changed:" << path;
    }
}

void SettingsWidget::onLogPathChanged()
{
    QQuickItem* root = rootObject();
    if (root) {
        const QString path = root->property("logPath").toString();
        SettingsManager::instance().setLogPath(path);
        qDebug() << "[SettingsWidget] Log path changed:" << path;
    }
}

void SettingsWidget::onRefreshPresenceRequested()
{
    OnlinePresenceManager::instance().requestStatusRefresh();
}

void SettingsWidget::onPresenceSnapshotChanged(const QString& account,
                                               const QString& sessionToken,
                                               bool online,
                                               int heartbeatIntervalSec,
                                               int onlineTtlSec,
                                               int ttlRemainingSec,
                                               const QString& statusMessage,
                                               qint64 lastSeenAt)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }

    QString lastSeenText = QStringLiteral("未上报");
    if (lastSeenAt > 0) {
        const QDateTime dt = QDateTime::fromSecsSinceEpoch(lastSeenAt);
        lastSeenText = dt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }

    QMetaObject::invokeMethod(root,
                              "setPresenceSnapshot",
                              Q_ARG(QVariant, QVariant(account)),
                              Q_ARG(QVariant, QVariant(sessionToken)),
                              Q_ARG(QVariant, QVariant(online)),
                              Q_ARG(QVariant, QVariant(heartbeatIntervalSec)),
                              Q_ARG(QVariant, QVariant(onlineTtlSec)),
                              Q_ARG(QVariant, QVariant(ttlRemainingSec)),
                              Q_ARG(QVariant, QVariant(statusMessage)),
                              Q_ARG(QVariant, QVariant(lastSeenText)));
}
