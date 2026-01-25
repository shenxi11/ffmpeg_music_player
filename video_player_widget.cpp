#include "video_player_widget.h"
#include "video_frame_item.h"
#include <QVBoxLayout>
#include <QFileDialog>
#include <QQmlContext>
#include <QDebug>

VideoPlayerWidget::VideoPlayerWidget(QWidget *parent)
    : QWidget(parent)
    , qmlWidget(nullptr)
    , videoWorker(nullptr)
    , isPlaying(false)
{
    qDebug() << "VideoPlayerWidget created";
    
    // 注册VideoFrameItem类型到QML
    qmlRegisterType<VideoFrameItem>("VideoPlayer", 1, 0, "VideoFrameItem");
    
    // 创建 QML 界面
    qmlWidget = new QQuickWidget(this);
    qmlWidget->setSource(QUrl("qrc:/qml/components/VideoPlayerWidget.qml"));
    qmlWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(qmlWidget);
    setLayout(layout);
    
    // 创建视频worker
    videoWorker = new VideoWorker(this);
    
    // 连接worker信号
    connect(videoWorker, &VideoWorker::videoOpened,
            this, &VideoPlayerWidget::onVideoOpened);
    connect(videoWorker, &VideoWorker::videoFrameReady,
            this, &VideoPlayerWidget::onVideoFrameReady);
    connect(videoWorker, &VideoWorker::playbackFinished,
            this, &VideoPlayerWidget::onDecodingFinished);
    connect(videoWorker, &VideoWorker::errorOccurred,
            this, &VideoPlayerWidget::onErrorOccurred);
    
    // 连接 QML 信号（如果 QML 中定义了信号）
    QQuickItem* root = qmlWidget->rootObject();
    if (root) {
        // 连接 QML 的播放/暂停按钮
        QObject::connect(root, SIGNAL(playPauseClicked()), 
                this, SLOT(on_signal_play_pause()));
        QObject::connect(root, SIGNAL(stopClicked()), 
                this, SLOT(on_signal_stop()));
        QObject::connect(root, SIGNAL(seekRequested(int)), 
                this, SLOT(on_signal_seek(qint64)));
        QObject::connect(root, SIGNAL(openVideoClicked()), 
                this, SLOT(on_signal_open_video_dialog()));
    }
}

VideoPlayerWidget::~VideoPlayerWidget()
{
    qDebug() << "VideoPlayerWidget destructor";
}

void VideoPlayerWidget::on_signal_open_video(const QString& filePath)
{
    qDebug() << "Opening video:" << filePath;
    videoWorker->openVideo(filePath);
}

void VideoPlayerWidget::on_signal_open_video_dialog()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择视频文件",
        "",
        "视频文件 (*.mp4 *.avi *.mkv *.mov *.flv *.wmv *.webm);;所有文件 (*.*)"
    );
    
    if (!filePath.isEmpty()) {
        qDebug() << "Selected video file:" << filePath;
        on_signal_open_video(filePath);
    }
}

void VideoPlayerWidget::on_signal_play_pause()
{
    qDebug() << "[VIDEO_PLAYER] on_signal_play_pause called, isPlaying:" << isPlaying;
    
    if (isPlaying) {
        qDebug() << "Pausing video";
        videoWorker->pause();
        isPlaying = false;
    } else {
        qDebug() << "Playing video";
        videoWorker->play();
        isPlaying = true;
    }
    
    emit signal_play_state_changed(isPlaying);
    
    // 更新 QML 播放状态
    QQuickItem* root = qmlWidget->rootObject();
    if (root) {
        root->setProperty("isPlaying", isPlaying);
    }
}

void VideoPlayerWidget::on_signal_stop()
{
    qDebug() << "Stopping video";
    videoWorker->stop();
    isPlaying = false;
    
    emit signal_play_state_changed(false);
    
    QQuickItem* root = qmlWidget->rootObject();
    if (root) {
        root->setProperty("isPlaying", false);
    }
}

void VideoPlayerWidget::on_signal_seek(qint64 positionMs)
{
    qDebug() << "Seeking to:" << positionMs << "ms";
    videoWorker->seek(positionMs);
}

void VideoPlayerWidget::onVideoOpened(int width, int height, double fps, qint64 durationMs)
{
    qDebug() << "Video opened -" 
             << "Size:" << width << "x" << height
             << "FPS:" << fps
             << "Duration:" << durationMs << "ms";
    
    // 更新 QML 界面的视频信息
    QQuickItem* root = qmlWidget->rootObject();
    if (root) {
        root->setProperty("videoWidth", width);
        root->setProperty("videoHeight", height);
        root->setProperty("videoFPS", fps);
        root->setProperty("videoDuration", durationMs);
    }
}

void VideoPlayerWidget::onVideoFrameReady(QImage frame, qint64 ptsMs)
{
    // 将视频帧传递给 QML 的 VideoFrameItem 显示
    QQuickItem* root = qmlWidget->rootObject();
    if (root) {
        // 查找VideoFrameItem对象
        QQuickItem* videoFrameItem = root->findChild<QQuickItem*>("videoFrameItem");
        if (videoFrameItem) {
            VideoFrameItem* frameItem = qobject_cast<VideoFrameItem*>(videoFrameItem);
            if (frameItem) {
                frameItem->setFrame(frame);
            }
        }
        
        // 更新播放进度
        root->setProperty("currentPosition", ptsMs);
        qint64 duration = root->property("videoDuration").toLongLong();
        if (duration > 0) {
            qreal progress = static_cast<qreal>(ptsMs) / duration;
            QMetaObject::invokeMethod(root, "updateProgress",
                                     Q_ARG(QVariant, progress));
        }
    }
}

void VideoPlayerWidget::onDecodingFinished()
{
    qDebug() << "Video playback finished";
    isPlaying = false;
    
    emit signal_video_finished();
    
    QQuickItem* root = qmlWidget->rootObject();
    if (root) {
        root->setProperty("isPlaying", false);
    }
}

void VideoPlayerWidget::onErrorOccurred(QString error)
{
    qDebug() << "Video error:" << error;
    
    QQuickItem* root = qmlWidget->rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "showError",
                                 Q_ARG(QVariant, error));
    }
}
