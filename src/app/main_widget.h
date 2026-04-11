#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H
#include "VideoPlayerWindow.h"
#include "favorite_music_widget.h"
#include "local_and_download_widget.h"
#include "loginwidget_qml.h"
#include "main_menu.h"
#include "music_list_widget.h"
#include "music_list_widget_local.h"
#include "music_list_widget_net.h"
#include "play_history_widget.h"
#include "play_widget.h"
#include "playlist_widget.h"
#include "recommend_music_widget.h"
#include "searchbox_qml.h"
#include "settings_widget.h"
#include "user_profile_widget.h"
#include "user_widget.h"
#include "userwidget_qml.h"
#include "video_list_widget.h"
#include "viewmodels/MainShellViewModel.h"

#include <QButtonGroup>
#include <QGuiApplication>
#include <QIcon>
#include <QLinearGradient>
#include <QList>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QWidget>

class PlaybackStateManager;
class QCloseEvent;
class QTimer;
class QUrl;
class ClientAutomationHostService;
class QNetworkAccessManager;
class QScrollArea;
class QVBoxLayout;
class QLabel;

class MainWidget : public QWidget {
    Q_OBJECT
  public:
    explicit MainWidget(bool localOnlyMode = false, QWidget* parent = nullptr);
    ~MainWidget();
    void updatePaint();

    // 检查登录状态（Q_INVOKABLE 供 QMetaObject::invokeMethod 调用）
    Q_INVOKABLE bool isUserLoggedIn() const {
        return userWidgetQml ? userWidgetQml->getLoginState() : false;
    }
    Q_INVOKABLE QVariantMap agentVideoWindowState() const;
    Q_INVOKABLE bool agentPlayVideo(const QString& source);
    Q_INVOKABLE bool agentPauseVideo();
    Q_INVOKABLE bool agentResumeVideo();
    Q_INVOKABLE bool agentSeekVideo(qint64 positionMs);
    Q_INVOKABLE bool agentSetVideoFullScreen(bool enabled);
    Q_INVOKABLE bool agentSetVideoPlaybackRate(double rate);
    Q_INVOKABLE bool agentSetVideoQualityPreset(const QString& preset);
    Q_INVOKABLE bool agentCloseVideoWindow();
    Q_INVOKABLE QVariantMap agentDesktopLyricsState() const;
    Q_INVOKABLE bool agentSetDesktopLyricsVisible(bool visible);
    Q_INVOKABLE bool agentSetDesktopLyricsStyle(const QVariantMap& style);
    Q_INVOKABLE bool agentOfflineMode() const;
    Q_INVOKABLE QString agentCurrentPageKey() const;
    Q_INVOKABLE QString agentCurrentMusicTabKey() const;
    Q_INVOKABLE QString agentLocalDownloadSubTabKey() const;
    Q_INVOKABLE QString agentSidebarPlaylistTabKey() const;
    Q_INVOKABLE QVariantMap agentSelectedPlaylistSnapshot() const;
    Q_INVOKABLE QVariantList agentSelectedTrackIdsSnapshot() const;
    Q_INVOKABLE QVariantMap agentCurrentTrackSnapshot() const;
    Q_INVOKABLE QVariantMap agentQueueSnapshot() const;
    Q_INVOKABLE QVariantMap agentUiOverviewSnapshot() const;
    Q_INVOKABLE QVariantMap agentUiPageStateSnapshot(const QString& pageKey) const;
    Q_INVOKABLE QVariantList agentMusicTabItemsSnapshot(const QString& tabKey,
                                                        const QVariantMap& options = {}) const;
    Q_INVOKABLE QVariantMap agentResolveMusicTabItem(const QString& tabKey,
                                                     const QVariantMap& selector) const;
    Q_INVOKABLE QVariantMap agentInvokeSongAction(const QString& action,
                                                  const QVariantMap& songData);
    Q_INVOKABLE QVariantMap agentUserProfileSnapshot() const;
    Q_INVOKABLE bool agentRefreshUserProfile();
    Q_INVOKABLE bool agentUpdateUsername(const QString& username);
    Q_INVOKABLE bool agentUploadAvatar(const QString& filePath);
    Q_INVOKABLE bool agentLogoutUser();
    Q_INVOKABLE bool agentReturnToWelcome();

