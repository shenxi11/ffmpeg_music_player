#include "SettingsViewModel.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QWidget>

#include "AudioCacheManager.h"
#include "AudioService.h"
#include "network_service.h"
#include "online_presence_manager.h"
#include "settings_manager.h"

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

}

SettingsViewModel::SettingsViewModel(QObject* parent)
    : BaseViewModel(parent)
{
    connect(&OnlinePresenceManager::instance(),
            &OnlinePresenceManager::presenceSnapshotChanged,
            this,
            &SettingsViewModel::onPresenceSnapshotChanged);

    syncFromSettings();
    updatePresenceSnapshot(OnlinePresenceManager::instance().currentAccount(),
                           OnlinePresenceManager::instance().currentToken(),
                           OnlinePresenceManager::instance().currentOnline(),
                           OnlinePresenceManager::instance().currentHeartbeatIntervalSec(),
                           OnlinePresenceManager::instance().currentOnlineTtlSec(),
                           OnlinePresenceManager::instance().currentTtlRemainingSec(),
                           OnlinePresenceManager::instance().currentStatusMessage(),
                           OnlinePresenceManager::instance().currentLastSeenAt());
}

QString SettingsViewModel::downloadPath() const
{
    return SettingsManager::instance().downloadPath();
}

QString SettingsViewModel::audioCachePath() const
{
    return SettingsManager::instance().audioCachePath();
}

QString SettingsViewModel::logPath() const
{
    return SettingsManager::instance().logPath();
}

bool SettingsViewModel::downloadLyrics() const
{
    return SettingsManager::instance().downloadLyrics();
}

bool SettingsViewModel::downloadCover() const
{
    return SettingsManager::instance().downloadCover();
}

int SettingsViewModel::playerPageStyle() const
{
    return SettingsManager::instance().playerPageStyle();
}

QString SettingsViewModel::serverHost() const
{
    return SettingsManager::instance().serverHost();
}

int SettingsViewModel::serverPort() const
{
    return SettingsManager::instance().serverPort();
}

void SettingsViewModel::chooseDownloadPath(QWidget* parent)
{
    const QString dir = QFileDialog::getExistingDirectory(
        parent,
        QStringLiteral("选择下载保存路径"),
        downloadPath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        setDownloadPath(dir);
    }
}

void SettingsViewModel::chooseAudioCachePath(QWidget* parent)
{
    const QString dir = QFileDialog::getExistingDirectory(
        parent,
        QStringLiteral("选择音频缓存目录"),
        audioCachePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        setAudioCachePath(dir);
    }
}

void SettingsViewModel::chooseLogPath(QWidget* parent)
{
    QString filePath = QFileDialog::getSaveFileName(
        parent,
        QStringLiteral("选择日志文件路径"),
        logPath(),
        QStringLiteral("Text File (*.txt);;All Files (*)"));

    if (filePath.isEmpty()) {
        return;
    }

    QFileInfo info(filePath);
    if (info.suffix().isEmpty()) {
        filePath += QStringLiteral(".txt");
    }

    setLogPath(filePath);
}

void SettingsViewModel::clearLocalCache(QWidget* parent)
{
    const QString cachePath = audioCachePath().trimmed();
    if (cachePath.isEmpty()) {
        QMessageBox::warning(parent,
                             QStringLiteral("清理失败"),
                             QStringLiteral("当前缓存目录无效，请先设置缓存目录。"));
        return;
    }

    const bool isAudioActive = AudioService::instance().isPlaying() || AudioService::instance().isPaused();
    if (isAudioActive) {
        const auto stopReply = QMessageBox::question(
            parent,
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
        parent,
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
    Network::NetworkService::instance().clearCache();

    if (!cleared) {
        QMessageBox::warning(
            parent,
            QStringLiteral("清理完成（部分失败）"),
            QStringLiteral("缓存已部分清理，部分文件可能正在占用。\n已清理文件：%1\n已释放空间：%2\n详情：%3")
                .arg(QString::number(removedFiles),
                     formatBytes(removedBytes),
                     errorMessage.isEmpty() ? QStringLiteral("未知错误") : errorMessage));
        return;
    }

    QMessageBox::information(
        parent,
        QStringLiteral("清理完成"),
        QStringLiteral("本地缓存已清理完成。\n已清理文件：%1\n已释放空间：%2")
            .arg(QString::number(removedFiles), formatBytes(removedBytes)));
}

void SettingsViewModel::setDownloadPath(const QString& path)
{
    if (downloadPath() == path) {
        return;
    }
    SettingsManager::instance().setDownloadPath(path);
    emit downloadPathChanged();
}

void SettingsViewModel::setAudioCachePath(const QString& path)
{
    if (audioCachePath() == path) {
        return;
    }
    SettingsManager::instance().setAudioCachePath(path);
    emit audioCachePathChanged();
}

void SettingsViewModel::setLogPath(const QString& path)
{
    if (logPath() == path) {
        return;
    }
    SettingsManager::instance().setLogPath(path);
    emit logPathChanged();
}

void SettingsViewModel::setDownloadLyrics(bool enabled)
{
    if (downloadLyrics() == enabled) {
        return;
    }
    SettingsManager::instance().setDownloadLyrics(enabled);
    emit downloadLyricsChanged();
}

void SettingsViewModel::setDownloadCover(bool enabled)
{
    if (downloadCover() == enabled) {
        return;
    }
    SettingsManager::instance().setDownloadCover(enabled);
    emit downloadCoverChanged();
}

void SettingsViewModel::setPlayerPageStyle(int styleId)
{
    if (playerPageStyle() == styleId) {
        return;
    }
    SettingsManager::instance().setPlayerPageStyle(styleId);
    emit playerPageStyleChanged();
}

void SettingsViewModel::refreshPresence()
{
    OnlinePresenceManager::instance().requestStatusRefresh();
}

void SettingsViewModel::syncFromSettings()
{
    emit downloadPathChanged();
    emit audioCachePathChanged();
    emit logPathChanged();
    emit downloadLyricsChanged();
    emit downloadCoverChanged();
    emit playerPageStyleChanged();
}

void SettingsViewModel::onPresenceSnapshotChanged(const QString& account,
                                                  const QString& sessionToken,
                                                  bool online,
                                                  int heartbeatIntervalSec,
                                                  int onlineTtlSec,
                                                  int ttlRemainingSec,
                                                  const QString& statusMessage,
                                                  qint64 lastSeenAt)
{
    updatePresenceSnapshot(account,
                           sessionToken,
                           online,
                           heartbeatIntervalSec,
                           onlineTtlSec,
                           ttlRemainingSec,
                           statusMessage,
                           lastSeenAt);
}

void SettingsViewModel::updatePresenceSnapshot(const QString& account,
                                               const QString& sessionToken,
                                               bool online,
                                               int heartbeatIntervalSec,
                                               int onlineTtlSec,
                                               int ttlRemainingSec,
                                               const QString& statusMessage,
                                               qint64 lastSeenAt)
{
    m_presenceAccount = account;
    m_presenceSessionToken = sessionToken;
    m_presenceOnline = online;
    m_presenceHeartbeatIntervalSec = heartbeatIntervalSec;
    m_presenceOnlineTtlSec = onlineTtlSec;
    m_presenceTtlRemainingSec = ttlRemainingSec;
    m_presenceStatusMessage = statusMessage;

    m_presenceLastSeenText = QStringLiteral("未上报");
    if (lastSeenAt > 0) {
        const QDateTime dt = QDateTime::fromSecsSinceEpoch(lastSeenAt);
        m_presenceLastSeenText = dt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }

    emit presenceChanged();
}
