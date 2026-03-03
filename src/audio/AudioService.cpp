#include "AudioService.h"
#include "AudioCacheManager.h"
#include "settings_manager.h"
#include <QDebug>
#include <chrono>

namespace {
bool isNetworkUrl(const QUrl& url)
{
    const QString scheme = url.scheme().toLower();
    return scheme == "http" || scheme == "https";
}

bool isAudioFilePath(const QString& pathLower)
{
    return pathLower.endsWith(".mp3")
            || pathLower.endsWith(".flac")
            || pathLower.endsWith(".wav")
            || pathLower.endsWith(".ogg")
            || pathLower.endsWith(".m4a")
            || pathLower.endsWith(".aac");
}

bool isHlsPlaylistUrl(const QUrl& url)
{
    const QString lower = url.toString().toLower();
    return lower.endsWith(".m3u8") || lower.contains("/hls/");
}

bool isRemoteFlacUrl(const QUrl& url)
{
    if (!isNetworkUrl(url)) {
        return false;
    }
    return url.path().toLower().endsWith(".flac");
}

constexpr int kRemoteFlacBufferCapacityBytes = 4 * 1024 * 1024;

QUrl buildHlsCandidate(const QUrl& original)
{
    if (!isNetworkUrl(original)) {
        return QUrl();
    }

    const QString path = original.path();
    const QString pathLower = path.toLower();
    if (pathLower.endsWith(".m3u8")) {
        return QUrl();
    }
    // 远端 FLAC 走直链随机访问更稳定，避免切到 HLS 后 seek 长时间拿不到首包。
    if (pathLower.endsWith(".flac")) {
        return QUrl();
    }
    if (!path.startsWith("/uploads/") || !isAudioFilePath(pathLower)) {
        return QUrl();
    }

    const int lastSlash = path.lastIndexOf('/');
    if (lastSlash <= 0) {
        return QUrl();
    }

    QString hlsDir = path.left(lastSlash);
    hlsDir.replace(0, QString("/uploads/").size(), "/hls/");

    QUrl candidate = original;
    candidate.setPath(hlsDir + "/index.m3u8");
    return candidate;
}

bool loadSessionSourcePreferHls(AudioSession* session,
                                const QUrl& original,
                                bool forceOriginal,
                                QUrl* loadedSource)
{
    if (!session) {
        return false;
    }

    if (forceOriginal) {
        const bool ok = session->loadSource(original);
        if (ok && loadedSource) {
            *loadedSource = original;
        }
        return ok;
    }

    const QUrl hlsCandidate = buildHlsCandidate(original);
    if (hlsCandidate.isValid() && !hlsCandidate.isEmpty()) {
        qDebug() << "AudioService: trying HLS source first:" << hlsCandidate.toString();
        if (session->loadSource(hlsCandidate)) {
            if (loadedSource) {
                *loadedSource = hlsCandidate;
            }
            return true;
        }
        qWarning() << "AudioService: HLS source unavailable, fallback to original URL:" << original.toString();
    }

    const bool ok = session->loadSource(original);
    if (ok && loadedSource) {
        *loadedSource = original;
    }
    return ok;
}
}

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
      m_globalVolume(100),
      m_disableHlsGlobally(false),
      m_hasPendingSeek(false),
      m_pendingSeekPositionMs(0)
{
    // 预热 AudioPlayer 单例，尽量避免首次播放时的设备启动延迟
    qDebug() << "AudioService: Pre-warming AudioPlayer singleton...";
    auto& player = AudioPlayer::instance();
    qDebug() << "AudioService: AudioPlayer ready, all songs will have ZERO startup delay";
    Q_UNUSED(player);

    m_seekDispatchTimer.setSingleShot(true);
    connect(&m_seekDispatchTimer, &QTimer::timeout, this, &AudioService::onSeekDispatchTimeout);

    AudioCacheManager::instance().setCacheDirectory(SettingsManager::instance().audioCachePath());
    connect(&SettingsManager::instance(), &SettingsManager::audioCachePathChanged, this, []() {
        AudioCacheManager::instance().setCacheDirectory(SettingsManager::instance().audioCachePath());
    });
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
    m_sessionOriginalSource.remove(sessionId);
    m_sessionLoadedSource.remove(sessionId);
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
    m_hasPendingSeek = false;
    m_pendingSeekSessionId.clear();
    m_seekDispatchTimer.stop();
    
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
    
    QUrl playbackSource = url;
    if (isRemoteFlacUrl(url)) {
        qWarning() << "AudioService: disable proxy/cache for remote FLAC to keep seek stable:" << url.toString();
        AudioPlayer::instance().ensureBufferCapacity(kRemoteFlacBufferCapacityBytes);
    } else {
        AudioCacheManager::instance().warmupCache(url);
        playbackSource = AudioCacheManager::instance().resolvePlaybackUrl(url);
    }
    if (playbackSource != url) {
        qDebug() << "AudioService: using cache/proxy playback source:" << playbackSource.toString();
    }

    QUrl loadedSource;
    const bool forceOriginal = isRemoteFlacUrl(url)
            || m_disableHlsGlobally
            || m_disableHlsForTrack.contains(url.toString());
    if (loadSessionSourcePreferHls(session, playbackSource, forceOriginal, &loadedSource)
            || (playbackSource != url
                && loadSessionSourcePreferHls(session, url, forceOriginal, &loadedSource))) {
        if (loadedSource == url && playbackSource != url) {
            qWarning() << "AudioService: proxy source unavailable, fallback to direct URL.";
        }
        m_sessionOriginalSource[sessionId] = url;
        m_sessionLoadedSource[sessionId] = loadedSource;
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
    m_hasPendingSeek = false;
    m_pendingSeekSessionId.clear();
    m_seekDispatchTimer.stop();
    if (auto session = currentSession()) {
        session->stop();
        emit playbackStopped();
    }
}

void AudioService::seekTo(qint64 positionMs)
{
    AudioSession* session = currentSession();
    if (!session) {
        return;
    }

    const qint64 targetMs = qMax<qint64>(0, positionMs);
    const QUrl loadedSource = m_sessionLoadedSource.value(m_currentSessionId);
    const QUrl originalSource = m_sessionOriginalSource.value(m_currentSessionId);
    const qint64 currentPos = qMax<qint64>(0, session->position());
    const qint64 jumpDistance = qAbs(targetMs - currentPos);
    const bool largeJump = jumpDistance >= 45 * 1000;

    if (originalSource.isValid() && !originalSource.isEmpty() && !isRemoteFlacUrl(originalSource)) {
        AudioCacheManager::instance().prefetchForSeek(originalSource, targetMs, session->duration());
    }

    // 对 HLS 的首次大跨度跳转做主动优化：直接切换到可随机访问更稳定的直链。
    if (!m_disableHlsGlobally
            && !isRemoteFlacUrl(originalSource)
            && isHlsPlaylistUrl(loadedSource)
            && originalSource.isValid()
            && !originalSource.isEmpty()
            && largeJump) {
        qWarning() << "AudioService: Large seek on HLS, proactive switch to direct stream."
                   << "jumpMs:" << jumpDistance
                   << "targetMs:" << targetMs
                   << "track:" << originalSource.toString();

        m_disableHlsForTrack.insert(originalSource.toString());

        const QString newSessionId = createSession();
        AudioSession* newSession = getSession(newSessionId);
        if (newSession) {
            QUrl loadedDirect;
            if (loadSessionSourcePreferHls(newSession, originalSource, true, &loadedDirect)) {
                m_sessionOriginalSource[newSessionId] = originalSource;
                m_sessionLoadedSource[newSessionId] = loadedDirect;

                m_hasPendingSeek = false;
                m_pendingSeekSessionId.clear();
                m_seekDispatchTimer.stop();

                switchToSession(newSessionId);
                newSession->play();
                newSession->seekTo(targetMs);
                return;
            }
        }
        destroySession(newSessionId);
        qWarning() << "AudioService: Proactive direct-switch seek failed, fallback to current session seek.";
    }

    m_pendingSeekPositionMs = targetMs;
    m_pendingSeekSessionId = m_currentSessionId;
    m_hasPendingSeek = true;

    // 进度条频繁拖动时，合并 seek 请求，只执行最后一次。
    m_seekDispatchTimer.start(60);
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
    QUrl playbackSource = url;
    if (isRemoteFlacUrl(url)) {
        qWarning() << "AudioService: disable proxy/cache for remote FLAC in playAtIndex:" << url.toString();
        AudioPlayer::instance().ensureBufferCapacity(kRemoteFlacBufferCapacityBytes);
    } else {
        AudioCacheManager::instance().warmupCache(url);
        playbackSource = AudioCacheManager::instance().resolvePlaybackUrl(url);
    }
    
    emit currentIndexChanged(m_currentIndex);
    
    // 直接按索引创建并切换会话，避免 play(url) 的索引副作用
    QString sessionId = createSession();
    AudioSession* session = getSession(sessionId);
    
    if (!session) return;
    
    QUrl loadedSource;
    const bool forceOriginal = isRemoteFlacUrl(url)
            || m_disableHlsGlobally
            || m_disableHlsForTrack.contains(url.toString());
    if (loadSessionSourcePreferHls(session, playbackSource, forceOriginal, &loadedSource)
            || (playbackSource != url
                && loadSessionSourcePreferHls(session, url, forceOriginal, &loadedSource))) {
        if (loadedSource == url && playbackSource != url) {
            qWarning() << "AudioService: proxy source unavailable in playAtIndex, fallback to direct URL.";
        }
        m_sessionOriginalSource[sessionId] = url;
        m_sessionLoadedSource[sessionId] = loadedSource;
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
    const QUrl originalSource = m_sessionOriginalSource.value(m_currentSessionId);
    if (originalSource.isValid() && !originalSource.isEmpty()) {
        AudioCacheManager::instance().noteTrackDuration(originalSource, durationMs);
    }
    emit durationChanged(durationMs);
}

void AudioService::onSeekDispatchTimeout()
{
    if (!m_hasPendingSeek) {
        return;
    }

    const QString targetSessionId = m_pendingSeekSessionId;
    const qint64 targetPositionMs = m_pendingSeekPositionMs;
    m_hasPendingSeek = false;

    if (targetSessionId.isEmpty() || targetSessionId != m_currentSessionId) {
        qDebug() << "AudioService: Skip stale coalesced seek, session switched";
        return;
    }

    AudioSession* session = currentSession();
    if (!session) {
        return;
    }

    const QString currentId = m_currentSessionId;
    session->seekTo(targetPositionMs);
    if (currentId != m_currentSessionId) {
        qDebug() << "Warning: Session switched during seekTo operation";
    }
}

void AudioService::onSessionError(const QString& error)
{
    if (!m_currentSessionId.isEmpty()) {
        const QUrl loadedSource = m_sessionLoadedSource.value(m_currentSessionId);
        const QUrl originalSource = m_sessionOriginalSource.value(m_currentSessionId);
        const bool isHlsLoaded = loadedSource.path().toLower().endsWith(".m3u8");
        const bool canFallbackToDirect = isHlsLoaded && originalSource.isValid() && !originalSource.isEmpty();
        const bool isSeekTimeout = error.contains(QStringLiteral("缓冲超时"));

        if (isSeekTimeout && canFallbackToDirect) {
            qint64 targetMs = m_pendingSeekPositionMs;
            if (targetMs <= 0) {
                if (auto session = currentSession()) {
                    targetMs = session->position();
                }
            }
            if (targetMs < 0) {
                targetMs = 0;
            }

            qWarning() << "AudioService: HLS seek timeout, fallback to direct stream."
                       << "track:" << originalSource.toString()
                       << "targetMs:" << targetMs;

            m_disableHlsForTrack.insert(originalSource.toString());
            if (!m_disableHlsGlobally) {
                m_disableHlsGlobally = true;
                qWarning() << "AudioService: Disable HLS globally for current run due to seek timeout instability.";
            }

            const QString newSessionId = createSession();
            AudioSession* newSession = getSession(newSessionId);
            if (newSession) {
                QUrl loadedDirect;
                if (loadSessionSourcePreferHls(newSession, originalSource, true, &loadedDirect)) {
                    m_sessionOriginalSource[newSessionId] = originalSource;
                    m_sessionLoadedSource[newSessionId] = loadedDirect;
                    switchToSession(newSessionId);
                    newSession->play();
                    if (targetMs > 0) {
                        newSession->seekTo(targetMs);
                    }
                    return;
                }
            }
            destroySession(newSessionId);
        }
    }

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
    if (sessionId != m_currentSessionId) {
        m_hasPendingSeek = false;
        m_pendingSeekSessionId.clear();
        m_seekDispatchTimer.stop();
    }

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



