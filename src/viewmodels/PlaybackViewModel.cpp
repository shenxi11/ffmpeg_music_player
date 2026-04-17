#include "PlaybackViewModel.h"

#include "cover_lookup.h"
#include "local_music_cache.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QTime>

PlaybackViewModel::PlaybackViewModel(QObject* parent)
    : BaseViewModel(parent), m_audioService(&AudioService::instance()) {
    m_volume = m_audioService->volume();

    connect(m_audioService, &AudioService::playbackStarted, this,
            &PlaybackViewModel::onAudioServicePlaybackStarted);
    connect(m_audioService, &AudioService::playbackPaused, this,
            &PlaybackViewModel::onAudioServicePlaybackPaused);
    connect(m_audioService, &AudioService::playbackResumed, this,
            &PlaybackViewModel::onAudioServicePlaybackResumed);
    connect(m_audioService, &AudioService::playbackStopped, this,
            &PlaybackViewModel::onAudioServicePlaybackStopped);
    connect(m_audioService, &AudioService::positionChanged, this,
            &PlaybackViewModel::onAudioServicePositionChanged);
    connect(m_audioService, &AudioService::durationChanged, this,
            &PlaybackViewModel::onAudioServiceDurationChanged);
    connect(m_audioService, &AudioService::bufferingStarted, this,
            &PlaybackViewModel::onAudioServiceBufferingStarted);
    connect(m_audioService, &AudioService::bufferingFinished, this,
            &PlaybackViewModel::onAudioServiceBufferingFinished);
    connect(m_audioService, &AudioService::playlistChanged, this, [this]() {
        prunePlaylistMetadataCache();
        emit playlistSnapshotChanged();
    });
    connect(m_audioService, &AudioService::currentIndexChanged, this,
            &PlaybackViewModel::playlistSnapshotChanged);

    connect(m_audioService, &AudioService::currentTrackChanged, this,
            [this](const QString& title, const QString& artist, qint64 duration) {
                updateMetadata(title, artist, QString(), QString(), m_currentUrl);
                updateDuration(duration);
                cacheCurrentTrackMetadata();
                emit playlistSnapshotChanged();
            });
    connect(m_audioService, &AudioService::albumArtChanged, this, [this](const QString& imagePath) {
        const QString trimmed = imagePath.trimmed();
        if (!trimmed.isEmpty() && m_currentAlbumArt != trimmed) {
            m_currentAlbumArt = trimmed;
            emit currentAlbumArtChanged();
            cacheCurrentTrackMetadata();
            emit playlistSnapshotChanged();
        }
    });
}

PlaybackViewModel::~PlaybackViewModel() {}

int PlaybackViewModel::currentIndex() const {
    return m_audioService ? m_audioService->currentIndex() : -1;
}

int PlaybackViewModel::playModeValue() const {
    return m_audioService ? static_cast<int>(m_audioService->playMode()) : 0;
}

bool PlaybackViewModel::hasActiveTrack() const {
    return m_audioService && m_audioService->currentIndex() >= 0 &&
           m_audioService->playlistSize() > 0;
}

QVariantMap PlaybackViewModel::currentTrackSnapshot() const {
    QVariantMap snapshot;

    QString title = m_currentTitle.trimmed();
    if (title.isEmpty()) {
        title = titleFromUrl(m_currentUrl);
    }

    snapshot.insert(QStringLiteral("url"), m_currentUrl.toString());
    snapshot.insert(QStringLiteral("filePath"), m_currentFilePath);
    snapshot.insert(QStringLiteral("title"), title);
    snapshot.insert(QStringLiteral("artist"), m_currentArtist.trimmed());
    snapshot.insert(QStringLiteral("album"), m_currentAlbum.trimmed());
    snapshot.insert(QStringLiteral("albumArt"), m_currentAlbumArt.trimmed());
    snapshot.insert(QStringLiteral("durationMs"), qMax<qint64>(0, m_duration));
    snapshot.insert(QStringLiteral("isLocal"), m_currentUrl.isLocalFile());
    return snapshot;
}

