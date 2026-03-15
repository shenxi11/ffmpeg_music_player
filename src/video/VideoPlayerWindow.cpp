#include "VideoPlayerWindow.h"
#include "VideoSession.h"
#include "VideoRendererGL.h"
#include <QDebug>
#include <QPainterPath>
#include <QFileInfo>
#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <QStyle>
#include <QFontMetrics>


VideoPlayerWindow::VideoPlayerWindow(QWidget *parent)
    : QWidget(parent)
    , m_renderWidget(nullptr)
    , m_playPauseBtn(nullptr)
    , m_openFileBtn(nullptr)
    , m_displayModeBtn(nullptr)
    , m_fullScreenBtn(nullptr)
    , m_qualityPresetBox(nullptr)
    , m_playbackRateBox(nullptr)
    , m_progressSlider(nullptr)
    , m_timeLabel(nullptr)
    , m_fileNameLabel(nullptr)
    , m_mediaSession(nullptr)
    , m_isPlaying(false)
    , m_sliderPressed(false)
    , m_duration(0)
    , m_currentPosition(0)
    , m_replayPendingSeek(false)
    , m_fillDisplayMode(false)
    , m_pendingStoppedSeekPosition(0)
{
    setupUI();
    
    setWindowTitle(QStringLiteral(u"\u89c6\u9891\u64ad\u653e\u5668"));
    setMinimumSize(720, 480);
    resize(1080, 700);
    setFocusPolicy(Qt::StrongFocus);
    
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
    titleBar->setObjectName("VideoTitleBar");
    titleBar->setAttribute(Qt::WA_StyledBackground, true);
    titleBar->setStyleSheet("background:#171A20; border-bottom:1px solid #262A33;");
    
    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 15, 0);
    
    m_fileNameLabel = new QLabel(QStringLiteral(u"\u672a\u52a0\u8f7d\u89c6\u9891"), titleBar);
    m_fileNameLabel->setObjectName("VideoFileNameLabel");
    m_fileNameLabel->setStyleSheet("color:#F4F6FA; font-size:14px; font-weight:600;");
    
    QPushButton* closeBtn = new QPushButton("X", titleBar);
    closeBtn->setFixedSize(30, 30);
    closeBtn->setObjectName("VideoTitleCloseButton");
    closeBtn->setStyleSheet(
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  color: #F4F6FA;"
        "  border-radius: 12px;"
        "  font-size: 16px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(255, 76, 76, 0.85);"
        "  color: #FFFFFF;"
        "}"
    );
    
    titleLayout->addWidget(m_fileNameLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(closeBtn);
    
    m_renderWidget = new VideoRendererGL(this);
    m_renderWidget->setDisplayMode(0);  // Fit
    m_renderWidget->setQualityPreset(static_cast<int>(VideoRendererGL::Standard1080P));
    
    QWidget* controlBar = new QWidget(this);
    controlBar->setFixedHeight(100);
    controlBar->setObjectName("VideoControlBar");
    controlBar->setAttribute(Qt::WA_StyledBackground, true);
    controlBar->setStyleSheet("background:#171A20; border-top:1px solid #262A33;");
    
    QVBoxLayout* controlLayout = new QVBoxLayout(controlBar);
    controlLayout->setContentsMargins(20, 10, 20, 10);
    controlLayout->setSpacing(8);
    
    m_progressSlider = new QSlider(Qt::Horizontal, controlBar);
    m_progressSlider->setRange(0, 1000);
    m_progressSlider->setValue(0);
    m_progressSlider->setObjectName("VideoProgressSlider");
    m_progressSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 4px; border-radius: 2px; background: #3A414E; }"
        "QSlider::sub-page:horizontal { border-radius: 2px; background: #EC4141; }"
        "QSlider::handle:horizontal { width: 12px; height: 12px; margin: -4px 0;"
        "border-radius: 6px; border: 1px solid #EC4141; background: #FFFFFF; }"
    );
    
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(15);
    
    m_playPauseBtn = new QPushButton(QStringLiteral(u"\u64ad\u653e"), controlBar);
    m_playPauseBtn->setFixedSize(50, 50);
    m_playPauseBtn->setObjectName("VideoPlayPauseButton");
    m_playPauseBtn->setProperty("accent", true);
    m_playPauseBtn->setEnabled(false);
    m_playPauseBtn->setStyleSheet(
        "QPushButton {"
        "  min-width: 50px; min-height: 50px; max-width: 50px; max-height: 50px;"
        "  border-radius: 25px; border: 1px solid #DE3B3B; background: #EC4141; color: #FFFFFF;"
        "  font-size: 18px; font-weight: 700;"
        "}"
        "QPushButton:hover { background: #FF5757; border-color: #EC4141; }"
        "QPushButton:pressed { background: #D93636; border-color: #C53030; }"
        "QPushButton:disabled { background: #7F3E3E; border-color: #7F3E3E; color: #E9CACA; }"
    );
    
    m_openFileBtn = new QPushButton(QStringLiteral(u"\u6253\u5f00\u89c6\u9891"), controlBar);
    m_openFileBtn->setFixedHeight(40);
    m_openFileBtn->setObjectName("VideoActionButton");
    m_openFileBtn->setStyleSheet(
        "QPushButton { border: 1px solid #3A414E; border-radius: 8px; background: #262B33; color: #EDF1F7; padding: 0 14px; }"
        "QPushButton:hover { border-color: #EC4141; background: #2D3440; }"
        "QPushButton:pressed { background: #232830; }"
    );

    // 显示“下一步动作”：当前默认是 Fit，因此按钮显示“填充”
    m_displayModeBtn = new QPushButton(QStringLiteral(u"\u586b\u5145"), controlBar);
    m_displayModeBtn->setFixedHeight(40);
    m_displayModeBtn->setObjectName("VideoActionButton");
    m_displayModeBtn->setStyleSheet(
        "QPushButton { border: 1px solid #3A414E; border-radius: 8px; background: #262B33; color: #EDF1F7; padding: 0 14px; }"
        "QPushButton:hover { border-color: #EC4141; background: #2D3440; }"
        "QPushButton:pressed { background: #232830; }"
    );

    m_fullScreenBtn = new QPushButton(QStringLiteral(u"\u5168\u5c4f"), controlBar);
    m_fullScreenBtn->setFixedHeight(40);
    m_fullScreenBtn->setObjectName("VideoActionButton");
    m_fullScreenBtn->setStyleSheet(
        "QPushButton { border: 1px solid #3A414E; border-radius: 8px; background: #262B33; color: #EDF1F7; padding: 0 14px; }"
        "QPushButton:hover { border-color: #EC4141; background: #2D3440; }"
        "QPushButton:pressed { background: #232830; }"
    );

    QLabel* qualityLabel = new QLabel(QStringLiteral(u"\u753b\u8d28"), controlBar);
    qualityLabel->setProperty("secondary", true);

    m_qualityPresetBox = new QComboBox(controlBar);
    m_qualityPresetBox->addItem(QStringLiteral(u"\u6807\u51c6 1080P"), static_cast<int>(VideoRendererGL::Standard1080P));
    m_qualityPresetBox->addItem(QStringLiteral(u"\u589e\u5f3a 2K"), static_cast<int>(VideoRendererGL::Enhanced2K));
    m_qualityPresetBox->setCurrentIndex(0);
    m_qualityPresetBox->setEnabled(false);
    m_qualityPresetBox->setFixedHeight(36);
    m_qualityPresetBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    const int qualityTextWidth = QFontMetrics(m_qualityPresetBox->font()).horizontalAdvance(QStringLiteral(u"\u6807\u51c6 1080P"));
    m_qualityPresetBox->setMinimumWidth(qualityTextWidth + 56);
    m_qualityPresetBox->setObjectName("VideoControlCombo");
    m_qualityPresetBox->setStyleSheet(
        "QComboBox { border: 1px solid #3A414E; border-radius: 8px; background: #262B33; color: #EDF1F7; padding: 0 28px 0 10px; }"
        "QComboBox:hover { border-color: #EC4141; }"
        "QComboBox::drop-down { border: none; width: 22px; }"
        "QComboBox QAbstractItemView { border: 1px solid #3A414E; border-radius: 8px; background: #262B33; color: #EDF1F7;"
        "selection-background-color: #EC4141; selection-color: #FFFFFF; }"
    );

    QLabel* rateLabel = new QLabel(QStringLiteral(u"\u500d\u901f"), controlBar);
    rateLabel->setProperty("secondary", true);

    m_playbackRateBox = new QComboBox(controlBar);
    m_playbackRateBox->addItem("0.5x", 0.5);
    m_playbackRateBox->addItem("0.75x", 0.75);
    m_playbackRateBox->addItem("1.0x", 1.0);
    m_playbackRateBox->addItem("1.25x", 1.25);
    m_playbackRateBox->addItem("1.5x", 1.5);
    m_playbackRateBox->addItem("2.0x", 2.0);
    m_playbackRateBox->setCurrentIndex(2);
    m_playbackRateBox->setEnabled(false);
    m_playbackRateBox->setFixedHeight(36);
    m_playbackRateBox->setObjectName("VideoControlCombo");
    m_playbackRateBox->setStyleSheet(
        "QComboBox { border: 1px solid #3A414E; border-radius: 8px; background: #262B33; color: #EDF1F7; padding: 0 28px 0 10px; }"
        "QComboBox:hover { border-color: #EC4141; }"
        "QComboBox::drop-down { border: none; width: 22px; }"
        "QComboBox QAbstractItemView { border: 1px solid #3A414E; border-radius: 8px; background: #262B33; color: #EDF1F7;"
        "selection-background-color: #EC4141; selection-color: #FFFFFF; }"
    );
    
    m_timeLabel = new QLabel("00:00 / 00:00", controlBar);
    m_timeLabel->setProperty("secondary", true);
    m_timeLabel->setMinimumWidth(120);
    m_timeLabel->setStyleSheet("color:#B8BEC9;");
    
    buttonLayout->addWidget(m_playPauseBtn);
    buttonLayout->addWidget(m_timeLabel);
    buttonLayout->addWidget(qualityLabel);
    buttonLayout->addWidget(m_qualityPresetBox);
    buttonLayout->addWidget(rateLabel);
    buttonLayout->addWidget(m_playbackRateBox);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_fullScreenBtn);
    buttonLayout->addWidget(m_displayModeBtn);
    buttonLayout->addWidget(m_openFileBtn);
    
    controlLayout->addWidget(m_progressSlider);
    controlLayout->addLayout(buttonLayout);
    
    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(m_renderWidget, 1);
    mainLayout->addWidget(controlBar);
    
    setLayout(mainLayout);
    
    setObjectName("VideoPlayerWindow");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("QWidget#VideoPlayerWindow { background:#0F1115; }");

    connectUiSignals(closeBtn);

    // 强制重刷样式，确保对象名选择器和本地样式立即生效
    auto repolishWidget = [](QWidget* w) {
        if (!w || !w->style()) return;
        w->style()->unpolish(w);
        w->style()->polish(w);
        w->update();
    };
    repolishWidget(this);
    repolishWidget(titleBar);
    repolishWidget(controlBar);
    repolishWidget(closeBtn);
    repolishWidget(m_playPauseBtn);
    repolishWidget(m_openFileBtn);
    repolishWidget(m_displayModeBtn);
    repolishWidget(m_fullScreenBtn);
    repolishWidget(m_qualityPresetBox);
    repolishWidget(m_playbackRateBox);
    repolishWidget(m_progressSlider);
}

