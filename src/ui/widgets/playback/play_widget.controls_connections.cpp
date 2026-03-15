#include "play_widget.h"

#include <QDebug>

/*
模块名称: PlayWidget 控制与队列连接
功能概述: 集中维护桌面歌词控制、控制栏交互、播放列表面板与播放模式相关连接。
对外接口: PlayWidget::setupControlAndPlaylistConnections()
维护说明: 仅负责连接关系与 UI 同步，不处理解码或会话生命周期。
*/

// 建立播放控制域连接，保持构造函数可读性。
void PlayWidget::setupControlAndPlaylistConnections()
{
    // 设置桌面歌词 ProcessSlider 引用，让它直接调 ControlBar 方法。
    desk->setProcessSlider(process_slider);
    connect(desk, &DeskLrcQml::signalForwardClicked, this, &PlayWidget::handleDeskForwardClicked);
    connect(desk, &DeskLrcQml::signalBackwardClicked, this, &PlayWidget::handleDeskBackwardClicked);
    connect(desk, &DeskLrcQml::signalCloseClicked, this, &PlayWidget::handleDeskCloseClicked);

    // 同步桌面歌词播放状态。
    connect(this, &PlayWidget::signalPlayState, this, &PlayWidget::handleDeskPlayStateChanged);
    connect(desk, &DeskLrcQml::signalSettingsClicked, this, &PlayWidget::handleDeskSettingsClicked);

    // ProcessSlider QML 控件连接。
    connect(process_slider, &ProcessSliderQml::signalPlayClicked, this, &PlayWidget::onPlayClick);
    connect(this, &PlayWidget::signalPlayState, process_slider, &ProcessSliderQml::setState);

    // 音量同步：UI 写入 ViewModel，ViewModel 回写 UI。
    connect(process_slider, &ProcessSliderQml::signalVolumeChanged, this, &PlayWidget::handleVolumeChanged);
    connect(m_playbackViewModel, &PlaybackViewModel::volumeChanged, this, &PlayWidget::handleViewModelVolumeChanged);

    // 上一首/下一首/停止。
    connect(process_slider, &ProcessSliderQml::signalNextSong, this, &PlayWidget::handleNextSongRequested);
    connect(process_slider, &ProcessSliderQml::signalLastSong, this, &PlayWidget::handleLastSongRequested);
    connect(process_slider, &ProcessSliderQml::signalStop, this, &PlayWidget::handleStopRequested);
    connect(process_slider, &ProcessSliderQml::signalMlistToggled, this, &PlayWidget::signalListShow);
    connect(process_slider, &ProcessSliderQml::signalRePlay, this, &PlayWidget::handleReplayRequested);
    connect(process_slider, &ProcessSliderQml::signalDeskToggled, this, &PlayWidget::onDeskToggled);
    connect(process_slider, &ProcessSliderQml::signalLoopChange, this, &PlayWidget::handleLoopChanged);

    // 播放列表面板显示与定位。
    connect(process_slider, &ProcessSliderQml::signalMlistToggled, this, &PlayWidget::handlePlaylistToggled);

    // 播放历史列表操作。
    connect(playlistHistory, &PlaylistHistoryQml::playRequested, this, &PlayWidget::handlePlaylistPlayRequested);
    connect(playlistHistory, &PlaylistHistoryQml::removeRequested, this, &PlayWidget::handlePlaylistRemoveRequested);
    connect(playlistHistory, &PlaylistHistoryQml::clearAllRequested, this, &PlayWidget::handlePlaylistClearAllRequested);
    connect(playlistHistory, &PlaylistHistoryQml::pauseToggled, this, &PlayWidget::handlePlaylistPauseToggled);

    // 播放模式控制。
    connect(process_slider, &ProcessSliderQml::signalPlayModeChanged, this, &PlayWidget::handlePlayModeChanged);

    // 新架构进度更新（通过 ViewModel）。
    connect(process_slider, &ProcessSliderQml::signalSliderPressed, this, &PlayWidget::handleSliderPressed);
    connect(process_slider, &ProcessSliderQml::signalSliderReleased, this, &PlayWidget::handleSliderReleased);

    // 展开/收起状态同步到底部控制条。
    connect(this, &PlayWidget::signalIsUpChanged, process_slider, &ProcessSliderQml::onIsUpChanged);
}

void PlayWidget::handleDeskForwardClicked()
{
    qint64 currentMs = m_playbackViewModel ? m_playbackViewModel->position() : 0;

    qint64 targetMs = currentMs + 5000;
    const qint64 totalMs = m_playbackViewModel ? m_playbackViewModel->duration() : 0;
    if (totalMs > 0) {
        targetMs = qMin(targetMs, totalMs);
    }
    if (targetMs < 0) {
        targetMs = 0;
    }

    if (m_playbackViewModel) {
        if (process_slider) {
            process_slider->setSeekPendingSeconds(static_cast<int>(targetMs / 1000));
        }
        m_playbackViewModel->seekTo(targetMs);
    }
}

