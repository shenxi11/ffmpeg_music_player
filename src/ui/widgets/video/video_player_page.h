#ifndef VIDEO_PLAYER_PAGE_H
#define VIDEO_PLAYER_PAGE_H

#include "MediaSession.h"
#include "VideoRendererGL.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;
class QSlider;
class QTimer;

class VideoPlayerPage : public QWidget {
    Q_OBJECT

  public:
    explicit VideoPlayerPage(QWidget* parent = nullptr);
    ~VideoPlayerPage() override;

    void loadVideo(const QString& filePath);
    void pausePlayback();
    bool resumePlayback();
    bool seekToPosition(qint64 positionMs);
    void setVideoInfo(const QString& title, const QString& sourcePath, qint64 sizeBytes);
    bool hasLoadedVideo() const;

  signals:
    void playStateChanged(bool isPlaying);
    void progressChanged(qint64 position);
    void videoLoaded(const QString& filePath);
    void backRequested();

  public slots:
    void onPlayPauseClicked();
    void onOpenFileClicked();
    void onSliderPressed();
    void onSliderReleased();
    void onSliderValueChanged(int value);
    void onDisplayModeClicked();
    void onFullScreenClicked();
    void onPlaybackRateChanged(int index);
    void onQualityPresetChanged(int index);
    void onPositionChanged(qint64 positionMs);
    void onDurationChanged(qint64 durationMs);
    void onPlaybackFinished();
    void onVideoSizeChanged(const QSize& size);
    void onMediaSessionStateChanged(MediaSession::PlaybackState state);
    void onDeferredSeekAfterStopped();

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

  private:
    void setupUi();
    void configureRendererWidget(VideoRendererGL* renderer);
    void connectUiSignals();
    void connectMediaSessionSignals();
    void attachWindowObserver();
    void updateStageLayout();
    void updateStageState();
    void updateInfoPanel();
    void updatePlaybackButtons();
    void updateTimeLabel(qint64 currentMs, qint64 totalMs);
    void applyHostWindowState(bool enabled);
    void syncHostWindowButtonText();
    void scheduleRendererRecovery();
    void recoverRendererAfterWindowModeChange();
    void switchActiveRenderer();
    void moveControlBarToFullscreenHost();
    void restoreControlBarToEmbeddedHost();
    void applyRendererPlaybackState(VideoRendererGL* renderer, MediaSession::PlaybackState state);
    VideoRendererGL* activeRenderer() const;
    void resetPlaybackUi();
    QRect calculateVisibleVideoRect() const;
    bool isCursorInsideStage() const;
    void setStageHovered(bool hovered);
    QString formatTime(qint64 ms) const;
    QString formatSizeBytes(qint64 bytes) const;

    QWidget* m_contentCard = nullptr;
    QWidget* m_playerContainer = nullptr;
    QWidget* m_stageCard = nullptr;
    QWidget* m_stageRenderHost = nullptr;
    QWidget* m_stageChromeHost = nullptr;
    QWidget* m_videoFullscreenHost = nullptr;
    QWidget* m_videoFullscreenStageHost = nullptr;
    QWidget* m_controlBar = nullptr;
    QWidget* m_stageTitlePanel = nullptr;
    QWidget* m_infoPanel = nullptr;
    QWidget* m_placeholderPanel = nullptr;
    QWidget* m_observedWindow = nullptr;

    QLabel* m_placeholderTitleLabel = nullptr;
    QLabel* m_placeholderSubtitleLabel = nullptr;
    QLabel* m_stageTitleLabel = nullptr;
    QLabel* m_stageMetaLabel = nullptr;
    QLabel* m_watermarkLabel = nullptr;
    QLabel* m_subtitleOverlayLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_metaLabel = nullptr;
    QLabel* m_timeLabel = nullptr;

    QPushButton* m_backButton = nullptr;
    QPushButton* m_centerPlayButton = nullptr;
    QPushButton* m_stageNextButton = nullptr;
    QPushButton* m_playPauseButton = nullptr;
    QPushButton* m_displayModeButton = nullptr;
    QPushButton* m_fullScreenButton = nullptr;
    QPushButton* m_openFileButton = nullptr;
    QPushButton* m_nextStubButton = nullptr;
    QPushButton* m_volumeStubButton = nullptr;
    QPushButton* m_playlistStubButton = nullptr;
    QPushButton* m_qualityButton = nullptr;
    QPushButton* m_pipStubButton = nullptr;
    QPushButton* m_favoriteActionButton = nullptr;
    QPushButton* m_downloadActionButton = nullptr;
    QPushButton* m_shareActionButton = nullptr;
    QPushButton* m_commentActionButton = nullptr;
    QPushButton* m_reportActionButton = nullptr;

    QSlider* m_progressSlider = nullptr;
    QComboBox* m_qualityPresetBox = nullptr;
    QComboBox* m_playbackRateBox = nullptr;
    VideoRendererGL* m_renderWidget = nullptr;
    VideoRendererGL* m_fullscreenRenderWidget = nullptr;
    MediaSession* m_mediaSession = nullptr;
    QTimer* m_overlayHideTimer = nullptr;

    bool m_isPlaying = false;
    bool m_sliderPressed = false;
    bool m_stageHovered = false;
    bool m_videoFullscreenActive = false;
    bool m_replayPendingSeek = false;
    bool m_fillDisplayMode = false;
    qint64 m_duration = 0;
    qint64 m_currentPosition = 0;
    qint64 m_pendingStoppedSeekPosition = 0;

    QString m_currentFilePath;
    QString m_displayTitle;
    QString m_sourcePath;
    qint64 m_sourceSizeBytes = 0;
};

#endif // VIDEO_PLAYER_PAGE_H
