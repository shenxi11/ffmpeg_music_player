#ifndef PLAY_WIDGET_H
#define PLAY_WIDGET_H

#include <QWidget>
#include <QSlider>
#include <QScrollBar>
#include <QTextBlock>
#include <QVariantMap>
#include <QResizeEvent>
#include "headers.h"
// 音频播放核心服务（会话、解码、渲染由该服务统一调度）。
#include "AudioService.h"
#include "viewmodels/PlaybackViewModel.h"  // 播放页面 ViewModel（UI 层入口）
#include "lrc_analyze.h"
#include "lyricdisplay_qml.h"
#include "rotatingcircleimage.h"
#include "process_slider_qml.h"
#include "controlbar_qml.h"
#include "desklrc_qml.h"
#include "playlist_history_qml.h"
#include "comment_panel_qml.h"

class QQuickWidget;
class QGraphicsDropShadowEffect;
class MainShellViewModel;

class PlayWidget : public QWidget
{
    Q_OBJECT
    
    // MVVM 架构说明
    Q_PROPERTY(PlaybackViewModel* playbackViewModel READ playbackViewModel CONSTANT)

public:
    enum class RightOverlayPage {
        None = 0,
        Playlist,
        Comments
    };

    PlayWidget(QWidget *parent = nullptr, QWidget *mainWidget = nullptr);
    ~PlayWidget();

    // 获取 ViewModel 指针，供 QML 绑定使用
    PlaybackViewModel* playbackViewModel() const;

    bool isUp = false;
    bool getNetFlag();
    void setIsUp(bool flag);
    bool checkAndWarnIfPathNotExists(const QString &path);
    int collapsedPlaybackHeight() const;
    QVariantMap desktopLyricSnapshot() const;
    bool setDesktopLyricVisible(bool visible);
    bool setDesktopLyricStyleFromMap(const QVariantMap& style);
    void setMainShellViewModel(MainShellViewModel* viewModel);
    void setEmbeddedCommentPanel(CommentPanelQml* panel);
    void setCommentTrackContext(const QVariantMap& context);
    void clearCommentTrackContext();
    void setMainContentCommentVisible(bool visible);
    bool isMainContentCommentVisible() const;
    void syncCommentOverlayGeometry();
    void closePlaylistHistoryPanel();
    void closeCommentDrawerPanel();
public slots:

    void beginTakeLrc(QString str);
    void playClick(QString songName);
    void removeClick(QString songName);
    void openfile();
    void setPianWidgetEnable(bool flag);