void VideoPlayerWindow::loadVideo(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return;
    }
    
    qDebug() << "[VideoPlayerWindow] Loading video:" << filePath;
    
    if (m_mediaSession) {
        m_mediaSession->stop();
        delete m_mediaSession;
        m_mediaSession = nullptr;
    }
    
    m_isPlaying = false;
    m_currentPosition = 0;
    m_duration = 0;
    m_sliderPressed = false;
    m_replayPendingSeek = false;
    m_fillDisplayMode = false;
    
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
    if (m_displayModeBtn) {
        // 当前模式重置为 Fit，按钮显示可切换到 Fill
        m_displayModeBtn->setText(QStringLiteral(u"\u586b\u5145"));
    }
    if (m_qualityPresetBox) {
        m_qualityPresetBox->setCurrentIndex(0);
        m_qualityPresetBox->setEnabled(false);
    }
    if (m_playbackRateBox) {
        m_playbackRateBox->setEnabled(false);
    }
    if (m_renderWidget) {
        m_renderWidget->setDisplayMode(0);
        m_renderWidget->setQualityPreset(static_cast<int>(VideoRendererGL::Standard1080P));
    }
    
    m_currentFilePath = filePath;
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    if (m_fileNameLabel) {
        m_fileNameLabel->setText(fileName);
    }
    
    m_mediaSession = new MediaSession(this);
    connectMediaSessionSignals();
    
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
    if (m_qualityPresetBox) {
        m_qualityPresetBox->setEnabled(true);
        onQualityPresetChanged(m_qualityPresetBox->currentIndex());
    }
    if (m_playbackRateBox) {
        m_playbackRateBox->setEnabled(true);
        onPlaybackRateChanged(m_playbackRateBox->currentIndex());
    }
    
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

        // After reaching EOF, always seek back to 0 before replay.
        if (m_replayPendingSeek && m_mediaSession) {
            m_mediaSession->seekTo(0);
            m_currentPosition = 0;
            if (m_progressSlider) {
                m_progressSlider->setValue(0);
            }
            updateTimeLabel(0, m_duration);
            m_replayPendingSeek = false;
        }

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

