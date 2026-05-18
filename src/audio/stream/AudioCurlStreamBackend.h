#ifndef AUDIOCURLSTREAMBACKEND_H
#define AUDIOCURLSTREAMBACKEND_H

#include <QByteArray>
#include <QString>
#include <QUrl>
#include <atomic>
#include <cstdint>
#include <functional>

#ifdef CLOUDMUSIC_AUDIO_USE_LIBCURL
#include <curl/curl.h>
#endif

/*
模块名: AudioCurlStreamBackend
功能概述: 为音频预取层提供可选 libcurl HTTP Range 后端，目标是降低远程媒体
          首包延迟并增强连接复用能力。
对外接口: AudioCurlStreamBackend
依赖关系: libcurl、Qt Core
输入输出: 输入媒体 URL 与 Range 起点，输出连续字节块、状态码和 Content-Range 元数据。
异常与错误: 网络错误、Range 不匹配或主动中断时返回失败状态，调用方负责回退或重试。
维护说明: 仅在 CLOUDMUSIC_AUDIO_USE_LIBCURL 开启时可用；未链接 libcurl 时保持不可用。
*/
class AudioCurlStreamBackend {
  public:
    using InterruptCallback = std::function<bool()>;
    using DataCallback = std::function<bool(const QByteArray& data)>;

    struct Response {
        bool ok = false;
        bool eof = false;
        bool interrupted = false;
        int statusCode = 0;
        qint64 responseStart = -1;
        qint64 totalLength = -1;
        QString error;
    };

    AudioCurlStreamBackend();
    ~AudioCurlStreamBackend();

    static bool isAvailable();

    void setInterruptCallback(InterruptCallback callback);
    bool open(const QUrl& url);
    Response fetchFrom(qint64 offset, const DataCallback& onData);
    void abort();
    void clearAbort();

    qint64 contentLength() const { return m_contentLength; }
    bool canSeek() const { return m_canSeek; }
    QString lastError() const { return m_lastError; }

  private:
    bool shouldAbort() const;

    QUrl m_url;
    qint64 m_contentLength = -1;
    bool m_canSeek = true;
    QString m_lastError;
    InterruptCallback m_interruptCallback;
    std::atomic<bool> m_abortRequested{false};

#ifdef CLOUDMUSIC_AUDIO_USE_LIBCURL
    void* m_easy = nullptr;
    QByteArray m_headerBuffer;
    QByteArray m_encodedUrl;
    QByteArray m_rangeSpec;
    qint64 m_requestedOffset = 0;
    Response* m_activeResponse = nullptr;
    DataCallback m_activeDataCallback;

    static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t headerCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                curl_off_t ultotal, curl_off_t ulnow);
    bool configureCommonOptions();
    bool parseHeaders(Response* response);
#endif
};

#endif // AUDIOCURLSTREAMBACKEND_H
