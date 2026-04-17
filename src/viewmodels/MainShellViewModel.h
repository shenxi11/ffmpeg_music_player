#ifndef MAINSHELLVIEWMODEL_H
#define MAINSHELLVIEWMODEL_H

#include "BaseViewModel.h"
#include "httprequest_v2.h"
#include "plugin_manager.h"

#include <QList>
#include <QUrl>
#include <QVariantMap>

/**
 * @brief 主窗口壳层视图模型。
 *
 * 负责承接主界面的网络请求、账号缓存、在线会话、音频事件转发与插件管理能力，
 * 让 MainWidget 专注于布局编排和交互转发。
 */
class MainShellViewModel : public BaseViewModel {
    Q_OBJECT

  public:
    explicit MainShellViewModel(QObject* parent = nullptr);

    HttpRequestV2* requestGateway() {
        return &m_request;
    }
    QObject* audioServiceObject() const;

    QString cachedAccount() const;
    QString cachedPassword() const;
    QString cachedUsername() const;
    QString cachedAvatarUrl() const;
    QString cachedOnlineSessionToken() const;
    QString cachedProfileCreatedAt() const;
    QString cachedProfileUpdatedAt() const;
    bool autoLoginEnabled() const;
    bool shouldAutoLogin() const;
    QString currentUserAccount() const;
    QString currentUserPassword() const;
    QString currentOnlineSessionToken() const;
    bool hasLoggedInUser() const;
    void clearCurrentUserProfile();

    void searchMusic(const QString& keyword);
    void requestRecommendations(const QString& userAccount,
                                const QString& scene = QStringLiteral("home"), int limit = 24,
                                bool excludePlayed = true);
    void requestSimilarRecommendations(const QString& songId, int limit = 12);
    void submitRecommendationFeedback(const QString& userId, const QString& songId,
                                      const QString& eventType, const QString& scene,
                                      const QString& requestId, const QString& modelVersion,
                                      qint64 playMs, qint64 durationMs);
    void requestHistory(const QString& userAccount, int limit = 50, bool useCache = true);
    void addPlayHistory(const QString& userAccount, const QString& path, const QString& title,
                        const QString& artist, const QString& album, const QString& duration,
                        bool isLocal);
    void removeHistory(const QString& userAccount, const QStringList& paths);
    void requestFavorites(const QString& userAccount, bool useCache = true);
    void addFavorite(const QString& userAccount, const QString& path, const QString& title,
                     const QString& artist, const QString& duration, bool isLocal);
    void removeFavorite(const QString& userAccount, const QStringList& paths);
    void requestPlaylists(const QString& userAccount, int page = 1, int pageSize = 20,
                          bool useCache = true);
    void createPlaylist(const QString& userAccount, const QString& name,
                        const QString& description = QString(),
                        const QString& coverPath = QString());
    void requestPlaylistDetail(const QString& userAccount, qint64 playlistId,
                               bool useCache = false);
    void prefetchPlaylistCoverDetail(const QString& userAccount, qint64 playlistId,
                                     bool useCache = true);
    void deletePlaylist(const QString& userAccount, qint64 playlistId);
    void updatePlaylist(const QString& userAccount, qint64 playlistId, const QString& name,
                        const QString& description = QString(),
                        const QString& coverPath = QString());
    void addPlaylistItems(const QString& userAccount, qint64 playlistId, const QVariantList& items);
    void removePlaylistItems(const QString& userAccount, qint64 playlistId,
                             const QStringList& musicPaths);
    void reorderPlaylistItems(const QString& userAccount, qint64 playlistId,
                              const QVariantList& orderedItems);
    void handleLoginSuccess(const QString& account, const QString& password,
                            const QString& username, const QString& avatarUrl = QString(),
                            const QString& onlineSessionToken = QString());
    void requestUserProfile();
    void updateCurrentUsername(const QString& username);
    void uploadCurrentAvatar(const QString& filePath);
    void logoutCurrentUser(bool graceful, int gracefulTimeoutMs = 0);
    void returnToWelcomeAndKeepAccountCache(bool graceful, int gracefulTimeoutMs = 0);
    void shutdownUserSessionOnAppExit(bool graceful, int gracefulTimeoutMs = 0);

