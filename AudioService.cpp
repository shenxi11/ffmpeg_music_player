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
    // 预热 AudioPlayer 单例，尽量避免首次播放时的设备启动延迟
    qDebug() << "AudioService: Pre-warming AudioPlayer singleton...";
    auto& player = AudioPlayer::instance();
    qDebug() << "AudioService: AudioPlayer ready, all songs will have ZERO startup delay";
    Q_UNUSED(player);
}

AudioService::~AudioService()
{
    // 统一释放所有会话实例
    for (auto session : m_sessions) {
        delete session;
    }
    m_sessions.clear();
}

QString AudioService::createSession()
{
    QString sessionId = QString("session_%1").arg(++m_sessionCounter);
    AudioSession* session = new AudioSession(sessionId, this);
    
    // 转发会话层信号到服务层
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
        // 读取当前索引对应 URL，随 playbackStarted 一并通知 UI
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
    
    // URL 已存在则复用索引，否则追加到播放历史尾部
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
    
    // 为本次播放创建独立会话
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
    }
}

void AudioService::resume()
{
    if (auto session = currentSession()) {
        session->resume();
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
        // 记录 seek 前会话，后续用于一致性校验
        QString currentId = m_currentSessionId;
        session->seekTo(positionMs);
        
        // seek 执行期间若会话被切换，打印告警方便排查
        if (currentId != m_currentSessionId) {
            qDebug() << "Warning: Session switched during seekTo operation";
        }
    }
}

void AudioService::setPlaylist(const QList<QUrl>& urls)
{
    // 播放列表由服务内部维护为“播放历史”，不再接受外部整体覆盖
    // 若需要清空历史，请调用 clearPlaylist()
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
    
    // 删除项后修正当前索引
    if (index < m_currentIndex) {
        m_currentIndex--;
    } else if (index == m_currentIndex) {
        // 删除的是当前播放项：停止播放并重置索引
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
    
    // 在当前项之前插入时，当前索引整体后移
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
    
    // 拖拽重排后修正当前索引
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
    
    // 直接按索引创建并切换会话，避免 play(url) 的索引副作用
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
    
    // 新一轮列表播放前清空随机历史
    m_shuffleHistory.clear();
    
    // 列表播放从第 1 首开始
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
    return session ? (session->isPlaying() && !session->isPaused()) : false;
}

bool AudioService::isPaused() const
{
    auto session = const_cast<AudioService*>(this)->currentSession();
    return session ? (session->isActive() && session->isPaused()) : false;
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
    
    // 根据播放模式决定是否自动切歌
    switch (m_playMode) {
    case Sequential:
        // 顺序模式：仅在非最后一首时自动下一首
        if (m_currentIndex < m_playlist.size() - 1) {
            playNext();
        } else {
            emit playbackStopped();
        }
        break;
    case RepeatOne:
    case RepeatAll:
    case Shuffle:
        // 其余模式均继续自动播放下一首
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
    // 切换会话前先清理旧会话，避免旧解码器继续写入共享播放器
    if (!m_currentSessionId.isEmpty() && m_currentSessionId != sessionId) {
        if (auto oldSession = currentSession()) {
            oldSession->stop();  // 同步停止旧会话解码线程
        }
        // 立即销毁旧会话，避免状态残留
        destroySession(m_currentSessionId);
        qDebug() << "AudioService: Destroyed old session" << m_currentSessionId;
        
        // 旧会话释放后清空共享缓冲，防止跨会话数据串音
        AudioPlayer::instance().resetBuffer();
        qDebug() << "AudioService: Shared AudioPlayer buffer reset";
    }
    
    m_currentSessionId = sessionId;
    
    // 将全局音量同步到新会话
    if (auto newSession = currentSession()) {
        newSession->setVolume(m_globalVolume);
    }
    
    emit currentSessionChanged(sessionId);
}

void AudioService::cleanupOldSessions()
{
    // 仅保留当前会话与最近会话，控制资源占用
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
    
    // 只有一首歌时不做“下一首”切换
    if (m_playlist.size() == 1) {
        return -1;
    }
    
    switch (m_playMode) {
    case Sequential:
    case RepeatAll:
        // 顺序/列表循环：最后一首回到第一首
        return (m_currentIndex + 1) % m_playlist.size();
        
    case RepeatOne:
        // 单曲循环：保持当前索引
        return m_currentIndex;
        
    case Shuffle: {
        // 随机模式：尽量避免短时间内重复
        if (m_playlist.size() <= 1) return 0;
        
        // 记录当前索引到随机历史
        if (m_currentIndex >= 0) {
            m_shuffleHistory.enqueue(m_currentIndex);
            // 限制随机历史长度，避免无限增长
            if (m_shuffleHistory.size() > m_playlist.size() / 2) {
                m_shuffleHistory.dequeue();
            }
        }
        
        // 随机抽取下一首，优先避开最近历史
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
    
    // 只有一首歌时不做“上一首”切换
    if (m_playlist.size() == 1) {
        return -1;
    }
    
    switch (m_playMode) {
    case Sequential:
    case RepeatAll:
        // 顺序/列表循环：第一首回到最后一首
        return (m_currentIndex > 0) ? m_currentIndex - 1 : m_playlist.size() - 1;
        
    case RepeatOne:
        // 单曲循环：保持当前索引
        return m_currentIndex;
        
    case Shuffle: {
        // 随机模式优先使用历史回退
        if (!m_shuffleHistory.isEmpty()) {
            return m_shuffleHistory.takeLast();
        }
        // 无历史时回退到随机选择（避开当前曲目）
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
