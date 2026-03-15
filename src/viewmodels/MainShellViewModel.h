#ifndef MAINSHELLVIEWMODEL_H
#define MAINSHELLVIEWMODEL_H

#include "BaseViewModel.h"
#include "httprequest_v2.h"
#include "plugin_manager.h"

#include <QUrl>

/**
 * @brief 主窗口壳层视图模型。
 *
 * 负责承接主界面的网络请求、账号缓存、在线会话、音频事件转发与插件管理能力，
 * 让 MainWidget 专注于布局编排和交互转发。
 */
class MainShellViewModel : public BaseViewModel
{
    Q_OBJECT

public:
    explicit MainShellViewModel(QObject* parent = nullptr);

    HttpRequestV2* requestGateway() { return &m_request; }
    QObject* audioServiceObject() const;

    QString cachedAccount() const;
    QString cachedPassword() const;
    QString cachedUsername() const;
    bool autoLoginEnabled() const;
    QString currentUserAccount() const;
    QString currentUserPassword() const;
    bool hasLoggedInUser() const;
    void clearCurrentUserProfile();

    void searchMusic(const QString& keyword);
    void requestRecommendations(const QString& userAccount,
                                const QString& scene = QStringLiteral("home"),
                                int limit = 24,
                                bool excludePlayed = true);
    void requestSimilarRecommendations(const QString& songId, int limit = 12);
    void submitRecommendationFeedback(const QString& userId,
                                      const QString& songId,
                                      const QString& eventType,
                                      const QString& scene,
                                      const QString& requestId,
                                      const QString& modelVersion,
                                      qint64 playMs,
                                      qint64 durationMs);
    void requestHistory(const QString& userAccount, int limit = 50, bool useCache = true);
    void addPlayHistory(const QString& userAccount,
                        const QString& path,
                        const QString& title,
                        const QString& artist,
                        const QString& album,
                        const QString& duration,
                        bool isLocal);
    void removeHistory(const QString& userAccount, const QStringList& paths);
    void requestFavorites(const QString& userAccount, bool useCache = true);
    void addFavorite(const QString& userAccount,
                     const QString& path,
                     const QString& title,
                     const QString& artist,
                     const QString& duration,
                     bool isLocal);
    void removeFavorite(const QString& userAccount, const QStringList& paths);
    void handleLoginSuccess(const QString& account,
                            const QString& password,
                            const QString& username);
    void logoutCurrentUser(bool graceful, int gracefulTimeoutMs = 0);

    void pauseAudioIfPlaying();
    void stopAudio();
    void shutdownAudioPipeline();

    int loadPlugins(const QString& pluginPath);
    QVector<PluginLoadFailure> pluginLoadFailures() const;
    QVector<PluginInfo> pluginInfos() const;
    PluginInterface* pluginById(const QString& pluginId) const;
    QString pluginDiagnosticsReport() const;
    void registerPluginHostService(const QString& serviceName, QObject* service);
    void unloadAllPlugins();

    void addLocalMusicCacheEntry(const QString& filePath, const QString& fileName);
    void updateLocalMusicCacheMetadata(const QString& filePath,
                                       const QString& coverUrl,
                                       const QString& duration);
    void removeLocalMusicCacheEntry(const QString& filePath);
    int localMusicCacheDurationSeconds(const QString& filePath) const;

    bool resolveHistorySnapshot(const QString& sessionId,
                                const QString& filePath,
                                const QString& fallbackArtist,
                                QString* title,
                                QString* artist,
                                QString* album,
                                qint64* durationMs,
                                bool* isLocal) const;

signals:
    void searchResultsReady(const QList<Music>& musicList);
    void recommendationListReady(const QVariantMap& meta, const QVariantList& items);
    void similarRecommendationListReady(const QVariantMap& meta,
                                        const QVariantList& items,
                                        const QString& anchorSongId);
    void historyListReady(const QVariantList& history);
    void favoritesListReady(const QVariantList& favorites);
    void addFavoriteResultReady(bool success);
    void removeFavoriteResultReady(bool success);
    void removeHistoryResultReady(bool success);
    void sessionExpired();
    void accountCacheChanged();

    void audioPlaybackStarted(const QString& sessionId, const QUrl& url);
    void audioPlaybackPaused();
    void audioPlaybackResumed();
    void audioPlaybackStopped();

    void pluginLoadFailed(const QString& pluginFilePath, const QString& reason);

private slots:
    void onSessionExpired();

private:
    // 连接拆分：集中管理请求层、播放层、在线状态层事件绑定。
    void setupConnections();

    HttpRequestV2 m_request;
};

#endif // MAINSHELLVIEWMODEL_H
