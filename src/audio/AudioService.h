#ifndef AUDIOSERVICE_H
#define AUDIOSERVICE_H

#include <QObject>
#include <QMap>
#include <QHash>
#include <QQueue>
#include <QRandomGenerator>
#include <QSet>
#include <QTimer>
#include "AudioSession.h"

/**
 * @brief 音频服务协调模块
 * 职责：管理音频会话、播放队列与全局播放状态，并向 UI 提供统一信号。
 */
class AudioService : public QObject
{
    Q_OBJECT
public:
    static AudioService& instance();
    
    // 会话管理
    QString createSession();
    bool destroySession(const QString& sessionId);
    AudioSession* getSession(const QString& sessionId);
    AudioSession* currentSession();
    
    // 当前会话播放控制
    bool play(const QUrl& url);
    void pause();
    void resume();
    void stop();
    void seekTo(qint64 positionMs);
    
    // 播放列表管理
    void setPlaylist(const QList<QUrl>& urls);
    void addToPlaylist(const QUrl& url);
    void removeFromPlaylist(int index);
    void clearPlaylist();
    QList<QUrl> playlist() const { return m_playlist; }
    int playlistSize() const { return m_playlist.size(); }
    
    // 批量编辑操作
    void insertToPlaylist(int index, const QUrl& url);
    void moveInPlaylist(int from, int to);
    
    // 播放模式
    enum PlayMode {
        Sequential,     // 顺序播放
        RepeatOne,      // 单曲循环
        RepeatAll,      // 列表循环
        Shuffle         // 随机播放
    };
    void setPlayMode(PlayMode mode);
    PlayMode playMode() const { return m_playMode; }
    
    // 列表切歌控制
    void playNext();
    void playPrevious();
    void playAtIndex(int index);
    void playPlaylist();  // 从列表第一个条目开始播放
    
    // 全局音量
    void setVolume(int volume);
    int volume() const { return m_globalVolume; }
    
    // 状态查询
    bool isPlaying() const;
    bool isPaused() const;
    int currentIndex() const { return m_currentIndex; }
    QUrl currentUrl() const;
    
signals:
    // 播放状态信号
    void playbackStarted(const QString& sessionId, const QUrl& url);
    void playbackPaused();
    void playbackResumed();
    void playbackStopped();
    
    // 当前会话变化
    void currentSessionChanged(const QString& sessionId);
    
    // 播放列表变化
    void playlistChanged();
    void currentIndexChanged(int index);
    
    // 元数据
    void currentTrackChanged(const QString& title, const QString& artist, qint64 duration);
    void albumArtChanged(const QString& imagePath);
    
    // 播放进度
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    
    // 缓冲状态（网络音频）
    void bufferingStarted();
    void bufferingProgress(int percent);
    void bufferingFinished();
    
    // 错误信号
    void serviceError(const QString& error);

private slots:
    void onAudioCachePathChanged();
    void onSessionStarted();
    void onSessionPaused();
    void onSessionResumed();
    void onSessionFinished();
    void onSessionError(const QString& error);
    void onMetadataReady(const QString& title, const QString& artist, qint64 duration);
    void onAlbumArtReady(const QString& imagePath);
    void onPositionChanged(qint64 positionMs);
    void onBufferingStarted();
    void onBufferingProgress(int percent);
    void onBufferingFinished();
    void onDurationChanged(qint64 durationMs);
    void onSeekDispatchTimeout();

private:
    AudioService(QObject* parent = nullptr);
    ~AudioService();
    AudioService(const AudioService&) = delete;
    AudioService& operator=(const AudioService&) = delete;

    void setupServiceConnections();
    void connectSessionSignals(AudioSession* session);
    
    void switchToSession(const QString& sessionId);
    void playCurrentIndex();
    void cleanupOldSessions();
    int getNextIndex();
    int getPreviousIndex();
    
    // 会话池
    QMap<QString, AudioSession*> m_sessions;
    QString m_currentSessionId;
    int m_sessionCounter;
    
    // 播放列表
    QList<QUrl> m_playlist;
    int m_currentIndex;
    PlayMode m_playMode;
    
    // 全局配置
    int m_globalVolume;
    
    // 随机播放历史
    QQueue<int> m_shuffleHistory;
    QHash<QString, QUrl> m_sessionOriginalSource;
    QHash<QString, QUrl> m_sessionLoadedSource;
    QSet<QString> m_disableHlsForTrack;
    bool m_disableHlsGlobally;

    // seek 防抖合并：快速连续拖动只派发最后一次位置
    QTimer m_seekDispatchTimer;
    bool m_hasPendingSeek;
    qint64 m_pendingSeekPositionMs;
    QString m_pendingSeekSessionId;
};

#endif // AUDIOSERVICE_H



