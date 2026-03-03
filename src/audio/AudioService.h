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
 * @brief 闊抽鏈嶅姟鍗忚皟妯″潡
 * 鑱岃矗锛氱鐞嗗涓煶棰戜細璇濄€佹挱鏀鹃槦鍒楀拰鍏ㄥ眬鐘舵€?
 */
class AudioService : public QObject
{
    Q_OBJECT
public:
    static AudioService& instance();
    
    // 浼氳瘽绠＄悊
    QString createSession();
    bool destroySession(const QString& sessionId);
    AudioSession* getSession(const QString& sessionId);
    AudioSession* currentSession();
    
    // 鎾斁鎺у埗锛堝綋鍓嶄細璇濓級
    bool play(const QUrl& url);
    void pause();
    void resume();
    void stop();
    void seekTo(qint64 positionMs);
    
    // 鎾斁鍒楄〃绠＄悊
    void setPlaylist(const QList<QUrl>& urls);
    void addToPlaylist(const QUrl& url);
    void removeFromPlaylist(int index);
    void clearPlaylist();
    QList<QUrl> playlist() const { return m_playlist; }
    int playlistSize() const { return m_playlist.size(); }
    
    // 鎵归噺鎿嶄綔
    void insertToPlaylist(int index, const QUrl& url);
    void moveInPlaylist(int from, int to);
    
    // 鎾斁妯″紡
    enum PlayMode {
        Sequential,     // 椤哄簭鎾斁
        RepeatOne,      // 鍗曟洸寰幆
        RepeatAll,      // 鍒楄〃寰幆
        Shuffle         // 闅忔満鎾斁
    };
    void setPlayMode(PlayMode mode);
    PlayMode playMode() const { return m_playMode; }
    
    // 鍒楄〃鎺у埗
    void playNext();
    void playPrevious();
    void playAtIndex(int index);
    void playPlaylist();  // 浠庡ご寮€濮嬫挱鏀惧垪琛?
    
    // 鍏ㄥ眬闊抽噺
    void setVolume(int volume);
    int volume() const { return m_globalVolume; }
    
    // 鐘舵€佹煡璇?
    bool isPlaying() const;
    bool isPaused() const;
    int currentIndex() const { return m_currentIndex; }
    QUrl currentUrl() const;
    
signals:
    // 鎾斁鐘舵€?
    void playbackStarted(const QString& sessionId, const QUrl& url);
    void playbackPaused();
    void playbackResumed();
    void playbackStopped();
    
    // 褰撳墠浼氳瘽鍙樺寲
    void currentSessionChanged(const QString& sessionId);
    
    // 鎾斁鍒楄〃鍙樺寲
    void playlistChanged();
    void currentIndexChanged(int index);
    
    // 鍏冩暟鎹?
    void currentTrackChanged(const QString& title, const QString& artist, qint64 duration);
    void albumArtChanged(const QString& imagePath);
    
    // 杩涘害
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    
    // 缂撳啿鐘舵€侊紙缃戠粶闊抽鍗￠】鏃讹級
    void bufferingStarted();
    void bufferingProgress(int percent);
    void bufferingFinished();
    
    // 閿欒
    void serviceError(const QString& error);

private slots:
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
    
    void switchToSession(const QString& sessionId);
    void playCurrentIndex();
    void cleanupOldSessions();
    int getNextIndex();
    int getPreviousIndex();
    
    // 浼氳瘽姹?
    QMap<QString, AudioSession*> m_sessions;
    QString m_currentSessionId;
    int m_sessionCounter;
    
    // 鎾斁鍒楄〃
    QList<QUrl> m_playlist;
    int m_currentIndex;
    PlayMode m_playMode;
    
    // 鍏ㄥ眬璁剧疆
    int m_globalVolume;
    
    // 闅忔満鎾斁鍘嗗彶
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



