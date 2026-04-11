#ifndef VIDEOPLAYERWINDOW_H
#define VIDEOPLAYERWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QTime>
#include <QTimer>
#include <QKeyEvent>
#include <QSize>
#include <QEvent>
#include <QRect>
#include <QScreen>
#include "MediaService.h"
#include "MediaSession.h"
#include "VideoRendererGL.h"

class VideoRenderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoRenderWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_placeholderText;
};

class VideoPlayerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit VideoPlayerWindow(QWidget *parent = nullptr);
    ~VideoPlayerWindow();

    void loadVideo(const QString& filePath);
    void pausePlayback();
    bool resumePlayback();
    bool seekToPosition(qint64 positionMs);
    bool setFullScreenEnabled(bool enabled);
    bool setPlaybackRateValue(double rate);
    bool setQualityPresetValue(const QString& preset);
    QVariantMap snapshot() const;

signals:
    void playStateChanged(bool isPlaying);
    void progressChanged(qint64 position);
    void videoLoaded(const QString& filePath);

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

private:
    void setupUI();
    void connectUiSignals(QPushButton* closeBtn);
    void connectMediaSessionSignals();
    void updateTimeLabel(qint64 currentMs, qint64 totalMs);
    void updateMetaInfo();
    void updateButtonStates();
    void updateResponsiveUi();
    void requestFullScreenChange(bool enabled, const char* source);
    void applyImmersiveMaximize();
    void restoreFromImmersiveMaximize();
    void scheduleFullScreenTransitionSettle();
    void finalizeFullScreenTransition();
    void performCloseCleanup();
    bool isImmersiveMaximizeActive() const;
    QString formatTime(qint64 ms);

protected:
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    VideoRendererGL* m_renderWidget;
    QPushButton* m_playPauseBtn;
    QPushButton* m_stopBtn;
    QPushButton* m_openFileBtn;
    QPushButton* m_displayModeBtn;
    QPushButton* m_fullScreenBtn;
    QComboBox* m_qualityPresetBox;
    QComboBox* m_playbackRateBox;
    QSlider* m_progressSlider;
    QLabel* m_timeLabel;
    QLabel* m_fileNameLabel;
    QLabel* m_metaInfoLabel;
    QLabel* m_qualityLabel;
    QLabel* m_rateLabel;
    QLabel* m_qualityIconLabel;
    QLabel* m_rateIconLabel;
    QWidget* m_titleBar;
    QWidget* m_controlBar;
    QWidget* m_trailingControls;

    MediaSession* m_mediaSession;

    bool m_isPlaying;
    bool m_sliderPressed;
    QString m_currentFilePath;
    qint64 m_duration;
    qint64 m_currentPosition;
    bool m_replayPendingSeek;
    bool m_fillDisplayMode;
    qint64 m_pendingStoppedSeekPosition;
    QSize m_videoFrameSize;
    QTimer* m_fullScreenTransitionTimer;
    bool m_fullScreenTransitionInProgress;
    bool m_targetFullScreenState;
    bool m_immersiveMaximizeActive;
    bool m_closePending;
    bool m_cleanupDone;
    QRect m_savedWindowGeometry;
    bool m_savedWasMaximized;
};

#endif // VIDEOPLAYERWINDOW_H