    bool pauseAudioIfPlaying();
    bool resumeAudioIfPaused();
    bool stopAudio();
    bool seekAudio(qint64 positionMs);
    bool playNextAudio();
    bool playPreviousAudio();
    bool playAudioAtIndex(int index);
    bool setAudioVolume(int volume);
    bool setAudioPlayMode(int mode);
    bool setAudioPlaylist(const QList<QUrl>& urls, bool playNow, int startIndex);
    bool appendAudioToQueue(const QUrl& url);
    bool removeAudioFromQueue(int index);
    bool clearAudioQueue();
    bool playAudioUrl(const QUrl& url);
    bool playAudioPlaylist(const QList<QUrl>& urls);
    QVariantMap audioQueueSnapshot() const;
    QVariantMap audioCurrentTrackSnapshot() const;
    QVariantMap audioStateSnapshot() const;
    void shutdownAudioPipeline();

    int loadPlugins(const QString& pluginPath);
    QVector<PluginLoadFailure> pluginLoadFailures() const;
    QVector<PluginInfo> pluginInfos() const;
    PluginInterface* pluginById(const QString& pluginId) const;
    QString pluginDiagnosticsReport() const;
    QVariantMap pluginsSnapshot() const;
    QVariantMap pluginDiagnosticsSnapshot() const;
    QVariantMap reloadPluginsSnapshot();
    bool unloadPluginByKey(const QString& pluginKey);
    int unloadAllPluginsAndReturnCount();
    void registerPluginHostService(const QString& serviceName, QObject* service);
    void unloadAllPlugins();

    QVariantMap settingsSnapshot() const;
    bool updateSettingValue(const QString& key, const QVariant& value);

    void addLocalMusicCacheEntry(const QString& filePath, const QString& fileName);
    void updateLocalMusicCacheMetadata(const QString& filePath, const QString& coverUrl,
                                       const QString& duration, const QString& artist);
    void removeLocalMusicCacheEntry(const QString& filePath);
    int localMusicCacheDurationSeconds(const QString& filePath) const;

    bool resolveHistorySnapshot(const QString& sessionId, const QString& filePath,
                                const QString& fallbackArtist, QString* title, QString* artist,
                                QString* album, qint64* durationMs, bool* isLocal) const;

  signals:
    void searchResultsReady(const QList<Music>& musicList);
    void recommendationListReady(const QVariantMap& meta, const QVariantList& items);
    void similarRecommendationListReady(const QVariantMap& meta, const QVariantList& items,
                                        const QString& anchorSongId);
    void recommendationFeedbackResultReady(bool success, const QString& eventType,
                                           const QString& songId);
    void historyListReady(const QVariantList& history);
    void addHistoryResultReady(bool success);
    void favoritesListReady(const QVariantList& favorites);
    void addFavoriteResultReady(bool success);
    void removeFavoriteResultReady(bool success);
    void removeHistoryResultReady(bool success);
    void playlistsListReady(const QVariantList& playlists, int page, int pageSize, int total);
    void playlistDetailReady(const QVariantMap& detail);
    void playlistCoverDetailReady(const QVariantMap& detail);
    void createPlaylistResultReady(bool success, qint64 playlistId, const QString& message);
    void deletePlaylistResultReady(bool success, qint64 playlistId, const QString& message);
    void updatePlaylistResultReady(bool success, qint64 playlistId, const QString& message);
    void addPlaylistItemsResultReady(bool success, qint64 playlistId, int addedCount,
                                     int skippedCount, const QString& message);
    void removePlaylistItemsResultReady(bool success, qint64 playlistId, int deletedCount,
                                        const QString& message);
    void reorderPlaylistItemsResultReady(bool success, qint64 playlistId, const QString& message);
    void userProfileReady(const QVariantMap& profile);
    void userProfileRequestFailed(const QString& message, int statusCode);
    void updateUsernameResultReady(bool success, const QString& username, const QString& message,
                                   int statusCode);
    void uploadAvatarResultReady(bool success, const QString& avatarUrl, const QString& message,
                                 int statusCode);
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
