#ifndef CLIENT_AUTOMATION_HOST_SERVICE_H
#define CLIENT_AUTOMATION_HOST_SERVICE_H

#include <QObject>
#include <QList>
#include <QVariantList>
#include <QVariantMap>

class MainShellViewModel;
class MainWidget;
class Music;

/**
 * @brief 主程序对插件开放的客户端自动化服务。
 *
 * 该服务是 AI 插件访问宿主状态与业务动作的唯一稳定入口，避免插件直接依赖
 * MainWidget 或 MainShellViewModel 的具体实现类型。
 */
class ClientAutomationHostService : public QObject
{
    Q_OBJECT

public:
    explicit ClientAutomationHostService(MainWidget* mainWidget,
                                         MainShellViewModel* shellViewModel,
                                         QObject* parent = nullptr);

    Q_INVOKABLE QString currentUserAccount() const;
    Q_INVOKABLE QVariantMap hostContextSnapshot() const;
    Q_INVOKABLE QVariantMap currentTrackSnapshot() const;
    Q_INVOKABLE QVariantList localMusicItems(int limit = -1) const;
    Q_INVOKABLE QVariantMap playbackQueueSnapshot() const;
    Q_INVOKABLE QVariantMap settingsSnapshot() const;
    Q_INVOKABLE bool updateSetting(const QString& key, const QVariant& value);

    Q_INVOKABLE bool pausePlayback();
    Q_INVOKABLE bool resumePlayback();
    Q_INVOKABLE bool stopPlayback();
    Q_INVOKABLE bool seekPlayback(qint64 positionMs);
    Q_INVOKABLE bool playNext();
    Q_INVOKABLE bool playPrevious();
    Q_INVOKABLE bool playAtIndex(int index);
    Q_INVOKABLE bool setVolume(int volume);
    Q_INVOKABLE bool setPlayMode(int mode);
    Q_INVOKABLE bool setPlaybackQueue(const QVariantList& trackItems,
                                      int startIndex,
                                      bool playNow);
    Q_INVOKABLE bool addToPlaybackQueue(const QString& musicPath);
    Q_INVOKABLE bool removeFromPlaybackQueue(int index);
    Q_INVOKABLE bool clearPlaybackQueue();
    Q_INVOKABLE bool playTrack(const QString& musicPath);
    Q_INVOKABLE bool playPlaylist(const QVariantList& trackItems);

    Q_INVOKABLE bool addLocalTrack(const QVariantMap& track);
    Q_INVOKABLE bool removeLocalTrack(const QString& filePath);

    Q_INVOKABLE QVariantMap downloadTasksSnapshot(const QString& scope) const;
    Q_INVOKABLE QVariantMap downloadTaskSnapshot(const QString& taskId) const;
    Q_INVOKABLE QVariantMap startDownloadTrack(const QString& relativePath, const QString& coverUrl);
    Q_INVOKABLE bool pauseDownloadTask(const QString& taskId);
    Q_INVOKABLE bool resumeDownloadTask(const QString& taskId);
    Q_INVOKABLE bool cancelDownloadTask(const QString& taskId);
    Q_INVOKABLE QVariantMap removeDownloadTask(const QString& taskId);

    Q_INVOKABLE QVariantMap videoWindowState() const;
    Q_INVOKABLE bool playVideo(const QString& source);
    Q_INVOKABLE bool pauseVideo();
    Q_INVOKABLE bool resumeVideo();
    Q_INVOKABLE bool seekVideo(qint64 positionMs);
    Q_INVOKABLE bool setVideoFullScreen(bool enabled);
    Q_INVOKABLE bool setVideoPlaybackRate(double rate);
    Q_INVOKABLE bool setVideoQualityPreset(const QString& preset);
    Q_INVOKABLE bool closeVideoWindow();

    Q_INVOKABLE QVariantMap desktopLyricsState() const;
    Q_INVOKABLE bool setDesktopLyricsVisible(bool visible);
    Q_INVOKABLE bool setDesktopLyricsStyle(const QVariantMap& style);

    Q_INVOKABLE QVariantMap uiOverviewSnapshot() const;
    Q_INVOKABLE QVariantMap uiPageStateSnapshot(const QString& pageKey) const;
    Q_INVOKABLE QVariantList musicTabItemsSnapshot(const QString& tabKey,
                                                   const QVariantMap& options) const;
    Q_INVOKABLE QVariantMap resolveMusicTabItem(const QString& tabKey,
                                                const QVariantMap& selector) const;
    Q_INVOKABLE QVariantMap invokeSongAction(const QString& action, const QVariantMap& songData);

    Q_INVOKABLE QVariantMap userProfileSnapshot() const;
    Q_INVOKABLE bool refreshUserProfile();
    Q_INVOKABLE bool updateUsername(const QString& username);
    Q_INVOKABLE bool uploadAvatar(const QString& filePath);
    Q_INVOKABLE bool logoutUser();
    Q_INVOKABLE bool returnToWelcome();

    Q_INVOKABLE QVariantMap pluginsSnapshot() const;
    Q_INVOKABLE QVariantMap pluginDiagnosticsSnapshot() const;
    Q_INVOKABLE QVariantMap reloadPlugins();
    Q_INVOKABLE bool unloadPlugin(const QString& pluginKey);
    Q_INVOKABLE int unloadAllPlugins();