void VideoPlayerWindow::onDisplayModeClicked()
{
    m_fillDisplayMode = !m_fillDisplayMode;
    const int mode = m_fillDisplayMode ? 1 : 0;

    if (m_renderWidget) {
        m_renderWidget->setDisplayMode(mode);
    }

    if (m_displayModeBtn) {
        // 按钮文案表示“下一步动作”，避免出现“点适应却先切到填充”的反直觉行为
        m_displayModeBtn->setText(m_fillDisplayMode
                                      ? QStringLiteral(u"\u9002\u5e94")
                                      : QStringLiteral(u"\u586b\u5145"));
    }

    qDebug() << "[VideoPlayerWindow] Display mode changed to"
             << (m_fillDisplayMode ? "Fill" : "Fit");
}

void VideoPlayerWindow::onFullScreenClicked()
{
    if (isFullScreen()) {
        showNormal();
        if (m_fullScreenBtn) {
            m_fullScreenBtn->setText(QStringLiteral(u"\u5168\u5c4f"));
        }
        qDebug() << "[VideoPlayerWindow] Exit fullscreen";
    } else {
        showFullScreen();
        if (m_fullScreenBtn) {
            m_fullScreenBtn->setText(QStringLiteral(u"\u9000\u51fa\u5168\u5c4f"));
        }
        qDebug() << "[VideoPlayerWindow] Enter fullscreen";
    }
}

