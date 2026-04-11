#include "VideoPlayerWindow.h"

#include "VideoRendererGL.h"
#include "VideoSession.h"

#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFrame>
#include <QIcon>
#include <QPainterPath>
#include <QStyle>
#include <QTimer>

namespace {

QIcon videoWindowIcon(const QString& name) {
    return QIcon(QStringLiteral(":/qml/assets/ai/icons/%1.svg").arg(name));
}

QLabel* createIconLabel(const QString& iconName, QWidget* parent,
                        const QSize& size = QSize(16, 16)) {
    auto* label = new QLabel(parent);
    label->setObjectName("VideoInlineIconLabel");
    label->setFixedSize(size);
    label->setPixmap(videoWindowIcon(iconName).pixmap(size));
    return label;
}

} // namespace

VideoPlayerWindow::VideoPlayerWindow(QWidget* parent)
    : QWidget(parent), m_renderWidget(nullptr), m_playPauseBtn(nullptr), m_stopBtn(nullptr),
      m_openFileBtn(nullptr), m_displayModeBtn(nullptr), m_fullScreenBtn(nullptr),
      m_qualityPresetBox(nullptr), m_playbackRateBox(nullptr), m_progressSlider(nullptr),
      m_timeLabel(nullptr), m_fileNameLabel(nullptr), m_metaInfoLabel(nullptr),
      m_qualityLabel(nullptr), m_rateLabel(nullptr), m_qualityIconLabel(nullptr),
      m_rateIconLabel(nullptr), m_titleBar(nullptr), m_controlBar(nullptr),
      m_trailingControls(nullptr), m_mediaSession(nullptr), m_isPlaying(false),
      m_sliderPressed(false), m_duration(0), m_currentPosition(0), m_replayPendingSeek(false),
      m_fillDisplayMode(false), m_pendingStoppedSeekPosition(0), m_videoFrameSize(),
      m_fullScreenTransitionTimer(new QTimer(this)), m_fullScreenTransitionInProgress(false),
      m_targetFullScreenState(false), m_immersiveMaximizeActive(false), m_closePending(false),
      m_cleanupDone(false), m_savedWindowGeometry(), m_savedWasMaximized(false) {
    setupUI();

    setWindowTitle(QStringLiteral(u"\u89c6\u9891\u64ad\u653e\u5668"));
    setMinimumSize(720, 480);
    resize(1080, 700);
    setFocusPolicy(Qt::StrongFocus);

    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint |
                   Qt::WindowMaximizeButtonHint);

    m_fullScreenTransitionTimer->setSingleShot(true);
    connect(m_fullScreenTransitionTimer, &QTimer::timeout, this,
            &VideoPlayerWindow::finalizeFullScreenTransition);
}

VideoPlayerWindow::~VideoPlayerWindow() {
    qDebug() << "[VideoPlayerWindow] Destructor";
}