    // 显示登录窗口
    void showLoginWindow() {
        if (m_localOnlyMode) {
            showLocalOnlyUnavailableMessage();
            return;
        }
        if (loginWidget) {
            loginWidget->isVisible = true;
            loginWidget->show();
        }
    }

  signals:
    void loginRequired(); // 需要登录时发出的信号
    void requestReturnToWelcome();

  private:
    PlayWidget* w;
    MusicListWidget* list;
    MusicListWidgetLocal* main_list;
    LocalAndDownloadWidget* localAndDownloadWidget; // 本地与下载页面组件
    MusicListWidgetNet* net_list;
    PlayHistoryWidget* playHistoryWidget;           // 最近播放页面组件
    FavoriteMusicWidget* favoriteMusicWidget;       // 喜欢音乐页面组件
    RecommendMusicWidget* recommendMusicWidget;     // 推荐音乐页面组件
    PlaylistWidget* playlistWidget;                 // 我的歌单页面组件
    UserProfileWidget* userProfileWidget = nullptr; // 个人主页页面组件
    LoginWidgetQml* loginWidget;
    UserWidget* userWidget;
    UserWidgetQml* userWidgetQml;
    SearchBoxQml* searchBox = nullptr;
    QWidget* topWidget;
    QWidget* leftWidget = nullptr;
    QWidget* brandWidget = nullptr;
    MainMenu* mainMenu;
    QPushButton* menuButton;
    QPushButton* recommendButton = nullptr;
    QPushButton* localButton = nullptr;
    QPushButton* netButton = nullptr;
    QPushButton* historyButton = nullptr;
    QPushButton* favoriteButton = nullptr;
    QPushButton* playlistButton = nullptr;
    QPushButton* videoButton = nullptr;
    QWidget* sidebarPlaylistSection = nullptr;
    QPushButton* sidebarOwnedTabButton = nullptr;
    QPushButton* sidebarSubscribedTabButton = nullptr;
    QPushButton* sidebarPlaylistAddButton = nullptr;
    QScrollArea* sidebarPlaylistScrollArea = nullptr;
    QWidget* sidebarPlaylistListContainer = nullptr;
    QVBoxLayout* sidebarPlaylistListLayout = nullptr;
    QLabel* sidebarPlaylistEmptyLabel = nullptr;
    QPushButton* aiAssistantTopButton = nullptr;
    VideoPlayerWindow* videoPlayerWindow;
    VideoListWidget* videoListWidget; // 在线视频列表窗口
    SettingsWidget* settingsWidget;   // 设置窗口

    QPushButton* Login;
    MainShellViewModel* m_viewModel = nullptr;
    ClientAutomationHostService* m_clientAutomationHostService = nullptr;

    // 在线音乐元数据缓存（用于追加最近播放记录）
    QString m_networkMusicArtist;
    QString m_networkMusicCover;
    PlaybackStateManager* m_playbackStateManager = nullptr;
    QTimer* m_pluginErrorDialogTimer = nullptr;
    QStringList m_pendingPluginErrors;
    QVariantMap m_pendingSimilarSongItem;
    QString m_pendingHistorySessionId;
    QString m_pendingHistoryFilePath;
    QString m_pendingHistoryUserAccount;
    int m_pendingHistoryRetryCount = -1;
    QVariantList m_ownedSidebarPlaylists;
    QVariantList m_subscribedSidebarPlaylists;
    QList<QPushButton*> m_sidebarPlaylistButtons;
    qint64 m_sidebarSelectedPlaylistId = -1;
    bool m_sidebarShowingSubscribedPlaylists = false;
    bool m_returningToWelcome = false;
    QVariantMap m_pendingAddToNewPlaylistSong;
    int m_profileFavoritesCount = 0;
    int m_profileHistoryCount = 0;
    int m_profileOwnedPlaylistsCount = 0;
    QVariantList m_profileFavoritesPreview;
    QVariantList m_profileHistoryPreview;
    QVariantList m_profileOwnedPlaylistsPreview;
    QHash<qint64, QString> m_playlistDerivedCoverUrls;
    QSet<qint64> m_playlistCoverPrefetchInFlight;
    QSet<qint64> m_playlistCoverPrefetchResolved;
    QNetworkAccessManager* m_sidebarCoverNetworkManager = nullptr;
    QHash<QString, QIcon> m_sidebarCoverIconCache;
    QSet<QString> m_sidebarCoverRequestsInFlight;
    bool m_localOnlyMode = false;

