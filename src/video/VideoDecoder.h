#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
#include <QImage>
#include <QSize>
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

// 前向声明
class VideoBuffer;

/**
 * @brief 视频帧结构
 */
struct VideoFrame {
    // QImage image;       // 转换后的RGB图像（已移除，OpenGL不需要）
    qint64 pts;         // 显示时间戳（毫秒）
    qint64 duration;    // 帧持续时间（毫秒）
    bool isKeyFrame;    // 是否关键帧
    QSize size;         // 帧尺寸
    
    // OpenGL渲染所需的YUV原始数据
    uint8_t* data[3];   // Y, U, V 平面数据指针
    int linesize[3];    // 每个平面的行跨度
    int width;          // 帧宽度
    int height;         // 帧高度
    
    VideoFrame() : pts(0), duration(0), isKeyFrame(false), width(0), height(0) {
        data[0] = data[1] = data[2] = nullptr;
        linesize[0] = linesize[1] = linesize[2] = 0;
    }
    
    ~VideoFrame() {
        // qDebug() << "[VideoFrame] Destructor started";
        // 释放YUV数据
        for (int i = 0; i < 3; i++) {
            if (data[i]) {
                // qDebug() << "[VideoFrame] Deleting YUV plane" << i;
                delete[] data[i];
                data[i] = nullptr;
                // qDebug() << "[VideoFrame] YUV plane" << i << "deleted";
            }
        }
        // qDebug() << "[VideoFrame] Destructor complete";
    }
};

/**
 * @brief 视频解码器 - FFmpeg视频解码
 * 
 * 设计思路：
 * - 接收 AVPacket 进行解码
 * - 将解码后的 YUV 帧转换为 RGB QImage
 * - 发送 VideoFrame 信号
 */
class VideoDecoder : public QObject
{
    Q_OBJECT
    
public:
    explicit VideoDecoder(QObject* parent = nullptr);
    ~VideoDecoder() override;
    
    // ===== 初始化 =====
    bool init(AVStream* stream);
    bool init(AVCodecContext* codecCtx);
    
    // ===== 解码控制 =====
    void decodePacket(AVPacket* packet);
    void flush();   // Seek时清空缓冲
    void start();   // 启动解码器
    void stop();
    
    // ===== 状态查询 =====
    bool isRunning() const { return m_running; }
    QSize videoSize() const { return m_videoSize; }
    
signals:
    void frameDecoded(VideoFrame* frame);
    void decodingError(QString error);
    
private:
    VideoFrame* convertFrame(AVFrame* srcFrame);
    void cleanup();
    
private:
    // FFmpeg 解码器
    AVCodecContext* m_codecCtx;
    AVCodec* m_codec;
    SwsContext* m_swsCtx;       // 格式转换上下文
    AVFrame* m_frame;           // 临时帧
    
    // 状态
    bool m_running;
    QSize m_videoSize;
    AVRational m_timeBase;      // 时间基准
};

#endif // VIDEODECODER_H