void VideoPlayerWindow::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_titleBar = new QWidget(this);
    m_titleBar->setFixedHeight(68);
    m_titleBar->setObjectName("VideoTitleBar");
    m_titleBar->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout* titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(16, 10, 12, 10);
    titleLayout->setSpacing(12);

    QWidget* titleInfoWidget = new QWidget(m_titleBar);
    titleInfoWidget->setObjectName("VideoTitleInfoWidget");
    titleInfoWidget->setAttribute(Qt::WA_StyledBackground, true);
    QVBoxLayout* titleInfoLayout = new QVBoxLayout(titleInfoWidget);
    titleInfoLayout->setContentsMargins(0, 0, 0, 0);
    titleInfoLayout->setSpacing(1);

    m_fileNameLabel =
        new QLabel(QStringLiteral(u"\u672a\u52a0\u8f7d\u89c6\u9891"), titleInfoWidget);
    m_fileNameLabel->setObjectName("VideoFileNameLabel");
    m_fileNameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_fileNameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_metaInfoLabel = new QLabel(QStringLiteral(u"\u5f71\u9662\u6697\u573a\u00b7\u9009\u62e9\u89c6"
                                                u"\u9891\u540e\u5f00\u59cb\u64ad\u653e"),
                                 titleInfoWidget);
    m_metaInfoLabel->setObjectName("VideoMetaInfoLabel");

    titleInfoLayout->addWidget(m_fileNameLabel);
    titleInfoLayout->addWidget(m_metaInfoLabel);

    m_openFileBtn = new QPushButton(QStringLiteral(u"\u6253\u5f00\u89c6\u9891"), m_titleBar);
    m_openFileBtn->setObjectName("VideoActionButton");
    m_openFileBtn->setProperty("compact", true);
    m_openFileBtn->setFixedHeight(34);
    m_openFileBtn->setIcon(videoWindowIcon(QStringLiteral("video-open")));
    m_openFileBtn->setIconSize(QSize(18, 18));
    m_openFileBtn->setCursor(Qt::PointingHandCursor);

    QPushButton* closeBtn = new QPushButton(m_titleBar);
    closeBtn->setFixedSize(34, 34);
    closeBtn->setObjectName("VideoTitleCloseButton");
    closeBtn->setIcon(videoWindowIcon(QStringLiteral("video-close")));
    closeBtn->setIconSize(QSize(16, 16));
    closeBtn->setCursor(Qt::PointingHandCursor);

    titleLayout->addWidget(titleInfoWidget, 1);
    titleLayout->addWidget(m_openFileBtn);
    titleLayout->addWidget(closeBtn);

    m_renderWidget = new VideoRendererGL(this);
    m_renderWidget->setDisplayMode(0);
    m_renderWidget->setQualityPreset(static_cast<int>(VideoRendererGL::Standard1080P));

    m_controlBar = new QWidget(this);
    m_controlBar->setFixedHeight(128);
    m_controlBar->setObjectName("VideoControlBar");
    m_controlBar->setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout* controlLayout = new QVBoxLayout(m_controlBar);
    controlLayout->setContentsMargins(16, 12, 16, 12);
    controlLayout->setSpacing(10);

    m_progressSlider = new QSlider(Qt::Horizontal, m_controlBar);
    m_progressSlider->setRange(0, 1000);
    m_progressSlider->setValue(0);
    m_progressSlider->setObjectName("VideoProgressSlider");

    QHBoxLayout* progressLayout = new QHBoxLayout();
    progressLayout->setSpacing(10);

    m_timeLabel = new QLabel("00:00 / 00:00", m_controlBar);
    m_timeLabel->setObjectName("VideoTimeLabel");
    m_timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_timeLabel->setMinimumWidth(124);

    progressLayout->addWidget(m_progressSlider, 1);
    progressLayout->addWidget(m_timeLabel);

    QHBoxLayout* bottomRowLayout = new QHBoxLayout();
    bottomRowLayout->setContentsMargins(0, 0, 0, 0);
    bottomRowLayout->setSpacing(12);

    m_playPauseBtn = new QPushButton(m_controlBar);
    m_playPauseBtn->setFixedSize(50, 50);
    m_playPauseBtn->setObjectName("VideoPlayPauseButton");
    m_playPauseBtn->setEnabled(false);
    m_playPauseBtn->setCursor(Qt::PointingHandCursor);

    m_stopBtn = new QPushButton(m_controlBar);
    m_stopBtn->setFixedSize(38, 38);
    m_stopBtn->setObjectName("VideoRoundButton");
    m_stopBtn->setIcon(videoWindowIcon(QStringLiteral("video-stop")));
    m_stopBtn->setIconSize(QSize(16, 16));
    m_stopBtn->setToolTip(QStringLiteral(u"\u505c\u6b62"));
    m_stopBtn->setEnabled(false);
    m_stopBtn->setCursor(Qt::PointingHandCursor);

    m_displayModeBtn = new QPushButton(m_controlBar);
    m_displayModeBtn->setFixedHeight(38);
    m_displayModeBtn->setObjectName("VideoActionButton");
    m_displayModeBtn->setCursor(Qt::PointingHandCursor);

    m_fullScreenBtn = new QPushButton(m_controlBar);
    m_fullScreenBtn->setFixedHeight(38);
    m_fullScreenBtn->setObjectName("VideoActionButton");
    m_fullScreenBtn->setCursor(Qt::PointingHandCursor);

    m_qualityLabel = new QLabel(QStringLiteral(u"\u753b\u8d28"), m_controlBar);
    m_qualityLabel->setObjectName("VideoInlineLabel");

    m_qualityPresetBox = new QComboBox(m_controlBar);
    m_qualityPresetBox->addItem(QStringLiteral(u"\u6807\u51c6 1080P"),
                                static_cast<int>(VideoRendererGL::Standard1080P));
    m_qualityPresetBox->addItem(QStringLiteral(u"\u589e\u5f3a 2K"),
                                static_cast<int>(VideoRendererGL::Enhanced2K));
    m_qualityPresetBox->setCurrentIndex(0);
    m_qualityPresetBox->setEnabled(false);
    m_qualityPresetBox->setFixedHeight(36);
    m_qualityPresetBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    const int qualityTextWidth = QFontMetrics(m_qualityPresetBox->font())
                                     .horizontalAdvance(QStringLiteral(u"\u6807\u51c6 1080P"));
    m_qualityPresetBox->setMinimumWidth(qualityTextWidth + 56);
    m_qualityPresetBox->setObjectName("VideoControlCombo");

    m_rateLabel = new QLabel(QStringLiteral(u"\u500d\u901f"), m_controlBar);
    m_rateLabel->setObjectName("VideoInlineLabel");

    m_playbackRateBox = new QComboBox(m_controlBar);
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

    m_trailingControls = new QWidget(m_controlBar);
    m_trailingControls->setObjectName("VideoTrailingControls");
    m_trailingControls->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout* trailingLayout = new QHBoxLayout(m_trailingControls);
    trailingLayout->setContentsMargins(0, 0, 0, 0);
    trailingLayout->setSpacing(10);
    m_qualityIconLabel = createIconLabel(QStringLiteral("video-quality"), m_trailingControls);
    m_rateIconLabel = createIconLabel(QStringLiteral("video-speed"), m_trailingControls);
    trailingLayout->addWidget(m_qualityIconLabel);
    trailingLayout->addWidget(m_qualityLabel);
    trailingLayout->addWidget(m_qualityPresetBox);
    trailingLayout->addSpacing(6);
    trailingLayout->addWidget(m_rateIconLabel);
    trailingLayout->addWidget(m_rateLabel);
    trailingLayout->addWidget(m_playbackRateBox);
    trailingLayout->addSpacing(6);
    trailingLayout->addWidget(m_displayModeBtn);
    trailingLayout->addWidget(m_fullScreenBtn);
    m_trailingControls->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    QWidget* transportControls = new QWidget(m_controlBar);
    transportControls->setObjectName("VideoTransportControls");
    transportControls->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout* transportLayout = new QHBoxLayout(transportControls);
    transportLayout->setContentsMargins(0, 0, 0, 0);
    transportLayout->setSpacing(10);
    transportLayout->addWidget(m_playPauseBtn);
    transportLayout->addWidget(m_stopBtn);

    bottomRowLayout->addWidget(transportControls, 0, Qt::AlignLeft);
    bottomRowLayout->addStretch(1);
    bottomRowLayout->addWidget(m_trailingControls, 0, Qt::AlignRight);

    controlLayout->addLayout(progressLayout);
    controlLayout->addLayout(bottomRowLayout);

    mainLayout->addWidget(m_titleBar);
    mainLayout->addWidget(m_renderWidget, 1);
    mainLayout->addWidget(m_controlBar);

    setLayout(mainLayout);

    setObjectName("VideoPlayerWindow");
    setAttribute(Qt::WA_StyledBackground, true);

    connectUiSignals(closeBtn);
    updateMetaInfo();
    updateButtonStates();
    updateResponsiveUi();

    auto repolishWidget = [](QWidget* w) {
        if (!w || !w->style())
            return;
        w->style()->unpolish(w);
        w->style()->polish(w);
        w->update();
    };
    repolishWidget(this);
    repolishWidget(m_titleBar);
    repolishWidget(m_controlBar);
    repolishWidget(closeBtn);
    repolishWidget(m_playPauseBtn);
    repolishWidget(m_stopBtn);
    repolishWidget(m_openFileBtn);
    repolishWidget(m_displayModeBtn);
    repolishWidget(m_fullScreenBtn);
    repolishWidget(m_qualityPresetBox);
    repolishWidget(m_playbackRateBox);
    repolishWidget(m_progressSlider);
}