    QPoint pos_ = QPoint(0, 0);
    bool dragging = false;

  protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // QQ 音乐风格：浅灰白色渐变背景
        QLinearGradient gradient(0, 0, 0, height());
        gradient.setColorAt(0, QColor("#F5F5F7"));   // 浅灰白色
        gradient.setColorAt(0.5, QColor("#FAFAFA")); // 接近纯白
        gradient.setColorAt(1, QColor("#F0F0F2"));   // 浅灰色
        painter.fillRect(rect(), gradient);
    }

    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

  private:
    // 菜单与账号相关信号连接拆分到独立实现文件，避免构造函数臃肿。
    void setupMenuAndAccountConnections();
    // 播放控制、列表联动与播放状态相关连接拆分到独立实现文件。
    void setupPlaybackAndListConnections();
    // 收藏、历史、登录态联动连接拆分到独立实现文件。
    void setupLibraryConnections();

    void submitPlayHistoryWithRetry(const QString& sessionId, const QString& filePath,
                                    const QString& userAccount, int retryCount);
    static QString normalizeArtistForHistory(const QString& artist);
    static QString extractSongIdFromMediaPath(const QString& rawPath);
    QRect computeContentRect() const;
    void updateAdaptiveLayout();
    void updateSideNavLayout();
    void updateSidebarPlaylistTabs();
    void rebuildSidebarPlaylistButtons();
    void clearSidebarPlaylistButtons();
    void syncSidebarPlaylistSelection(qint64 playlistId);
    QIcon sidebarPlaylistCoverIcon(const QString& coverUrl);
    QString normalizePlaylistCoverUrl(const QString& coverUrl) const;
    void applySidebarPlaylistButtonIcon(QPushButton* button, const QString& coverUrl);
    void requestSidebarPlaylistCoverIcon(const QString& coverUrl);
    void refreshSidebarPlaylistCoverIcon(qint64 playlistId, const QString& coverUrl);
    QString lookupPlaylistCoverCache(const QString& account, qint64 playlistId,
                                     const QString& updatedAt, bool* found) const;
    void storePlaylistCoverCache(qint64 playlistId, const QString& updatedAt,
                                 const QString& coverUrl);
    void clearPlaylistCoverCacheForPlaylist(qint64 playlistId);
    QVariantMap cachedUserProfileSnapshot() const;
    void refreshUserProfileStats();
    void syncUserProfileStatsToPage();
    void syncUserProfilePreviewToPage();
    void applyUserIdentityToUi(const QString& username, const QString& avatarUrl, bool loggedIn,
                               bool forceAvatarRefresh = false);
    bool isLocalOnlyMode() const { return m_localOnlyMode; }
    void applyLocalOnlyModeUi();
    void showLocalOnlyUnavailableMessage();
    void updateSearchBoxForMode();
    void updateAiAssistantButtonState();
    void enqueuePluginLoadError(const QString& pluginFilePath, const QString& reason);
    void showPluginDiagnosticsDialog();

    // 播放连接层命名处理函数，避免大量 lambda 堆叠影响可读性。
    void handleNetLoginRequired();
    void handlePlayWidgetLast(const QString& songName, bool netFlag);
    void handlePlayWidgetNext(const QString& songName, bool netFlag);
    void handlePlayWidgetNetFlagChanged(bool netFlag);
    void handlePlayWidgetAddSongToCache(const QString& fileName, const QString& path);
    void handlePlayWidgetButtonState(bool playing, const QString& filename);
    void handlePlayWidgetMetadataUpdated(const QString& filePath, const QString& coverUrl,
                                         const QString& duration, const QString& artist);
    void handleLocalListPlayClick(const QString& name, bool flag);
    void handleLocalAndDownloadPlayMusic(const QString& filename);
    void handleLocalAndDownloadDeleteMusic(const QString& filename);
    void handleNetListPlayClick(const QString& name, const QString& artist, const QString& cover,
                                bool flag);
    void handleHistoryPlayMusic(const QString& filePath);
    void handleHistoryPlayMusicWithMetadata(const QString& filePath, const QString& title,
                                            const QString& artist, const QString& cover);
    void handleAudioPlaybackStarted(const QString& sessionId, const QUrl& url);
    void handleAudioPlaybackPaused();
    void handleAudioPlaybackResumed();
    void handleAudioPlaybackStopped();
    void handlePlayWidgetBigClicked(bool checked);

    void handleMenuButtonClicked();
    void ensureMainMenuCreated();
    void handleMainMenuPluginRequested(const QString& pluginId);
    void handleMainMenuSettingsRequested();
    void handleMainMenuPluginDiagnosticsRequested();
    void handleMainMenuAboutRequested();
    void handleUserLoginRequested();
    void handleUserProfileRequested();
    void handleUserLogoutRequested();
    void handleSettingsReturnToWelcomeRequested();
    void handleSessionExpired();
    void handleLoginWidgetSuccess(const QString& username, const QString& avatarUrl,
                                  const QString& onlineSessionToken);
    void triggerAutoLoginIfNeeded();
    void handleHistoryDeleteRequested(const QStringList& paths);
    void handleRemoveHistoryResult(bool success);
    void handleHistoryAddToFavorite(const QString& path, const QString& title,
                                    const QString& artist, const QString& duration, bool isLocal);
    void handleHistoryLoginRequested();
    void handleHistoryRefreshRequested();
    void handleFavoritePlayMusic(const QString& filePath);
    void handleFavoriteRemoveRequested(const QStringList& paths);
    void handleFavoriteRefreshRequested();
    void handleFavoritesListUpdated(const QVariantList& favorites);
    void handleRemoveFavoriteResult(bool success);
    void handleLocalAddToFavorite(const QString& path, const QString& title, const QString& artist,
                                  const QString& duration);
    void handleNetAddToFavorite(const QString& path, const QString& title, const QString& artist,
                                const QString& duration);
    void handleAddFavoriteResult(bool success);
    void handleUserLoginStateChanged(bool loggedIn);
    void handleUserProfileRefreshRequested();
    void handleUserProfileUsernameSaveRequested(const QString& username);
    void handleUserProfileAvatarFileSelected(const QString& filePath);
    void handleUserProfileFavoritesShortcutRequested();
    void handleUserProfileHistoryShortcutRequested();
    void handleUserProfilePlaylistsShortcutRequested();
    void handleUserProfileReloginRequested();
    void handleUserProfileReady(const QVariantMap& profile);
    void handleUserProfileRequestFailed(const QString& message, int statusCode);
    void handleUpdateUsernameResultReady(bool success, const QString& username,
                                         const QString& message, int statusCode);
    void handleUploadAvatarResultReady(bool success, const QString& avatarUrl,
                                       const QString& message, int statusCode);
    void handlePlaylistLoginRequested();
    void handlePlaylistRefreshRequested();
    void handlePlaylistOpenRequested(qint64 playlistId);
    void handleSidebarOwnedTabClicked();
    void handleSidebarSubscribedTabClicked();
    void handleSidebarCreatePlaylistClicked();
    void handleSidebarPlaylistItemClicked();
    void handlePlaylistCreateRequested(const QString& name, const QString& description);
    void handlePlaylistUpdateRequested(qint64 playlistId, const QString& name,
                                       const QString& description);
    void handlePlaylistDeleteRequested(qint64 playlistId);
    void handlePlaylistPlayMusicWithMetadata(const QString& filePath, const QString& title,
                                             const QString& artist, const QString& cover);
    void handlePlaylistRemoveSongsRequested(qint64 playlistId, const QStringList& musicPaths);
    void handlePlaylistReorderSongsRequested(qint64 playlistId, const QVariantList& orderedItems);
    void handlePlaylistAddCurrentSongRequested(qint64 playlistId);
    void handlePlaylistsListReady(const QVariantList& playlists, int page, int pageSize, int total);
    void handlePlaylistDetailReady(const QVariantMap& detail);
    void handlePlaylistCoverDetailReady(const QVariantMap& detail);
    void handleCreatePlaylistResultReady(bool success, qint64 playlistId, const QString& message);
    void handleUpdatePlaylistResultReady(bool success, qint64 playlistId, const QString& message);
    void handleDeletePlaylistResultReady(bool success, qint64 playlistId, const QString& message);
    void handleAddPlaylistItemsResultReady(bool success, qint64 playlistId, int addedCount,
                                           int skippedCount, const QString& message);
    void handleRemovePlaylistItemsResultReady(bool success, qint64 playlistId, int deletedCount,
                                              const QString& message);
    void handleReorderPlaylistItemsResultReady(bool success, qint64 playlistId,
                                               const QString& message);
    void handleSongActionRequested(const QString& action, const QVariantMap& songData);
    void prefetchPlaylistCoverDetails(const QVariantList& playlists);
    void applyPlaylistCoverToUiCaches(qint64 playlistId, const QString& coverUrl,
                                      const QString& updatedAt = QString());
    void queueSongAsNext(const QVariantMap& songData);
    void appendSongToPlaybackQueue(const QVariantMap& songData);
    void addSongToPlaylistByAction(const QVariantMap& songData);
    void createPlaylistAndAddSong(const QVariantMap& songData);
    void toggleFavoriteByAction(const QString& action, const QVariantMap& songData);
    void removeOrDeleteSongByAction(const QVariantMap& songData);
    void playSongByAction(const QVariantMap& songData);
    void rememberPlaybackQueueMetadata(const QString& filePath, const QString& title,
                                       const QString& artist, const QString& cover);

    // 主窗口首页联动处理（替代构造函数内大段 lambda）。
    void handlePlaybackPauseAudioRequested();
    void handlePlaybackPauseVideoRequested();
    void handlePluginErrorDialogTimeout();
    void handleWindowToggleRequested();
    void showContentPanel(QWidget* activeWidget);
    void handleRecommendTabToggled(bool checked);
    void handleLocalTabToggled(bool checked);
    void handleNetTabToggled(bool checked);
    void handleHistoryTabToggled(bool checked);
    void handleFavoriteTabToggled(bool checked);
    void handlePlaylistTabToggled(bool checked);
    void handleVideoTabToggled(bool checked);
    void handleAiAssistantClicked();
    void handlePlayStateChanged(ProcessSliderQml::State state);
    void handleVideoPlayerWindowReady(VideoPlayerWindow* window);
    void handleVideoPlaybackStateChanged(bool isPlaying);
    void handleSearchRequested(const QString& keyword);
    void handleSearchResultsReady();
    void handleSimilarRecommendationListReady(const QVariantMap& meta, const QVariantList& items,
                                              const QString& anchorSongId);
    void handleRecommendRefreshRequested();
    void handleRecommendLoginRequested();
    void handleRecommendPlayMusicWithMetadata(const QString& filePath, const QString& title,
                                              const QString& artist, const QString& cover,
                                              const QString& duration, const QString& songId,
                                              const QString& requestId, const QString& modelVersion,
                                              const QString& scene);
    void handleRecommendAddToFavorite(const QString& path, const QString& title,
                                      const QString& artist, const QString& duration, bool isLocal);
    void handleRecommendFeedbackEvent(const QString& songId, const QString& eventType, int playMs,
                                      int durationMs, const QString& scene,
                                      const QString& requestId, const QString& modelVersion);
    void handleSimilarSongSelected(const QVariantMap& item);
    void handlePendingSimilarSongPlayback();
    void schedulePlayHistoryRetry(const QString& sessionId, const QString& filePath,
                                  const QString& userAccount, int retryCount);
    void handlePlayHistoryRetryTimeout();
    int placeSideNavButton(int row, QPushButton* button, int navStartY, int itemHeight,
                           int panelWidth);
    QVariantList agentOwnedPlaylistEntries() const;
    QVariantList agentSubscribedPlaylistEntries() const;
};

#endif // TEST_WIDGET_H