void VideoPlayerWindow::onPlaybackRateChanged(int index)
{
    if (!m_playbackRateBox || index < 0) {
        return;
    }

    bool ok = false;
    const double rate = m_playbackRateBox->itemData(index).toDouble(&ok);
    if (!ok) {
        return;
    }

    if (m_mediaSession) {
        m_mediaSession->setPlaybackRate(rate);
    }

    qDebug() << "[VideoPlayerWindow] Playback rate selected:" << rate << "x";
}

void VideoPlayerWindow::onQualityPresetChanged(int index)
{
    if (!m_qualityPresetBox || index < 0 || !m_renderWidget) {
        return;
    }

    bool ok = false;
    const int preset = m_qualityPresetBox->itemData(index).toInt(&ok);
    if (!ok) {
        return;
    }

    m_renderWidget->setQualityPreset(preset);

    qDebug() << "[VideoPlayerWindow] Quality preset selected:"
             << (preset == static_cast<int>(VideoRendererGL::Enhanced2K) ? "Enhanced2K" : "Standard1080P");
}

void VideoPlayerWindow::onPlaybackFinished()
{
    qDebug() << "[VideoPlayerWindow] Playback finished";
    
    if (m_mediaSession) {
        m_mediaSession->stop();
    }
    m_replayPendingSeek = true;
    
    m_isPlaying = false;
    emit playStateChanged(false);
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

    if (!m_mediaSession || !m_progressSlider) {
        return;
    }

    qint64 targetPosition = (m_duration * m_progressSlider->value()) / 1000;
    m_currentPosition = targetPosition;

    qDebug() << "[VideoPlayerWindow] Seek to:" << targetPosition << "ms";

    const bool wasStopped = (m_mediaSession->state() == MediaSession::Stopped);
    const bool wasPaused = (m_mediaSession->state() == MediaSession::Paused);
    const bool wasPlaying = m_isPlaying;

    // 拖动视频进度条表示切换到视频播放焦点，先通知外层暂停音频。
    emit playStateChanged(true);

    if (wasStopped) {
        // Stopped态先进入播放，再定位到目标时间，避免play()重置到0。
        m_mediaSession->play();
        m_pendingStoppedSeekPosition = targetPosition;
        QTimer::singleShot(0, this, &VideoPlayerWindow::onDeferredSeekAfterStopped);
    } else if (wasPaused) {
        // Paused态先显式seek，再恢复播放，避免play()里的handoff强制seek
        // 与用户seek形成双重seek链路。
        m_mediaSession->seekTo(targetPosition);
        m_mediaSession->play();
    } else {
        m_mediaSession->seekTo(targetPosition);
        if (m_mediaSession->state() != MediaSession::Playing) {
            m_mediaSession->play();
        }
    }

    m_isPlaying = true;
    if (m_playPauseBtn) {
        m_playPauseBtn->setText(QStringLiteral(u"\u6682\u505c"));
    }

    if (!wasPlaying) {
        qDebug() << "[VideoPlayerWindow] Auto-resume playback after seek";
    }

    m_replayPendingSeek = false;

    emit progressChanged(targetPosition);
}

