#ifndef AUDIOAVIOBRIDGE_H
#define AUDIOAVIOBRIDGE_H

#include "AudioPrefetchStream.h"

#include "headers.h"

#include <QUrl>
#include <functional>
#include <memory>

/*
模块名: AudioAvioBridge
功能概述: 将 AudioPrefetchStream 暴露为 FFmpeg AVIOContext，使解复用器从后台预取
          缓冲读取网络字节流，而不是在 read 回调中同步发起 HTTP 请求。
对外接口: AudioAvioBridge
依赖关系: FFmpeg AVFormat、AudioPrefetchStream
输入输出: 输入音频 URL，输出挂载到 AVFormatContext 的自定义 AVIO 回调。
异常与错误: 打开失败或读取失败时返回 false/FFmpeg 负错误码。
维护说明: AVIOContext 生命周期必须长于 AVFormatContext 的打开和读取过程。
*/
class AudioAvioBridge {
  public:
    using InterruptCallback = AudioPrefetchStream::InterruptCallback;

    AudioAvioBridge();
    ~AudioAvioBridge();

    void setInterruptCallback(InterruptCallback callback);
    bool open(const QUrl& url);
    bool attachTo(AVFormatContext* formatContext);
    void prepareSeek(qint64 targetMs);

    qint64 contentLength() const;
    bool canSeek() const;
    QString lastError() const { return m_lastError; }

  private:
    static int readPacket(void* opaque, uint8_t* buffer, int bufferSize);
    static int64_t seek(void* opaque, int64_t offset, int whence);

    int readPacket(uint8_t* buffer, int bufferSize);
    int64_t seek(int64_t offset, int whence);

    std::unique_ptr<AudioPrefetchStream> m_stream;
    QUrl m_url;
    AVIOContext* m_avioContext = nullptr;
    unsigned char* m_avioBuffer = nullptr;
    QString m_lastError;
    InterruptCallback m_interruptCallback;
};

#endif // AUDIOAVIOBRIDGE_H