    void setPlayNet(bool flag);
    void setNetworkMetadata(const QString& artist, const QString& cover);
    void setNetworkMetadata(const QString& title, const QString& artist, const QString& cover);
    void onPlayClick();
    void onLrcSendLrc(const std::map<int, std::string> lyrics);
    void onWorkStop();
    void onWorkPlay();
    void onDeskToggled(bool checked);
    void onUpdateBackground(QString picPath);  // 根据封面更新播放页背景
    void onLyricSeek(int timeMs);  // 歌词点击跳转到指定时间
    void onLyricPreview(int timeMs);  // 歌词拖动时预览目标时间
    void onLyricDragStart();  // 歌词开始拖动，进入预览态
    void onLyricDragEnd();
    void setSimilarRecommendations(const QVariantList& items);
    void clearSimilarRecommendations();
signals:
    void signalWorkerPlay();
    void signalFilepath(QString filePath);
    void signalBeginToPlay(QString path);
    void signalBeginTakeLrc(QString str);
    void signalPlayChanged(bool flag);
    void signalSetSliderMove(bool flag);
    void signalProcessChange(qint64 newPosition, bool back_flag);
    void signalBigClicked(bool checked);
    void signalListShow(bool flag);
    void signalAddSong(const QString fileName,const QString path);
    void signalPlayButtonClick(bool flag, QString fileName);
    void signalNext(QString songName, bool net_flag);
    void signalLast(QString songName, bool net_flag);
    void signalRemoveClick();
    void signalStopRotate(bool flag);
    void signalBeginNetDecode();
    void signalPlayState(ProcessSliderQml::State state);
    void signalIsUpChanged(bool flag);
    void signalDeskLrc(const QString lrc_);
    void signalNetFlagChanged(bool net_flag);
    void signalMetadataUpdated(QString filePath, QString coverUrl, QString duration,
                               QString artist);
    void signalSimilarSongSelected(const QVariantMap& item);
    void signalCommentLoginRequired();
    void signalMainCommentPageRequested(bool show);
private:
    void setupCoreConnections();
    // 播放页 ViewModel 连接在独立实现文件中维护，降低构造函数复杂度。
    void setupPlaybackViewModelConnections();
    // 播放控制、桌面歌词与播放队列连接在独立实现文件中维护。
    void setupControlAndPlaylistConnections();
    void handleDeskForwardClicked();
    void handleDeskBackwardClicked();
    void handleDeskCloseClicked();
    void handleDeskPlayStateChanged(ProcessSliderQml::State state);
    void handleDeskSettingsClicked();
    void handleVolumeChanged(int volume);
    void handleViewModelVolumeChanged();
    void handleNextSongRequested();
    void handleLastSongRequested();
    void handleStopRequested();
    void handleReplayRequested();
    void handleLoopChanged(bool isLoop);
    void handlePlaylistToggled(bool show);
    void handlePlaylistPlayRequested(const QString& filePath);
    void handlePlaylistRemoveRequested(const QString& filePath);
    void handlePlaylistClearAllRequested();
    void handlePlaylistPauseToggled();
    void handlePlayModeChanged(int mode);
    void handleCommentToggled(bool show);
    void handleCommentPanelCloseRequested();
    void handleCommentPanelLoadMoreRequested();
    void handleCommentPanelToggleRepliesRequested(qint64 rootCommentId, bool expanded);
    void handleCommentPanelLoadMoreRepliesRequested(qint64 rootCommentId);
    void handleCommentPanelSubmitMainRequested(const QString& content);
    void handleCommentPanelSubmitReplyRequested(qint64 rootCommentId, qint64 targetCommentId,
                                                const QString& content);
    void handleCommentPanelDeleteRequested(qint64 commentId);
    void handleCommentPanelStartReplyRequested(qint64 rootCommentId, qint64 targetCommentId,
                                               const QString& username);
    void handleCommentPanelLoginRequested();
    void handleSliderPressed();
    void handleSliderReleased();
    void handleVmIsPlayingChanged();
    void handleVmPositionChanged();
    void handleVmDurationChanged();
    void handleVmCurrentTitleChanged();
    void handleVmCurrentArtistChanged();
    void handleVmCurrentAlbumArtChanged();
    void handleVmPlaybackStarted();
    void handleVmPlaybackStopped();
    void handleVmShouldStartRotation();
    void handleVmShouldStopRotation();
    void handleVmShouldLoadLyrics(const QString& songPath);
    void refreshPlaylistHistoryFromViewModel();
    void handleMusicButtonClicked();
    void handleBufferingStateChanged(bool active);
    void handleProcessChangeRequested(qint64 milliseconds, bool back_flag);
    void handleSliderMoveRequested(int seconds);
    void handleDeferredSeekAfterPlay();
    void handleCoverExpandRequested();
    void handleLyricPositionChanged();
    int resolveTargetLyricLine(qint64 positionMs) const;
    void refreshCurrentLyricHighlight(bool forceRecenter);
    void handleSimilarPlayRequested(const QVariantMap& item);
    void handlePlayerPageStyleRequested(int styleId);
    void queuePlayButtonStateUpdate(bool playing, const QString& path);
    void handleDeferredPlayButtonStateUpdate();
    void handlePlayerPageStyleChanged();
    void shutdownQuickWidget(QQuickWidget* widget);
    void applyPlayerPageStyle();
    void updateCoverPresentation(const QString& imagePath);
    QPixmap defaultCoverPixmapForSize(const QSize& requestedSize) const;
    void refreshStageTexts();
    QString displayArtistText() const;
    void invalidateStageBackgroundCache();
    void rebuildStageBackgroundCache();
    void emitLocalMetadataUpdateIfNeeded();
    void refreshCommentAvailability();
    QString resolveCommentMusicPath() const;
    QVariantMap resolvedCommentTrackContext() const;
    bool isCommentableMusicPath(const QString& musicPath) const;
    void connectCommentPanel(CommentPanelQml* panel);
    void syncCommentToggleState();
    void syncPlaylistToggleState();
    void showRightOverlayPage(RightOverlayPage page);
    void closeRightOverlay();
    void updateRightOverlayChildLayout();
    void syncCommentPanelTrackContext();
    void syncCommentPanelAuthState();
    void syncCommentPanelComments();
    void syncCommentPanelReplies();
    void loadMusicComments(int page = 1);
    void loadCommentReplies(qint64 rootCommentId, int page = 1);
    void closeCommentPanel();
    void connectCommentSignals();
    void handleMusicCommentsReady(const QVariantMap& threadMeta, const QVariantList& items,
                                  const QString& musicPath, int page, int pageSize);
    void handleMusicCommentsRequestFailed(const QString& message, int statusCode,
                                          const QString& musicPath, int page, int pageSize);
    void handleMusicCommentRepliesReady(qint64 rootCommentId, const QVariantList& items, int total,
                                        int page, int pageSize);
    void handleMusicCommentRepliesRequestFailed(qint64 rootCommentId, const QString& message,
                                                int statusCode, int page, int pageSize);
    void handleCreateMusicCommentResultReady(bool success, const QVariantMap& comment,
                                             const QString& message, int statusCode,
                                             const QString& musicPath);
    void handleCreateMusicCommentReplyResultReady(bool success, qint64 rootCommentId,
                                                  const QVariantMap& comment,
                                                  const QString& message, int statusCode,
                                                  qint64 targetCommentId);
    void handleDeleteMusicCommentResultReady(bool success, qint64 commentId,
                                             const QString& message, int statusCode);