void VideoPlayerWindow::onVideoSizeChanged(const QSize& size)
{
    Q_UNUSED(size);
    if (m_renderWidget) {
        m_renderWidget->setDisplayMode(m_fillDisplayMode ? 1 : 0);
    }
}

void VideoPlayerWindow::onMediaSessionStateChanged(MediaSession::PlaybackState state)
{
    qDebug() << "[VideoPlayerWindow] State changed:" << static_cast<int>(state);
}

void VideoPlayerWindow::onDeferredSeekAfterStopped()
{
    if (m_mediaSession) {
        m_mediaSession->seekTo(m_pendingStoppedSeekPosition);
    }
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
    if (isFullScreen()) {
        showNormal();
    }
    emit playStateChanged(false);
    
    if (m_mediaSession) {
        m_mediaSession->stop();
        delete m_mediaSession;
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
    if (m_qualityPresetBox) {
        m_qualityPresetBox->setCurrentIndex(0);
        m_qualityPresetBox->setEnabled(false);
    }
    if (m_playbackRateBox) {
        m_playbackRateBox->setCurrentIndex(2);
        m_playbackRateBox->setEnabled(false);
    }
    if (m_fullScreenBtn) {
        m_fullScreenBtn->setText(QStringLiteral(u"\u5168\u5c4f"));
    }
    
    if (m_renderWidget) {
        m_renderWidget->stop();
    }
    
    qDebug() << "[VideoPlayerWindow] Window closed, all resources cleaned";
    QWidget::closeEvent(event);
}

void VideoPlayerWindow::keyPressEvent(QKeyEvent *event)
{
    if (!event) {
        QWidget::keyPressEvent(event);
        return;
    }

    if (event->key() == Qt::Key_F11) {
        onFullScreenClicked();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Escape && isFullScreen()) {
        onFullScreenClicked();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
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

