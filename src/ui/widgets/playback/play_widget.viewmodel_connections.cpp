#include "play_widget.h"

#include <QChar>
#include <QDebug>

/*
模块名称: PlayWidget 与 ViewModel 连接
功能概述: 维护 PlaybackViewModel 到 PlayWidget 的状态绑定与 UI 回写。
对外接口: PlayWidget::setupPlaybackViewModelConnections()
维护说明: 仅做信号映射与界面同步，业务决策应保留在 ViewModel 层。
*/

// 建立播放 ViewModel 与界面层的数据绑定连接。
void PlayWidget::setupPlaybackViewModelConnections()
{
    qDebug() << "[MVVM-UI] Connecting ViewModel signals to UI...";

    connect(m_playbackViewModel, &PlaybackViewModel::isPlayingChanged, this, &PlayWidget::handleVmIsPlayingChanged);
    connect(m_playbackViewModel, &PlaybackViewModel::positionChanged, this, &PlayWidget::handleVmPositionChanged);
    connect(m_playbackViewModel, &PlaybackViewModel::durationChanged, this, &PlayWidget::handleVmDurationChanged);
    connect(m_playbackViewModel, &PlaybackViewModel::currentTitleChanged, this, &PlayWidget::handleVmCurrentTitleChanged);
    connect(m_playbackViewModel, &PlaybackViewModel::currentArtistChanged, this, &PlayWidget::handleVmCurrentArtistChanged);
    connect(m_playbackViewModel, &PlaybackViewModel::currentAlbumArtChanged, this, &PlayWidget::handleVmCurrentAlbumArtChanged);
    connect(m_playbackViewModel, &PlaybackViewModel::playlistSnapshotChanged,
            this, &PlayWidget::refreshPlaylistHistoryFromViewModel);
    connect(m_playbackViewModel, &PlaybackViewModel::playbackStarted, this, &PlayWidget::handleVmPlaybackStarted);
    connect(m_playbackViewModel, &PlaybackViewModel::playbackStopped, this, &PlayWidget::handleVmPlaybackStopped);
    connect(m_playbackViewModel, &PlaybackViewModel::shouldStartRotation, this, &PlayWidget::handleVmShouldStartRotation);
    connect(m_playbackViewModel, &PlaybackViewModel::shouldStopRotation, this, &PlayWidget::handleVmShouldStopRotation);
    connect(m_playbackViewModel, &PlaybackViewModel::shouldLoadLyrics, this, &PlayWidget::handleVmShouldLoadLyrics);

    qDebug() << "[MVVM-UI] ViewModel signals connected successfully";
}

void PlayWidget::handleVmIsPlayingChanged()
{
    const bool playing = m_playbackViewModel->isPlaying();
    qDebug() << "[MVVM-UI] isPlaying changed:" << playing;
    emit signalPlayState(playing ? ProcessSliderQml::Play : ProcessSliderQml::Pause);

    QString currentPath = m_playbackViewModel->currentFilePath();
    if (currentPath.isEmpty()) {
        currentPath = filePath;
    }
    if (playing || !currentPath.isEmpty()) {
        emit signalPlayButtonClick(playing, currentPath);
    }

    playlistHistory->updatePlayingState(m_playbackViewModel->currentFilePath(), playing);
}

void PlayWidget::handleVmPositionChanged()
{
    const qint64 positionMs = m_playbackViewModel->position();
    process_slider->setCurrentSeconds(static_cast<int>(positionMs / 1000));
}

void PlayWidget::handleVmDurationChanged()
{
    const qint64 durationMs = m_playbackViewModel->duration();
    qDebug() << "[MVVM-UI] Duration changed:" << durationMs << "ms";
    duration = durationMs * 1000;
    process_slider->setMaxSeconds(durationMs / 1000);
}

void PlayWidget::handleVmCurrentTitleChanged()
{
    const QString title = m_playbackViewModel->currentTitle();
    if (!title.isEmpty()) {
        qDebug() << "[MVVM-UI] Title changed:" << title;
        currentSongTitle = title;
        refreshStageTexts();
    }
}

