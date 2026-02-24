#include "VideoPlayerWindow.h"
#include "VideoSession.h"
#include "VideoRendererGL.h"
#include <QDebug>
#include <QPainterPath>
#include <QFileInfo>
#include <QApplication>
#include <QScreen>
#include <QTimer>


VideoPlayerWindow::VideoPlayerWindow(QWidget *parent)
    : QWidget(parent)
    , m_renderWidget(nullptr)
    , m_playPauseBtn(nullptr)
    , m_openFileBtn(nullptr)
    , m_progressSlider(nullptr)
    , m_timeLabel(nullptr)
    , m_fileNameLabel(nullptr)
    , m_mediaSession(nullptr)
    , m_isPlaying(false)
    , m_sliderPressed(false)
    , m_duration(0)
    , m_currentPosition(0)
{
    setupUI();
    
    setWindowTitle(QStringLiteral(u"\u89c6\u9891\u64ad\u653e\u5668"));
    setFixedSize(720, 480);
    
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
}

VideoPlayerWindow::~VideoPlayerWindow()
{
    qDebug() << "[VideoPlayerWindow] Destructor";
}

void VideoPlayerWindow::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    QWidget* titleBar = new QWidget(this);
    titleBar->setFixedHeight(40);
    titleBar->setStyleSheet(
        "QWidget {"
        "    background: #2C2C2E;"
        "}"
    );
    
    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 15, 0);
    
    m_fileNameLabel = new QLabel(QStringLiteral(u"\u672a\u52a0\u8f7d\u89c6\u9891"), titleBar);
    m_fileNameLabel->setStyleSheet(
        "QLabel {"
        "    color: #FFFFFF;"
        "    font-size: 14px;"
        "    font-weight: 500;"
        "}"
    );
    
    QPushButton* closeBtn = new QPushButton("X", titleBar);
    closeBtn->setFixedSize(30, 30);
    closeBtn->setStyleSheet(
        "QPushButton {"
        "    background: transparent;"
        "    color: #FFFFFF;"
        "    border: none;"
        "    font-size: 16px;"
        "    border-radius: 15px;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(255, 59, 48, 0.8);"
        "}"
    );
    connect(closeBtn, &QPushButton::clicked, this, &VideoPlayerWindow::close);
    
    titleLayout->addWidget(m_fileNameLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(closeBtn);
    
    m_renderWidget = new VideoRendererGL(this);
    
    QWidget* controlBar = new QWidget(this);
    controlBar->setFixedHeight(100);
    controlBar->setStyleSheet(
        "QWidget {"
        "    background: #1C1C1E;"
        "}"
    );
    
    QVBoxLayout* controlLayout = new QVBoxLayout(controlBar);
    controlLayout->setContentsMargins(20, 10, 20, 10);
    controlLayout->setSpacing(8);
    
    m_progressSlider = new QSlider(Qt::Horizontal, controlBar);
    m_progressSlider->setRange(0, 1000);
    m_progressSlider->setValue(0);
    m_progressSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "    background: #3A3A3C;"
        "    height: 4px;"
        "    border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "    background: #31C27C;"
        "    width: 14px;"
        "    height: 14px;"
        "    margin: -5px 0;"
        "    border-radius: 7px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "    background: #31C27C;"
        "    border-radius: 2px;"
        "}"
    );
    
    connect(m_progressSlider, &QSlider::sliderPressed, this, &VideoPlayerWindow::onSliderPressed);
    connect(m_progressSlider, &QSlider::sliderReleased, this, &VideoPlayerWindow::onSliderReleased);
    connect(m_progressSlider, &QSlider::valueChanged, this, &VideoPlayerWindow::onSliderValueChanged);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(15);
    
    m_playPauseBtn = new QPushButton(QStringLiteral(u"\u64ad\u653e"), controlBar);
    m_playPauseBtn->setFixedSize(50, 50);
    m_playPauseBtn->setStyleSheet(
        "QPushButton {"
        "    background: #31C27C;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 25px;"
        "    font-size: 20px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background: #28A869;"
        "}"
        "QPushButton:pressed {"
        "    background: #229D5F;"
        "}"
        "QPushButton:disabled {"
        "    background: #3A3A3C;"
        "    color: #666666;"
        "}"
    );
    m_playPauseBtn->setEnabled(false);
    connect(m_playPauseBtn, &QPushButton::clicked, this, &VideoPlayerWindow::onPlayPauseClicked);
    
    m_openFileBtn = new QPushButton(QStringLiteral(u"\u6253\u5f00\u89c6\u9891"), controlBar);
    m_openFileBtn->setFixedHeight(40);
    m_openFileBtn->setStyleSheet(
        "QPushButton {"
        "    background: #2C2C2E;"
        "    color: #FFFFFF;"
        "    border: 1px solid #3A3A3C;"
        "    border-radius: 8px;"
        "    padding: 0 20px;"
        "    font-size: 13px;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background: #3A3A3C;"
        "    border-color: #31C27C;"
        "}"
        "QPushButton:pressed {"
        "    background: #48484A;"
        "}"
    );
    connect(m_openFileBtn, &QPushButton::clicked, this, &VideoPlayerWindow::onOpenFileClicked);
    
    m_timeLabel = new QLabel("00:00 / 00:00", controlBar);
    m_timeLabel->setStyleSheet(
        "QLabel {"
        "    color: #AEAEB2;"
        "    font-size: 13px;"
        "    font-weight: 500;"
        "}"
    );
    m_timeLabel->setMinimumWidth(120);
    
    buttonLayout->addWidget(m_playPauseBtn);
    buttonLayout->addWidget(m_timeLabel);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_openFileBtn);
    
    controlLayout->addWidget(m_progressSlider);
    controlLayout->addLayout(buttonLayout);
    
    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(m_renderWidget, 1);
    mainLayout->addWidget(controlBar);
    
    setLayout(mainLayout);
    
    setStyleSheet(
        "QWidget#VideoPlayerWindow {"
        "    background: #000000;"
        "}"
    );
}

