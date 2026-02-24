#include "AudioService.h"
#include <QDebug>
#include <chrono>

AudioService& AudioService::instance()
{
    static AudioService instance;
    return instance;
}

AudioService::AudioService(QObject* parent)
    : QObject(parent),
      m_sessionCounter(0),
      m_currentIndex(-1),
      m_playMode(Sequential),
      m_globalVolume(100)
{
    // 预热AudioPlayer单例（在程序启动时初始化音频设备，避免第一次播放延迟）
    qDebug() << "AudioService: Pre-warming AudioPlayer singleton...";
    auto& player = AudioPlayer::instance();
    qDebug() << "AudioService: AudioPlayer ready, all songs will have ZERO startup delay";
    Q_UNUSED(player);
}

AudioService::~AudioService()
{
    // 清理所有会话
    for (auto session : m_sessions) {
        delete session;
    }
    m_sessions.clear();
}

QString AudioService::createSession()
{
    QString sessionId = QString("session_%1").arg(++m_sessionCounter);
    AudioSession* session = new AudioSession(sessionId, this);
    
    // 连接会话信号
    connect(session, &AudioSession::sessionFinished, this, &AudioService::onSessionFinished);
    connect(session, &AudioSession::sessionError, this, &AudioService::onSessionError);
    connect(session, &AudioSession::metadataReady, this, &AudioService::onMetadataReady);
    connect(session, &AudioSession::albumArtReady, this, &AudioService::onAlbumArtReady);
    connect(session, &AudioSession::positionChanged, this, &AudioService::onPositionChanged);
    connect(session, &AudioSession::durationChanged, this, &AudioService::onDurationChanged);
    connect(session, &AudioSession::bufferingStarted, this, &AudioService::onBufferingStarted);
    connect(session, &AudioSession::bufferingProgress, this, &AudioService::onBufferingProgress);
    connect(session, &AudioSession::bufferingFinished, this, &AudioService::onBufferingFinished);
    connect(session, &AudioSession::sessionStarted, this, [this, sessionId]() {
        // 获取当前播放的URL
        QUrl currentUrl;
        if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
            currentUrl = m_playlist[m_currentIndex];
        }
        emit playbackStarted(sessionId, currentUrl);
    });
    connect(session, &AudioSession::sessionPaused, this, [this]() {
        emit playbackPaused();
    });
    connect(session, &AudioSession::sessionResumed, this, [this]() {
        emit playbackResumed();
    });
    
    m_sessions[sessionId] = session;
    return sessionId;
}

bool AudioService::destroySession(const QString& sessionId)
{
    if (!m_sessions.contains(sessionId)) return false;
    
    AudioSession* session = m_sessions.take(sessionId);
    delete session;
    return true;
}

AudioSession* AudioService::getSession(const QString& sessionId)
{
    return m_sessions.value(sessionId, nullptr);
}

AudioSession* AudioService::currentSession()
{
    return getSession(m_currentSessionId);
}

bool AudioService::play(const QUrl& url)
{
    auto t0 = std::chrono::high_resolution_clock::now();
    
    // 查找URL在播放历史中的索引
    int foundIndex = m_playlist.indexOf(url);
    if (foundIndex >= 0) {
        m_currentIndex = foundIndex;
        qDebug() << "AudioService: Using existing playlist index" << foundIndex
                 << "total" << m_playlist.size();
    } else {
        m_playlist.append(url);
        m_currentIndex = m_playlist.size() - 1;
        qDebug() << "AudioService: Added to playlist tail at index" << m_currentIndex
                 << "total" << m_playlist.size();
    }
    emit currentIndexChanged(m_currentIndex);
    
    // 创建新会话
    QString sessionId = createSession();
    auto t1 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING] AudioService::play - createSession:" 
             << std::chrono::duration<double, std::milli>(t1 - t0).count() << "ms";
    
    AudioSession* session = getSession(sessionId);
    
    if (!session) return false;
    
    if (session->loadSource(url)) {
        auto t2 = std::chrono::high_resolution_clock::now();
        qDebug() << "[TIMING] AudioService::play - loadSource:" 
                 << std::chrono::duration<double, std::milli>(t2 - t1).count() << "ms";
        
        switchToSession(sessionId);
        session->play();
        
        auto t3 = std::chrono::high_resolution_clock::now();
        qDebug() << "[TIMING] AudioService::play - play() call:" 
                 << std::chrono::duration<double, std::milli>(t3 - t2).count() << "ms";
        qDebug() << "[TIMING] AudioService::play - TOTAL:" 
                 << std::chrono::duration<double, std::milli>(t3 - t0).count() << "ms";
        return true;
    }
    
    destroySession(sessionId);
    return false;
}