void PlaybackViewModel::play(const QUrl& url) {
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

void PlaybackViewModel::pause() {
    m_audioService->pause();
}

void PlaybackViewModel::resume() {
    m_audioService->resume();
}

void PlaybackViewModel::stop() {
    m_audioService->stop();
}

void PlaybackViewModel::seekTo(qint64 positionMs) {
    m_audioService->seekTo(positionMs);
    updatePosition(positionMs);
}

void PlaybackViewModel::togglePlayPause() {
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

void PlaybackViewModel::playNext() {
    m_audioService->playNext();
}

void PlaybackViewModel::playPrevious() {
    m_audioService->playPrevious();
}

void PlaybackViewModel::appendTrackToQueue(const QUrl& url, bool playIfIdle) {
    if (!url.isValid() || url.isEmpty()) {
        return;
    }

    const bool hasActiveTrack =
        m_audioService->currentIndex() >= 0 && m_audioService->playlistSize() > 0;
    if (!hasActiveTrack && playIfIdle) {
        play(url);
        return;
    }

    m_audioService->appendToQueue(url);
}

void PlaybackViewModel::queueTrackAsNext(const QUrl& url) {
    if (!url.isValid() || url.isEmpty()) {
        return;
    }

    const bool hasActiveTrack =
        m_audioService->currentIndex() >= 0 && m_audioService->playlistSize() > 0;
    if (!hasActiveTrack) {
        play(url);
        return;
    }

    m_audioService->insertNextToQueue(url);
}

void PlaybackViewModel::rememberTrackMetadata(const QUrl& url, const QVariantMap& songData) {
    const QString key = normalizePlaylistEntryPath(url);
    if (key.isEmpty()) {
        return;
    }

    QVariantMap normalized = m_playlistMetadataCache.value(key);
    const QString title = songData.value(QStringLiteral("title")).toString().trimmed();
    const QString artist = songData.value(QStringLiteral("artist")).toString().trimmed();
    QString cover = songData.value(QStringLiteral("cover")).toString().trimmed();
    if (cover.isEmpty()) {
        cover = songData.value(QStringLiteral("cover_art_url")).toString().trimmed();
    }

    if (!title.isEmpty()) {
        normalized.insert(QStringLiteral("title"), title);
    }
    if (!artist.isEmpty()) {
        normalized.insert(QStringLiteral("artist"), artist);
    }
    if (!cover.isEmpty()) {
        normalized.insert(QStringLiteral("cover"), cover);
        rememberCoverForMusicPath(key, cover);
        rememberCoverForSongMeta(title, artist, cover);
    }
    m_playlistMetadataCache.insert(key, normalized);
}

void PlaybackViewModel::removeFromPlaylistUrl(const QString& filePath) {
    const QString normalizedTarget = normalizePlaylistEntryPath(QUrl::fromUserInput(filePath));
    const QList<QUrl> playlist = m_audioService->playlist();
    for (int index = 0; index < playlist.size(); ++index) {
        if (normalizePlaylistEntryPath(playlist.at(index)) == normalizedTarget) {
            m_audioService->removeFromPlaylist(index);
            return;
        }
    }
}

void PlaybackViewModel::clearPlaylist() {
    m_audioService->clearPlaylist();
}

void PlaybackViewModel::setPlayModeValue(int mode) {
    m_audioService->setPlayMode(static_cast<AudioService::PlayMode>(mode));
}

QVariantList PlaybackViewModel::playlistSnapshot() const {
    QVariantList snapshot;
    const QList<QUrl> playlist = m_audioService->playlist();
    snapshot.reserve(playlist.size());

    for (int index = 0; index < playlist.size(); ++index) {
        snapshot.append(buildPlaylistSnapshotItem(playlist.at(index), index));
    }
    return snapshot;
}

void PlaybackViewModel::setVolume(int volume) {
    const int boundedVolume = qBound(0, volume, 100);
    if (m_volume == boundedVolume) {
        return;
    }

    m_volume = boundedVolume;
    m_audioService->setVolume(m_volume);
    emit volumeChanged();
}

void PlaybackViewModel::onAudioServicePlaybackStarted(const QString& sessionId, const QUrl& url) {
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
    cacheCurrentTrackMetadata();
    emit playlistSnapshotChanged();
    emit shouldStartRotation();
    emit playbackStarted();
}

void PlaybackViewModel::onAudioServicePlaybackPaused() {
    updatePlayingState(false);
    updatePausedState(true);
    updateBufferingState(false);
    emit shouldStopRotation();
}

void PlaybackViewModel::onAudioServicePlaybackResumed() {
    updatePlayingState(true);
    updatePausedState(false);
    updateBufferingState(false);
    emit shouldStartRotation();
}

void PlaybackViewModel::onAudioServicePlaybackStopped() {
    updatePlayingState(false);
    updatePausedState(false);
    updateBufferingState(false);
    updatePosition(0);
    emit playlistSnapshotChanged();
    emit shouldStopRotation();
    emit playbackStopped();
}

void PlaybackViewModel::onAudioServicePositionChanged(qint64 position) {
    updatePosition(position);
}

void PlaybackViewModel::onAudioServiceDurationChanged(qint64 duration) {
    updateDuration(duration);
}

void PlaybackViewModel::onAudioServiceBufferingStarted() {
    updateBufferingState(true);
    syncPlaybackStateFromService("bufferingStarted");
}

void PlaybackViewModel::onAudioServiceBufferingFinished() {
    updateBufferingState(false);
    syncPlaybackStateFromService("bufferingFinished");
}

void PlaybackViewModel::syncPlaybackStateFromService(const char* sourceTag) {
    const bool servicePlaying = m_audioService->isPlaying();
    const bool servicePaused = m_audioService->isPaused();

    qDebug() << "[MVVM-UI] Sync playback state from service:" << sourceTag
             << "servicePlaying=" << servicePlaying << "servicePaused=" << servicePaused;

    updatePlayingState(servicePlaying);
    updatePausedState(servicePaused);

    if (servicePlaying && !servicePaused) {
        emit shouldStartRotation();
    } else {
        emit shouldStopRotation();
    }
}

void PlaybackViewModel::updatePlayingState(bool playing) {
    if (m_isPlaying != playing) {
        m_isPlaying = playing;
        emit isPlayingChanged();
    }
}

void PlaybackViewModel::updatePausedState(bool paused) {
    if (m_isPaused != paused) {
        m_isPaused = paused;
        emit isPausedChanged();
    }
}

void PlaybackViewModel::updateBufferingState(bool buffering) {
    if (m_isBuffering != buffering) {
        m_isBuffering = buffering;
        emit isBufferingChanged();
        emit bufferingStateChanged(buffering);
    }
}

void PlaybackViewModel::updatePosition(qint64 pos) {
    if (m_position != pos) {
        m_position = pos;
        emit positionChanged();
    }
}

void PlaybackViewModel::updateDuration(qint64 dur) {
    if (m_duration != dur) {
        m_duration = dur;
        emit durationChanged();
    }
}

void PlaybackViewModel::updateMetadata(const QString& title, const QString& artist,
                                       const QString& album, const QString& albumArt,
                                       const QUrl& url) {
    const bool urlChanged = !url.isEmpty() && m_currentUrl != url;
    if (urlChanged) {
        if (title.trimmed().isEmpty() && !m_currentTitle.isEmpty()) {
            m_currentTitle.clear();
            emit currentTitleChanged();
        }
        if (artist.trimmed().isEmpty() && !m_currentArtist.isEmpty()) {
            m_currentArtist.clear();
            emit currentArtistChanged();
        }
        if (album.trimmed().isEmpty() && !m_currentAlbum.isEmpty()) {
            m_currentAlbum.clear();
            emit currentAlbumChanged();
        }
        if (albumArt.trimmed().isEmpty() && !m_currentAlbumArt.isEmpty()) {
            m_currentAlbumArt.clear();
            emit currentAlbumArtChanged();
        }
    }

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

QString PlaybackViewModel::formatTime(qint64 milliseconds) {
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

    return QStringLiteral("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

QVariantMap PlaybackViewModel::buildPlaylistSnapshotItem(const QUrl& url, int index) const {
    const bool isCurrent = index == m_audioService->currentIndex();
    const QVariantMap cachedMetadata =
        m_playlistMetadataCache.value(normalizePlaylistEntryPath(url));
    const QString fallbackTitle = titleFromUrl(url);
    const QString title =
        isCurrent && !m_currentTitle.trimmed().isEmpty()
            ? m_currentTitle.trimmed()
            : (!cachedMetadata.value(QStringLiteral("title")).toString().trimmed().isEmpty()
                   ? cachedMetadata.value(QStringLiteral("title")).toString().trimmed()
                   : fallbackTitle);
    const QString artist =
        isCurrent && !m_currentArtist.trimmed().isEmpty()
            ? m_currentArtist.trimmed()
            : (!cachedMetadata.value(QStringLiteral("artist")).toString().trimmed().isEmpty()
                   ? cachedMetadata.value(QStringLiteral("artist")).toString().trimmed()
                   : resolveArtistForPlaylistEntry(normalizePlaylistEntryPath(url),
                                                   cachedMetadata));
    const QString cover =
        isCurrent && !m_currentAlbumArt.trimmed().isEmpty()
            ? m_currentAlbumArt.trimmed()
            : (!cachedMetadata.value(QStringLiteral("cover")).toString().trimmed().isEmpty()
                   ? cachedMetadata.value(QStringLiteral("cover")).toString().trimmed()
                   : resolveCoverForPlaylistEntry(normalizePlaylistEntryPath(url), title, artist));

    QVariantMap item;
    item.insert(QStringLiteral("filePath"), normalizePlaylistEntryPath(url));
    item.insert(QStringLiteral("title"), title);
    item.insert(QStringLiteral("artist"), artist);
    item.insert(QStringLiteral("cover"), cover);
    item.insert(QStringLiteral("isCurrent"), isCurrent);
    return item;
}

QString PlaybackViewModel::normalizePlaylistEntryPath(const QUrl& url) {
    if (url.isLocalFile()) {
        return QDir::fromNativeSeparators(url.toLocalFile());
    }
    return url.toString(QUrl::FullyDecoded).trimmed();
}

QString PlaybackViewModel::titleFromUrl(const QUrl& url) {
    QString title;
    if (url.isLocalFile()) {
        title = QFileInfo(url.toLocalFile()).completeBaseName().trimmed();
    } else {
        QString path = QUrl::fromPercentEncoding(url.path().toUtf8()).trimmed();
        const int slashIndex = path.lastIndexOf('/');
        if (slashIndex >= 0) {
            path = path.mid(slashIndex + 1);
        }
        const int dotIndex = path.lastIndexOf('.');
        if (dotIndex > 0) {
            path = path.left(dotIndex);
        }
        title = path.trimmed();
    }

    return title.isEmpty() ? QStringLiteral("未知歌曲") : title;
}

void PlaybackViewModel::prunePlaylistMetadataCache() {
    const QList<QUrl> playlist = m_audioService->playlist();
    QSet<QString> activeKeys;
    activeKeys.reserve(playlist.size());
    for (const QUrl& url : playlist) {
        activeKeys.insert(normalizePlaylistEntryPath(url));
    }

    auto it = m_playlistMetadataCache.begin();
    while (it != m_playlistMetadataCache.end()) {
        if (!activeKeys.contains(it.key())) {
            it = m_playlistMetadataCache.erase(it);
        } else {
            ++it;
        }
    }
}

void PlaybackViewModel::cacheCurrentTrackMetadata() {
    QString key;
    if (!m_currentUrl.isEmpty()) {
        key = normalizePlaylistEntryPath(m_currentUrl);
    } else if (!m_currentFilePath.trimmed().isEmpty()) {
        if (m_currentFilePath.startsWith(QStringLiteral("http"), Qt::CaseInsensitive)) {
            key = normalizePlaylistEntryPath(QUrl(m_currentFilePath));
        } else {
            key = normalizeMusicPathForLookup(m_currentFilePath);
        }
    }

    if (key.isEmpty()) {
        return;
    }

    QVariantMap metadata = m_playlistMetadataCache.value(key);
    const QString title = m_currentTitle.trimmed();
    const QString artist = m_currentArtist.trimmed();
    const QString cover = m_currentAlbumArt.trimmed();

    if (!title.isEmpty()) {
        metadata.insert(QStringLiteral("title"), title);
    }
    if (!artist.isEmpty()) {
        metadata.insert(QStringLiteral("artist"), artist);
    }
    if (!cover.isEmpty()) {
        metadata.insert(QStringLiteral("cover"), cover);
        rememberCoverForMusicPath(key, cover);
        rememberCoverForSongMeta(title, artist, cover);
    }

    if (!metadata.isEmpty()) {
        m_playlistMetadataCache.insert(key, metadata);
    }
}

QString PlaybackViewModel::resolveCoverForPlaylistEntry(const QString& filePath,
                                                        const QString& title,
                                                        const QString& artist) const {
    const QString normalizedPath = normalizeMusicPathForLookup(filePath);
    if (!normalizedPath.isEmpty()) {
        const QList<LocalMusicInfo> localMusicList = LocalMusicCache::instance().getMusicList();
        for (const LocalMusicInfo& info : localMusicList) {
            if (normalizeMusicPathForLookup(info.filePath) == normalizedPath &&
                !info.coverUrl.trimmed().isEmpty()) {
                return info.coverUrl.trimmed();
            }
        }
    }

    const QString resolved = queryBestCoverForTrack(filePath, title, artist);
    if (!resolved.trimmed().isEmpty()) {
        return resolved.trimmed();
    }

    return QStringLiteral("qrc:/qml/assets/ai/icons/default-music-cover.svg");
}

QString PlaybackViewModel::resolveArtistForPlaylistEntry(const QString& filePath,
                                                         const QVariantMap& cachedMetadata) const {
    const QString cachedArtist =
        cachedMetadata.value(QStringLiteral("artist")).toString().trimmed();
    if (!cachedArtist.isEmpty()) {
        return cachedArtist;
    }

    const QString normalizedPath = normalizeMusicPathForLookup(filePath);
    if (!normalizedPath.isEmpty()) {
        const QList<LocalMusicInfo> localMusicList = LocalMusicCache::instance().getMusicList();
        for (const LocalMusicInfo& info : localMusicList) {
            if (normalizeMusicPathForLookup(info.filePath) == normalizedPath &&
                !info.artist.trimmed().isEmpty()) {
                return info.artist.trimmed();
            }
        }
    }

    return QStringLiteral("未知艺术家");
}
