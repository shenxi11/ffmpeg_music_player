#include "PlaybackViewModel.h"

#include <QTime>

PlaybackViewModel::PlaybackViewModel(QObject* parent)
    : BaseViewModel(parent)
    , m_audioService(&AudioService::instance())
{
    m_volume = m_audioService->volume();

    connect(m_audioService, &AudioService::playbackStarted,
            this, &PlaybackViewModel::onAudioServicePlaybackStarted);
    connect(m_audioService, &AudioService::playbackPaused,
            this, &PlaybackViewModel::onAudioServicePlaybackPaused);
    connect(m_audioService, &AudioService::playbackResumed,
            this, &PlaybackViewModel::onAudioServicePlaybackResumed);
    connect(m_audioService, &AudioService::playbackStopped,
            this, &PlaybackViewModel::onAudioServicePlaybackStopped);
    connect(m_audioService, &AudioService::positionChanged,
            this, &PlaybackViewModel::onAudioServicePositionChanged);
    connect(m_audioService, &AudioService::durationChanged,
            this, &PlaybackViewModel::onAudioServiceDurationChanged);
    connect(m_audioService, &AudioService::bufferingStarted,
            this, &PlaybackViewModel::onAudioServiceBufferingStarted);
    connect(m_audioService, &AudioService::bufferingFinished,
            this, &PlaybackViewModel::onAudioServiceBufferingFinished);

    connect(m_audioService, &AudioService::currentTrackChanged,
            this, [this](const QString& title, const QString& artist, qint64 duration) {
                updateMetadata(title, artist, QString(), QString(), m_currentUrl);
                updateDuration(duration);
            });
    connect(m_audioService, &AudioService::albumArtChanged,
            this, [this](const QString& imagePath) {
                const QString trimmed = imagePath.trimmed();
                if (!trimmed.isEmpty() && m_currentAlbumArt != trimmed) {
                    m_currentAlbumArt = trimmed;
                    emit currentAlbumArtChanged();
                }
            });
}

PlaybackViewModel::~PlaybackViewModel()
{
}

void PlaybackViewModel::play(const QUrl& url)
{
    setIsBusy(true);
    clearError();

    const QString filePath = url.isLocalFile() ? url.toLocalFile() : url.toString();
    if (m_currentFilePath != filePath) {
        m_currentFilePath = filePath;
        emit currentFilePathChanged();
        emit shouldLoadLyrics(filePath);
    }

    if (m_audioService->play(url)) {
        updateMetadata(QString(), QString(), QString(), QString(), url);
    } else {
        setErrorMessage(QStringLiteral("播放失败：") + url.toString());
    }

    setIsBusy(false);
}

void PlaybackViewModel::pause()
{
    m_audioService->pause();
}

void PlaybackViewModel::resume()
{
    m_audioService->resume();
}

void PlaybackViewModel::stop()
{
    m_audioService->stop();
}

void PlaybackViewModel::seekTo(qint64 positionMs)
{
    m_audioService->seekTo(positionMs);
    updatePosition(positionMs);
}

void PlaybackViewModel::togglePlayPause()
{
    const bool servicePlaying = m_audioService->isPlaying();
    const bool servicePaused = m_audioService->isPaused();

    if (servicePaused) {
        resume();
        return;
    }
    if (servicePlaying && !servicePaused) {
        pause();
        return;
    }
    if (m_isPlaying && !m_isPaused) {
        pause();
        return;
    }
    if (m_isPaused) {
        resume();
        return;
    }
    if (!m_currentUrl.isEmpty()) {
        play(m_currentUrl);
        return;
    }

    setErrorMessage(QStringLiteral("当前没有可播放的音频源"));
}

void PlaybackViewModel::playNext()
{
    m_audioService->playNext();
}

void PlaybackViewModel::playPrevious()
{
    m_audioService->playPrevious();
}

void PlaybackViewModel::removeFromPlaylistUrl(const QString& filePath)
{
    const QUrl url = QUrl::fromUserInput(filePath);
    const int index = m_audioService->playlist().indexOf(url);
    if (index >= 0) {
        m_audioService->removeFromPlaylist(index);
    }
}

void PlaybackViewModel::clearPlaylist()
{
    m_audioService->clearPlaylist();
}