void AudioService::pause()
{
    if (auto session = currentSession()) {
        session->pause();
        // 信号由session的sessionPaused转发
    }
}

void AudioService::resume()
{
    if (auto session = currentSession()) {
        session->resume();
        // 信号由session的sessionResumed转发
    }
}

void AudioService::stop()
{
    if (auto session = currentSession()) {
        session->stop();
        emit playbackStopped();
    }
}

void AudioService::seekTo(qint64 positionMs)
{
    AudioSession* session = currentSession();
    if (session) {
        // 保存当前sessionId，确保在seekTo期间session不被切换
        QString currentId = m_currentSessionId;
        session->seekTo(positionMs);
        
        // 检查seekTo执行期间session是否被切换（理论上不应该发生）
        if (currentId != m_currentSessionId) {
            qDebug() << "Warning: Session switched during seekTo operation";
        }
    }
}

void AudioService::setPlaylist(const QList<QUrl>& urls)
{
    // 注意：现在播放列表是播放历史，不再同步外部列表
    // 如果需要清空历史，可以调用 clearPlaylist()
    Q_UNUSED(urls);
    qDebug() << "AudioService: setPlaylist is deprecated, playback history is auto-managed";
}

void AudioService::addToPlaylist(const QUrl& url)
{
    m_playlist.append(url);
    emit playlistChanged();
}

void AudioService::removeFromPlaylist(int index)
{
    if (index < 0 || index >= m_playlist.size()) return;
    
    m_playlist.removeAt(index);
    
    // 调整当前索引
    if (index < m_currentIndex) {
        m_currentIndex--;
    } else if (index == m_currentIndex) {
        // 如果删除的是当前播放的，停止播放
        stop();
        m_currentIndex = -1;
    }
    
    emit playlistChanged();
}

void AudioService::clearPlaylist()
{
    m_playlist.clear();
    m_currentIndex = -1;
    m_shuffleHistory.clear();
    emit playlistChanged();
    qDebug() << "AudioService: Playback history cleared";
}

void AudioService::insertToPlaylist(int index, const QUrl& url)
{
    if (index < 0 || index > m_playlist.size()) {
        addToPlaylist(url);
        return;
    }
    
    m_playlist.insert(index, url);
    
    // 调整当前索引
    if (index <= m_currentIndex) {
        m_currentIndex++;
    }
    
    emit playlistChanged();
}

void AudioService::moveInPlaylist(int from, int to)
{
    if (from < 0 || from >= m_playlist.size() || 
        to < 0 || to >= m_playlist.size() || from == to) {
        return;
    }
    
    QUrl url = m_playlist.takeAt(from);
    m_playlist.insert(to, url);
    
    // 调整当前索引
    if (from == m_currentIndex) {
        m_currentIndex = to;
    } else if (from < m_currentIndex && to >= m_currentIndex) {
        m_currentIndex--;
    } else if (from > m_currentIndex && to <= m_currentIndex) {
        m_currentIndex++;
    }
    
    emit playlistChanged();
}

void AudioService::setPlayMode(PlayMode mode)
{
    m_playMode = mode;
}