void VideoPlayerWindow::loadVideo(const QString& filePath) {
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
    m_videoFrameSize = QSize();

    if (m_playPauseBtn) {
        m_playPauseBtn->setEnabled(false);
    }
    if (m_stopBtn) {
        m_stopBtn->setEnabled(false);
    }
    if (m_progressSlider) {
        m_progressSlider->setValue(0);
    }
    if (m_timeLabel) {
        m_timeLabel->setText("00:00 / 00:00");
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
    const QString fileName = fileInfo.fileName();
    if (m_fileNameLabel) {
        m_fileNameLabel->setText(fileName);
    }
    updateMetaInfo();
    updateButtonStates();

    m_mediaSession = new MediaSession(this);
    connectMediaSessionSignals();

    QUrl url(filePath);
    if (!url.isValid() || url.scheme().isEmpty()) {
        url = QUrl::fromLocalFile(filePath);
    }
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
    if (m_stopBtn) {
        m_stopBtn->setEnabled(true);
    }
    if (m_qualityPresetBox) {
        m_qualityPresetBox->setEnabled(true);
        onQualityPresetChanged(m_qualityPresetBox->currentIndex());
    }
    if (m_playbackRateBox) {
        m_playbackRateBox->setEnabled(true);
        onPlaybackRateChanged(m_playbackRateBox->currentIndex());
    }

    updateMetaInfo();
    updateButtonStates();

    emit videoLoaded(filePath);
}

void VideoPlayerWindow::onPlayPauseClicked() {
    if (!m_mediaSession) {
        qWarning() << "[VideoPlayerWindow] No media session";
        return;
    }

    m_isPlaying = !m_isPlaying;

    if (m_isPlaying) {
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
        qDebug() << "[VideoPlayerWindow] Pause";
        m_mediaSession->pause();

        emit playStateChanged(m_isPlaying);
    }
    updateButtonStates();
}

void VideoPlayerWindow::pausePlayback() {
    if (!m_mediaSession || !m_isPlaying) {
        return;
    }

    m_isPlaying = false;
    qDebug() << "[VideoPlayerWindow] Pause (external)";
    m_mediaSession->pause();

    emit playStateChanged(m_isPlaying);
    updateButtonStates();
}

bool VideoPlayerWindow::resumePlayback() {
    if (!m_mediaSession) {
        return false;
    }
    if (m_isPlaying) {
        return true;
    }
    onPlayPauseClicked();
    return m_isPlaying;
}

bool VideoPlayerWindow::seekToPosition(qint64 positionMs) {
    if (!m_mediaSession || m_duration <= 0) {
        return false;
    }

    const qint64 target = qBound<qint64>(0, positionMs, m_duration);
    m_currentPosition = target;

    if (m_progressSlider) {
        const int sliderValue = static_cast<int>((target * 1000) / m_duration);
        m_progressSlider->setValue(sliderValue);
    }
    updateTimeLabel(target, m_duration);

    const bool wasStopped = (m_mediaSession->state() == MediaSession::Stopped);
    const bool wasPaused = (m_mediaSession->state() == MediaSession::Paused);
    emit playStateChanged(true);

    if (wasStopped) {
        m_mediaSession->play();
        m_pendingStoppedSeekPosition = target;
        QTimer::singleShot(0, this, &VideoPlayerWindow::onDeferredSeekAfterStopped);
    } else {
        m_mediaSession->seekTo(target);
        if (wasPaused || m_mediaSession->state() != MediaSession::Playing) {
            m_mediaSession->play();
        }
    }

    m_isPlaying = true;
    updateButtonStates();
    emit progressChanged(target);
    return true;
}

bool VideoPlayerWindow::setFullScreenEnabled(bool enabled) {
    if (enabled == isImmersiveMaximizeActive()) {
        return true;
    }
    requestFullScreenChange(enabled, "api");
    return enabled == isImmersiveMaximizeActive();
}

bool VideoPlayerWindow::setPlaybackRateValue(double rate) {
    if (!m_playbackRateBox) {
        return false;
    }

    for (int index = 0; index < m_playbackRateBox->count(); ++index) {
        bool ok = false;
        const double candidate = m_playbackRateBox->itemData(index).toDouble(&ok);
        if (ok && qAbs(candidate - rate) < 0.0001) {
            m_playbackRateBox->setCurrentIndex(index);
            onPlaybackRateChanged(index);
            return true;
        }
    }
    return false;
}

bool VideoPlayerWindow::setQualityPresetValue(const QString& preset) {
    if (!m_qualityPresetBox) {
        return false;
    }

    const QString normalized = preset.trimmed().toLower();
    for (int index = 0; index < m_qualityPresetBox->count(); ++index) {
        const QString text = m_qualityPresetBox->itemText(index).trimmed().toLower();
        bool ok = false;
        const int value = m_qualityPresetBox->itemData(index).toInt(&ok);
        if (text == normalized || (ok && QString::number(value) == normalized) ||
            (normalized == QStringLiteral("1080p") && ok &&
             value == static_cast<int>(VideoRendererGL::Standard1080P)) ||
            (normalized == QStringLiteral("2k") && ok &&
             value == static_cast<int>(VideoRendererGL::Enhanced2K))) {
            m_qualityPresetBox->setCurrentIndex(index);
            onQualityPresetChanged(index);
            return true;
        }
    }
    return false;
}

QVariantMap VideoPlayerWindow::snapshot() const {
    QString qualityText;
    int qualityValue = -1;
    if (m_qualityPresetBox && m_qualityPresetBox->currentIndex() >= 0) {
        qualityText = m_qualityPresetBox->currentText();
        qualityValue = m_qualityPresetBox->currentData().toInt();
    }

    double rate = 1.0;
    if (m_playbackRateBox && m_playbackRateBox->currentIndex() >= 0) {
        rate = m_playbackRateBox->currentData().toDouble();
    }

    return {{QStringLiteral("visible"), isVisible()},
            {QStringLiteral("fullScreen"), isImmersiveMaximizeActive()},
            {QStringLiteral("playing"), m_isPlaying},
            {QStringLiteral("filePath"), m_currentFilePath},
            {QStringLiteral("durationMs"), m_duration},
            {QStringLiteral("positionMs"), m_currentPosition},
            {QStringLiteral("playbackRate"), rate},
            {QStringLiteral("qualityPreset"), qualityText},
            {QStringLiteral("qualityValue"), qualityValue},
            {QStringLiteral("displayMode"),
             m_fillDisplayMode ? QStringLiteral("fill") : QStringLiteral("fit")}};
}

void VideoPlayerWindow::onOpenFileClicked() {
    QString filePath = QFileDialog::getOpenFileName(
        this, QStringLiteral(u"\u9009\u62e9\u89c6\u9891\u6587\u4ef6"), QDir::homePath(),
        QStringLiteral(u"\u89c6\u9891\u6587\u4ef6 (*.mp4 *.avi *.mkv *.mov *.flv "
                       u"*.wmv);;\u6240\u6709\u6587\u4ef6 (*.*)"));

    if (!filePath.isEmpty()) {
        loadVideo(filePath);
    }
}

void VideoPlayerWindow::onSliderPressed() {
    m_sliderPressed = true;
    qDebug() << "[VideoPlayerWindow] Slider pressed";
}

void VideoPlayerWindow::onDisplayModeClicked() {
    m_fillDisplayMode = !m_fillDisplayMode;
    const int mode = m_fillDisplayMode ? 1 : 0;

    if (m_renderWidget) {
        m_renderWidget->setDisplayMode(mode);
    }

    if (m_displayModeBtn) {
        updateButtonStates();
    }

    updateMetaInfo();
    qDebug() << "[VideoPlayerWindow] Display mode changed to"
             << (m_fillDisplayMode ? "Fill" : "Fit");
}

void VideoPlayerWindow::onFullScreenClicked() {
    qDebug() << "[VideoPlayerWindow] Fullscreen button clicked";
    requestFullScreenChange(!isImmersiveMaximizeActive(), "button");
}

void VideoPlayerWindow::requestFullScreenChange(bool enabled, const char* source) {
    if (!source) {
        source = "unknown";
    }

    if (m_fullScreenTransitionInProgress && m_targetFullScreenState == enabled) {
        qDebug() << "[VideoPlayerWindow] Ignore duplicate fullscreen request from" << source
                 << "target:" << enabled;
        return;
    }

    if (!m_fullScreenTransitionInProgress && enabled == isImmersiveMaximizeActive()) {
        qDebug() << "[VideoPlayerWindow] Fullscreen request already satisfied from" << source
                 << "state:" << enabled;
        return;
    }

    m_fullScreenTransitionInProgress = true;
    m_targetFullScreenState = enabled;

    if (m_renderWidget) {
        m_renderWidget->setFullscreenTransitionActive(true);
    }
    if (m_fullScreenBtn) {
        m_fullScreenBtn->setEnabled(false);
    }

    qDebug() << "[VideoPlayerWindow] Fullscreen request from" << source << "target:" << enabled
             << "current:" << isImmersiveMaximizeActive();

    if (enabled) {
        applyImmersiveMaximize();
    } else {
        restoreFromImmersiveMaximize();
    }

    scheduleFullScreenTransitionSettle();
}

void VideoPlayerWindow::applyImmersiveMaximize() {
    if (m_immersiveMaximizeActive) {
        return;
    }

    m_savedWindowGeometry = geometry();
    m_savedWasMaximized = isMaximized();

    qDebug() << "[VideoPlayerWindow] Enter immersive maximize requested"
             << "savedGeometry:" << m_savedWindowGeometry
             << "savedWasMaximized:" << m_savedWasMaximized;

    m_immersiveMaximizeActive = true;
    if (m_titleBar) {
        m_titleBar->setVisible(false);
    }

    if (!isMaximized()) {
        showMaximized();
    } else {
        updateResponsiveUi();
    }

    qDebug() << "[VideoPlayerWindow] Immersive maximize applied"
             << "geometry:" << geometry() << "maximized:" << isMaximized();
}

void VideoPlayerWindow::restoreFromImmersiveMaximize() {
    if (!m_immersiveMaximizeActive) {
        return;
    }

    qDebug() << "[VideoPlayerWindow] Exit immersive maximize requested"
             << "restoreGeometry:" << m_savedWindowGeometry
             << "restoreMaximized:" << m_savedWasMaximized;

    if (m_savedWasMaximized) {
        showMaximized();
    } else {
        showNormal();
        if (m_savedWindowGeometry.isValid()) {
            setGeometry(m_savedWindowGeometry);
        }
    }
    m_immersiveMaximizeActive = false;
    if (m_titleBar) {
        m_titleBar->setVisible(true);
    }
    updateResponsiveUi();

    qDebug() << "[VideoPlayerWindow] Immersive maximize restored"
             << "geometry:" << geometry() << "maximized:" << isMaximized();
}

void VideoPlayerWindow::scheduleFullScreenTransitionSettle() {
    if (!m_fullScreenTransitionTimer) {
        return;
    }
    m_fullScreenTransitionTimer->start(140);
}

void VideoPlayerWindow::finalizeFullScreenTransition() {
    if (!m_fullScreenTransitionInProgress) {
        return;
    }

    m_fullScreenTransitionInProgress = false;

    if (m_renderWidget) {
        m_renderWidget->setFullscreenTransitionActive(false);
    }
    if (m_fullScreenBtn) {
        m_fullScreenBtn->setEnabled(true);
    }

    qDebug() << "[VideoPlayerWindow] Fullscreen state settled:"
             << (isImmersiveMaximizeActive() ? "immersive-maximize" : "normal");

    updateButtonStates();
    updateMetaInfo();

    if (m_closePending && !isImmersiveMaximizeActive()) {
        qDebug() << "[VideoPlayerWindow] closePending satisfied after fullscreen exit";
        QTimer::singleShot(0, this, [this]() {
            close();
        });
    }
}

void VideoPlayerWindow::performCloseCleanup() {
    if (m_cleanupDone) {
        return;
    }

    qDebug() << "[VideoPlayerWindow] Cleaning up player resources";
    m_closePending = false;
    if (m_fullScreenTransitionTimer) {
        m_fullScreenTransitionTimer->stop();
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
    m_videoFrameSize = QSize();

    if (m_playPauseBtn) {
        m_playPauseBtn->setEnabled(false);
    }
    if (m_stopBtn) {
        m_stopBtn->setEnabled(false);
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

    updateMetaInfo();
    updateButtonStates();

    if (m_renderWidget) {
        m_renderWidget->stop();
    }

    m_cleanupDone = true;
    qDebug() << "[VideoPlayerWindow] Cleanup finished";
}

void VideoPlayerWindow::onPlaybackRateChanged(int index) {
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

void VideoPlayerWindow::onQualityPresetChanged(int index) {
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
             << (preset == static_cast<int>(VideoRendererGL::Enhanced2K) ? "Enhanced2K"
                                                                         : "Standard1080P");
}

void VideoPlayerWindow::onPlaybackFinished() {
    qDebug() << "[VideoPlayerWindow] Playback finished";

    if (m_mediaSession) {
        m_mediaSession->stop();
    }
    m_replayPendingSeek = true;

    m_isPlaying = false;
    emit playStateChanged(false);
    updateButtonStates();

    m_currentPosition = 0;
    if (m_progressSlider && !m_sliderPressed) {
        m_progressSlider->setValue(0);
    }
    updateTimeLabel(0, m_duration);
}

void VideoPlayerWindow::onSliderReleased() {
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
    updateButtonStates();

    if (!wasPlaying) {
        qDebug() << "[VideoPlayerWindow] Auto-resume playback after seek";
    }

    m_replayPendingSeek = false;

    emit progressChanged(targetPosition);
}

void VideoPlayerWindow::onVideoSizeChanged(const QSize& size) {
    m_videoFrameSize = size;
    if (m_renderWidget) {
        m_renderWidget->setDisplayMode(m_fillDisplayMode ? 1 : 0);
    }
    updateMetaInfo();
}

void VideoPlayerWindow::onMediaSessionStateChanged(MediaSession::PlaybackState state) {
    qDebug() << "[VideoPlayerWindow] State changed:" << static_cast<int>(state);
}

void VideoPlayerWindow::onDeferredSeekAfterStopped() {
    if (m_mediaSession) {
        m_mediaSession->seekTo(m_pendingStoppedSeekPosition);
    }
}

void VideoPlayerWindow::onSliderValueChanged(int value) {
    if (m_sliderPressed) {
        qint64 position = (m_duration * value) / 1000;
        updateTimeLabel(position, m_duration);
    }
}

void VideoPlayerWindow::updateTimeLabel(qint64 currentMs, qint64 totalMs) {
    QString current = formatTime(currentMs);
    QString total = formatTime(totalMs);
    m_timeLabel->setText(QString("%1 / %2").arg(current).arg(total));
}

QString VideoPlayerWindow::formatTime(qint64 ms) {
    int seconds = static_cast<int>((ms / 1000) % 60);
    int minutes = static_cast<int>((ms / (1000 * 60)) % 60);
    int hours = static_cast<int>(ms / (1000 * 60 * 60));

    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    }
}

void VideoPlayerWindow::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);

    if (!event || event->type() != QEvent::WindowStateChange) {
        return;
    }

    qDebug() << "[VideoPlayerWindow] Window state changed. fullscreen:"
             << isImmersiveMaximizeActive() << "transition:" << m_fullScreenTransitionInProgress
             << "target:" << m_targetFullScreenState;

    if (m_fullScreenTransitionInProgress) {
        scheduleFullScreenTransitionSettle();
    }
}

void VideoPlayerWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_fullScreenTransitionInProgress) {
        scheduleFullScreenTransitionSettle();
        return;
    }
    updateResponsiveUi();
}

