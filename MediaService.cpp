#include "MediaService.h"
#include "MediaSession.h"
#include <QFileInfo>
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
}

MediaService::MediaService(QObject *parent)
    : QObject(parent)
    , m_currentMediaSession(nullptr)
    , m_currentHasVideo(false)
{
    qDebug() << "[MediaService] Initialized";
}

MediaService::~MediaService()
{
    // 清理所有媒体会话
    for (auto it = m_mediaSessions.begin(); it != m_mediaSessions.end(); ++it) {
        delete it.value();
    }
    m_mediaSessions.clear();
    
    qDebug() << "[MediaService] Destroyed";
}

MediaService& MediaService::instance()
{
    static MediaService instance;
    return instance;
}

bool MediaService::playMedia(const QUrl& url)
{
    qDebug() << "[MediaService] Play media:" << url;
    
    // 检测媒体类型
    bool hasVideo = detectHasVideo(url);
    
    if (hasVideo) {
        return playVideo(url);
    } else {
        return playAudioOnly(url);
    }
}

bool MediaService::playVideo(const QUrl& url)
{
    qDebug() << "[MediaService] Play video:" << url;
    
    // 创建或获取媒体会话
    QString sessionKey = url.toString();
    MediaSession* session = m_mediaSessions.value(sessionKey, nullptr);
    
    if (!session) {
        session = createMediaSession(url);
        if (!session) {
            qWarning() << "[MediaService] Failed to create media session";
            return false;
        }
        m_mediaSessions[sessionKey] = session;
    }
    
    // 设置为当前会话
    m_currentMediaSession = session;
    m_currentHasVideo = true;
    
    // 加载并播放
    if (!session->loadSource(url)) {
        qWarning() << "[MediaService] Failed to load video source";
        return false;
    }
    
    session->play();
    
    emit videoMode();
    emit mediaSessionCreated(session);
    
    return true;
}

bool MediaService::playAudioOnly(const QUrl& url)
{
    qDebug() << "[MediaService] Play audio only:" << url;
    
    // 使用 AudioService 单例的播放功能
    m_currentMediaSession = nullptr;
    m_currentHasVideo = false;
    
    emit audioOnlyMode();
    
    return AudioService::instance().play(url);
}

MediaSession* MediaService::currentMediaSession() const
{
    return m_currentMediaSession;
}

bool MediaService::hasVideo() const
{
    return m_currentHasVideo && m_currentMediaSession != nullptr;
}

bool MediaService::isAudioOnly() const
{
    return !m_currentHasVideo;
}

bool MediaService::detectHasVideo(const QUrl& url)
{
    QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    
    // 方法1：通过文件扩展名快速判断
    QFileInfo fileInfo(path);
    QString suffix = fileInfo.suffix().toLower();
    
    QStringList videoExtensions = {"mp4", "avi", "mkv", "mov", "flv", "wmv", "webm", "m4v", "mpg", "mpeg"};
    QStringList audioExtensions = {"mp3", "wav", "flac", "aac", "ogg", "m4a", "wma"};
    
    if (videoExtensions.contains(suffix)) {
        qDebug() << "[MediaService] Detected video by extension:" << suffix;
        return true;
    }
    
    if (audioExtensions.contains(suffix)) {
        qDebug() << "[MediaService] Detected audio only by extension:" << suffix;
        return false;
    }
    
    // 方法2：通过 FFmpeg 检测流类型（更准确但较慢）
    AVFormatContext* formatContext = nullptr;
    int ret = avformat_open_input(&formatContext, path.toUtf8().data(), nullptr, nullptr);
    
    if (ret < 0) {
        qWarning() << "[MediaService] Failed to open file for detection:" << path;
        return false;
    }
    
    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0) {
        avformat_close_input(&formatContext);
        return false;
    }
    
    // 查找视频流
    bool hasVideo = false;
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            hasVideo = true;
            break;
        }
    }
    
    avformat_close_input(&formatContext);
    
    qDebug() << "[MediaService] Detected by FFmpeg - hasVideo:" << hasVideo;
    return hasVideo;
}

QString MediaService::detectMediaType(const QUrl& url)
{
    return detectHasVideo(url) ? "video" : "audio";
}

MediaSession* MediaService::createMediaSession(const QUrl& url)
{
    qDebug() << "[MediaService] Creating media session for:" << url;
    
    MediaSession* session = new MediaSession(this);
    
    // 连接信号
    connect(session, &MediaSession::positionChanged, this, [this](qint64 pos) {
        // 转发位置变化信号（如果需要）
        qDebug() << "[MediaService] Position:" << pos << "ms";
    });
    
    connect(session, &MediaSession::metadataReady, this, [this](QString title, QString artist, qint64 duration) {
        qDebug() << "[MediaService] Metadata ready - Title:" << title << "Duration:" << duration;
        // TODO: 发送视频元数据信号
    });
    
    connect(session, &MediaSession::stateChanged, this, [](MediaSession::PlaybackState state) {
        qDebug() << "[MediaService] Playback state changed:" << (int)state;
    });
    
    return session;
}

void MediaService::destroyMediaSession(const QString& sessionId)
{
    if (m_mediaSessions.contains(sessionId)) {
        MediaSession* session = m_mediaSessions.take(sessionId);
        
        if (session == m_currentMediaSession) {
            m_currentMediaSession = nullptr;
            m_currentHasVideo = false;
        }
        
        delete session;
        
        emit mediaSessionDestroyed(sessionId);
    }
}