void VideoPlayerWindow::loadVideo(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return;
    }
    
    qDebug() << "[VideoPlayerWindow] Loading video:" << filePath;
    
    if (m_mediaSession && m_isPlaying) {
        m_mediaSession->stop();
    }
    
    m_isPlaying = false;
    m_currentPosition = 0;
    m_duration = 0;
    m_sliderPressed = false;
    
    if (m_playPauseBtn) {
        m_playPauseBtn->setText(QStringLiteral(u"\u64ad\u653e"));
        m_playPauseBtn->setEnabled(false);
    }
    if (m_progressSlider) {
        m_progressSlider->setValue(0);
    }
    if (m_timeLabel) {
        m_timeLabel->setText("00:00 / 00:00");
    }
    
    m_currentFilePath = filePath;
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    if (m_fileNameLabel) {
        m_fileNameLabel->setText(fileName);
    }
    
    if (m_mediaSession) {
        delete m_mediaSession;
        m_mediaSession = nullptr;
    }
    
    m_mediaSession = new MediaSession(this);
    
    connect(m_mediaSession, &MediaSession::positionChanged,
            this, &VideoPlayerWindow::onPositionChanged);
    connect(m_mediaSession, &MediaSession::durationChanged,
            this, &VideoPlayerWindow::onDurationChanged);
    connect(m_mediaSession, &MediaSession::playbackFinished,
            this, &VideoPlayerWindow::onPlaybackFinished);
    connect(m_mediaSession, &MediaSession::stateChanged,
            this, [this](MediaSession::PlaybackState state) {
        qDebug() << "[VideoPlayerWindow] State changed:" << (int)state;
    });
    
    QUrl url = QUrl::fromLocalFile(filePath);
    if (!m_mediaSession->loadSource(url)) {
        qWarning() << "[VideoPlayerWindow] Failed to load video";
        return;
    }
    
    if (m_mediaSession->hasVideo()) {
        VideoSession* videoSession = m_mediaSession->videoSession();
        if (videoSession && m_renderWidget) {
            videoSession->setVideoRenderer(m_renderWidget);
            qDebug() << "[VideoPlayerWindow] VideoRenderer connected to VideoSession";
        }
    }
    
    m_playPauseBtn->setEnabled(true);
    
    emit videoLoaded(filePath);
}

void VideoPlayerWindow::onPlayPauseClicked()
{
    if (!m_mediaSession) {
        qWarning() << "[VideoPlayerWindow] No media session";
        return;
    }

    m_isPlaying = !m_isPlaying;

    if (m_isPlaying) {
        m_playPauseBtn->setText(QStringLiteral(u"\u6682\u505c"));
        qDebug() << "[VideoPlayerWindow] Play";

        // Notify outside first (pause music) to avoid overlap window.
        emit playStateChanged(m_isPlaying);

        // Direct playback without seek.
        m_mediaSession->play();
    } else {
        m_playPauseBtn->setText(QStringLiteral(u"\u64ad\u653e"));
        qDebug() << "[VideoPlayerWindow] Pause";
        m_mediaSession->pause();

        emit playStateChanged(m_isPlaying);
    }
}

void VideoPlayerWindow::pausePlayback()
{
    if (!m_mediaSession || !m_isPlaying) {
        return;
    }
    
    m_isPlaying = false;
    m_playPauseBtn->setText(QStringLiteral(u"\u64ad\u653e"));
    qDebug() << "[VideoPlayerWindow] Pause (external)";
    m_mediaSession->pause();
    
    emit playStateChanged(m_isPlaying);
}