void VideoPlayerWindow::closeEvent(QCloseEvent* event) {
    qDebug() << "[VideoPlayerWindow] Close requested"
             << "fullscreen:" << isImmersiveMaximizeActive()
             << "transition:" << m_fullScreenTransitionInProgress
             << "closePending:" << m_closePending;

    if (m_cleanupDone) {
        QWidget::closeEvent(event);
        return;
    }

    if (m_fullScreenTransitionInProgress) {
        m_closePending = true;
        qDebug()
            << "[VideoPlayerWindow] Close requested while immersive transition active, deferring";
        event->ignore();
        return;
    }

    performCloseCleanup();
    QWidget::closeEvent(event);
}

void VideoPlayerWindow::keyPressEvent(QKeyEvent* event) {
    if (!event) {
        QWidget::keyPressEvent(event);
        return;
    }

    if (event->key() == Qt::Key_F11) {
        requestFullScreenChange(!isImmersiveMaximizeActive(), "shortcut:F11");
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Escape && isImmersiveMaximizeActive()) {
        requestFullScreenChange(false, "shortcut:Escape");
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void VideoPlayerWindow::onPositionChanged(qint64 positionMs) {
    m_currentPosition = positionMs;

    if (!m_sliderPressed && m_duration > 0) {
        int sliderValue = (positionMs * 1000) / m_duration;
        m_progressSlider->setValue(sliderValue);
    }

    updateTimeLabel(positionMs, m_duration);
}

void VideoPlayerWindow::onDurationChanged(qint64 durationMs) {
    m_duration = durationMs;
    updateTimeLabel(m_currentPosition, m_duration);
    updateMetaInfo();

    qDebug() << "[VideoPlayerWindow] Duration:" << durationMs << "ms";
}

void VideoPlayerWindow::updateMetaInfo() {
    if (!m_metaInfoLabel) {
        return;
    }

    if (m_currentFilePath.isEmpty()) {
        m_metaInfoLabel->setText(
            QStringLiteral(u"\u9009\u62e9\u89c6\u9891\u540e\u5f00\u59cb\u64ad\u653e"));
        return;
    }

    QStringList parts;
    if (m_videoFrameSize.isValid() && !m_videoFrameSize.isEmpty()) {
        parts << QStringLiteral("%1 × %2")
                     .arg(m_videoFrameSize.width())
                     .arg(m_videoFrameSize.height());
    }
    if (m_duration > 0) {
        parts << formatTime(m_duration);
    }
    parts << (m_fillDisplayMode ? QStringLiteral(u"\u586b\u5145")
                                : QStringLiteral(u"\u9002\u5e94"));

    m_metaInfoLabel->setText(parts.join(QStringLiteral("  ·  ")));
}

void VideoPlayerWindow::updateButtonStates() {
    if (m_playPauseBtn) {
        const QString playPauseIcon =
            m_isPlaying ? QStringLiteral("video-pause") : QStringLiteral("video-play");
        m_playPauseBtn->setIcon(videoWindowIcon(playPauseIcon));
        m_playPauseBtn->setIconSize(QSize(20, 20));
        m_playPauseBtn->setText(QString());
        m_playPauseBtn->setToolTip(m_isPlaying ? QStringLiteral(u"\u6682\u505c")
                                               : QStringLiteral(u"\u64ad\u653e"));
    }

    if (m_stopBtn) {
        m_stopBtn->setIcon(videoWindowIcon(QStringLiteral("video-stop")));
        m_stopBtn->setIconSize(QSize(16, 16));
        m_stopBtn->setText(QString());
        m_stopBtn->setToolTip(QStringLiteral(u"\u505c\u6b62"));
    }

    if (m_displayModeBtn) {
        const bool fillMode = m_fillDisplayMode;
        m_displayModeBtn->setIcon(
            videoWindowIcon(fillMode ? QStringLiteral("video-fill") : QStringLiteral("video-fit")));
        m_displayModeBtn->setIconSize(QSize(18, 18));
        m_displayModeBtn->setText(fillMode ? QStringLiteral(u"\u586b\u5145")
                                           : QStringLiteral(u"\u9002\u5e94"));
    }

    if (m_fullScreenBtn) {
        const bool fullScreen = isImmersiveMaximizeActive();
        m_fullScreenBtn->setIcon(videoWindowIcon(fullScreen
                                                     ? QStringLiteral("video-fullscreen-exit")
                                                     : QStringLiteral("video-fullscreen-enter")));
        m_fullScreenBtn->setIconSize(QSize(18, 18));
        m_fullScreenBtn->setText(fullScreen ? QStringLiteral(u"\u9000\u51fa")
                                            : QStringLiteral(u"\u5168\u5c4f"));
        m_fullScreenBtn->setEnabled(!m_fullScreenTransitionInProgress);
    }

    if (!m_fullScreenTransitionInProgress) {
        updateResponsiveUi();
    }
}

void VideoPlayerWindow::updateResponsiveUi() {
    if (m_fullScreenTransitionInProgress) {
        return;
    }

    const int windowWidth = width();
    const bool compact = windowWidth < 1040;
    const bool dense = windowWidth < 900;
    const bool iconOnly = windowWidth < 780;
    const bool immersive = isImmersiveMaximizeActive();

    if (m_titleBar) {
        m_titleBar->setVisible(!immersive);
        m_titleBar->setFixedHeight(compact ? 56 : 60);
    }
    if (m_controlBar) {
        m_controlBar->setFixedHeight(immersive ? (dense ? 90 : 98) : (dense ? 96 : 108));
    }

    if (m_fileNameLabel) {
        m_fileNameLabel->setMinimumWidth(iconOnly ? 160 : 240);
    }

    if (m_metaInfoLabel) {
        m_metaInfoLabel->setVisible(!immersive && !iconOnly);
    }

    if (m_openFileBtn) {
        m_openFileBtn->setText(iconOnly ? QString() : QStringLiteral(u"\u6253\u5f00\u89c6\u9891"));
        m_openFileBtn->setToolTip(QStringLiteral(u"\u6253\u5f00\u89c6\u9891\u6587\u4ef6"));
        m_openFileBtn->setProperty("iconOnly", iconOnly);
        m_openFileBtn->setFixedWidth(iconOnly ? 34 : (compact ? 104 : 116));
    }

    if (m_playPauseBtn) {
        const int buttonSize = dense ? 44 : 50;
        m_playPauseBtn->setFixedSize(buttonSize, buttonSize);
        m_playPauseBtn->setIconSize(QSize(dense ? 18 : 20, dense ? 18 : 20));
    }

    if (m_stopBtn) {
        const int stopSize = dense ? 34 : 38;
        m_stopBtn->setFixedSize(stopSize, stopSize);
        m_stopBtn->setIconSize(QSize(dense ? 14 : 16, dense ? 14 : 16));
    }

    if (m_timeLabel) {
        m_timeLabel->setMinimumWidth(dense ? 96 : 112);
    }

    if (m_qualityIconLabel) {
        m_qualityIconLabel->setVisible(!iconOnly);
    }
    if (m_rateIconLabel) {
        m_rateIconLabel->setVisible(!iconOnly);
    }
    if (m_qualityLabel) {
        m_qualityLabel->setVisible(!dense);
    }
    if (m_rateLabel) {
        m_rateLabel->setVisible(!dense);
    }

    if (m_qualityPresetBox) {
        m_qualityPresetBox->setMinimumWidth(iconOnly ? 82 : (dense ? 92 : 108));
    }

    if (m_playbackRateBox) {
        m_playbackRateBox->setMinimumWidth(iconOnly ? 70 : (dense ? 80 : 88));
    }

    if (m_displayModeBtn) {
        m_displayModeBtn->setProperty("iconOnly", dense);
        m_displayModeBtn->setToolTip(m_fillDisplayMode
                                         ? QStringLiteral(u"\u586b\u5145\u663e\u793a")
                                         : QStringLiteral(u"\u9002\u5e94\u663e\u793a"));
        m_displayModeBtn->setFixedWidth(dense ? 38 : (compact ? 82 : 92));
    }

    if (m_fullScreenBtn) {
        m_fullScreenBtn->setProperty("iconOnly", dense);
        m_fullScreenBtn->setToolTip(isImmersiveMaximizeActive()
                                        ? QStringLiteral(u"\u9000\u51fa\u6c89\u6d78\u6a21\u5f0f")
                                        : QStringLiteral(u"\u5168\u5c4f"));
        m_fullScreenBtn->setFixedWidth(dense ? 38 : (compact ? 72 : 82));
    }

    auto repolish = [](QWidget* widget) {
        if (!widget || !widget->style()) {
            return;
        }
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    };

    repolish(m_openFileBtn);
    repolish(m_playPauseBtn);
    repolish(m_displayModeBtn);
    repolish(m_fullScreenBtn);
    repolish(m_qualityPresetBox);
    repolish(m_playbackRateBox);
}

bool VideoPlayerWindow::isImmersiveMaximizeActive() const {
    return m_immersiveMaximizeActive;
}
