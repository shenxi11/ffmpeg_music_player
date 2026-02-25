#include "PlaybackViewModel.h"
#include <QTime>

PlaybackViewModel::PlaybackViewModel(QObject *parent)
    : BaseViewModel(parent)
    , m_audioService(&AudioService::instance())
{
    qDebug() << "[MVVM] PlaybackViewModel: Initializing and connecting to AudioService";
    
    // 连接播放状态相关信号
    connect(m_audioService, &AudioService::playbackStarted,
            this, &PlaybackViewModel::onAudioServicePlaybackStarted);
    connect(m_audioService, &AudioService::playbackPaused,
            this, &PlaybackViewModel::onAudioServicePlaybackPaused);
    connect(m_audioService, &AudioService::playbackResumed,
            this, &PlaybackViewModel::onAudioServicePlaybackResumed);
    connect(m_audioService, &AudioService::playbackStopped,
            this, &PlaybackViewModel::onAudioServicePlaybackStopped);
    
    // 连接进度与时长信号
    connect(m_audioService, &AudioService::positionChanged,
            this, &PlaybackViewModel::onAudioServicePositionChanged);
    connect(m_audioService, &AudioService::durationChanged,
            this, &PlaybackViewModel::onAudioServiceDurationChanged);
    
    // 连接曲目信息信号
    connect(m_audioService, &AudioService::currentTrackChanged,
            this, [this](const QString& title, const QString& artist, qint64 duration) {
                updateMetadata(title, artist, "", "", m_currentUrl);
                updateDuration(duration);
            });
    
    // 连接封面变化信号
    connect(m_audioService, &AudioService::albumArtChanged,
            this, [this](const QString& imagePath) {
                if (m_currentAlbumArt != imagePath) {
                    m_currentAlbumArt = imagePath;
                    emit currentAlbumArtChanged();
                }
            });
    
    qDebug() << "[MVVM] PlaybackViewModel: All signals connected successfully";
}

PlaybackViewModel::~PlaybackViewModel()
{
}

// 播放控制命令

void PlaybackViewModel::play(const QUrl& url)
{
    setIsBusy(true);
    clearError();
    
    qDebug() << "[MVVM-ViewModel] play() called with URL:" << url;
    
    // 统一成本地路径/网络 URL 字符串，便于 UI 与歌词模块使用
    QString filePath;
    if (url.isLocalFile()) {
        filePath = url.toLocalFile();
    } else {
        filePath = url.toString();
    }
    
    if (m_currentFilePath != filePath) {
        m_currentFilePath = filePath;
        emit currentFilePathChanged();
        
        // 仅在曲目变化时触发歌词重载
        emit shouldLoadLyrics(filePath);
    }
    
    if (m_audioService->play(url)) {
        updateMetadata("", "", "", "", url);
        qDebug() << "[MVVM-ViewModel] AudioService play succeeded";
    } else {
        setErrorMessage("Failed to play: " + url.toString());
        qDebug() << "[MVVM-ViewModel] AudioService play failed";
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
    // Prefer realtime service state over cached ViewModel flags.
    // Cached flags can lag during cross-media owner handoff/buffering.
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

    // Fallback to cached state if service has not fully synced yet.
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
    } else {
        setErrorMessage("No audio source loaded");
    }
}

void PlaybackViewModel::playNext()
{
    m_audioService->playNext();
}

void PlaybackViewModel::playPrevious()
{
    m_audioService->playPrevious();
}

void PlaybackViewModel::setVolume(int volume)
{
    if (m_volume != volume) {
        m_volume = qBound(0, volume, 100);
        m_audioService->setVolume(m_volume);
        emit volumeChanged();
    }
}

// AudioService 信号处理

void PlaybackViewModel::onAudioServicePlaybackStarted(const QString& sessionId, const QUrl& url)
{
    Q_UNUSED(sessionId);
    
    qDebug() << "[MVVM-ViewModel] onAudioServicePlaybackStarted, URL:" << url;

    QString filePath;
    if (url.isLocalFile()) {
        filePath = url.toLocalFile();
    } else {
        filePath = url.toString();
    }
    if (m_currentFilePath != filePath) {
        m_currentFilePath = filePath;
        emit currentFilePathChanged();
        emit shouldLoadLyrics(filePath);
    }
    
    updatePlayingState(true);
    updatePausedState(false);
    updateMetadata("", "", "", "", url);
    
    // 通知 UI 启动封面旋转动画
    emit shouldStartRotation();
    
    emit playbackStarted();
}

void PlaybackViewModel::onAudioServicePlaybackPaused()
{
    qDebug() << "[MVVM-ViewModel] onAudioServicePlaybackPaused";
    
    updatePlayingState(false);  // 暂停时播放态应为 false（按钮显示“播放”）
    updatePausedState(true);
    
    // 暂停时停止封面旋转动画
    emit shouldStopRotation();
}

void PlaybackViewModel::onAudioServicePlaybackResumed()
{
    qDebug() << "[MVVM-ViewModel] onAudioServicePlaybackResumed";
    updatePlayingState(true);   // 恢复播放后进入播放态（按钮显示“暂停”）
    updatePausedState(false);
}

void PlaybackViewModel::onAudioServicePlaybackStopped()
{
    qDebug() << "[MVVM-ViewModel] onAudioServicePlaybackStopped";
    
    updatePlayingState(false);
    updatePausedState(false);
    updatePosition(0);
    
    // 停止时同步停止封面旋转动画
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

// ViewModel 内部状态更新

void PlaybackViewModel::updatePlayingState(bool playing)
{
    if (m_isPlaying != playing) {
        m_isPlaying = playing;
        qDebug() << "[MVVM-ViewModel] isPlaying changed to:" << playing;
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

void PlaybackViewModel::updateMetadata(const QString& title, const QString& artist,
                                       const QString& album, const QString& albumArt,
                                       const QUrl& url)
{
    bool changed = false;
    
    if (m_currentTitle != title) {
        m_currentTitle = title;
        emit currentTitleChanged();
        changed = true;
    }
    
    if (m_currentArtist != artist) {
        m_currentArtist = artist;
        emit currentArtistChanged();
        changed = true;
    }
    
    if (m_currentAlbum != album) {
        m_currentAlbum = album;
        emit currentAlbumChanged();
        changed = true;
    }
    
    if (m_currentAlbumArt != albumArt) {
        m_currentAlbumArt = albumArt;
        emit currentAlbumArtChanged();
        changed = true;
    }
    
    if (m_currentUrl != url) {
        m_currentUrl = url;
        emit currentUrlChanged();
        changed = true;
    }
}

// 工具函数

QString PlaybackViewModel::formatTime(qint64 milliseconds)
{
    if (milliseconds < 0) {
        return "00:00";
    }
    
    int totalSeconds = milliseconds / 1000;
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
}