void PlayWidget::handleDeskBackwardClicked()
{
    qint64 currentMs = m_playbackViewModel ? m_playbackViewModel->position() : 0;
    qint64 targetMs = currentMs - 5000;
    if (targetMs < 0) {
        targetMs = 0;
    }

    if (m_playbackViewModel) {
        if (process_slider) {
            process_slider->setSeekPendingSeconds(static_cast<int>(targetMs / 1000));
        }
        m_playbackViewModel->seekTo(targetMs);
    }
}

void PlayWidget::handleDeskCloseClicked()
{
    desk->hide();
    if (process_slider) {
        process_slider->setDeskChecked(false);
        qDebug() << "Desktop lyrics closed via X button - updated main interface button state to unchecked";
    }
}

void PlayWidget::handleDeskPlayStateChanged(ProcessSliderQml::State state)
{
    desk->setPlayingState(state == ProcessSliderQml::Play);
    qDebug() << "Desktop lyric playing state updated to:" << (state == ProcessSliderQml::Play);
}

void PlayWidget::handleDeskSettingsClicked()
{
    qDebug() << "Desktop lyric settings clicked - opening settings dialog";
    desk->showSettingsDialog();
}

void PlayWidget::handleVolumeChanged(int volume)
{
    qDebug() << "[MVVM-UI] Volume changed to:" << volume;
    m_playbackViewModel->setVolume(volume);
}

void PlayWidget::handleViewModelVolumeChanged()
{
    if (process_slider) {
        process_slider->setVolume(m_playbackViewModel->volume());
    }
}

void PlayWidget::handleNextSongRequested()
{
    qDebug() << "[MVVM-UI] Next song clicked";
    m_playbackViewModel->playNext();
}

void PlayWidget::handleLastSongRequested()
{
    qDebug() << "[MVVM-UI] Previous song clicked";
    m_playbackViewModel->playPrevious();
}

void PlayWidget::handleStopRequested()
{
    if (filePath.size()) {
        emit signalPlayButtonClick(false, filePath);
    }
}

void PlayWidget::handleReplayRequested()
{
    rePlay(this->filePath);
}

void PlayWidget::handleLoopChanged(bool isLoop)
{
    qDebug() << "Loop state changed:" << isLoop;
}

void PlayWidget::handlePlaylistToggled(bool show)
{
    qDebug() << "Playlist toggle:" << show;
    if (show) {
        QWidget* mainWidget = playlistHistory->parentWidget();
        if (mainWidget) {
            const int listWidth = qBound(320, mainWidget->width() / 3, 460);
            const int topBarHeight = 60;
            const int bottomPadding = qMax(10, collapsedPlaybackHeight() + 8);
            int windowX = mainWidget->width() - listWidth;
            int windowY = topBarHeight;
            int windowHeight = mainWidget->height() - topBarHeight - bottomPadding;
            windowHeight = qMax(220, windowHeight);

            qDebug() << "MainWidget size:" << mainWidget->size();
            qDebug() << "PlaylistHistory position:" << windowX << windowY << "size:" << listWidth << windowHeight;

            playlistHistory->setGeometry(windowX, windowY, listWidth, windowHeight);
            playlistHistory->show();
            playlistHistory->raise();

            QWidget* topWidget = mainWidget->findChild<QWidget*>();
            if (topWidget && topWidget->y() == 0 && topWidget->height() == 60) {
                topWidget->raise();
            }
        }
    } else {
        playlistHistory->hide();
    }
}

void PlayWidget::handlePlaylistPlayRequested(const QString& filePath)
{
    qDebug() << "Play from history:" << filePath;
    playClick(filePath);
}

void PlayWidget::handlePlaylistRemoveRequested(const QString& filePath)
{
    qDebug() << "[MVVM-UI] Remove from history:" << filePath;
    m_playbackViewModel->removeFromPlaylistUrl(filePath);
}

void PlayWidget::handlePlaylistClearAllRequested()
{
    qDebug() << "[MVVM-UI] Clear all history";
    m_playbackViewModel->clearPlaylist();
}

void PlayWidget::handlePlaylistPauseToggled()
{
    qDebug() << "[MVVM-UI] Pause toggled from playlist history";
    m_playbackViewModel->togglePlayPause();
}

void PlayWidget::handlePlayModeChanged(int mode)
{
    qDebug() << "[MVVM-UI] Play mode changed to:" << mode;
    m_playbackViewModel->setPlayModeValue(mode);
}

void PlayWidget::handleSliderPressed()
{
    qDebug() << "[MVVM-UI] Slider pressed - temporarily disconnecting position updates";
}

void PlayWidget::handleSliderReleased()
{
    qDebug() << "[MVVM-UI] Slider released - reconnecting position updates";
}