void PlaybackViewModel::setPlayModeValue(int mode)
{
    m_audioService->setPlayMode(static_cast<AudioService::PlayMode>(mode));
}

void PlaybackViewModel::setVolume(int volume)
{
    const int boundedVolume = qBound(0, volume, 100);
    if (m_volume == boundedVolume) {
        return;
    }

    m_volume = boundedVolume;
    m_audioService->setVolume(m_volume);
    emit volumeChanged();
}

void PlaybackViewModel::onAudioServicePlaybackStarted(const QString& sessionId, const QUrl& url)
{
    Q_UNUSED(sessionId);

    const QString filePath = url.isLocalFile() ? url.toLocalFile() : url.toString();
    if (m_currentFilePath != filePath) {
        m_currentFilePath = filePath;
        emit currentFilePathChanged();
        emit shouldLoadLyrics(filePath);
    }

    updatePlayingState(true);
    updatePausedState(false);
    updateBufferingState(false);
    updateMetadata(QString(), QString(), QString(), QString(), url);
    emit shouldStartRotation();
    emit playbackStarted();
}

void PlaybackViewModel::onAudioServicePlaybackPaused()
{
    updatePlayingState(false);
    updatePausedState(true);
    emit shouldStopRotation();
}

void PlaybackViewModel::onAudioServicePlaybackResumed()
{
    updatePlayingState(true);
    updatePausedState(false);
    emit shouldStartRotation();
}

void PlaybackViewModel::onAudioServicePlaybackStopped()
{
    updatePlayingState(false);
    updatePausedState(false);
    updateBufferingState(false);
    updatePosition(0);
    emit shouldStopRotation();
    emit playbackStopped();
}

void PlaybackViewModel::onAudioServicePositionChanged(qint64 position)
{
    updatePosition(position);
}

void PlaybackViewModel::onAudioServiceDurationChanged(qint64 duration)
{
    updateDuration(duration);
}

void PlaybackViewModel::onAudioServiceBufferingStarted()
{
    updateBufferingState(true);
}

void PlaybackViewModel::onAudioServiceBufferingFinished()
{
    updateBufferingState(false);
}

void PlaybackViewModel::updatePlayingState(bool playing)
{
    if (m_isPlaying != playing) {
        m_isPlaying = playing;
        emit isPlayingChanged();
    }
}

void PlaybackViewModel::updatePausedState(bool paused)
{
    if (m_isPaused != paused) {
        m_isPaused = paused;
        emit isPausedChanged();
    }
}

void PlaybackViewModel::updateBufferingState(bool buffering)
{
    if (m_isBuffering != buffering) {
        m_isBuffering = buffering;
        emit isBufferingChanged();
        emit bufferingStateChanged(buffering);
    }
}

void PlaybackViewModel::updatePosition(qint64 pos)
{
    if (m_position != pos) {
        m_position = pos;
        emit positionChanged();
    }
}

void PlaybackViewModel::updateDuration(qint64 dur)
{
    if (m_duration != dur) {
        m_duration = dur;
        emit durationChanged();
    }
}

void PlaybackViewModel::updateMetadata(const QString& title,
                                       const QString& artist,
                                       const QString& album,
                                       const QString& albumArt,
                                       const QUrl& url)
{
    if (m_currentTitle != title) {
        m_currentTitle = title;
        emit currentTitleChanged();
    }
    if (m_currentArtist != artist) {
        m_currentArtist = artist;
        emit currentArtistChanged();
    }
    if (m_currentAlbum != album) {
        m_currentAlbum = album;
        emit currentAlbumChanged();
    }
    const QString trimmedAlbumArt = albumArt.trimmed();
    if (!trimmedAlbumArt.isEmpty() && m_currentAlbumArt != trimmedAlbumArt) {
        m_currentAlbumArt = trimmedAlbumArt;
        emit currentAlbumArtChanged();
    }
    if (m_currentUrl != url) {
        m_currentUrl = url;
        emit currentUrlChanged();
    }
}

QString PlaybackViewModel::formatTime(qint64 milliseconds)
{
    if (milliseconds < 0) {
        return QStringLiteral("00:00");
    }

    const int totalSeconds = static_cast<int>(milliseconds / 1000);
    const int hours = totalSeconds / 3600;
    const int minutes = (totalSeconds % 3600) / 60;
    const int seconds = totalSeconds % 60;

    if (hours > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }

    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}
