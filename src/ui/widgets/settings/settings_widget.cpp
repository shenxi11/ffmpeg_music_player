#include "settings_widget.h"

#include <QDateTime>
#include <QMessageBox>

#include "AudioCacheManager.h"
#include "AudioService.h"
#include "network_service.h"

namespace {

QString formatBytes(qint64 bytes)
{
    static const char* kUnits[] = {"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(qMax<qint64>(0, bytes));
    int unitIndex = 0;
    while (value >= 1024.0 && unitIndex < 4) {
        value /= 1024.0;
        ++unitIndex;
    }

    if (unitIndex == 0) {
        return QString::number(static_cast<qint64>(value)) + QStringLiteral(" ") + kUnits[unitIndex];
    }
    return QString::number(value, 'f', 2) + QStringLiteral(" ") + kUnits[unitIndex];
}

} // namespace

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
        connect(root, SIGNAL(clearLocalCacheRequested()), this, SLOT(onClearLocalCacheRequested()));
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
    resize(860, 700);
    setMinimumSize(720, 560);
    // Force top-level window behavior, prevent mixed rendering inside main window.
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
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

void SettingsWidget::onClearLocalCacheRequested()
{
    SettingsManager& settings = SettingsManager::instance();
    const QString cachePath = settings.audioCachePath();

    if (cachePath.trimmed().isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("清理失败"),
                             QStringLiteral("当前缓存目录无效，请先设置缓存目录。"));
        return;
    }

    const bool isAudioActive = AudioService::instance().isPlaying() || AudioService::instance().isPaused();
    if (isAudioActive) {
        const auto stopReply = QMessageBox::question(
            this,
            QStringLiteral("清理本地缓存"),
            QStringLiteral("检测到正在播放音频，清理缓存前需要先停止播放。是否继续？"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (stopReply != QMessageBox::Yes) {
            return;
        }
        AudioService::instance().stop();
    }

    const auto confirmReply = QMessageBox::question(
        this,
        QStringLiteral("确认清理"),
        QStringLiteral("将删除缓存目录中的本地缓存文件：\n%1\n\n该操作不可恢复，是否继续？").arg(cachePath),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (confirmReply != QMessageBox::Yes) {
        return;
    }

    qint64 removedBytes = 0;
    qint64 removedFiles = 0;
    QString errorMessage;
    const bool cleared = AudioCacheManager::instance().clearCache(&errorMessage, &removedBytes, &removedFiles);

    // Also clear in-memory HTTP response cache.
    Network::NetworkService::instance().clearCache();

    if (!cleared) {
        QMessageBox::warning(
            this,
            QStringLiteral("清理完成（部分失败）"),
            QStringLiteral("缓存已部分清理，部分文件可能正在占用。\n已清理文件：%1\n已释放空间：%2\n详情：%3")
                .arg(QString::number(removedFiles),
                     formatBytes(removedBytes),
                     errorMessage.isEmpty() ? QStringLiteral("未知错误") : errorMessage));
        return;
    }

    QMessageBox::information(
        this,
        QStringLiteral("清理完成"),
        QStringLiteral("本地缓存已清理完成。\n已清理文件：%1\n已释放空间：%2")
            .arg(QString::number(removedFiles), formatBytes(removedBytes)));
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
