#ifndef MEDIASERVICE_H
#define MEDIASERVICE_H

#include "AudioService.h"
#include "MediaSession.h"
#include <QObject>
#include <QMap>
#include <QUrl>

/**
 * @brief 媒体服务 - 扩展音频服务以支持视频
 * 
 * 设计思路：
 * - 组合 AudioService（不能继承，因为它是单例）
 * - 新增视频播放支持
 * - 自动检测媒体类型（纯音频 vs 音视频）
 * - 管理 MediaSession（音视频混合会话）
 */
class MediaService : public QObject
{
    Q_OBJECT
    
public:
    static MediaService& instance();
    
    // 播放媒体（自动检测类型）
    bool playMedia(const QUrl& url);
    
    // 显式指定类型
    bool playVideo(const QUrl& url);
    bool playAudioOnly(const QUrl& url);
    
    // 获取当前媒体会话
    MediaSession* currentMediaSession() const;
    
    // 查询当前媒体类型
    bool hasVideo() const;
    bool isAudioOnly() const;
    
signals:
    // 媒体会话信号
    void mediaSessionCreated(MediaSession* session);
    void mediaSessionDestroyed(const QString& sessionId);
    
    // 媒体类型信号
    void audioOnlyMode();  // 切换到纯音频模式
    void videoMode();      // 切换到视频模式
    
    // 视频元数据信号
    void videoMetadataReady(QString title, qint64 duration, int width, int height);
    
private:
    explicit MediaService(QObject *parent = nullptr);
    ~MediaService() override;

private slots:
    void onMediaSessionPositionChanged(qint64 pos);
    void onMediaSessionMetadataReady(const QString& title, const QString& artist, qint64 duration);
    void onMediaSessionStateChanged(MediaSession::PlaybackState state);

private:
    // 禁止拷贝
    MediaService(const MediaService&) = delete;
    MediaService& operator=(const MediaService&) = delete;
    
    // 媒体类型检测
    bool detectHasVideo(const QUrl& url);
    QString detectMediaType(const QUrl& url);
    void connectMediaSessionSignals(MediaSession* session);
    
    // 会话管理
    MediaSession* createMediaSession(const QUrl& url);
    void destroyMediaSession(const QString& sessionId);
    
private:
    // 媒体会话映射 (URL -> MediaSession)
    QMap<QString, MediaSession*> m_mediaSessions;
    
    // 当前播放的媒体会话
    MediaSession* m_currentMediaSession;
    
    // 当前媒体类型
    bool m_currentHasVideo;
};

#endif // MEDIASERVICE_H