    void updateAdaptiveLayout();

    void initLyricDisplay();
    void rePlay(QString path);

    // 播放页内部遵循“会话驱动 + UI被动同步”模式：
    // 1) AudioSession 提供时间轴与状态；2) PlayWidget 只负责展示与交互回传。
    // 这样可以减少播放状态在多列表间分叉。
    
    // MVVM 架构说明
    PlaybackViewModel* m_playbackViewModel;  // 播放页面 ViewModel（UI 层入口）
    
    // 当前音频会话句柄，切歌时会替换并重新绑定信号。
    AudioSession* currentSession;  // 当前播放会话（用于歌词与进度同步）
    
    std::shared_ptr<LrcAnalyze> lrc;// 歌词解析模块
    std::map<int, std::string> lyrics;

    QString filePath;
    QString fileName;
    QString currentSongTitle;   // 当前显示的歌曲标题
    QString currentSongArtist;  // 当前显示的歌手名
    QString networkSongArtist;  // 在线歌曲元数据中的歌手名
    QString networkSongCover;   // 在线歌曲元数据中的封面地址
    
    LyricDisplayQml *lyricDisplay;
    QPushButton *music;
    QPushButton* net;
    qint64 duration = 0;  // 当前曲目时长（微秒）
    std::mutex mtx;
    QLabel* nameLabel;
    QLabel* artistInfoLabel = nullptr;
    QLabel* sceneLabel = nullptr;
    QLabel* backgroundLabel;  // 背景图层（用于封面模糊或默认渐变）
    RotatingCircleImage* rotatingCircle;  // 唱片旋转控件
    QWidget* rotatingCircleHost = nullptr;
    QWidget* squareCoverHost = nullptr;
    QLabel* squareCoverLabel = nullptr;
    QGraphicsDropShadowEffect* rotatingCircleShadow = nullptr;
    QGraphicsDropShadowEffect* squareCoverShadow = nullptr;
    QPixmap m_originalBackgroundPixmap;
    QPixmap m_stageBackgroundCache;
    bool m_stageBackgroundCacheDirty = true;
    
    // 歌词滚动信号连接，切歌时需断开旧连接防止重复触发。
    QMetaObject::Connection lyricUpdateConnection;
    // 历史线程字段已废弃，保留注释以便追踪旧实现。
    //QThread *a;
    //QThread *b;
    //QThread *c;
    // 歌词解析任务使用独立线程，避免阻塞 UI 渲染。
    
    // 解析线程与播放进度连接句柄。
    QThread *b;  // 歌词解析线程
    QMetaObject::Connection positionUpdateConnection;  // 播放位置更新连接
    qint64 lastSeekPosition;  // 最近一次 seek 的目标位置（微秒）
    qint64 pendingSeekPositionMs = -1;  // 延迟 seek 的目标毫秒值（等待恢复播放后执行）
    bool m_pendingPlayButtonStateValid = false;
    bool m_pendingPlayButtonPlaying = false;
    QString m_pendingPlayButtonPath;

    ProcessSliderQml* process_slider;
    ProcessSliderQml* controlBar;  // controlBar 当前复用 process_slider 组件
    DeskLrcQml* desk;
    QWidget* m_rightOverlayHost = nullptr;
    PlaylistHistoryQml* playlistHistory;  // 播放历史列表组件
    CommentPanelQml* commentPanel = nullptr;
    CommentPanelQml* m_embeddedCommentPanel = nullptr;
    MainShellViewModel* m_shellViewModel = nullptr;
    QVariantMap m_commentTrackContext;
    QVariantMap m_commentThreadMeta;
    QVariantList m_commentItems;
    QVariantList m_commentReplyItems;
    QString m_commentErrorMessage;
    QString m_commentReplyErrorMessage;
    int m_commentPage = 1;
    int m_commentPageSize = 20;
    bool m_commentHasMore = false;
    bool m_commentLoading = false;
    qint64 m_expandedCommentId = 0;
    int m_replyPage = 1;
    int m_replyPageSize = 50;
    int m_replyTotal = 0;
    bool m_replyHasMore = false;
    bool m_replyLoading = false;
    qint64 m_replyTargetRootCommentId = 0;
    qint64 m_replyTargetCommentId = 0;
    QString m_replyTargetUsername;
    bool m_commentSubmitting = false;
    bool m_commentSignalsConnected = false;
    bool m_mainContentCommentVisible = false;
    RightOverlayPage m_rightOverlayPage = RightOverlayPage::None;

    bool play_net = false;
    int m_playerPageStyle = 0;
protected:
    void resizeEvent(QResizeEvent *event) override;

    void paintEvent(QPaintEvent *event) override {
        QWidget::paintEvent(event);

        if (m_stageBackgroundCacheDirty || m_stageBackgroundCache.size() != size()) {
            rebuildStageBackgroundCache();
        }

        QPainter painter(this);
        if (!m_stageBackgroundCache.isNull()) {
            painter.drawPixmap(rect(), m_stageBackgroundCache);
        }
    }
};
#endif // MAINWINDOW_H

