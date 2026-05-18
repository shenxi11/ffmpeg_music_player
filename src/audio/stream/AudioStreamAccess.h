#ifndef AUDIOSTREAMACCESS_H
#define AUDIOSTREAMACCESS_H

#include "AudioNetworkAccess.h"

#include <QByteArray>
#include <QUrl>
#include <atomic>
#include <functional>

/*
模块名: AudioStreamAccess
功能概述: 为网络音频流提供 VLC access 层语义，集中处理 HTTP Range、长度探测、
          中断和错误传播，不直接参与解码或缓冲策略。
对外接口: AudioStreamAccess
依赖关系: AudioNetworkAccess、Qt Network
输入输出: 输入远程媒体 URL 与字节偏移，输出经过 Content-Range 校验的字节块。
异常与错误: 网络失败、Range 不匹配或主动中断时返回 ReadResult 错误状态。
维护说明: 只允许后台预取线程执行 readAt；seek/stop 线程通过 abortCurrentRequest 抢占。
*/
class AudioStreamAccess {
  public:
    using InterruptCallback = std::function<bool()>;

    struct ReadResult {
        bool ok = false;
        bool eof = false;
        bool interrupted = false;
        int statusCode = 0;
        qint64 responseStart = -1;
        qint64 totalLength = -1;
        QByteArray data;
        QString error;
    };

    AudioStreamAccess();

    void setInterruptCallback(InterruptCallback callback);
    bool open(const QUrl& url);
    ReadResult readAt(qint64 offset, qint64 maxBytes, int timeoutMs, qint64 minReturnBytes);
    void abortCurrentRequest();

    qint64 contentLength() const { return m_network.contentLength(); }
    bool canSeek() const { return m_network.canSeek(); }
    QString lastError() const { return m_lastError.isEmpty() ? m_network.lastError() : m_lastError; }

  private:
    bool shouldAbort() const;

    AudioNetworkAccess m_network;
    InterruptCallback m_interruptCallback;
    std::atomic<bool> m_abortRequested{false};
    QString m_lastError;
};

#endif // AUDIOSTREAMACCESS_H
