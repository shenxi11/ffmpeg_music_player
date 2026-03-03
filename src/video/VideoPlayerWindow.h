#ifndef VIDEOPLAYERWINDOW_H
#define VIDEOPLAYERWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QTime>
#include <QTimer>
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
    void onPlaybackRateChanged(int index);
    void onQualityPresetChanged(int index);
    void onPositionChanged(qint64 positionMs);
    void onDurationChanged(qint64 durationMs);
    void onPlaybackFinished();

private:
    void setupUI();
    void updateTimeLabel(qint64 currentMs, qint64 totalMs);
    QString formatTime(qint64 ms);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    VideoRendererGL* m_renderWidget;
    QPushButton* m_playPauseBtn;
    QPushButton* m_openFileBtn;
    QPushButton* m_displayModeBtn;
    QComboBox* m_qualityPresetBox;
    QComboBox* m_playbackRateBox;
    QSlider* m_progressSlider;
    QLabel* m_timeLabel;
    QLabel* m_fileNameLabel;

    MediaSession* m_mediaSession;

    bool m_isPlaying;
    bool m_sliderPressed;
    QString m_currentFilePath;
    qint64 m_duration;
    qint64 m_currentPosition;
    bool m_replayPendingSeek;
    bool m_fillDisplayMode;
};

#endif // VIDEOPLAYERWINDOW_H
