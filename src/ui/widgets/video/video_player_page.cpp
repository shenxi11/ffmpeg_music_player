#include "video_player_page.h"

#include "VideoSession.h"

#include <QComboBox>
#include <QCursor>
#include <QDebug>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

namespace {
constexpr int kRendererRecoveryDelayMs = 50;
constexpr int kOverlayHideDelayMs = 180;

QPushButton* createActionChip(const QString& text, QWidget* parent, bool enabled) {
    auto* button = new QPushButton(text, parent);
    button->setEnabled(enabled);
    button->setCursor(enabled ? Qt::PointingHandCursor : Qt::ArrowCursor);
    return button;
}

QString fileNameFromSource(const QString& source) {
    const QFileInfo info(source);
    const QString fileName = info.completeBaseName().trimmed();
    return fileName.isEmpty() ? info.fileName().trimmed() : fileName;
}

QString leafNameFromSource(const QString& source) {
    const QFileInfo info(source);
    const QString fileName = info.fileName().trimmed();
    return fileName.isEmpty() ? source.trimmed() : fileName;
}

bool sourceIsLocal(const QString& sourcePath, const QString& currentFilePath) {
    return QFileInfo(sourcePath).isAbsolute() ||
           currentFilePath.startsWith(QStringLiteral("file:///"), Qt::CaseInsensitive);
}
} // namespace

VideoPlayerPage::VideoPlayerPage(QWidget* parent) : QWidget(parent) {
    setupUi();
    setFocusPolicy(Qt::StrongFocus);
}

VideoPlayerPage::~VideoPlayerPage() {
    if (m_mediaSession) {
        m_mediaSession->stop();
        delete m_mediaSession;
        m_mediaSession = nullptr;
    }
}

bool VideoPlayerPage::hasLoadedVideo() const {
    return !m_currentFilePath.trimmed().isEmpty() && m_mediaSession;
}

void VideoPlayerPage::setVideoInfo(const QString& title, const QString& sourcePath,
                                   qint64 sizeBytes) {
    m_displayTitle = title.trimmed();
    m_sourcePath = sourcePath.trimmed();
    m_sourceSizeBytes = qMax<qint64>(0, sizeBytes);
    updateInfoPanel();
}

void VideoPlayerPage::configureRendererWidget(VideoRendererGL* renderer) {
    if (!renderer) {
        return;
    }

    renderer->setDisplayMode(m_fillDisplayMode ? 1 : 0);
    const int qualityPreset = (m_qualityPresetBox && m_qualityPresetBox->currentIndex() >= 0)
                                  ? m_qualityPresetBox->currentData().toInt()
                                  : static_cast<int>(VideoRendererGL::Standard1080P);
    renderer->setQualityPreset(qualityPreset);
    renderer->setAttribute(Qt::WA_Hover, true);
    renderer->setMouseTracking(true);
    renderer->installEventFilter(this);

    connect(renderer, &VideoRendererGL::videoSizeChanged, this,
            &VideoPlayerPage::onVideoSizeChanged);
}