void AudioService::playNext()
{
    qDebug() << "AudioService::playNext - Current index:" << m_currentIndex << "Playlist size:" << m_playlist.size() << "Play mode:" << (int)m_playMode;
    int nextIndex = getNextIndex();
    qDebug() << "AudioService::playNext - Next index:" << nextIndex;
    if (nextIndex >= 0) {
        playAtIndex(nextIndex);
    }
}

void AudioService::playPrevious()
{
    qDebug() << "AudioService::playPrevious - Current index:" << m_currentIndex << "Playlist size:" << m_playlist.size() << "Play mode:" << (int)m_playMode;
    int prevIndex = getPreviousIndex();
    qDebug() << "AudioService::playPrevious - Previous index:" << prevIndex;
    if (prevIndex >= 0) {
        playAtIndex(prevIndex);
    }
}

void AudioService::playAtIndex(int index)
{
    if (index < 0 || index >= m_playlist.size()) {
        qWarning() << "AudioService: Invalid playlist index:" << index;
        return;
    }
    
    qDebug() << "AudioService: Playing track at index" << index << "of" << m_playlist.size();
    
    m_currentIndex = index;
    QUrl url = m_playlist[index];
    
    emit currentIndexChanged(m_currentIndex);
    
    // 创建新会话并播放（不调用play，避免索引混乱）
    QString sessionId = createSession();
    AudioSession* session = getSession(sessionId);
    
    if (!session) return;
    
    if (session->loadSource(url)) {
        switchToSession(sessionId);
        session->play();
    } else {
        destroySession(sessionId);
    }
}

void AudioService::playPlaylist()
{
    if (m_playlist.isEmpty()) {
        qWarning() << "AudioService: Cannot play empty playlist";
        return;
    }
    
    // 清空历史
    m_shuffleHistory.clear();
    
    // 从第一首开始播放
    playAtIndex(0);
}

void AudioService::setVolume(int volume)
{
    m_globalVolume = qBound(0, volume, 100);
    
    if (auto session = currentSession()) {
        session->setVolume(m_globalVolume);
    }
}

bool AudioService::isPlaying() const
{
    auto session = const_cast<AudioService*>(this)->currentSession();
    return session ? session->isPlaying() : false;
}

bool AudioService::isPaused() const
{
    auto session = const_cast<AudioService*>(this)->currentSession();
    return session ? (session->isActive() && !session->isPlaying()) : false;
}

QUrl AudioService::currentUrl() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        return m_playlist[m_currentIndex];
    }
    return QUrl();
}

void AudioService::onSessionFinished()
{
    qDebug() << "AudioService: Session finished, current mode:" << m_playMode;
    
    // 根据播放模式决定是否自动播放下一首
    switch (m_playMode) {
    case Sequential:
        // 顺序播放：如果不是最后一首，播放下一首
        if (m_currentIndex < m_playlist.size() - 1) {
            playNext();
        } else {
            emit playbackStopped();
        }
        break;
    case RepeatOne:
    case RepeatAll:
    case Shuffle:
        // 其他模式：自动播放下一首
        playNext();
        break;
    }
}

void AudioService::onDurationChanged(qint64 durationMs)
{
    emit durationChanged(durationMs);
}

void AudioService::onSessionError(const QString& error)
{
    emit serviceError(error);
}

void AudioService::onMetadataReady(const QString& title, const QString& artist, qint64 duration)
{
    emit currentTrackChanged(title, artist, duration);
}

void AudioService::onAlbumArtReady(const QString& imagePath)
{
    emit albumArtChanged(imagePath);
}

void AudioService::onPositionChanged(qint64 positionMs)
{
    emit positionChanged(positionMs);
}

void AudioService::onBufferingStarted()
{
    qDebug() << "AudioService: Buffering started (network may be slow)";
    emit bufferingStarted();
}

void AudioService::onBufferingProgress(int percent)
{
    emit bufferingProgress(percent);
}

void AudioService::onBufferingFinished()
{
    qDebug() << "AudioService: Buffering finished";
    emit bufferingFinished();
}