void PlayWidget::handleVmCurrentArtistChanged()
{
    const QString artist = m_playbackViewModel->currentArtist();
    if (!artist.isEmpty()) {
        qDebug() << "[MVVM-UI] Artist changed:" << artist;
        currentSongArtist = artist;
        refreshStageTexts();
    }
}

void PlayWidget::handleVmCurrentAlbumArtChanged()
{
    const QString imagePath = m_playbackViewModel->currentAlbumArt();
    qDebug() << "[MVVM-UI] Album art changed:" << imagePath;

    if (imagePath.trimmed().isEmpty()) {
        qDebug() << "[MVVM-UI] Ignore empty album art update";
        return;
    }

    onUpdateBackground(imagePath);
    updateCoverPresentation(imagePath);

    QString currentPath = m_playbackViewModel->currentFilePath();
    if (currentPath.isEmpty() && !filePath.isEmpty()) {
        currentPath = filePath;
    }

    if (!currentPath.isEmpty()) {
        const QString latestTitle = m_playbackViewModel->currentTitle();
        const QString latestArtist = m_playbackViewModel->currentArtist();

        const QString title = !latestTitle.isEmpty()
            ? latestTitle
            : (currentSongTitle.isEmpty() ? fileName : currentSongTitle);

        const QString artist = !latestArtist.isEmpty()
            ? latestArtist
            : (!currentSongArtist.isEmpty()
                ? currentSongArtist
                : (!networkSongArtist.isEmpty()
                    ? networkSongArtist
                    : QStringLiteral(u"\u672a\u77e5\u827a\u672f\u5bb6")));

        playlistHistory->setCurrentPlayingPath(currentPath);
    }

    if (!currentPath.isEmpty() && !play_net) {
        const QString durationStr = QString("%1:%2")
            .arg(duration / 60000000)
            .arg((duration / 1000000) % 60, 2, 10, QChar('0'));
        emit signalMetadataUpdated(currentPath, imagePath, durationStr);
    }
}

void PlayWidget::handleVmPlaybackStarted()
{
    qDebug() << "[MVVM-UI] Playback started";
    emit signalPlayState(ProcessSliderQml::Play);
    refreshPlaylistHistoryFromViewModel();

    QString currentPath = m_playbackViewModel->currentFilePath();
    if (currentPath.isEmpty()) {
        currentPath = filePath;
    }
    if (!currentPath.isEmpty()) {
        emit signalPlayButtonClick(true, currentPath);
    }

    currentSession = nullptr;
}

void PlayWidget::handleVmPlaybackStopped()
{
    qDebug() << "[MVVM-UI] Playback stopped";
    emit signalPlayState(ProcessSliderQml::Stop);
    refreshPlaylistHistoryFromViewModel();

    QString currentPath = m_playbackViewModel->currentFilePath();
    if (currentPath.isEmpty()) {
        currentPath = filePath;
    }
    emit signalPlayButtonClick(false, currentPath);

    currentSession = nullptr;
}

void PlayWidget::handleVmShouldStartRotation()
{
    emit signalStopRotate(true);
}

void PlayWidget::handleVmShouldStopRotation()
{
    emit signalStopRotate(false);
}

void PlayWidget::handleVmShouldLoadLyrics(const QString& songPath)
{
    qDebug() << "[MVVM-UI] Should load lyrics for:" << songPath;
    filePath = songPath;
    beginTakeLrc(songPath);
}

void PlayWidget::refreshPlaylistHistoryFromViewModel()
{
    if (!playlistHistory || !m_playbackViewModel) {
        return;
    }

    const QVariantList snapshot = m_playbackViewModel->playlistSnapshot();
    playlistHistory->loadPlaylist(snapshot);
    playlistHistory->setCurrentPlayingPath(m_playbackViewModel->currentFilePath());
    playlistHistory->setPaused(!m_playbackViewModel->isPlaying());

    qDebug() << "[MVVM-UI] Playlist history synced from queue snapshot, count:" << snapshot.size();
}
