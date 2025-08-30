#ifndef MOVIEDECODER_H
#define MOVIEDECODER_H

#include <QObject>
#include <QSize>
#include <atomic>
#include "headers.h"

class MovieDecoder : public QObject {
    Q_OBJECT
public:
    explicit MovieDecoder(QObject *parent = nullptr);
    ~MovieDecoder();

    // 基本控制接口
    bool open(const QString &url);
    void close();
    void start();
    void pause();
    void stop();

    // 状态查询
    bool isRunning() const;
    QSize videoSize() const;
    double frameRate() const;

    struct YUVFrame {
        int width;
        int height;
        unsigned char* data;
        int ySize;    // Y 分量长度
        int uvSize;   // U/V 分量长度
    };
public slots:
    void decodeLoop();
    void startDecode(QString url);
signals:
    void signal_resize(int width, int height);
    void frameDecoded(const QImage& frame); // 解码后的帧信号
    void errorOccurred(const QString& error); // 错误信号
    void signal_frameDecoded(AVFrame* frame);
private:
    QImage convertAVFrameToQImage(AVFrame *frame);
    bool initHWAccel(); // 初始化硬件加速
    void cleanup();     // 统一清理资源

    // 解码线程主循环

    // FFmpeg成员
    AVFormatContext* m_fmtCtx = nullptr;
    AVCodecContext* m_codecCtx = nullptr;
    AVBufferRef* m_hwDeviceCtx = nullptr;
    SwsContext* m_swsCtx = nullptr;
    int m_swsWidth = -1;                   // 新增：记录上一次的宽度
    int m_swsHeight = -1;                  // 新增：记录上一次的高度
    AVPixelFormat m_swsFormat = AV_PIX_FMT_NONE; // 新增：记录上一次的像素格式
    // 状态控制
    std::atomic_bool m_running{false};
    std::atomic_bool m_paused{false};
    QThread m_decodeThread;

    QMutex m_mutex;

    // 视频信息
    int m_videoStream = -1;
    QSize m_videoSize;
    double m_frameRate = 0.0;
};


#endif // MOVIEDECODER_H