    Q_INVOKABLE void searchMusic(const QString& keyword);
    Q_INVOKABLE void requestHistory(const QString& userAccount, int limit, bool useCache);
    Q_INVOKABLE void requestFavorites(const QString& userAccount, bool useCache);
    Q_INVOKABLE void requestPlaylists(const QString& userAccount, int page, int pageSize,
                                      bool useCache);
    Q_INVOKABLE void requestPlaylistDetail(const QString& userAccount, qint64 playlistId,
                                           bool useCache);
    Q_INVOKABLE void requestRecommendations(const QString& userAccount,
                                            const QString& scene,
                                            int limit,
                                            bool excludePlayed);
    Q_INVOKABLE void requestSimilarRecommendations(const QString& songId, int limit);
    Q_INVOKABLE void submitRecommendationFeedback(const QString& userId,
                                                  const QString& songId,
                                                  const QString& eventType,
                                                  const QString& scene,
                                                  const QString& requestId,
                                                  const QString& modelVersion,
                                                  qint64 playMs,
                                                  qint64 durationMs);
    Q_INVOKABLE void addPlayHistory(const QString& userAccount,
                                    const QString& path,
                                    const QString& title,
                                    const QString& artist,
                                    const QString& album,
                                    const QString& duration,
                                    bool isLocal);
    Q_INVOKABLE void removeHistory(const QString& userAccount, const QStringList& paths);
    Q_INVOKABLE void addFavorite(const QString& userAccount,
                                 const QString& path,
                                 const QString& title,
                                 const QString& artist,
                                 const QString& duration,
                                 bool isLocal);
    Q_INVOKABLE void removeFavorite(const QString& userAccount, const QStringList& paths);
    Q_INVOKABLE void createPlaylist(const QString& userAccount,
                                    const QString& name,
                                    const QString& description,
                                    const QString& coverPath);
    Q_INVOKABLE void updatePlaylist(const QString& userAccount,
                                    qint64 playlistId,
                                    const QString& name,
                                    const QString& description,
                                    const QString& coverPath);
    Q_INVOKABLE void deletePlaylist(const QString& userAccount, qint64 playlistId);
    Q_INVOKABLE void addPlaylistItems(const QString& userAccount,
                                      qint64 playlistId,
                                      const QVariantList& items);
    Q_INVOKABLE void removePlaylistItems(const QString& userAccount,
                                         qint64 playlistId,
                                         const QStringList& musicPaths);
    Q_INVOKABLE void reorderPlaylistItems(const QString& userAccount,
                                          qint64 playlistId,
                                          const QVariantList& orderedItems);

    Q_INVOKABLE void requestLyrics(const QString& musicPath);
    Q_INVOKABLE void requestVideoList();
    Q_INVOKABLE void requestVideoStreamUrl(const QString& videoPath);
    Q_INVOKABLE void searchArtist(const QString& artist);
    Q_INVOKABLE void requestArtistTracks(const QString& artist);

signals:
    void searchResultsReady(const QVariantList& musicList);
    void historyListReady(const QVariantList& history);
    void favoritesListReady(const QVariantList& favorites);
    void playlistsListReady(const QVariantList& playlists, int page, int pageSize, int total);
    void playlistDetailReady(const QVariantMap& detail);
    void recommendationListReady(const QVariantMap& meta, const QVariantList& items);
    void similarRecommendationListReady(const QVariantMap& meta,
                                        const QVariantList& items,
                                        const QString& anchorSongId);
    void recommendationFeedbackResultReady(bool success,
                                           const QString& eventType,
                                           const QString& songId);
    void lyricsReady(const QStringList& lines);
    void videoListReady(const QVariantList& videoList);
    void videoStreamUrlReady(const QString& videoUrl);
    void artistSearchReady(bool exists, const QString& artist);
    void artistTracksReady(const QVariantList& musicList, const QString& artist);
    void addFavoriteResultReady(bool success);
    void removeFavoriteResultReady(bool success);
    void addHistoryResultReady(bool success);
    void removeHistoryResultReady(bool success);
    void createPlaylistResultReady(bool success, qint64 playlistId, const QString& message);
    void deletePlaylistResultReady(bool success, qint64 playlistId, const QString& message);
    void updatePlaylistResultReady(bool success, qint64 playlistId, const QString& message);
    void addPlaylistItemsResultReady(bool success,
                                     qint64 playlistId,
                                     int addedCount,
                                     int skippedCount,
                                     const QString& message);
    void removePlaylistItemsResultReady(bool success,
                                        qint64 playlistId,
                                        int deletedCount,
                                        const QString& message);
    void reorderPlaylistItemsResultReady(bool success, qint64 playlistId, const QString& message);

private:
    QVariantList convertMusicList(const QList<Music>& musicList, int limit = -1) const;

private:
    MainWidget* m_mainWidget = nullptr;
    MainShellViewModel* m_shellViewModel = nullptr;
};

#endif // CLIENT_AUTOMATION_HOST_SERVICE_H
