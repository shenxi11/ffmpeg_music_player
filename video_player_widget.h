#ifndef VIDEO_PLAYER_WIDGET_H
#define VIDEO_PLAYER_WIDGET_H

#include <QWidget>
#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlContext>
#include "video_worker.h"

class VideoPlayerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoPlayerWidget(QWidget *parent = nullptr);
    ~VideoPlayerWidget();

public slots:
    void on_signal_open_video(const QString& filePath);
    void on_signal_open_video_dialog();
    void on_signal_play_pause();
    void on_signal_stop();
    void on_signal_seek(qint64 positionMs);

signals:
    void signal_video_opened(QString filePath);
    void signal_play_state_changed(bool isPlaying);
    void signal_video_finished();

private slots:
    void onVideoOpened(int width, int height, double fps, qint64 durationMs);
    void onVideoFrameReady(QImage frame, qint64 ptsMs);
    void onDecodingFinished();
    void onErrorOccurred(QString error);

private:
    QQuickWidget* qmlWidget;
    VideoWorker* videoWorker;
    bool isPlaying;
};

#endif // VIDEO_PLAYER_WIDGET_H
