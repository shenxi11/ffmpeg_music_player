#ifndef AUDIOSTREAMCACHE_H
#define AUDIOSTREAMCACHE_H

#include "AudioNetworkAccess.h"

#include <QByteArray>
#include <QVector>

/*
模块名: AudioStreamCache
功能概述: 参考 VLC cache_read 的多窗口流缓存，优先复用已读取字节窗口，减少网络
          Range hard seek 次数。
对外接口: AudioStreamCache
依赖关系: AudioNetworkAccess
输入输出: 输入 AVIO read/seek 字节请求，输出连续媒体字节。
异常与错误: 读取失败返回负值，EOF 返回 0，调用方负责映射为 FFmpeg 错误码。
维护说明: 该类只处理字节流，不理解音频格式和时间戳。
*/
class AudioStreamCache {
  public:
    enum ReadResult {
        ReadError = -1,
        ReadInterrupted = -2
    };

    explicit AudioStreamCache(AudioNetworkAccess* access);

    int read(uint8_t* dst, int size);
    bool seek(qint64 offset);
    void prepareSeekHint(qint64 targetMs, qint64 targetByte, qint64 contentLength, bool exact);
    qint64 position() const { return m_pos; }
    qint64 size() const;
    bool canSeek() const;
    bool wasLastReadInterrupted() const { return m_lastInterrupted; }

  private:
    struct Track {
        qint64 start = 0;
        QByteArray data;
        qint64 lastUse = 0;
    };

    qint64 trackEnd(const Track& track) const;
    int findTrackContaining(qint64 offset) const;
    int chooseReusableTrack() const;
    bool fetchIntoTrack(Track& track, qint64 minEndExclusive);
    qint64 fetchBlockBytesForOffset(const Track& track, qint64 fetchOffset,
                                    qint64 minEndExclusive) const;
    qint64 minReturnBytesForFetch(const Track& track, qint64 requestedBytes) const;
    void warmSequentialWindow(Track& track);
    void trimTrack(Track& track);
    qint64 resolveSeekOffset(qint64 requestedOffset);

    AudioNetworkAccess* m_access = nullptr;
    QVector<Track> m_tracks;
    int m_currentTrack = 0;
    qint64 m_pos = 0;
    qint64 m_tick = 0;
    bool m_eof = false;
    bool m_lastInterrupted = false;
    bool m_hasPendingSeekHint = false;
    bool m_pendingSeekHintExact = false;
    qint64 m_pendingSeekHintTargetMs = -1;
    qint64 m_pendingSeekHintByte = -1;
    qint64 m_pendingSeekHintContentLength = -1;
    int m_skipWarmReadCount = 0;

    const qint64 m_trackMaxBytes = 16LL * 1024 * 1024;
    const qint64 m_fetchBlockBytes = 1024LL * 1024;
    const qint64 m_readAheadBytes = 4LL * 1024 * 1024;
};

#endif // AUDIOSTREAMCACHE_H
