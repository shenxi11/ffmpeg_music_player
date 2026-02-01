#ifndef VIDEOSESSION_H
#define VIDEOSESSION_H

#include <QObject>
#include <QThread>
#include <QSize>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

// 前向声明
class VideoDecoder;
class VideoRenderer;
class VideoBuffer;
struct VideoFrame;

/**
 * @brief 视频会话 - 管理视频解码和渲染
 * 
 * 设计思路：
 * - 管理 VideoDecoder（解码）、VideoBuffer（缓冲）、VideoRenderer（渲染）
 * - 提供PTS查询用于音视频同步
 * - 帧率控制和跳帧策略
 */
class VideoSession : public QObject
{
    Q_OBJECT
    
public:
    explicit VideoSession(QObject* parent = nullptr);
    ~VideoSession() override;
    
    // ===== 初始化 =====
    bool initVideoStream(AVStream* stream);
    
    // ===== 解码控制 =====
    void start();
    void pause();
    void stop();
    void seekTo(qint64 positionMs);
    void flush();  // 清空缓冲区
    
    // ===== 数据输入 =====
    void pushPacket(AVPacket* packet);
    
    // ===== 同步控制 =====
    qint64 getCurrentPTS() const;     // 当前显示帧的PTS
    void holdFrame();                 // 暂停渲染（等待音频）
    void skipNonKeyFrames();          // 跳帧加速
    void resumeRendering();           // 恢复渲染
    
    // ===== 获取组件 =====
    QWidget* videoRenderer() const { return m_renderer; }
    VideoBuffer* videoBuffer() const { return m_buffer; }
    
    // ===== 设置组件 =====
    void setVideoRenderer(QWidget* renderer);
    
    // ===== 状态查询 =====
    bool isRunning() const { return m_running; }
    QSize videoSize() const { return m_videoSize; }
    
signals:
    void frameRendered(qint64 pts);           // 帧已渲染
    void videoSizeChanged(QSize size);        // 视频尺寸改变
    void decodingError(QString error);        // 解码错误
    void bufferFull();                        // 缓冲区满
    void bufferEmpty();                       // 缓冲区空
    
private slots:
    void onFrameDecoded(VideoFrame* frame);
    
private:
    // 组件
    VideoDecoder* m_decoder;
    QWidget* m_renderer;  // 可以是VideoRenderer或VideoRendererGL
    VideoBuffer* m_buffer;
    
    // 线程
    QThread* m_decodeThread;
    
    // 状态
    bool m_running;
    bool m_holdFrame;           // 是否暂停渲染
    qint64 m_currentPTS;        // 当前PTS
    QSize m_videoSize;          // 视频尺寸
    
    // 流信息
    AVStream* m_stream;
};

#endif // VIDEOSESSION_H
