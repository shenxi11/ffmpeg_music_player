#ifndef PLAYBACKVIEWMODEL_H
#define PLAYBACKVIEWMODEL_H

#include "BaseViewModel.h"
#include "AudioService.h"

#include <QUrl>

/**
 * @brief 播放页视图模型。
 *
 * 对接 AudioService，统一暴露播放状态、曲目信息、缓冲状态与播放命令。
 * 播放视图只处理展示和交互，底层播放逻辑由服务层与该 ViewModel 协同完成。
 */
class PlaybackViewModel : public BaseViewModel
{
    Q_OBJECT

    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY isPausedChanged)
    Q_PROPERTY(bool isBuffering READ isBuffering NOTIFY isBufferingChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY currentTitleChanged)
    Q_PROPERTY(QString currentArtist READ currentArtist NOTIFY currentArtistChanged)
    Q_PROPERTY(QString currentAlbum READ currentAlbum NOTIFY currentAlbumChanged)
    Q_PROPERTY(QString currentAlbumArt READ currentAlbumArt NOTIFY currentAlbumArtChanged)
    Q_PROPERTY(QUrl currentUrl READ currentUrl NOTIFY currentUrlChanged)
    Q_PROPERTY(QString currentFilePath READ currentFilePath NOTIFY currentFilePathChanged)
    Q_PROPERTY(QString positionText READ positionText NOTIFY positionChanged)
    Q_PROPERTY(QString durationText READ durationText NOTIFY durationChanged)

public:
    explicit PlaybackViewModel(QObject* parent = nullptr);
    ~PlaybackViewModel() override;

    bool isPlaying() const { return m_isPlaying; }
    bool isPaused() const { return m_isPaused; }
    bool isBuffering() const { return m_isBuffering; }
    qint64 position() const { return m_position; }
    qint64 duration() const { return m_duration; }
    int volume() const { return m_volume; }
    QString currentTitle() const { return m_currentTitle; }
    QString currentArtist() const { return m_currentArtist; }
    QString currentAlbum() const { return m_currentAlbum; }
    QString currentAlbumArt() const { return m_currentAlbumArt; }
    QUrl currentUrl() const { return m_currentUrl; }
    QString currentFilePath() const { return m_currentFilePath; }
    QString positionText() const { return formatTime(m_position); }
    QString durationText() const { return formatTime(m_duration); }

    Q_INVOKABLE void play(const QUrl& url);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seekTo(qint64 positionMs);
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void playNext();
    Q_INVOKABLE void playPrevious();
    Q_INVOKABLE void removeFromPlaylistUrl(const QString& filePath);
    Q_INVOKABLE void clearPlaylist();
    Q_INVOKABLE void setPlayModeValue(int mode);

    void setVolume(int volume);

signals:
    void isPlayingChanged();
    void isPausedChanged();
    void isBufferingChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void currentTitleChanged();
    void currentArtistChanged();
    void currentAlbumChanged();
    void currentAlbumArtChanged();
    void currentUrlChanged();
    void currentFilePathChanged();

    void playbackStarted();
    void playbackStopped();
    void playbackCompleted();
    void shouldStartRotation();
    void shouldStopRotation();
    void shouldLoadLyrics(const QString& filePath);
    void bufferingStateChanged(bool active);

private slots:
    void onAudioServicePlaybackStarted(const QString& sessionId, const QUrl& url);
    void onAudioServicePlaybackPaused();
    void onAudioServicePlaybackResumed();
    void onAudioServicePlaybackStopped();
    void onAudioServicePositionChanged(qint64 position);
    void onAudioServiceDurationChanged(qint64 duration);
    void onAudioServiceBufferingStarted();
    void onAudioServiceBufferingFinished();

private:
    static QString formatTime(qint64 milliseconds);
    void updatePlayingState(bool playing);
    void updatePausedState(bool paused);
    void updateBufferingState(bool buffering);
    void updatePosition(qint64 pos);
    void updateDuration(qint64 dur);
    void updateMetadata(const QString& title,
                        const QString& artist,
                        const QString& album,
                        const QString& albumArt,
                        const QUrl& url);

    AudioService* m_audioService = nullptr;
    bool m_isPlaying = false;
    bool m_isPaused = false;
    bool m_isBuffering = false;
    qint64 m_position = 0;
    qint64 m_duration = 0;
    int m_volume = 50;
    QString m_currentTitle;
    QString m_currentArtist;
    QString m_currentAlbum;
    QString m_currentAlbumArt;
    QUrl m_currentUrl;
    QString m_currentFilePath;
};

#endif // PLAYBACKVIEWMODEL_H