void VideoPlayerPage::setupUi() {
    setObjectName(QStringLiteral("VideoPlayerPage"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("QWidget#VideoPlayerPage { background: transparent; }"
                  "QWidget#VideoPageCard {"
                  "  background: #FFFFFF;"
                  "  border: 1px solid #EEF1F5;"
                  "  border-radius: 0px;"
                  "}"
                  "QWidget#VideoStageCard {"
                  "  background: #05070B;"
                  "  border-radius: 0px;"
                  "}"
                  "QWidget#VideoStageRenderHost {"
                  "  background: #05070B;"
                  "  border: none;"
                  "}"
                  "QWidget#VideoStageChromeHost {"
                  "  background: transparent;"
                  "  border: none;"
                  "}"
                  "QWidget#VideoControlBar {"
                  "  background: #0B0D12;"
                  "  border: none;"
                  "}"
                  "QWidget#VideoStageTitlePanel {"
                  "  background: transparent;"
                  "}"
                  "QPushButton#VideoCenterPlayButton {"
                  "  min-width: 68px;"
                  "  min-height: 68px;"
                  "  max-width: 68px;"
                  "  max-height: 68px;"
                  "  border-radius: 34px;"
                  "  border: 1px solid rgba(255,255,255,0.82);"
                  "  background: rgba(15,18,23,0.36);"
                  "  color: #FFFFFF;"
                  "  font-size: 26px;"
                  "  font-weight: 700;"
                  "}"
                  "QPushButton#VideoCenterPlayButton:hover {"
                  "  background: rgba(236,65,65,0.82);"
                  "  border-color: rgba(255,255,255,0.92);"
                  "}"
                  "QPushButton#VideoOverlayIconButton {"
                  "  min-width: 32px;"
                  "  min-height: 32px;"
                  "  max-width: 32px;"
                  "  max-height: 32px;"
                  "  border-radius: 16px;"
                  "  border: none;"
                  "  background: transparent;"
                  "}"
                  "QPushButton#VideoOverlayIconButton:hover {"
                  "  background: rgba(255,255,255,0.10);"
                  "}"
                  "QPushButton#VideoQualityButton {"
                  "  min-height: 24px;"
                  "  padding: 0 10px;"
                  "  border-radius: 12px;"
                  "  border: 1px solid rgba(255,255,255,0.34);"
                  "  background: transparent;"
                  "  color: #F8FAFD;"
                  "  font-size: 12px;"
                  "}"
                  "QPushButton#VideoQualityButton:hover {"
                  "  border-color: rgba(255,255,255,0.62);"
                  "  background: rgba(255,255,255,0.08);"
                  "}"
                  "QPushButton#VideoStageArrowButton {"
                  "  min-width: 44px;"
                  "  min-height: 44px;"
                  "  max-width: 44px;"
                  "  max-height: 44px;"
                  "  border-radius: 22px;"
                  "  border: none;"
                  "  background: rgba(255,255,255,0.06);"
                  "}"
                  "QPushButton#VideoStageArrowButton:hover {"
                  "  background: rgba(255,255,255,0.14);"
                  "}"
                  "QSlider#VideoProgressSlider {"
                  "  background: transparent;"
                  "  border: none;"
                  "  padding: 0;"
                  "}"
                  "QSlider#VideoProgressSlider::groove:horizontal {"
                  "  height: 4px;"
                  "  border-radius: 2px;"
                  "  background: rgba(255,255,255,0.14);"
                  "}"
                  "QSlider#VideoProgressSlider::sub-page:horizontal {"
                  "  border-radius: 2px;"
                  "  background: #1ECE9B;"
                  "}"
                  "QSlider#VideoProgressSlider::add-page:horizontal {"
                  "  border-radius: 2px;"
                  "  background: rgba(255,255,255,0.14);"
                  "}"
                  "QSlider#VideoProgressSlider::handle:horizontal {"
                  "  width: 10px;"
                  "  height: 10px;"
                  "  margin: -3px 0;"
                  "  border-radius: 5px;"
                  "  border: none;"
                  "  background: #FFFFFF;"
                  "}"
                  "QSlider#VideoProgressSlider:disabled {"
                  "  background: transparent;"
                  "}"
                  "QSlider#VideoProgressSlider::groove:horizontal:disabled {"
                  "  background: rgba(255,255,255,0.14);"
                  "}"
                  "QSlider#VideoProgressSlider::sub-page:horizontal:disabled {"
                  "  background: rgba(30,206,155,0.58);"
                  "}"
                  "QSlider#VideoProgressSlider::add-page:horizontal:disabled {"
                  "  background: rgba(255,255,255,0.14);"
                  "}"
                  "QWidget#VideoInfoPanel {"
                  "  background: #FBFBFB;"
                  "  border-top: 1px solid #F0F2F6;"
                  "}"
                  "QPushButton#VideoActionChip {"
                  "  min-height: 34px;"
                  "  padding: 0 16px;"
                  "  border-radius: 17px;"
                  "  border: 1px solid #E9EDF2;"
                  "  background: #F2F4F7;"
                  "  color: #495261;"
                  "}"
                  "QPushButton#VideoActionChip:hover:enabled {"
                  "  background: #E9EDF2;"
                  "  color: #20242C;"
                  "}"
                  "QPushButton#VideoActionChip:disabled {"
                  "  color: #A2A8B2;"
                  "  background: #F2F4F7;"
                  "  border-color: #E9EDF2;"
                  "}"
                  "QPushButton#VideoReportButton {"
                  "  border: none;"
                  "  background: transparent;"
                  "  color: #9CA3AF;"
                  "  padding: 0;"
                  "  min-height: 20px;"
                  "}"
                  "QPushButton#VideoReportButton:hover {"
                  "  color: #6B7280;"
                  "}");

    auto* pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(18, 18, 18, 18);
    pageLayout->setSpacing(0);

    m_contentCard = new QWidget(this);
    m_contentCard->setObjectName(QStringLiteral("VideoPageCard"));
    auto* contentLayout = new QVBoxLayout(m_contentCard);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    m_playerContainer = new QWidget(m_contentCard);
    auto* playerLayout = new QVBoxLayout(m_playerContainer);
    playerLayout->setContentsMargins(0, 0, 0, 0);
    playerLayout->setSpacing(0);

    m_stageCard = new QWidget(m_playerContainer);
    m_stageCard->setObjectName(QStringLiteral("VideoStageCard"));
    m_stageCard->setMinimumHeight(320);
    m_stageCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_stageCard->setAttribute(Qt::WA_Hover, true);
    m_stageCard->setMouseTracking(true);
    m_stageCard->installEventFilter(this);

    m_stageRenderHost = new QWidget(m_stageCard);
    m_stageRenderHost->setObjectName(QStringLiteral("VideoStageRenderHost"));
    m_stageRenderHost->setAttribute(Qt::WA_Hover, true);
    m_stageRenderHost->setMouseTracking(true);
    m_stageRenderHost->installEventFilter(this);

    m_renderWidget = new VideoRendererGL(m_stageRenderHost);
    configureRendererWidget(m_renderWidget);

    m_stageChromeHost = new QWidget(m_stageCard);
    m_stageChromeHost->setObjectName(QStringLiteral("VideoStageChromeHost"));
    m_stageChromeHost->setAttribute(Qt::WA_Hover, true);
    m_stageChromeHost->setMouseTracking(true);
    m_stageChromeHost->installEventFilter(this);

    m_videoFullscreenHost = new QWidget(this, Qt::Window | Qt::FramelessWindowHint);
    m_videoFullscreenHost->setObjectName(QStringLiteral("VideoFullscreenHost"));
    m_videoFullscreenHost->setAttribute(Qt::WA_DeleteOnClose, false);
    m_videoFullscreenHost->setAttribute(Qt::WA_StyledBackground, true);
    m_videoFullscreenHost->setStyleSheet(QStringLiteral("background:#000000;"));
    m_videoFullscreenHost->setMouseTracking(true);
    m_videoFullscreenHost->installEventFilter(this);
    m_videoFullscreenHost->hide();

    m_videoFullscreenStageHost = new QWidget(m_videoFullscreenHost);
    m_videoFullscreenStageHost->setObjectName(QStringLiteral("VideoFullscreenStageHost"));
    m_videoFullscreenStageHost->setAttribute(Qt::WA_Hover, true);
    m_videoFullscreenStageHost->setMouseTracking(true);
    m_videoFullscreenStageHost->installEventFilter(this);

    m_fullscreenRenderWidget = new VideoRendererGL(m_videoFullscreenStageHost);
    configureRendererWidget(m_fullscreenRenderWidget);
    m_fullscreenRenderWidget->hide();

    m_placeholderPanel = new QWidget(m_stageChromeHost);
    auto* placeholderLayout = new QVBoxLayout(m_placeholderPanel);
    placeholderLayout->setContentsMargins(0, 0, 0, 0);
    placeholderLayout->setSpacing(8);

    auto* placeholderIcon = new QLabel(QStringLiteral("▶"), m_placeholderPanel);
    placeholderIcon->setAlignment(Qt::AlignCenter);
    placeholderIcon->setStyleSheet("color:#FFFFFF; font-size:28px; font-weight:700;");

    m_placeholderTitleLabel =
        new QLabel(QStringLiteral("从左侧列表选择视频开始播放"), m_placeholderPanel);
    m_placeholderTitleLabel->setAlignment(Qt::AlignCenter);
    m_placeholderTitleLabel->setStyleSheet("color:#F5F7FA; font-size:18px; font-weight:600;");

    m_placeholderSubtitleLabel = new QLabel(
        QStringLiteral("视频会直接显示在主页面，不再弹出独立窗口。"), m_placeholderPanel);
    m_placeholderSubtitleLabel->setAlignment(Qt::AlignCenter);
    m_placeholderSubtitleLabel->setStyleSheet("color:rgba(245,247,250,0.68); font-size:13px;");
    m_placeholderPanel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    placeholderLayout->addWidget(placeholderIcon);
    placeholderLayout->addWidget(m_placeholderTitleLabel);
    placeholderLayout->addWidget(m_placeholderSubtitleLabel);

    m_stageTitlePanel = new QWidget(m_stageChromeHost);
    m_stageTitlePanel->setObjectName(QStringLiteral("VideoStageTitlePanel"));
    m_stageTitlePanel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    auto* stageTitleLayout = new QVBoxLayout(m_stageTitlePanel);
    stageTitleLayout->setContentsMargins(0, 0, 0, 0);
    stageTitleLayout->setSpacing(4);

    m_stageTitleLabel = new QLabel(QStringLiteral("未选择视频"), m_stageTitlePanel);
    m_stageTitleLabel->setStyleSheet(
        "color:#FFFFFF; font-size:26px; font-weight:700; letter-spacing:0.5px;");
    m_stageTitleLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    m_stageMetaLabel = new QLabel(QStringLiteral("FFmpeg Music · 视频"), m_stageTitlePanel);
    m_stageMetaLabel->setStyleSheet(
        "color:rgba(255,255,255,0.82); font-size:13px; font-style:italic; font-weight:600;");
    m_stageMetaLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    stageTitleLayout->addWidget(m_stageTitleLabel);
    stageTitleLayout->addWidget(m_stageMetaLabel);
    m_stageTitlePanel->hide();

    m_watermarkLabel = new QLabel(QStringLiteral("FFmpeg Music · 视频"), m_stageChromeHost);
    m_watermarkLabel->setStyleSheet("color:rgba(255,255,255,0.88);"
                                    "font-size:12px;"
                                    "font-weight:600;"
                                    "background:rgba(12,14,18,0.54);"
                                    "padding:8px 16px;"
                                    "border-radius:18px;");
    m_watermarkLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_watermarkLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    m_watermarkLabel->setAttribute(Qt::WA_NoSystemBackground, true);
    m_watermarkLabel->setAutoFillBackground(false);

    m_subtitleOverlayLabel = new QLabel(m_stageChromeHost);
    m_subtitleOverlayLabel->setStyleSheet("color:rgba(228,232,238,0.86); font-size:13px; "
                                          "background:transparent; border:none; padding:0;");
    m_subtitleOverlayLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_subtitleOverlayLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_subtitleOverlayLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    m_subtitleOverlayLabel->setAttribute(Qt::WA_NoSystemBackground, true);
    m_subtitleOverlayLabel->setAutoFillBackground(false);

    m_centerPlayButton = new QPushButton(QStringLiteral("▶"), m_stageChromeHost);
    m_centerPlayButton->setObjectName(QStringLiteral("VideoCenterPlayButton"));
    m_centerPlayButton->setCursor(Qt::PointingHandCursor);
    m_centerPlayButton->setIcon(QIcon(QStringLiteral(":/qml/assets/ai/icons/video-play.svg")));
    m_centerPlayButton->setIconSize(QSize(28, 28));
    m_centerPlayButton->setText(QString());

    m_stageNextButton = new QPushButton(m_stageChromeHost);
    m_stageNextButton->setObjectName(QStringLiteral("VideoStageArrowButton"));
    m_stageNextButton->setCursor(Qt::PointingHandCursor);
    m_stageNextButton->setIcon(
        QIcon(QStringLiteral(":/qml/assets/ai/icons/video-chevron-right.svg")));
    m_stageNextButton->setIconSize(QSize(28, 28));
    m_stageNextButton->installEventFilter(this);
    m_stageNextButton->setVisible(false);

    m_controlBar = new QWidget(m_playerContainer);
    m_controlBar->setObjectName(QStringLiteral("VideoControlBar"));
    m_controlBar->setAttribute(Qt::WA_StyledBackground, true);
    m_controlBar->setFixedHeight(68);
    m_controlBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* controlLayout = new QVBoxLayout(m_controlBar);
    controlLayout->setContentsMargins(14, 8, 14, 8);
    controlLayout->setSpacing(6);

    auto* progressRow = new QHBoxLayout();
    progressRow->setContentsMargins(0, 0, 0, 0);
    progressRow->setSpacing(10);

    m_progressSlider = new QSlider(Qt::Horizontal, m_controlBar);
    m_progressSlider->setObjectName(QStringLiteral("VideoProgressSlider"));
    m_progressSlider->setAttribute(Qt::WA_StyledBackground, true);
    m_progressSlider->setAutoFillBackground(false);
    m_progressSlider->setStyleSheet(QStringLiteral(
        "QSlider { background: transparent; border: none; padding: 0; }"
        "QSlider::groove:horizontal { height: 4px; border-radius: 2px; background: "
        "rgba(255,255,255,0.14); }"
        "QSlider::sub-page:horizontal { border-radius: 2px; background: #1ECE9B; }"
        "QSlider::add-page:horizontal { border-radius: 2px; background: rgba(255,255,255,0.14); }"
        "QSlider::handle:horizontal { width: 10px; height: 10px; margin: -3px 0; border-radius: "
        "5px; border: none; background: #FFFFFF; }"
        "QSlider:disabled { background: transparent; }"
        "QSlider::groove:horizontal:disabled { background: rgba(255,255,255,0.14); }"
        "QSlider::sub-page:horizontal:disabled { background: rgba(30,206,155,0.58); }"
        "QSlider::add-page:horizontal:disabled { background: rgba(255,255,255,0.14); }"));
    m_progressSlider->setRange(0, 1000);
    m_progressSlider->setValue(0);

    m_timeLabel = new QLabel(QStringLiteral("00:00 / 00:00"), m_controlBar);
    m_timeLabel->setObjectName(QStringLiteral("VideoTimeLabel"));
    m_timeLabel->setStyleSheet(
        "color:rgba(248,250,253,0.78); font-size:12px; background: transparent; border: none;");
    m_timeLabel->setMinimumWidth(110);
    m_timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    progressRow->addWidget(m_progressSlider, 1);
    progressRow->addWidget(m_timeLabel, 0, Qt::AlignVCenter);

    auto* controlsRow = new QHBoxLayout();
    controlsRow->setContentsMargins(0, 0, 0, 0);
    controlsRow->setSpacing(0);

    auto* leftControlsLayout = new QHBoxLayout();
    leftControlsLayout->setContentsMargins(0, 0, 0, 0);
    leftControlsLayout->setSpacing(14);

    auto* rightControlsLayout = new QHBoxLayout();
    rightControlsLayout->setContentsMargins(0, 0, 0, 0);
    rightControlsLayout->setSpacing(12);

    m_playPauseButton = new QPushButton(m_controlBar);
    m_playPauseButton->setObjectName(QStringLiteral("VideoOverlayIconButton"));
    m_playPauseButton->setEnabled(true);
    m_playPauseButton->setIconSize(QSize(20, 20));
    m_playPauseButton->setText(QString());

    m_nextStubButton = new QPushButton(m_controlBar);
    m_nextStubButton->setObjectName(QStringLiteral("VideoOverlayIconButton"));
    m_nextStubButton->setEnabled(true);
    m_nextStubButton->setIcon(
        QIcon(QStringLiteral(":/qml/assets/ai/icons/video-control-next.svg")));
    m_nextStubButton->setIconSize(QSize(20, 20));
    m_nextStubButton->setText(QString());

    m_volumeStubButton = new QPushButton(m_controlBar);
    m_volumeStubButton->setObjectName(QStringLiteral("VideoOverlayIconButton"));
    m_volumeStubButton->setEnabled(true);
    m_volumeStubButton->setIcon(
        QIcon(QStringLiteral(":/qml/assets/ai/icons/video-control-volume.svg")));
    m_volumeStubButton->setIconSize(QSize(20, 20));
    m_volumeStubButton->setText(QString());

    m_playlistStubButton = new QPushButton(m_controlBar);
    m_playlistStubButton->setObjectName(QStringLiteral("VideoOverlayIconButton"));
    m_playlistStubButton->setEnabled(true);
    m_playlistStubButton->setIcon(
        QIcon(QStringLiteral(":/qml/assets/ai/icons/video-control-playlist.svg")));
    m_playlistStubButton->setIconSize(QSize(20, 20));
    m_playlistStubButton->setText(QString());

    m_qualityButton = new QPushButton(QStringLiteral("超清"), m_controlBar);
    m_qualityButton->setObjectName(QStringLiteral("VideoQualityButton"));
    m_qualityButton->setCursor(Qt::PointingHandCursor);

    m_pipStubButton = new QPushButton(m_controlBar);
    m_pipStubButton->setObjectName(QStringLiteral("VideoOverlayIconButton"));
    m_pipStubButton->setEnabled(true);
    m_pipStubButton->setIcon(QIcon(QStringLiteral(":/qml/assets/ai/icons/video-pip.svg")));
    m_pipStubButton->setIconSize(QSize(20, 20));
    m_pipStubButton->setText(QString());

    m_qualityPresetBox = new QComboBox(m_controlBar);
    m_qualityPresetBox->addItem(QStringLiteral("高清"),
                                static_cast<int>(VideoRendererGL::Standard1080P));
    m_qualityPresetBox->addItem(QStringLiteral("超清"),
                                static_cast<int>(VideoRendererGL::Enhanced2K));
    m_qualityPresetBox->setCurrentIndex(1);
    m_qualityPresetBox->setVisible(false);

    m_playbackRateBox = new QComboBox(m_controlBar);
    m_playbackRateBox->addItem(QStringLiteral("0.5x"), 0.5);
    m_playbackRateBox->addItem(QStringLiteral("0.75x"), 0.75);
    m_playbackRateBox->addItem(QStringLiteral("1.0x"), 1.0);
    m_playbackRateBox->addItem(QStringLiteral("1.25x"), 1.25);
    m_playbackRateBox->addItem(QStringLiteral("1.5x"), 1.5);
    m_playbackRateBox->addItem(QStringLiteral("2.0x"), 2.0);
    m_playbackRateBox->setCurrentIndex(2);
    m_playbackRateBox->setEnabled(false);
    m_playbackRateBox->setVisible(false);

    m_displayModeButton = new QPushButton(m_controlBar);
    m_displayModeButton->setVisible(false);

    m_fullScreenButton = new QPushButton(m_controlBar);
    m_fullScreenButton->setObjectName(QStringLiteral("VideoOverlayIconButton"));
    m_fullScreenButton->setCursor(Qt::PointingHandCursor);
    m_fullScreenButton->setIconSize(QSize(20, 20));
    m_fullScreenButton->setText(QString());

    leftControlsLayout->addWidget(m_playPauseButton);
    leftControlsLayout->addWidget(m_nextStubButton);

    rightControlsLayout->addWidget(m_volumeStubButton);
    rightControlsLayout->addWidget(m_playlistStubButton);
    rightControlsLayout->addWidget(m_qualityButton);
    rightControlsLayout->addWidget(m_pipStubButton);
    rightControlsLayout->addWidget(m_fullScreenButton);

    controlsRow->addLayout(leftControlsLayout);
    controlsRow->addStretch();
    controlsRow->addLayout(rightControlsLayout);

    controlLayout->addLayout(progressRow);
    controlLayout->addLayout(controlsRow);

    playerLayout->addWidget(m_stageCard);
    playerLayout->addWidget(m_controlBar);

    contentLayout->addWidget(m_playerContainer);

    m_infoPanel = new QWidget(m_contentCard);
    m_infoPanel->setObjectName(QStringLiteral("VideoInfoPanel"));
    auto* infoLayout = new QHBoxLayout(m_infoPanel);
    infoLayout->setContentsMargins(28, 24, 28, 22);
    infoLayout->setSpacing(16);

    auto* infoWidget = new QWidget(m_infoPanel);
    auto* infoTextHolder = new QVBoxLayout(infoWidget);
    infoTextHolder->setContentsMargins(0, 0, 0, 0);
    infoTextHolder->setSpacing(8);

    auto* footerRightWidget = new QWidget(m_infoPanel);
    auto* footerRightLayout = new QVBoxLayout(footerRightWidget);
    footerRightLayout->setContentsMargins(0, 0, 0, 0);
    footerRightLayout->setSpacing(0);
    footerRightLayout->addStretch();

    auto* reportLayout = new QHBoxLayout();
    reportLayout->setContentsMargins(0, 0, 0, 2);
    reportLayout->setSpacing(0);
    reportLayout->addStretch();

    m_titleLabel = new QLabel(QStringLiteral("未选择视频"), infoWidget);
    m_titleLabel->setStyleSheet("color:#20242C; font-size:17px; font-weight:600;");

    m_metaLabel = new QLabel(QStringLiteral("当前只展示可用的文件与来源信息。"), infoWidget);
    m_metaLabel->setWordWrap(false);
    m_metaLabel->setStyleSheet("color:#68707E; font-size:13px;");
    m_metaLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_metaLabel->setMinimumWidth(0);

    auto* actionLayout = new QHBoxLayout();
    actionLayout->setContentsMargins(0, 8, 0, 0);
    actionLayout->setSpacing(12);

    m_favoriteActionButton = createActionChip(QStringLiteral("收藏"), infoWidget, false);
    m_downloadActionButton = createActionChip(QStringLiteral("下载"), infoWidget, false);
    m_shareActionButton = createActionChip(QStringLiteral("分享"), infoWidget, false);
    m_commentActionButton = createActionChip(QStringLiteral("评论"), infoWidget, false);
    m_openFileButton = new QPushButton(QStringLiteral("打开本地视频"), infoWidget);
    m_openFileButton->setCursor(Qt::PointingHandCursor);
    m_openFileButton->setEnabled(true);
    m_openFileButton->setObjectName(QStringLiteral("VideoActionChip"));
    m_favoriteActionButton->setEnabled(true);
    m_downloadActionButton->setEnabled(true);
    m_shareActionButton->setEnabled(true);
    m_commentActionButton->setEnabled(true);
    m_favoriteActionButton->setObjectName(QStringLiteral("VideoActionChip"));
    m_downloadActionButton->setObjectName(QStringLiteral("VideoActionChip"));
    m_shareActionButton->setObjectName(QStringLiteral("VideoActionChip"));
    m_commentActionButton->setObjectName(QStringLiteral("VideoActionChip"));
    m_reportActionButton = nullptr;

    m_favoriteActionButton->setIcon(
        QIcon(QStringLiteral(":/qml/assets/ai/icons/song-action-favorite-default.svg")));
    m_favoriteActionButton->setIconSize(QSize(16, 16));
    m_downloadActionButton->setIcon(QIcon(QStringLiteral(":/qml/assets/ai/icons/download.svg")));
    m_downloadActionButton->setIconSize(QSize(16, 16));
    m_shareActionButton->setIcon(QIcon(QStringLiteral(":/qml/assets/ai/icons/action-share.svg")));
    m_shareActionButton->setIconSize(QSize(16, 16));
    m_commentActionButton->setIcon(
        QIcon(QStringLiteral(":/qml/assets/ai/icons/action-comment.svg")));
    m_commentActionButton->setIconSize(QSize(16, 16));
    m_openFileButton->setIcon(QIcon(QStringLiteral(":/qml/assets/ai/icons/action-open.svg")));
    m_openFileButton->setIconSize(QSize(16, 16));

    actionLayout->addWidget(m_favoriteActionButton);
    actionLayout->addWidget(m_downloadActionButton);
    actionLayout->addWidget(m_shareActionButton);
    actionLayout->addWidget(m_commentActionButton);
    actionLayout->addWidget(m_openFileButton);
    actionLayout->addStretch();

    infoTextHolder->addWidget(m_titleLabel);
    infoTextHolder->addWidget(m_metaLabel);
    infoTextHolder->addLayout(actionLayout);
    infoTextHolder->addStretch();

    m_reportActionButton = createActionChip(QStringLiteral("举报"), infoWidget, false);
    m_reportActionButton->setObjectName(QStringLiteral("VideoReportButton"));
    m_reportActionButton->setEnabled(true);

    reportLayout->addWidget(m_reportActionButton, 0, Qt::AlignBottom);
    footerRightLayout->addLayout(reportLayout);

    infoLayout->addWidget(infoWidget, 1);
    infoLayout->addWidget(footerRightWidget, 0);

    contentLayout->addWidget(m_infoPanel);
    pageLayout->addWidget(m_contentCard, 1);

    m_overlayHideTimer = new QTimer(this);
    m_overlayHideTimer->setSingleShot(true);
    m_overlayHideTimer->setInterval(kOverlayHideDelayMs);
    connect(m_overlayHideTimer, &QTimer::timeout, this, [this]() {
        setStageHovered(false);
    });

    connectUiSignals();
    resetPlaybackUi();
    updateInfoPanel();
    updateStageState();
}

void VideoPlayerPage::connectUiSignals() {
    if (m_backButton) {
        connect(m_backButton, &QPushButton::clicked, this, [this]() {
            pausePlayback();
            emit backRequested();
        });
    }
    connect(m_centerPlayButton, &QPushButton::clicked, this, &VideoPlayerPage::onPlayPauseClicked);
    connect(m_playPauseButton, &QPushButton::clicked, this, &VideoPlayerPage::onPlayPauseClicked);
    connect(m_qualityButton, &QPushButton::clicked, this, [this]() {
        if (!m_qualityPresetBox || m_qualityPresetBox->count() <= 0) {
            return;
        }
        const int nextIndex =
            (m_qualityPresetBox->currentIndex() + 1) % m_qualityPresetBox->count();
        m_qualityPresetBox->setCurrentIndex(nextIndex);
        onQualityPresetChanged(nextIndex);
        updatePlaybackButtons();
    });
    connect(m_openFileButton, &QPushButton::clicked, this, &VideoPlayerPage::onOpenFileClicked);
    connect(m_fullScreenButton, &QPushButton::clicked, this, &VideoPlayerPage::onFullScreenClicked);
    connect(m_progressSlider, &QSlider::sliderPressed, this, &VideoPlayerPage::onSliderPressed);
    connect(m_progressSlider, &QSlider::sliderReleased, this, &VideoPlayerPage::onSliderReleased);
    connect(m_progressSlider, &QSlider::valueChanged, this, &VideoPlayerPage::onSliderValueChanged);
    connect(m_qualityPresetBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &VideoPlayerPage::onQualityPresetChanged);
    connect(m_playbackRateBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &VideoPlayerPage::onPlaybackRateChanged);
}

void VideoPlayerPage::connectMediaSessionSignals() {
    if (!m_mediaSession) {
        return;
    }

    connect(m_mediaSession, &MediaSession::positionChanged, this,
            &VideoPlayerPage::onPositionChanged);
    connect(m_mediaSession, &MediaSession::durationChanged, this,
            &VideoPlayerPage::onDurationChanged);
    connect(m_mediaSession, &MediaSession::playbackFinished, this,
            &VideoPlayerPage::onPlaybackFinished);
    connect(m_mediaSession, &MediaSession::stateChanged, this,
            &VideoPlayerPage::onMediaSessionStateChanged);
}

void VideoPlayerPage::attachWindowObserver() {
    QWidget* hostWindow = window();
    if (hostWindow == m_observedWindow || !hostWindow) {
        return;
    }

    if (m_observedWindow) {
        m_observedWindow->removeEventFilter(this);
    }
    m_observedWindow = hostWindow;
    m_observedWindow->installEventFilter(this);
    syncHostWindowButtonText();
}

VideoRendererGL* VideoPlayerPage::activeRenderer() const {
    if (m_videoFullscreenActive && m_fullscreenRenderWidget) {
        return m_fullscreenRenderWidget;
    }
    return m_renderWidget;
}

void VideoPlayerPage::applyRendererPlaybackState(VideoRendererGL* renderer,
                                                 MediaSession::PlaybackState state) {
    if (!renderer) {
        return;
    }

    switch (state) {
        case MediaSession::Playing:
            renderer->start();
            break;
        case MediaSession::Paused:
            renderer->pause();
            break;
        case MediaSession::Stopped:
        case MediaSession::Error:
        default:
            renderer->stop();
            break;
    }
}

void VideoPlayerPage::moveControlBarToFullscreenHost() {
    if (!m_controlBar || !m_videoFullscreenHost) {
        return;
    }

    if (QLayout* playerLayout = m_playerContainer ? m_playerContainer->layout() : nullptr) {
        playerLayout->removeWidget(m_controlBar);
    }

    m_controlBar->setParent(m_videoFullscreenHost);
    m_controlBar->show();
    m_controlBar->raise();
}

void VideoPlayerPage::restoreControlBarToEmbeddedHost() {
    if (!m_controlBar || !m_playerContainer) {
        return;
    }

    m_controlBar->setParent(m_playerContainer);
    if (QVBoxLayout* playerLayout = qobject_cast<QVBoxLayout*>(m_playerContainer->layout())) {
        playerLayout->removeWidget(m_controlBar);
        playerLayout->addWidget(m_controlBar);
    }
    m_controlBar->show();
}

void VideoPlayerPage::switchActiveRenderer() {
    VideoSession* videoSession = m_mediaSession ? m_mediaSession->videoSession() : nullptr;
    VideoRendererGL* renderer = activeRenderer();
    if (!videoSession || !renderer) {
        return;
    }

    const MediaSession::PlaybackState state = m_mediaSession->state();
    qDebug() << "[VideoPlayerPage] switchActiveRenderer fullscreen:" << m_videoFullscreenActive
             << "renderer:" << static_cast<void*>(renderer) << "state:" << static_cast<int>(state);

    renderer->setDisplayMode(m_fillDisplayMode ? 1 : 0);
    if (m_qualityPresetBox && m_qualityPresetBox->currentIndex() >= 0) {
        renderer->setQualityPreset(m_qualityPresetBox->currentData().toInt());
    }
    videoSession->setVideoRenderer(renderer);
    renderer->show();
    renderer->refreshAfterSurfaceChange();
    applyRendererPlaybackState(renderer, state);
    renderer->update();

    if (renderer != m_renderWidget && m_renderWidget) {
        applyRendererPlaybackState(m_renderWidget, MediaSession::Paused);
    }
    if (renderer != m_fullscreenRenderWidget && m_fullscreenRenderWidget) {
        applyRendererPlaybackState(m_fullscreenRenderWidget, MediaSession::Paused);
    }
}

void VideoPlayerPage::resetPlaybackUi() {
    m_isPlaying = false;
    m_sliderPressed = false;
    m_replayPendingSeek = false;
    m_fillDisplayMode = false;
    m_duration = 0;
    m_currentPosition = 0;
    m_pendingStoppedSeekPosition = 0;

    if (m_progressSlider) {
        m_progressSlider->setValue(0);
    }
    if (m_qualityPresetBox) {
        m_qualityPresetBox->setCurrentIndex(1);
        m_qualityPresetBox->setEnabled(false);
    }
    if (m_playbackRateBox) {
        m_playbackRateBox->setCurrentIndex(2);
        m_playbackRateBox->setEnabled(false);
    }
    if (m_playPauseButton) {
        m_playPauseButton->setEnabled(false);
    }
    if (m_renderWidget) {
        m_renderWidget->setDisplayMode(0);
        m_renderWidget->setQualityPreset(static_cast<int>(VideoRendererGL::Standard1080P));
    }
    if (m_fullscreenRenderWidget) {
        m_fullscreenRenderWidget->setDisplayMode(0);
        m_fullscreenRenderWidget->setQualityPreset(
            static_cast<int>(VideoRendererGL::Standard1080P));
    }
    updateTimeLabel(0, 0);
    updatePlaybackButtons();
}

void VideoPlayerPage::loadVideo(const QString& filePath) {
    if (filePath.trimmed().isEmpty()) {
        return;
    }

    if (m_mediaSession) {
        m_mediaSession->stop();
        delete m_mediaSession;
        m_mediaSession = nullptr;
    }

    resetPlaybackUi();
    m_currentFilePath = filePath.trimmed();
    if (m_displayTitle.isEmpty()) {
        m_displayTitle = fileNameFromSource(m_currentFilePath);
    }
    if (m_sourcePath.isEmpty()) {
        m_sourcePath = m_currentFilePath;
    }

    m_mediaSession = new MediaSession(this);
    connectMediaSessionSignals();

    QUrl url(m_currentFilePath);
    if (!url.isValid() || url.scheme().isEmpty()) {
        url = QUrl::fromLocalFile(m_currentFilePath);
    }

    if (!m_mediaSession->loadSource(url)) {
        delete m_mediaSession;
        m_mediaSession = nullptr;
        m_placeholderTitleLabel->setText(QStringLiteral("视频加载失败"));
        m_placeholderSubtitleLabel->setText(
            QStringLiteral("当前资源暂时无法播放，请稍后重试或打开本地视频。"));
        updateStageState();
        return;
    }

    if (m_mediaSession->hasVideo()) {
        if (VideoSession* videoSession = m_mediaSession->videoSession()) {
            videoSession->setVideoRenderer(activeRenderer());
        }
    }

    m_qualityPresetBox->setEnabled(true);
    m_playPauseButton->setEnabled(true);
    onQualityPresetChanged(m_qualityPresetBox->currentIndex());
    onPlaybackRateChanged(2);
    updatePlaybackButtons();

    m_placeholderTitleLabel->setText(QStringLiteral("点击播放开始观看"));
    m_placeholderSubtitleLabel->setText(QStringLiteral("下方控制栏可直接调节播放、进度与全屏。"));
    updateInfoPanel();
    updateStageState();
    emit videoLoaded(m_currentFilePath);
}

void VideoPlayerPage::pausePlayback() {
    if (!m_mediaSession || !m_isPlaying) {
        return;
    }

    m_isPlaying = false;
    m_mediaSession->pause();
    updatePlaybackButtons();
    updateStageState();
    emit playStateChanged(false);
}

bool VideoPlayerPage::resumePlayback() {
    if (!m_mediaSession) {
        return false;
    }
    if (m_isPlaying) {
        return true;
    }
    onPlayPauseClicked();
    return m_isPlaying;
}

bool VideoPlayerPage::seekToPosition(qint64 positionMs) {
    if (!m_mediaSession || m_duration <= 0) {
        return false;
    }

    const qint64 target = qBound<qint64>(0, positionMs, m_duration);
    m_currentPosition = target;
    if (m_progressSlider) {
        m_progressSlider->setValue(static_cast<int>((target * 1000) / m_duration));
    }
    updateTimeLabel(target, m_duration);

    const bool wasStopped = (m_mediaSession->state() == MediaSession::Stopped);
    const bool wasPaused = (m_mediaSession->state() == MediaSession::Paused);
    emit playStateChanged(true);

    if (wasStopped) {
        m_mediaSession->play();
        m_pendingStoppedSeekPosition = target;
        QTimer::singleShot(0, this, &VideoPlayerPage::onDeferredSeekAfterStopped);
    } else {
        m_mediaSession->seekTo(target);
        if (wasPaused || m_mediaSession->state() != MediaSession::Playing) {
            m_mediaSession->play();
        }
    }

    m_isPlaying = true;
    updatePlaybackButtons();
    updateStageState();
    emit progressChanged(target);
    return true;
}

void VideoPlayerPage::onPlayPauseClicked() {
    if (!m_mediaSession) {
        return;
    }

    m_isPlaying = !m_isPlaying;
    if (m_isPlaying) {
        emit playStateChanged(true);
        if (m_replayPendingSeek) {
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
        m_mediaSession->pause();
        emit playStateChanged(false);
    }

    updatePlaybackButtons();
    updateStageState();
}

void VideoPlayerPage::onOpenFileClicked() {
    const QString filePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择视频文件"), QString(),
        QStringLiteral("视频文件 (*.mp4 *.avi *.mkv *.mov *.flv *.wmv);;所有文件 (*.*)"));
    if (filePath.isEmpty()) {
        return;
    }

    const QFileInfo info(filePath);
    setVideoInfo(fileNameFromSource(filePath), filePath, info.size());
    loadVideo(filePath);
    resumePlayback();
}

void VideoPlayerPage::onSliderPressed() {
    m_sliderPressed = true;
}

void VideoPlayerPage::onSliderReleased() {
    m_sliderPressed = false;
    if (!m_mediaSession || !m_progressSlider) {
        return;
    }

    const qint64 targetPosition = (m_duration * m_progressSlider->value()) / 1000;
    m_currentPosition = targetPosition;

    const bool wasStopped = (m_mediaSession->state() == MediaSession::Stopped);
    const bool wasPaused = (m_mediaSession->state() == MediaSession::Paused);

    emit playStateChanged(true);
    if (wasStopped) {
        m_mediaSession->play();
        m_pendingStoppedSeekPosition = targetPosition;
        QTimer::singleShot(0, this, &VideoPlayerPage::onDeferredSeekAfterStopped);
    } else if (wasPaused) {
        m_mediaSession->seekTo(targetPosition);
        m_mediaSession->play();
    } else {
        m_mediaSession->seekTo(targetPosition);
        if (m_mediaSession->state() != MediaSession::Playing) {
            m_mediaSession->play();
        }
    }

    m_isPlaying = true;
    m_replayPendingSeek = false;
    updatePlaybackButtons();
    updateStageState();
    emit progressChanged(targetPosition);
}

void VideoPlayerPage::onSliderValueChanged(int value) {
    if (m_sliderPressed) {
        const qint64 position = (m_duration * value) / 1000;
        updateTimeLabel(position, m_duration);
    }
}

void VideoPlayerPage::onDisplayModeClicked() {
    m_fillDisplayMode = !m_fillDisplayMode;
    if (m_renderWidget) {
        m_renderWidget->setDisplayMode(m_fillDisplayMode ? 1 : 0);
    }
    if (m_fullscreenRenderWidget) {
        m_fullscreenRenderWidget->setDisplayMode(m_fillDisplayMode ? 1 : 0);
    }
    updatePlaybackButtons();
}

void VideoPlayerPage::onFullScreenClicked() {
    applyHostWindowState(!m_videoFullscreenActive);
}

void VideoPlayerPage::onPlaybackRateChanged(int index) {
    if (index < 0) {
        return;
    }

    bool ok = false;
    double rate = 1.0;
    if (m_playbackRateBox && index < m_playbackRateBox->count()) {
        rate = m_playbackRateBox->itemData(index).toDouble(&ok);
    } else {
        rate = 1.0;
        ok = true;
    }
    if (ok && m_mediaSession) {
        m_mediaSession->setPlaybackRate(rate);
    }
}

void VideoPlayerPage::onQualityPresetChanged(int index) {
    if (!m_qualityPresetBox || index < 0) {
        return;
    }

    bool ok = false;
    const int preset = m_qualityPresetBox->itemData(index).toInt(&ok);
    if (ok) {
        if (m_renderWidget) {
            m_renderWidget->setQualityPreset(preset);
        }
        if (m_fullscreenRenderWidget) {
            m_fullscreenRenderWidget->setQualityPreset(preset);
        }
    }
}

void VideoPlayerPage::onPositionChanged(qint64 positionMs) {
    m_currentPosition = positionMs;
    if (!m_sliderPressed && m_duration > 0) {
        m_progressSlider->setValue(static_cast<int>((positionMs * 1000) / m_duration));
    }
    updateTimeLabel(positionMs, m_duration);
}

void VideoPlayerPage::onDurationChanged(qint64 durationMs) {
    m_duration = durationMs;
    updateTimeLabel(m_currentPosition, m_duration);
}

void VideoPlayerPage::onPlaybackFinished() {
    if (m_mediaSession) {
        m_mediaSession->stop();
    }
    m_replayPendingSeek = true;
    m_isPlaying = false;
    m_currentPosition = 0;
    if (m_progressSlider) {
        m_progressSlider->setValue(0);
    }
    updateTimeLabel(0, m_duration);
    updatePlaybackButtons();
    updateStageState();
    emit playStateChanged(false);
}

void VideoPlayerPage::onVideoSizeChanged(const QSize& size) {
    Q_UNUSED(size);
    if (m_renderWidget) {
        m_renderWidget->setDisplayMode(m_fillDisplayMode ? 1 : 0);
    }
    if (m_fullscreenRenderWidget) {
        m_fullscreenRenderWidget->setDisplayMode(m_fillDisplayMode ? 1 : 0);
    }
}

void VideoPlayerPage::onMediaSessionStateChanged(MediaSession::PlaybackState state) {
    if (state == MediaSession::Error) {
        m_isPlaying = false;
        updatePlaybackButtons();
        updateStageState();
    }
}

void VideoPlayerPage::onDeferredSeekAfterStopped() {
    if (m_mediaSession) {
        m_mediaSession->seekTo(m_pendingStoppedSeekPosition);
    }
}

void VideoPlayerPage::updateTimeLabel(qint64 currentMs, qint64 totalMs) {
    if (m_timeLabel) {
        m_timeLabel->setText(
            QStringLiteral("%1 / %2").arg(formatTime(currentMs), formatTime(totalMs)));
    }
}

void VideoPlayerPage::updatePlaybackButtons() {
    const bool hasVideo = hasLoadedVideo();
    if (m_playPauseButton) {
        const QString playIcon = QStringLiteral(":/qml/assets/ai/icons/video-play.svg");
        const QString pauseIcon = QStringLiteral(":/qml/assets/ai/icons/video-pause.svg");
        m_playPauseButton->setIcon(QIcon(m_isPlaying ? pauseIcon : playIcon));
        m_playPauseButton->setText(QString());
        m_playPauseButton->setEnabled(hasVideo);
    }
    if (m_centerPlayButton) {
        m_centerPlayButton->setIcon(QIcon(QStringLiteral(":/qml/assets/ai/icons/video-play.svg")));
        m_centerPlayButton->setText(QString());
    }
    if (m_qualityButton && m_qualityPresetBox) {
        m_qualityButton->setText(m_qualityPresetBox->currentText().trimmed().isEmpty()
                                     ? QStringLiteral("超清")
                                     : m_qualityPresetBox->currentText().trimmed());
    }
    if (m_fullScreenButton) {
        m_fullScreenButton->setIcon(
            QIcon(m_videoFullscreenActive
                      ? QStringLiteral(":/qml/assets/ai/icons/video-fullscreen-exit.svg")
                      : QStringLiteral(":/qml/assets/ai/icons/video-fullscreen-enter.svg")));
        m_fullScreenButton->setText(QString());
        m_fullScreenButton->setEnabled(hasVideo);
    }
    if (m_qualityButton) {
        m_qualityButton->setEnabled(hasVideo);
    }
    if (m_nextStubButton) {
        m_nextStubButton->setEnabled(hasVideo);
    }
    if (m_volumeStubButton) {
        m_volumeStubButton->setEnabled(hasVideo);
    }
    if (m_playlistStubButton) {
        m_playlistStubButton->setEnabled(hasVideo);
    }
    if (m_pipStubButton) {
        m_pipStubButton->setEnabled(hasVideo);
    }
    syncHostWindowButtonText();
}

void VideoPlayerPage::updateInfoPanel() {
    const QString title = m_displayTitle.trimmed().isEmpty() ? QStringLiteral("未选择视频")
                                                             : m_displayTitle.trimmed();
    m_titleLabel->setText(title);

    const QString sourceLeaf = leafNameFromSource(m_sourcePath);
    const bool hasVideo = !m_currentFilePath.trimmed().isEmpty();
    const bool isLocal = sourceIsLocal(m_sourcePath, m_currentFilePath);

    QStringList metaParts;
    metaParts.append(isLocal ? QStringLiteral("来源：本地视频") : QStringLiteral("来源：在线视频"));
    if (m_sourceSizeBytes > 0) {
        metaParts.append(QStringLiteral("文件大小：%1").arg(formatSizeBytes(m_sourceSizeBytes)));
    }
    if (!sourceLeaf.trimmed().isEmpty()) {
        metaParts.append(QStringLiteral("文件名：%1").arg(sourceLeaf.trimmed()));
    }

    const QString metaText = hasVideo ? metaParts.join(QStringLiteral("    "))
                                      : QStringLiteral("当前只展示可直接确认的文件与来源信息。");
    QString shownMetaText = metaText;
    if (m_metaLabel) {
        const int availableWidth = m_metaLabel->width() > 0 ? m_metaLabel->width() : 620;
        const QFontMetrics metrics(m_metaLabel->font());
        shownMetaText = metrics.elidedText(metaText, Qt::ElideRight, availableWidth);
        m_metaLabel->setText(shownMetaText);
    }
    m_metaLabel->setToolTip(hasVideo && !m_sourcePath.trimmed().isEmpty()
                                ? QStringLiteral("%1\n%2").arg(metaText, m_sourcePath.trimmed())
                                : metaText);
}

void VideoPlayerPage::updateStageState() {
    const bool hasVideo = hasLoadedVideo();
    const bool showOverlay = hasVideo && m_stageHovered;
    if (m_placeholderPanel) {
        m_placeholderPanel->setVisible(!hasVideo);
        m_placeholderPanel->raise();
    }
    if (m_stageChromeHost) {
        m_stageChromeHost->setVisible(true);
        m_stageChromeHost->raise();
    }
    if (m_controlBar) {
        m_controlBar->setVisible(true);
    }
    if (m_centerPlayButton) {
        m_centerPlayButton->setVisible(hasVideo && !m_isPlaying);
        m_centerPlayButton->raise();
    }
    if (m_stageTitlePanel) {
        m_stageTitlePanel->setVisible(false);
        m_stageTitlePanel->raise();
    }
    if (m_watermarkLabel) {
        m_watermarkLabel->setVisible(hasVideo);
        m_watermarkLabel->raise();
    }
    if (m_subtitleOverlayLabel) {
        m_subtitleOverlayLabel->setVisible(hasVideo);
        m_subtitleOverlayLabel->setText(hasVideo
                                            ? QStringLiteral("正在播放: %1")
                                                  .arg(m_displayTitle.trimmed().isEmpty()
                                                           ? fileNameFromSource(m_currentFilePath)
                                                           : m_displayTitle.trimmed())
                                            : QString());
        m_subtitleOverlayLabel->raise();
    }
    if (m_stageNextButton) {
        m_stageNextButton->setVisible(showOverlay);
        m_stageNextButton->raise();
    }
}

void VideoPlayerPage::updateStageLayout() {
    if (!m_playerContainer || !m_stageCard || !m_stageRenderHost || !m_stageChromeHost ||
        !m_renderWidget || !m_controlBar) {
        return;
    }

    const int width = m_playerContainer->width();
    if (width <= 0) {
        return;
    }

    const int embeddedTargetHeight = qBound(280, qRound(width * 9.0 / 16.0), 620);
    const bool heightChanged = (m_stageCard->height() != embeddedTargetHeight);
    const QRect embeddedStageRect(0, 0, width, embeddedTargetHeight);

    m_stageCard->setFixedHeight(embeddedTargetHeight);
    m_stageRenderHost->setGeometry(embeddedStageRect);
    m_renderWidget->setGeometry(embeddedStageRect);
    m_renderWidget->raise();
    m_stageChromeHost->setGeometry(embeddedStageRect);
    m_stageChromeHost->raise();

    const QRect visibleVideoRect = calculateVisibleVideoRect();
    const QRect overlayAnchorRect =
        visibleVideoRect.isValid() ? visibleVideoRect : embeddedStageRect;

    if (m_centerPlayButton) {
        const int buttonSize = 68;
        m_centerPlayButton->setGeometry((width - buttonSize) / 2,
                                        (embeddedTargetHeight - buttonSize) / 2, buttonSize,
                                        buttonSize);
    }

    if (m_watermarkLabel) {
        const QSize sizeHint = m_watermarkLabel->sizeHint();
        m_watermarkLabel->setGeometry(
            overlayAnchorRect.x() + overlayAnchorRect.width() - 28 - sizeHint.width(),
            overlayAnchorRect.y() + 24, sizeHint.width(), sizeHint.height());
    }

    if (m_stageNextButton) {
        const int arrowSize = 44;
        m_stageNextButton->setGeometry(
            width - 16 - arrowSize, (embeddedTargetHeight - arrowSize) / 2, arrowSize, arrowSize);
    }

    if (m_subtitleOverlayLabel) {
        const int subtitleWidth = qMin(300, qMax(180, overlayAnchorRect.width() - 56));
        const int subtitleBottomMargin = 24;
        const int subtitleX =
            overlayAnchorRect.x() + overlayAnchorRect.width() - 28 - subtitleWidth;
        const int subtitleY =
            qMax(overlayAnchorRect.y() + 20,
                 overlayAnchorRect.y() + overlayAnchorRect.height() - subtitleBottomMargin - 24);
        m_subtitleOverlayLabel->setGeometry(subtitleX, subtitleY, subtitleWidth, 24);
    }

    if (m_placeholderPanel) {
        const QSize sizeHint = m_placeholderPanel->sizeHint();
        m_placeholderPanel->setGeometry((width - sizeHint.width()) / 2,
                                        (embeddedTargetHeight - sizeHint.height()) / 2,
                                        sizeHint.width(), sizeHint.height());
    }

    const int controlBarHeight = m_controlBar->height() > 0 ? m_controlBar->height() : 68;
    if (m_videoFullscreenActive && m_videoFullscreenHost && m_videoFullscreenStageHost &&
        m_fullscreenRenderWidget) {
        const int fullscreenWidth = m_videoFullscreenHost->width();
        const int fullscreenHeight = m_videoFullscreenHost->height();
        const int fullscreenStageHeight = qMax(220, fullscreenHeight - controlBarHeight);
        if (fullscreenWidth > 0 && fullscreenStageHeight > 0) {
            const QRect fullscreenStageRect(0, 0, fullscreenWidth, fullscreenStageHeight);
            m_videoFullscreenStageHost->setGeometry(fullscreenStageRect);
            m_fullscreenRenderWidget->setGeometry(m_videoFullscreenStageHost->rect());
            m_fullscreenRenderWidget->raise();
            m_fullscreenRenderWidget->show();
            if (m_controlBar->parentWidget() == m_videoFullscreenHost) {
                m_controlBar->setGeometry(0, fullscreenStageHeight, fullscreenWidth,
                                          controlBarHeight);
                m_controlBar->raise();
            }
        }
    }

    if (heightChanged) {
        QTimer::singleShot(0, this, [this]() {
            updateStageLayout();
            updateStageState();
        });
    }
}

bool VideoPlayerPage::isCursorInsideStage() const {
    if (m_stageCard && m_stageCard->isVisible()) {
        const QPoint localPos = m_stageCard->mapFromGlobal(QCursor::pos());
        if (m_stageCard->rect().contains(localPos)) {
            return true;
        }
    }
    if (m_videoFullscreenStageHost && m_videoFullscreenStageHost->isVisible()) {
        const QPoint fullscreenPos = m_videoFullscreenStageHost->mapFromGlobal(QCursor::pos());
        if (m_videoFullscreenStageHost->rect().contains(fullscreenPos)) {
            return true;
        }
    }
    if (m_controlBar && m_controlBar->isVisible()) {
        const QPoint barPos = m_controlBar->mapFromGlobal(QCursor::pos());
        if (m_controlBar->rect().contains(barPos)) {
            return true;
        }
    }
    if (m_stageNextButton && m_stageNextButton->isVisible()) {
        const QPoint arrowPos = m_stageNextButton->mapFromGlobal(QCursor::pos());
        if (m_stageNextButton->rect().contains(arrowPos)) {
            return true;
        }
    }
    return false;
}

void VideoPlayerPage::setStageHovered(bool hovered) {
    if (hovered && m_overlayHideTimer) {
        m_overlayHideTimer->stop();
    }
    if (m_stageHovered == hovered) {
        return;
    }
    m_stageHovered = hovered;
    updateStageLayout();
    updateStageState();
}

QRect VideoPlayerPage::calculateVisibleVideoRect() const {
    if (!m_stageCard || !m_renderWidget) {
        return QRect();
    }

    const QRect stageRect = m_stageCard->rect();
    const QSize videoSize = m_renderWidget->videoSize();
    if (!stageRect.isValid() || videoSize.width() <= 0 || videoSize.height() <= 0) {
        return stageRect;
    }

    if (m_fillDisplayMode) {
        return stageRect;
    }

    const double videoAspect = static_cast<double>(videoSize.width()) / videoSize.height();
    const double stageAspect = static_cast<double>(stageRect.width()) / stageRect.height();

    int visibleWidth = stageRect.width();
    int visibleHeight = stageRect.height();

    if (videoAspect > stageAspect) {
        visibleHeight = qRound(stageRect.width() / videoAspect);
    } else {
        visibleWidth = qRound(stageRect.height() * videoAspect);
    }

    visibleWidth = qBound(1, visibleWidth, stageRect.width());
    visibleHeight = qBound(1, visibleHeight, stageRect.height());

    const int visibleX = (stageRect.width() - visibleWidth) / 2;
    const int visibleY = (stageRect.height() - visibleHeight) / 2;
    return QRect(visibleX, visibleY, visibleWidth, visibleHeight);
}

void VideoPlayerPage::applyHostWindowState(bool enabled) {
    if (!m_videoFullscreenHost || !m_playerContainer || !m_controlBar) {
        return;
    }

    if (enabled == m_videoFullscreenActive) {
        syncHostWindowButtonText();
        return;
    }
    qDebug() << "[VideoPlayerPage] applyHostWindowState enabled:" << enabled
             << "currentFullscreen:" << m_videoFullscreenActive;

    if (enabled) {
        m_videoFullscreenActive = true;
        moveControlBarToFullscreenHost();
        if (m_videoFullscreenStageHost) {
            m_videoFullscreenStageHost->show();
        }
        m_videoFullscreenHost->showFullScreen();
        m_videoFullscreenHost->raise();
        m_videoFullscreenHost->activateWindow();
    } else {
        m_videoFullscreenActive = false;
        restoreControlBarToEmbeddedHost();
        m_videoFullscreenHost->hide();
        if (m_videoFullscreenStageHost) {
            m_videoFullscreenStageHost->hide();
        }
    }

    syncHostWindowButtonText();
    updatePlaybackButtons();
    updateStageLayout();
    updateStageState();
    switchActiveRenderer();
    scheduleRendererRecovery();
}

void VideoPlayerPage::syncHostWindowButtonText() {
    if (!m_fullScreenButton) {
        return;
    }
    m_fullScreenButton->setText(QString());
    m_fullScreenButton->setToolTip(m_videoFullscreenActive ? QStringLiteral("退出视频全屏")
                                                           : QStringLiteral("进入视频全屏"));
}

void VideoPlayerPage::scheduleRendererRecovery() {
    if (!activeRenderer()) {
        return;
    }

    QTimer::singleShot(0, this, [this]() {
        recoverRendererAfterWindowModeChange();
    });
    QTimer::singleShot(kRendererRecoveryDelayMs, this, [this]() {
        recoverRendererAfterWindowModeChange();
    });
}

void VideoPlayerPage::recoverRendererAfterWindowModeChange() {
    VideoRendererGL* renderer = activeRenderer();
    if (!renderer) {
        return;
    }

    qDebug() << "[VideoPlayerPage] recoverRendererAfterWindowModeChange execute"
             << "fullscreen:" << m_videoFullscreenActive
             << "renderer:" << static_cast<void*>(renderer);
    renderer->refreshAfterSurfaceChange();
    renderer->setDisplayMode(m_fillDisplayMode ? 1 : 0);
    renderer->update();
}

QString VideoPlayerPage::formatTime(qint64 ms) const {
    const int seconds = static_cast<int>((ms / 1000) % 60);
    const int minutes = static_cast<int>((ms / (1000 * 60)) % 60);
    const int hours = static_cast<int>(ms / (1000 * 60 * 60));

    if (hours > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }

    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

QString VideoPlayerPage::formatSizeBytes(qint64 bytes) const {
    if (bytes <= 0) {
        return QString();
    }
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    if (bytes < 1024 * 1024) {
        return QStringLiteral("%1 KB").arg(QString::number(bytes / 1024.0, 'f', 2));
    }
    if (bytes < 1024LL * 1024LL * 1024LL) {
        return QStringLiteral("%1 MB").arg(QString::number(bytes / 1024.0 / 1024.0, 'f', 2));
    }
    return QStringLiteral("%1 GB").arg(QString::number(bytes / 1024.0 / 1024.0 / 1024.0, 'f', 2));
}

bool VideoPlayerPage::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_observedWindow && event && event->type() == QEvent::WindowStateChange) {
        syncHostWindowButtonText();
        if (!m_videoFullscreenActive) {
            scheduleRendererRecovery();
        }
    }
    if (watched == m_videoFullscreenHost && event) {
        switch (event->type()) {
            case QEvent::Resize:
            case QEvent::Show:
                updateStageLayout();
                updateStageState();
                scheduleRendererRecovery();
                break;
            case QEvent::Close:
                if (m_videoFullscreenActive) {
                    applyHostWindowState(false);
                    event->ignore();
                    return true;
                }
                break;
            case QEvent::KeyPress: {
                auto* keyEvent = static_cast<QKeyEvent*>(event);
                if (keyEvent &&
                    (keyEvent->key() == Qt::Key_Escape || keyEvent->key() == Qt::Key_F11)) {
                    applyHostWindowState(false);
                    keyEvent->accept();
                    return true;
                }
                break;
            }
            default:
                break;
        }
    }
    if ((watched == m_stageCard || watched == m_stageRenderHost || watched == m_renderWidget ||
         watched == m_videoFullscreenStageHost || watched == m_fullscreenRenderWidget ||
         watched == m_stageChromeHost || watched == m_controlBar || watched == m_stageNextButton) &&
        event) {
        switch (event->type()) {
            case QEvent::Enter:
            case QEvent::MouseMove:
            case QEvent::HoverMove:
                setStageHovered(true);
                break;
            case QEvent::Leave:
                if (isCursorInsideStage()) {
                    setStageHovered(true);
                } else if (m_overlayHideTimer) {
                    m_overlayHideTimer->start();
                } else {
                    setStageHovered(false);
                }
                break;
            default:
                break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void VideoPlayerPage::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    attachWindowObserver();
    syncHostWindowButtonText();
    updateStageLayout();
    updateStageState();
    updateInfoPanel();
}

void VideoPlayerPage::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateStageLayout();
    updateStageState();
    updateInfoPanel();
}

void VideoPlayerPage::keyPressEvent(QKeyEvent* event) {
    if (!event) {
        QWidget::keyPressEvent(event);
        return;
    }

    if (event->key() == Qt::Key_F11) {
        onFullScreenClicked();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Escape) {
        if (m_videoFullscreenActive) {
            applyHostWindowState(false);
            event->accept();
            return;
        }
    }

    QWidget::keyPressEvent(event);
}