void AudioService::switchToSession(const QString& sessionId)
{
    // 停止并销毁旧会话（防止旧的decoder继续往共享AudioPlayer写数据）
    if (!m_currentSessionId.isEmpty() && m_currentSessionId != sessionId) {
        if (auto oldSession = currentSession()) {
            oldSession->stop();  // 同步停止decoder线程
        }
        // 立即销毁旧Session，避免数据混乱
        destroySession(m_currentSessionId);
        qDebug() << "AudioService: Destroyed old session" << m_currentSessionId;
        
        // 旧Session完全清理后，清空共享AudioPlayer的缓冲区和时间戳队列
        AudioPlayer::instance().resetBuffer();
        qDebug() << "AudioService: Shared AudioPlayer buffer reset";
    }
    
    m_currentSessionId = sessionId;
    
    // 应用全局音量到新会话
    if (auto newSession = currentSession()) {
        newSession->setVolume(m_globalVolume);
    }
    
    emit currentSessionChanged(sessionId);
}

void AudioService::cleanupOldSessions()
{
    // 保留当前会话和最近的2个会话，删除其他
    if (m_sessions.size() <= 3) return;
    
    QStringList toRemove;
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it.key() != m_currentSessionId) {
            toRemove.append(it.key());
            if (m_sessions.size() - toRemove.size() <= 2) {
                break;
            }
        }
    }
    
    for (const QString& id : toRemove) {
        qDebug() << "AudioService: Cleaning up old session:" << id;
        destroySession(id);
    }
}

void AudioService::playCurrentIndex()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        play(m_playlist[m_currentIndex]);
    }
}

int AudioService::getNextIndex()
{
    if (m_playlist.isEmpty()) {
        qDebug() << "AudioService::getNextIndex - Playlist is empty";
        return -1;
    }
    
    // 只有一首歌时，不切换
    if (m_playlist.size() == 1) {
        return -1;
    }
    
    switch (m_playMode) {
    case Sequential:
    case RepeatAll:
        // 顺序播放和列表循环：最后一首循环到第一首
        return (m_currentIndex + 1) % m_playlist.size();
        
    case RepeatOne:
        // 单曲循环：保持当前
        return m_currentIndex;
        
    case Shuffle: {
        // 随机播放逻辑
        if (m_playlist.size() <= 1) return 0;
        
        // 记录当前索引到历史
        if (m_currentIndex >= 0) {
            m_shuffleHistory.enqueue(m_currentIndex);
            // 限制历史记录大小为播放列表的一半
            if (m_shuffleHistory.size() > m_playlist.size() / 2) {
                m_shuffleHistory.dequeue();
            }
        }
        
        // 生成随机索引，避免重复最近播放的
        int nextIndex;
        int attempts = 0;
        do {
            nextIndex = QRandomGenerator::global()->bounded(m_playlist.size());
            attempts++;
        } while (m_shuffleHistory.contains(nextIndex) && attempts < 10);
        
        return nextIndex;
    }
    }
    
    return -1;
}

int AudioService::getPreviousIndex()
{
    if (m_playlist.isEmpty()) return -1;
    
    // 只有一首歌时，不切换
    if (m_playlist.size() == 1) {
        return -1;
    }
    
    switch (m_playMode) {
    case Sequential:
    case RepeatAll:
        // 顺序播放和列表循环：第一首循环到最后一首
        return (m_currentIndex > 0) ? m_currentIndex - 1 : m_playlist.size() - 1;
        
    case RepeatOne:
        // 单曲循环：保持当前
        return m_currentIndex;
        
    case Shuffle: {
        // 随机播放：从历史记录中取出上一首
        if (!m_shuffleHistory.isEmpty()) {
            return m_shuffleHistory.takeLast();
        }
        // 如果没有历史，随机选择
        if (m_playlist.size() > 1) {
            int prevIndex;
            do {
                prevIndex = QRandomGenerator::global()->bounded(m_playlist.size());
            } while (prevIndex == m_currentIndex);
            return prevIndex;
        }
        return 0;
    }
    }
    
    return -1;
}
