#include "PlayViewModel.h"
#include <QDebug>

PlayViewModel::PlayViewModel(QObject* parent)
    : QObject(parent)
    , m_audioService(&AudioService::instance())
    , m_isPlaying(false)
    , m_isPaused(false)
    , m_volume(100)
    , m_position(0)
    , m_duration(0)
{
    qDebug() << "[PlayViewModel] Initializing...";
    connectAudioServiceSignals();
    
    // 初始化音量
    m_volume = m_audioService->volume();
    
    qDebug() << "[PlayViewModel] Initialized successfully";
}

PlayViewModel::~PlayViewModel()
{
    qDebug() << "[PlayViewModel] Destructor";
}

void PlayViewModel::connectAudioServiceSignals()
{
    // 播放状态变化
    connect(m_audioService, &AudioService::playbackStarted,
            this, &PlayViewModel::onAudioServicePlaybackStarted);
    connect(m_audioService, &AudioService::playbackPaused,
            this, &PlayViewModel::onAudioServicePlaybackPaused);
    connect(m_audioService, &AudioService::playbackResumed,
            this, &PlayViewModel::onAudioServicePlaybackResumed);
    connect(m_audioService, &AudioService::playbackStopped,
            this, &PlayViewModel::onAudioServicePlaybackStopped);
    connect(m_audioService, &AudioService::playbackFinished,
            this, &PlayViewModel::playbackFinished);
    
    // 播放进度和元数据
    connect(m_audioService, &AudioService::positionChanged,
            this, &PlayViewModel::onAudioServicePositionChanged);
    connect(m_audioService, &AudioService::durationChanged,
            this, &PlayViewModel::onAudioServiceDurationChanged);
    connect(m_audioService, &AudioService::volumeChanged,
            this, &PlayViewModel::onAudioServiceVolumeChanged);
    connect(m_audioService, &AudioService::metadataReady,
            this, &PlayViewModel::onAudioServiceMetadataReady);
    connect(m_audioService, &AudioService::albumArtChanged,
            this, &PlayViewModel::onAudioServiceAlbumArtChanged);
}

// ===== 公共方法实现 =====

void PlayViewModel::setVolume(int volume)
{
    if (m_volume != volume) {
        m_volume = qBound(0, volume, 100);
        m_audioService->setVolume(m_volume);
        emit volumeChanged();
    }
}

void PlayViewModel::play(const QString& filePath)
{
    qDebug() << "[PlayViewModel] play:" << filePath;
    m_currentFilePath = filePath;
    
    QUrl url = QUrl::fromLocalFile(filePath);
    if (!url.isValid()) {
        emit error("Invalid file path: " + filePath);
        return;
    }
    
    bool success = m_audioService->play(url);
    if (!success) {
        emit error("Failed to play: " + filePath);
    }
}

void PlayViewModel::playUrl(const QUrl& url)
{
    qDebug() << "[PlayViewModel] playUrl:" << url;
    m_currentFilePath = url.toString();
    
    bool success = m_audioService->play(url);
    if (!success) {
        emit error("Failed to play URL: " + url.toString());
    }
}

void PlayViewModel::pause()
{
    qDebug() << "[PlayViewModel] pause";
    m_audioService->pause();
}

void PlayViewModel::resume()
{
    qDebug() << "[PlayViewModel] resume";
    m_audioService->resume();
}

void PlayViewModel::stop()
{
    qDebug() << "[PlayViewModel] stop";
    m_audioService->stop();
}

void PlayViewModel::seek(qint64 positionMs)
{
    qDebug() << "[PlayViewModel] seek to" << positionMs << "ms";
    m_audioService->seekTo(positionMs);
}

void PlayViewModel::next()
{
    qDebug() << "[PlayViewModel] next";
    m_audioService->playNext();
}

void PlayViewModel::previous()
{
    qDebug() << "[PlayViewModel] previous";
    m_audioService->playPrevious();
}

void PlayViewModel::togglePlayPause()
{
    if (m_isPlaying && !m_isPaused) {
        pause();
    } else if (m_isPaused) {
        resume();
    } else {
        // 如果有当前文件，重新播放
        if (!m_currentFilePath.isEmpty()) {
            play(m_currentFilePath);
        }
    }
}

// ===== AudioService 信号处理 =====

void PlayViewModel::onAudioServicePlaybackStarted(const QString& sessionId, const QUrl& url)
{
    Q_UNUSED(sessionId);
    
    qDebug() << "[PlayViewModel] Playback started:" << url;
    m_isPlaying = true;
    m_isPaused = false;
    emit playStateChanged();
    emit playbackStarted(url.toString());
}

void PlayViewModel::onAudioServicePlaybackPaused()
{
    qDebug() << "[PlayViewModel] Playback paused";
    m_isPaused = true;
    emit playStateChanged();
    emit playbackPaused();
}

void PlayViewModel::onAudioServicePlaybackResumed()
{
    qDebug() << "[PlayViewModel] Playback resumed";
    m_isPaused = false;
    emit playStateChanged();
    emit playbackResumed();
}

void PlayViewModel::onAudioServicePlaybackStopped()
{
    qDebug() << "[PlayViewModel] Playback stopped";
    m_isPlaying = false;
    m_isPaused = false;
    m_position = 0;
    emit playStateChanged();
    emit positionChanged();
    emit playbackStopped();
}

void PlayViewModel::onAudioServicePositionChanged(qint64 position)
{
    if (m_position != position) {
        m_position = position;
        emit positionChanged();
    }
}

void PlayViewModel::onAudioServiceDurationChanged(qint64 duration)
{
    if (m_duration != duration) {
        m_duration = duration;
        emit durationChanged();
    }
}

void PlayViewModel::onAudioServiceVolumeChanged(int volume)
{
    if (m_volume != volume) {
        m_volume = volume;
        emit volumeChanged();
    }
}

void PlayViewModel::onAudioServiceMetadataReady(const QString& title, const QString& artist, qint64 duration)
{
    qDebug() << "[PlayViewModel] Metadata:" << title << "-" << artist << "duration:" << duration;
    
    bool changed = false;
    
    if (m_currentTrack != title) {
        m_currentTrack = title;
        emit currentTrackChanged();
        changed = true;
    }
    
    if (m_currentArtist != artist) {
        m_currentArtist = artist;
        emit currentArtistChanged();
        changed = true;
    }
    
    if (m_duration != duration) {
        m_duration = duration;
        emit durationChanged();
    }
}

void PlayViewModel::onAudioServiceAlbumArtChanged(const QUrl& artUrl)
{
    QString artPath = artUrl.toString();
    if (m_albumArt != artPath) {
        m_albumArt = artPath;
        emit albumArtChanged();
    }
}

void PlayViewModel::updatePlayState()
{
    // 查询当前会话状态
    auto session = m_audioService->currentSession();
    if (session) {
        bool wasPlaying = m_isPlaying;
        bool wasPaused = m_isPaused;
        
        m_isPlaying = session->isPlaying();
        m_isPaused = session->isPaused();
        
        if (wasPlaying != m_isPlaying || wasPaused != m_isPaused) {
            emit playStateChanged();
        }
    } else {
        if (m_isPlaying || m_isPaused) {
            m_isPlaying = false;
            m_isPaused = false;
            emit playStateChanged();
        }
    }
}
