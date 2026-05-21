#ifndef AUDIONETWORKACCESS_H
#define AUDIONETWORKACCESS_H

#include <QByteArray>
#include <QUrl>
#include <functional>

/*
模块名: AudioNetworkAccess
功能概述: 为音频自定义 AVIO 提供同步 HTTP Range 访问，并严格校验服务端返回的
          Content-Range，避免错误字节流进入解码器。
对外接口: AudioNetworkAccess
依赖关系: CurlNetworkTransport、libcurl
输入输出: 输入远程音频 URL 与字节范围，输出已验证的字节块和媒体长度。
异常与错误: 网络错误、Range 不匹配、服务端不支持随机访问时返回失败状态。
维护说明: 仅供解码线程/AVIO 回调使用，调用方必须提供中断回调用于 seek/stop 抢占。
*/
class AudioNetworkAccess {
  public:
    struct RangeResult {
        bool ok = false;
        bool eof = false;
        bool interrupted = false;
        bool partial = false;
        bool timedOut = false;
        int statusCode = 0;
        qint64 responseStart = -1;
        qint64 totalLength = -1;
        QByteArray data;
        QString error;
    };

    using InterruptCallback = std::function<bool()>;

    AudioNetworkAccess() = default;

    void setInterruptCallback(InterruptCallback callback);

    bool open(const QUrl& url);
    RangeResult fetchRange(qint64 offset, qint64 maxBytes, int timeoutMs,
                           qint64 minReturnBytes = 0);

    qint64 contentLength() const { return m_contentLength; }
    bool canSeek() const { return m_canSeek; }
    QString lastError() const { return m_lastError; }

  private:
    struct Response {
        bool ok = false;
        bool interrupted = false;
        bool partial = false;
        bool timedOut = false;
        int statusCode = 0;
        QByteArray body;
        QByteArray acceptRanges;
        QByteArray contentRange;
        qint64 contentLength = -1;
        QString error;
    };

    Response sendRequest(const QByteArray& method, const QByteArray& rangeHeader, int timeoutMs,
                         qint64 minReturnBytes = 0);
    bool shouldAbort() const;
    bool parseContentRange(const QByteArray& value, qint64* start, qint64* end,
                           qint64* total) const;

    QUrl m_url;
    qint64 m_contentLength = -1;
    bool m_canSeek = false;
    QString m_lastError;
    InterruptCallback m_interruptCallback;
};

#endif // AUDIONETWORKACCESS_H
