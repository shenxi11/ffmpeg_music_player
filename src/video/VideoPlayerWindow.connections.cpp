#include "VideoPlayerWindow.h"

void VideoPlayerWindow::connectUiSignals(QPushButton* closeBtn)
{
    if (closeBtn) {
        connect(closeBtn, &QPushButton::clicked, this, &VideoPlayerWindow::close);
    }

    if (m_renderWidget) {
        connect(m_renderWidget, &VideoRendererGL::videoSizeChanged,
                this, &VideoPlayerWindow::onVideoSizeChanged);
    }

    if (m_progressSlider) {
        connect(m_progressSlider, &QSlider::sliderPressed,
                this, &VideoPlayerWindow::onSliderPressed);
        connect(m_progressSlider, &QSlider::sliderReleased,
                this, &VideoPlayerWindow::onSliderReleased);
        connect(m_progressSlider, &QSlider::valueChanged,
                this, &VideoPlayerWindow::onSliderValueChanged);
    }

    if (m_playPauseBtn) {
        connect(m_playPauseBtn, &QPushButton::clicked,
                this, &VideoPlayerWindow::onPlayPauseClicked);
    }
    if (m_openFileBtn) {
        connect(m_openFileBtn, &QPushButton::clicked,
                this, &VideoPlayerWindow::onOpenFileClicked);
    }
    if (m_displayModeBtn) {
        connect(m_displayModeBtn, &QPushButton::clicked,
                this, &VideoPlayerWindow::onDisplayModeClicked);
    }
    if (m_fullScreenBtn) {
        connect(m_fullScreenBtn, &QPushButton::clicked,
                this, &VideoPlayerWindow::onFullScreenClicked);
    }
    if (m_qualityPresetBox) {
        connect(m_qualityPresetBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &VideoPlayerWindow::onQualityPresetChanged);
    }
    if (m_playbackRateBox) {
        connect(m_playbackRateBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &VideoPlayerWindow::onPlaybackRateChanged);
    }
}

void VideoPlayerWindow::connectMediaSessionSignals()
{
    if (!m_mediaSession) {
        return;
    }

    connect(m_mediaSession, &MediaSession::positionChanged,
            this, &VideoPlayerWindow::onPositionChanged);
    connect(m_mediaSession, &MediaSession::durationChanged,
            this, &VideoPlayerWindow::onDurationChanged);
    connect(m_mediaSession, &MediaSession::playbackFinished,
            this, &VideoPlayerWindow::onPlaybackFinished);
    connect(m_mediaSession, &MediaSession::stateChanged,
            this, &VideoPlayerWindow::onMediaSessionStateChanged);
}