void VideoPlayerWindow::onOpenFileClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral(u"\u9009\u62e9\u89c6\u9891\u6587\u4ef6"),
        QDir::homePath(),
        QStringLiteral(u"\u89c6\u9891\u6587\u4ef6 (*.mp4 *.avi *.mkv *.mov *.flv *.wmv);;\u6240\u6709\u6587\u4ef6 (*.*)")
    );
    
    if (!filePath.isEmpty()) {
        loadVideo(filePath);
    }
}

void VideoPlayerWindow::onSliderPressed()
{
    m_sliderPressed = true;
    qDebug() << "[VideoPlayerWindow] Slider pressed";
}

void VideoPlayerWindow::onPlaybackFinished()
{
    qDebug() << "[VideoPlayerWindow] Playback finished";
    
    if (m_mediaSession) {
        m_mediaSession->stop();
        
        QTimer::singleShot(100, this, [this]() {
            if (m_mediaSession) {
                qDebug() << "[VideoPlayerWindow] Seeking to beginning for next playback";
                m_mediaSession->seekTo(0);
            }
        });
    }
    
    m_isPlaying = false;
    if (m_playPauseBtn) {
        m_playPauseBtn->setText(QStringLiteral(u"\u64ad\u653e"));
    }
    
    m_currentPosition = 0;
    if (m_progressSlider && !m_sliderPressed) {
        m_progressSlider->setValue(0);
    }
    updateTimeLabel(0, m_duration);
}

void VideoPlayerWindow::onSliderReleased()
{
    m_sliderPressed = false;
    
    qint64 targetPosition = (m_duration * m_progressSlider->value()) / 1000;
    m_currentPosition = targetPosition;
    
    qDebug() << "[VideoPlayerWindow] Seek to:" << targetPosition << "ms";
    
    if (m_mediaSession) {
        m_mediaSession->seekTo(targetPosition);
    }
    
    emit progressChanged(targetPosition);
}

void VideoPlayerWindow::onSliderValueChanged(int value)
{
    if (m_sliderPressed) {
        qint64 position = (m_duration * value) / 1000;
        updateTimeLabel(position, m_duration);
    }
}

void VideoPlayerWindow::updateTimeLabel(qint64 currentMs, qint64 totalMs)
{
    QString current = formatTime(currentMs);
    QString total = formatTime(totalMs);
    m_timeLabel->setText(QString("%1 / %2").arg(current).arg(total));
}

QString VideoPlayerWindow::formatTime(qint64 ms)
{
    int seconds = static_cast<int>((ms / 1000) % 60);
    int minutes = static_cast<int>((ms / (1000 * 60)) % 60);
    int hours = static_cast<int>(ms / (1000 * 60 * 60));
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
}

void VideoPlayerWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void VideoPlayerWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "[VideoPlayerWindow] Closing window, cleaning up resources...";
    
    if (m_mediaSession) {
        if (m_isPlaying) {
            m_mediaSession->stop();
        }
        
        m_mediaSession->deleteLater();
        m_mediaSession = nullptr;
    }
    
    m_isPlaying = false;
    m_currentPosition = 0;
    m_duration = 0;
    m_currentFilePath.clear();
    
    if (m_playPauseBtn) {
        m_playPauseBtn->setText(QStringLiteral(u"\u64ad\u653e"));
        m_playPauseBtn->setEnabled(false);
    }
    if (m_progressSlider) {
        m_progressSlider->setValue(0);
    }
    if (m_timeLabel) {
        m_timeLabel->setText("00:00 / 00:00");
    }
    if (m_fileNameLabel) {
        m_fileNameLabel->setText(QStringLiteral(u"\u672a\u52a0\u8f7d\u89c6\u9891"));
    }
    
    if (m_renderWidget) {
        m_renderWidget->stop();
    }
    
    qDebug() << "[VideoPlayerWindow] Window closed, all resources cleaned";
    QWidget::closeEvent(event);
}

void VideoPlayerWindow::onPositionChanged(qint64 positionMs)
{
    m_currentPosition = positionMs;
    
    if (!m_sliderPressed && m_duration > 0) {
        int sliderValue = (positionMs * 1000) / m_duration;
        m_progressSlider->setValue(sliderValue);
    }
    
    updateTimeLabel(positionMs, m_duration);
}

void VideoPlayerWindow::onDurationChanged(qint64 durationMs)
{
    m_duration = durationMs;
    updateTimeLabel(m_currentPosition, m_duration);
    
    qDebug() << "[VideoPlayerWindow] Duration:" << durationMs << "ms";
}
