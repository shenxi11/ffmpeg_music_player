#include "AudioDecoder.h"
#include "AudioCacheManager.h"
#include "stream/AudioAvioBridge.h"

#include <QStandardPaths>
#include <QDateTime>
#include <QFile>
#include <QUrl>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <limits>
extern "C" {
#include <libavutil/time.h>
}

namespace {
constexpr int kNetworkIoDeadlineMaxMs = 5000;
constexpr int kNetworkOpenDeadlineMs = 5000;
constexpr int kNetworkStreamInfoDeadlineMs = 5000;
constexpr int kNetworkDefaultReadDeadlineMs = 2600;
constexpr int kSeekDeadlineHttpMs = 2200;
constexpr int kSeekDeadlineLocalProxyMs = 3600;
constexpr int kSeekDeadlineFlacHttpMs = 4500;
constexpr int kSeekDeadlineFlacLocalProxyMs = 5000;
constexpr int kSeekFirstPacketDeadlineHttpMs = 2500;
constexpr int kSeekFirstPacketDeadlineFlacHttpMs = 5000;
constexpr int kSeekFirstPacketDeadlineLocalProxyMs = 4200;
constexpr int kSeekFirstPacketDeadlineFlacLocalProxyMs = 5000;
constexpr int kSeekFirstPacketDeadlineHlsMs = 4500;
constexpr int kSeekEofRetryMax = 8;
constexpr int kSeekReadFailureRetryMax = 10;
constexpr qint64 kFallbackSeekAcceptToleranceMs = 12000;

QString describeSeekFlags(int flags)
{
    QString text;
    if (flags & AVSEEK_FLAG_BACKWARD) {
        text += QStringLiteral("BACKWARD|");
    }
    if (flags & AVSEEK_FLAG_BYTE) {
        text += QStringLiteral("BYTE|");
    }
    if (flags & AVSEEK_FLAG_ANY) {
        text += QStringLiteral("ANY|");
    }
    if (flags & AVSEEK_FLAG_FRAME) {
        text += QStringLiteral("FRAME|");
    }
    if (text.isEmpty()) {
        return QStringLiteral("0");
    }
    text.chop(1);
    return text;
}
}

AudioDecoder::AudioDecoder(QObject *parent)
    : QObject(parent),
      m_formatCtx(nullptr),
      m_avioBridge(nullptr),
      m_codecCtx(nullptr),
      m_swrCtx(nullptr),
      m_frame(nullptr),
      m_packet(nullptr),
      m_audioStreamIndex(-1),
      m_duration(0),
      m_outSampleRate(44100),
      m_outChannels(2),
      m_outSampleFormat(AV_SAMPLE_FMT_S16),
      m_decoding(false),
      m_paused(false),
      m_stopRequested(false),
      m_seekRequested(false),
      m_seekTarget(0),
      m_seekMode(static_cast<int>(SeekMode::Precise)),
      m_seekGeneration(0),
      m_pendingSeekId(0),
      m_pendingSeekGeneration(0),
      m_appliedSeekGeneration(0),
      m_currentPts(0),
      m_firstPts(0),
      m_firstPtsSet(false),
      m_ioDeadlineUs(0),
      m_ioAbortRequested(false),
      m_networkSource(false),
      m_waitingFirstPacketAfterSeek(false),
      m_seekFirstPacketDeadlineUs(0),
      m_waitingSeekTargetMs(0),
      m_waitingSeekByteAnchor(-1),
      m_waitingSeekId(0),
      m_waitingSeekGeneration(0),
      m_validateSeekGenerationOnFirstPacket(false),
      m_hlsSource(false),
      m_seekEofRetryCount(0),
      m_seekReadFailureCount(0)
{
    qDebug() << "AudioDecoder created in thread:" << QThread::currentThreadId();
}

AudioDecoder::~AudioDecoder()
{
    stopDecode();
    
    
    if (m_decodeThread.joinable()) {
        m_decodeThread.join();
    }
    
    cleanup();
}

int AudioDecoder::interruptCallback(void* opaque)
{
    auto* self = static_cast<AudioDecoder*>(opaque);
    if (!self) {
        return 0;
    }

    return self->shouldAbortIo() ? 1 : 0;
}

bool AudioDecoder::shouldAbortIo() const
{
    if (m_stopRequested.load() || m_ioAbortRequested.load()) {
        return true;
    }

    const qint64 deadlineUs = m_ioDeadlineUs.load();
    return deadlineUs > 0 && av_gettime_relative() > deadlineUs;
}

int AudioDecoder::boundedIoTimeoutMs(int timeoutMs) const
{
    if (timeoutMs <= 0) {
        return 0;
    }

    return qMin(timeoutMs, kNetworkIoDeadlineMaxMs);
}

void AudioDecoder::setIoDeadlineMs(int timeoutMs)
{
    if (!m_networkSource.load()) {
        m_ioDeadlineUs = 0;
        m_ioAbortRequested = false;
        return;
    }
    m_ioAbortRequested = false;
    const int boundedTimeoutMs = boundedIoTimeoutMs(timeoutMs);
    m_ioDeadlineUs = av_gettime_relative() + static_cast<qint64>(boundedTimeoutMs) * 1000;
}

void AudioDecoder::clearIoDeadline()
{
    m_ioDeadlineUs = 0;
    m_ioAbortRequested = false;
}

void AudioDecoder::beginSeekFirstPacketWait(int seekGeneration, int seekId, qint64 targetMs,
                                            int timeoutMs, bool applyGeneration,
                                            qint64 byteAnchor)
{
    if (applyGeneration) {
        m_appliedSeekGeneration = seekGeneration;
    }
    m_waitingSeekId = seekId;
    m_waitingSeekGeneration = seekGeneration;
    m_waitingSeekTargetMs = targetMs;
    m_waitingSeekByteAnchor = byteAnchor;
    m_validateSeekGenerationOnFirstPacket = !applyGeneration;
    m_waitingFirstPacketAfterSeek = true;
    m_seekEofRetryCount = 0;
    m_seekReadFailureCount = 0;
    const int boundedTimeoutMs = boundedIoTimeoutMs(timeoutMs);
    m_seekFirstPacketDeadlineUs =
        av_gettime_relative() + static_cast<qint64>(boundedTimeoutMs) * 1000;
}

void AudioDecoder::clearSeekFirstPacketWait()
{
    m_waitingFirstPacketAfterSeek = false;
    m_waitingSeekId = 0;
    m_waitingSeekGeneration = 0;
    m_waitingSeekTargetMs = 0;
    m_waitingSeekByteAnchor = -1;
    m_validateSeekGenerationOnFirstPacket = false;
    m_seekFirstPacketDeadlineUs = 0;
    m_seekEofRetryCount = 0;
    m_seekReadFailureCount = 0;
}

bool AudioDecoder::shouldKeepWaitingForSeekPacket(const char* reason, int retryCount,
                                                  int maxRetries)
{
    if (!m_networkSource.load() || !m_waitingFirstPacketAfterSeek.load()) {
        return false;
    }
    if (m_seekRequested.load() || m_stopRequested.load()) {
        return false;
    }

    const qint64 deadlineUs = m_seekFirstPacketDeadlineUs.load();
    if (deadlineUs > 0 && av_gettime_relative() > deadlineUs) {
        qWarning() << "AudioDecoder: first packet wait timed out after seek"
                   << "reason=" << reason
                   << "seek_id=" << m_waitingSeekId.load()
                   << "flac_seek_target_ms=" << m_waitingSeekTargetMs.load()
                   << "seek_generation=" << m_waitingSeekGeneration.load()
                   << "byte_anchor=" << m_waitingSeekByteAnchor.load();
        clearSeekFirstPacketWait();
        return false;
    }

    const int retryBudget = seekFirstPacketRetryBudget(reason, maxRetries);
    if (retryBudget > 0 && retryCount > retryBudget) {
        qWarning() << "AudioDecoder: first packet wait retry budget exhausted"
                   << "reason=" << reason << "retry=" << retryCount << "/" << retryBudget
                   << "seek_id=" << m_waitingSeekId.load()
                   << "flac_seek_target_ms=" << m_waitingSeekTargetMs.load()
                   << "seek_generation=" << m_waitingSeekGeneration.load()
                   << "byte_anchor=" << m_waitingSeekByteAnchor.load();
        clearSeekFirstPacketWait();
        return false;
    }

    return true;
}

int AudioDecoder::seekFirstPacketRetryBudget(const char* reason, int defaultMaxRetries) const
{
    if (m_networkSource.load() && m_filePath.toLower().contains(".flac")) {
        return qMax(defaultMaxRetries, 180);
    }
    if (reason && (std::strcmp(reason, "interrupted") == 0 ||
                   std::strcmp(reason, "again") == 0)) {
        return qMax(defaultMaxRetries, 60);
    }
    return defaultMaxRetries;
}

int AudioDecoder::seekFirstPacketRetrySleepMs(const char* reason) const
{
    if (m_networkSource.load() && m_filePath.toLower().contains(".flac")) {
        return 25;
    }
    if (reason && (std::strcmp(reason, "interrupted") == 0 ||
                   std::strcmp(reason, "again") == 0)) {
        return 10;
    }
    return 2;
}

qint64 AudioDecoder::remoteFlacByteAnchorForLog(qint64 targetMs, int seekGeneration,
                                                int seekId) const
{
    if (!m_avioBridge || targetMs < 0) {
        return -1;
    }

    const AudioCacheManager::DirectSeekHint hint =
        AudioCacheManager::instance().directSeekHintForPlaybackUrl(QUrl(m_filePath), targetMs);
    if (!hint.valid || hint.targetByte < 0) {
        qWarning() << "AudioDecoder: Remote FLAC byte anchor unavailable"
                   << "flac_seek_target_ms=" << targetMs
                   << "seek_id=" << seekId
                   << "seek_generation=" << seekGeneration
                   << "hint_valid=" << hint.valid
                   << "hint_target_byte=" << hint.targetByte;
        return -1;
    }

    const qint64 contentLength =
        hint.contentLength > 0 ? hint.contentLength : m_avioBridge->contentLength();
    qint64 targetByte = hint.targetByte;
    if (contentLength > 0) {
        targetByte = qBound<qint64>(0, targetByte, contentLength - 1);
    } else {
        targetByte = qMax<qint64>(0, targetByte);
    }

    qDebug() << "AudioDecoder: Remote FLAC byte anchor hint"
             << "flac_seek_target_ms=" << targetMs
             << "seek_id=" << seekId
             << "seek_generation=" << seekGeneration
             << "byte_anchor=" << targetByte
             << "content_length=" << contentLength
             << "seek_index_hit=" << hint.seekIndexHit
             << "exact=" << hint.exact
             << "confidence=" << hint.confidenceScore
             << "confidence_reason=" << hint.confidenceReason
             << "hint_generation=" << hint.generation;
    return targetByte;
}

bool AudioDecoder::initDecoder(const QString& filePath)
{
    auto t_start = std::chrono::high_resolution_clock::now();
    
    cleanup(); 
    
    m_filePath = filePath;
    m_seekGeneration = 0;
    m_pendingSeekId = 0;
    m_pendingSeekGeneration = 0;
    m_appliedSeekGeneration = 0;
    m_seekRequested = false;
    m_seekTarget = 0;
    m_seekMode = static_cast<int>(SeekMode::Precise);
    clearSeekFirstPacketWait();
    m_firstPts = 0;
    m_firstPtsSet = true;
    clearIoDeadline();
    
    auto t0 = std::chrono::high_resolution_clock::now();
    if (!openFile(filePath)) {
        emit decodeError("Failed to open file");
        return false;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING] AudioDecoder::openFile:" 
             << std::chrono::duration<double, std::milli>(t1 - t0).count() << "ms";
    
    if (!setupDecoder()) {
        emit decodeError("Failed to setup decoder");
        return false;
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING] AudioDecoder::setupDecoder:" 
             << std::chrono::duration<double, std::milli>(t2 - t1).count() << "ms";
    
    if (!setupResampler()) {
        emit decodeError("Failed to setup resampler");
        return false;
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING] AudioDecoder::setupResampler:" 
             << std::chrono::duration<double, std::milli>(t3 - t2).count() << "ms";
    
    
    // 网络 MP3 在自定义 AVIO 下可能返回 AV_NOPTS_VALUE，不能把无效时长传给 UI。
    const int64_t rawFormatDuration = m_formatCtx ? m_formatCtx->duration : AV_NOPTS_VALUE;
    if (rawFormatDuration != AV_NOPTS_VALUE && rawFormatDuration > 0) {
        m_duration = static_cast<qint64>(
            static_cast<double>(rawFormatDuration) * av_q2d(av_make_q(1, AV_TIME_BASE)) * 1000.0);
    } else {
        m_duration = 0;
    }
    qDebug() << "[AUDIO_DURATION] decoder_format_duration"
             << "raw=" << rawFormatDuration << "durationMs=" << m_duration
             << "source=" << (m_duration > 0 ? "ffmpeg" : "unknown");

    // 建立稳定的时间基准：优先使用 stream start_time，避免 seek 后时间轴漂移。
    if (m_formatCtx && m_audioStreamIndex >= 0) {
        AVStream* stream = m_formatCtx->streams[m_audioStreamIndex];
        if (stream && stream->start_time != AV_NOPTS_VALUE) {
            m_firstPts = av_rescale_q(stream->start_time, stream->time_base, AVRational{1, 1000});
        } else {
            m_firstPts = 0;
        }
    } else {
        m_firstPts = 0;
    }
    m_firstPtsSet = true;
    qDebug() << "AudioDecoder: Timestamp baseline set to" << m_firstPts << "ms";
    
    
    int sampleRate = m_codecCtx->sample_rate;
    int channels = 2; 
    emit metadataReady(m_duration, sampleRate, channels);

    
    extractAudioTags();

    
    extractAlbumArt();
    
    auto t_end = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING] AudioDecoder::initDecoder TOTAL:" 
             << std::chrono::duration<double, std::milli>(t_end - t_start).count() << "ms";
    
    qDebug() << "AudioDecoder initialized successfully. Duration:" << m_duration << "ms";
    return true;
}

void AudioDecoder::startDecode()
{
    if (m_decoding) {
        
        bool wasPaused = m_paused.load();
        if (wasPaused) {
            qDebug() << "AudioDecoder: Resuming from pause";
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_paused = false;
            }
            m_cv.notify_all();
        }
        return;
    }
    
    m_decoding = true;
    m_paused = false;
    m_stopRequested = false;
    m_firstPtsSet = false;  
    clearIoDeadline();
    
    
    if (m_decodeThread.joinable()) {
        m_decodeThread.join();
    }
    
    m_decodeThread = std::thread([this]() {
        qDebug() << "AudioDecoder: Decode thread started in std::thread";
        this->decode();
    });
    
    emit decodeStarted();
    qDebug() << "AudioDecoder: Decode thread launched";
}

void AudioDecoder::pauseDecode()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_paused = true;
    }
    m_cv.notify_all();
    emit decodePaused();
}

void AudioDecoder::stopDecode()
{
    m_stopRequested = true;
    m_ioAbortRequested = true;
    m_decoding = false;
    clearSeekFirstPacketWait();
    m_cv.notify_all();
    
    
    if (m_decodeThread.joinable()) {
        m_decodeThread.join();
        qDebug() << "AudioDecoder: Decode thread stopped synchronously";
    }
    
    emit decodeStopped();
}

int AudioDecoder::seekTo(qint64 positionMs, SeekMode mode, int seekId)
{
    if (!m_formatCtx || m_audioStreamIndex < 0) {
        return currentSeekGeneration();
    }

    // 只在这里投递请求，真正的 av_seek_frame 在解码线程执行，
    // 避免 UI 线程拖动进度条时被阻塞。
    int generation = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        generation = m_seekGeneration.fetch_add(1) + 1;
        m_seekTarget = positionMs;
        m_seekMode = static_cast<int>(mode);
        m_pendingSeekId = seekId;
        m_pendingSeekGeneration = generation;
        m_seekRequested = true;
    }
    m_ioAbortRequested = true;  // 打断可能阻塞中的 av_read_frame，尽快执行新 seek
    m_cv.notify_all();
    qDebug() << "AudioDecoder: Seek requested at" << positionMs << "ms, generation:" << generation
             << "seek_id:" << seekId
             << "mode:" << (mode == SeekMode::Fast ? "fast" : "precise");
    return generation;
}

void AudioDecoder::decode()
{
    qDebug() << "AudioDecoder::decode thread started in std::thread";
    
    while (!m_stopRequested) {
        // 暂停时等待恢复、停止或 seek 请求（seek 也需要唤醒线程）
        if (m_paused) {
            std::unique_lock<std::mutex> lock(m_mutex);
            qDebug() << "AudioDecoder: Paused, waiting for resume/seek/stop";
            m_cv.wait(lock, [this]() {
                return !m_paused.load() || m_stopRequested.load() || m_seekRequested.load();
            });

            if (m_stopRequested) {
                qDebug() << "AudioDecoder: Stop requested during pause";
                break;
            }
        }

        if (m_stopRequested) {
            break;
        }

        if (m_seekRequested) {
            qint64 targetMs = 0;
            SeekMode seekMode = SeekMode::Precise;
            int seekId = 0;
            int targetGeneration = 0;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                targetMs = m_seekTarget.load();
                seekMode = static_cast<SeekMode>(m_seekMode.load());
                seekId = m_pendingSeekId.load();
                targetGeneration = m_pendingSeekGeneration.load();
                m_seekRequested = false;
            }
            if (targetGeneration <= 0) {
                targetGeneration = m_seekGeneration.load();
            }

            if (!m_formatCtx || m_audioStreamIndex < 0) {
                continue;
            }

            m_ioAbortRequested = false;
            clearSeekFirstPacketWait();
            const bool likelyFlacNetwork =
                m_networkSource.load() && m_filePath.toLower().contains(".flac");
            const bool likelyMp3Network =
                m_networkSource.load() && m_filePath.toLower().contains(".mp3");
            const bool localProxySource =
                m_networkSource.load() &&
                (m_filePath.startsWith("http://127.0.0.1", Qt::CaseInsensitive) ||
                 m_filePath.startsWith("http://localhost", Qt::CaseInsensitive));
            const bool ordinaryRemoteMp3FastSeek =
                likelyMp3Network && !localProxySource && seekMode == SeekMode::Fast;
            bool remoteFlacAnchorScan = false;
            if (m_avioBridge && !ordinaryRemoteMp3FastSeek) {
                remoteFlacAnchorScan = m_avioBridge->prepareSeek(targetMs);
            }
            const int seekDeadlineMs = localProxySource
                                           ? (likelyFlacNetwork
                                                  ? kSeekDeadlineFlacLocalProxyMs
                                                  : kSeekDeadlineLocalProxyMs)
                                           : (likelyFlacNetwork ? kSeekDeadlineFlacHttpMs
                                                                : kSeekDeadlineHttpMs);
            const int firstPacketDeadlineMs =
                m_hlsSource.load()
                    ? kSeekFirstPacketDeadlineHlsMs
                    : (localProxySource
                           ? (likelyFlacNetwork ? kSeekFirstPacketDeadlineFlacLocalProxyMs
                                                : kSeekFirstPacketDeadlineLocalProxyMs)
                           : (likelyFlacNetwork ? kSeekFirstPacketDeadlineFlacHttpMs
                                                : kSeekFirstPacketDeadlineHttpMs));
            setIoDeadlineMs(seekDeadlineMs);
            int seekFlags = AVSEEK_FLAG_BACKWARD;
            if (seekMode == SeekMode::Fast && m_networkSource.load()) {
                seekFlags |= AVSEEK_FLAG_ANY;
            }
            int seekRet = AVERROR(EINVAL);
            qint64 seekTargetStream = AV_NOPTS_VALUE;
            qint64 seekTargetGlobalUs = targetMs * 1000;
            int demuxSeekRet = AVERROR(EINVAL);
            int fallbackSeekRet = AVERROR(EINVAL);
            qint64 flacByteAnchor = -1;
            QString seekRoute;
            qDebug() << "AudioDecoder: Processing seek request"
                     << "targetMs=" << targetMs
                     << "seek_id=" << seekId
                     << "seek_generation=" << targetGeneration
                     << "mode=" << (seekMode == SeekMode::Fast ? "fast" : "precise")
                     << "flags=" << seekFlags
                     << "flagsText=" << describeSeekFlags(seekFlags)
                     << "ordinaryRemoteMp3FastSeek=" << ordinaryRemoteMp3FastSeek
                     << "avioHintPrepared=" << (m_avioBridge && !ordinaryRemoteMp3FastSeek)
                     << "remoteFlacAnchorScan=" << remoteFlacAnchorScan;
            if (likelyFlacNetwork && m_avioBridge) {
                flacByteAnchor = remoteFlacByteAnchorForLog(targetMs, targetGeneration, seekId);
            }
            if (likelyFlacNetwork && remoteFlacAnchorScan) {
                seekRet = 0;
                demuxSeekRet = 0;
                fallbackSeekRet = 0;
                seekRoute = QStringLiteral("remote_flac_range_anchor_scan");
                const qint64 anchorByte = m_avioBridge ? m_avioBridge->activeSeekTargetByte() : -1;
                if (m_formatCtx->pb && anchorByte >= 0) {
                    const int64_t avioSeekRet =
                        avio_seek(m_formatCtx->pb, anchorByte, SEEK_SET);
                    qDebug() << "AudioDecoder: Remote FLAC AVIO anchor sync"
                             << "targetMs=" << targetMs
                             << "seek_id=" << seekId
                             << "anchorByte=" << anchorByte
                             << "avio_seek_ret=" << avioSeekRet;
                    if (avioSeekRet < 0) {
                        seekRet = static_cast<int>(avioSeekRet);
                        demuxSeekRet = static_cast<int>(avioSeekRet);
                    }
                }
                avformat_flush(m_formatCtx);
            } else if (m_hlsSource.load()) {
                // HLS 对全局时间轴 seek 更稳定，按 AV_TIME_BASE 进行。
                demuxSeekRet = avformat_seek_file(
                    m_formatCtx,
                    -1,
                    std::numeric_limits<int64_t>::min(),
                    seekTargetGlobalUs,
                    std::numeric_limits<int64_t>::max(),
                    AVSEEK_FLAG_BACKWARD
                );
                seekRet = demuxSeekRet;
                seekRoute = QStringLiteral("hls_global_avformat_seek_file");
                if (seekRet < 0) {
                    fallbackSeekRet =
                        av_seek_frame(m_formatCtx, -1, seekTargetGlobalUs, AVSEEK_FLAG_BACKWARD);
                    seekRet = fallbackSeekRet;
                    seekRoute = QStringLiteral("hls_global_av_seek_frame");
                }
            } else {
                AVRational msBase = {1, 1000};
                seekTargetStream = av_rescale_q(
                    targetMs,
                    msBase,
                    m_formatCtx->streams[m_audioStreamIndex]->time_base
                );
                // 先走 avformat_seek_file（时间窗口更明确），失败再回退 av_seek_frame。
                demuxSeekRet = avformat_seek_file(
                    m_formatCtx,
                    m_audioStreamIndex,
                    std::numeric_limits<int64_t>::min(),
                    seekTargetStream,
                    std::numeric_limits<int64_t>::max(),
                    seekFlags
                );
                seekRet = demuxSeekRet;
                seekRoute = QStringLiteral("stream_avformat_seek_file");
                if (seekRet < 0) {
                    fallbackSeekRet =
                        av_seek_frame(m_formatCtx, m_audioStreamIndex, seekTargetStream, seekFlags);
                    seekRet = fallbackSeekRet;
                    seekRoute = QStringLiteral("stream_av_seek_frame");
                }
                // 某些网络容器（尤其 FLAC）在 stream-index seek 上会返回 EPERM，回退到全局时基 seek。
                if (seekRet < 0 && m_networkSource.load()) {
                    fallbackSeekRet = avformat_seek_file(
                        m_formatCtx,
                        -1,
                        std::numeric_limits<int64_t>::min(),
                        seekTargetGlobalUs,
                        std::numeric_limits<int64_t>::max(),
                        AVSEEK_FLAG_BACKWARD
                    );
                    seekRet = fallbackSeekRet;
                    seekRoute = QStringLiteral("global_avformat_seek_file");
                }
                if (seekRet < 0 && m_networkSource.load()) {
                    fallbackSeekRet =
                        av_seek_frame(m_formatCtx, -1, seekTargetGlobalUs, AVSEEK_FLAG_BACKWARD);
                    seekRet = fallbackSeekRet;
                    seekRoute = QStringLiteral("global_av_seek_frame");
                }
                if (seekRet < 0 && m_networkSource.load() && seekMode == SeekMode::Fast) {
                    fallbackSeekRet = av_seek_frame(m_formatCtx, -1, seekTargetGlobalUs,
                                                    AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);
                    seekRet = fallbackSeekRet;
                    seekRoute = QStringLiteral("global_av_seek_frame_any");
                }
            }
            clearIoDeadline();
            qDebug() << "AudioDecoder: Seek execution result"
                     << "seek_id=" << seekId
                     << "seek_generation=" << targetGeneration
                     << "targetMs=" << targetMs
                     << "target_stream_time=" << seekTargetStream
                     << "target_global_us=" << seekTargetGlobalUs
                     << "flags=" << seekFlags
                     << "flagsText=" << describeSeekFlags(seekFlags)
                     << "demux_seek_ret=" << demuxSeekRet
                     << "fallback_seek_ret=" << fallbackSeekRet
                     << "flac_byte_anchor=" << flacByteAnchor
                     << "seek_ret=" << seekRet
                     << "route=" << seekRoute;
            if (seekRet >= 0) {
                avcodec_flush_buffers(m_codecCtx);
                beginSeekFirstPacketWait(targetGeneration, seekId, targetMs, firstPacketDeadlineMs,
                                         !likelyFlacNetwork,
                                         flacByteAnchor);
                emit progressUpdated(targetMs);
            } else {
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(seekRet, errbuf, sizeof(errbuf));
                if (likelyFlacNetwork) {
                    clearSeekFirstPacketWait();
                    qWarning() << "AudioDecoder: Remote FLAC seek failed before first packet"
                               << "error=" << errbuf
                               << "flac_seek_target_ms=" << targetMs
                               << "seek_id=" << seekId
                               << "seek_generation=" << targetGeneration
                               << "byte_anchor=" << flacByteAnchor
                               << "route=" << seekRoute;
                    emit decodeError(QStringLiteral("FLAC seek failed: decoder seek returned %1")
                                         .arg(QString::fromUtf8(errbuf)));
                    break;
                } else {
                    clearSeekFirstPacketWait();
                    qDebug() << "AudioDecoder: Seek failed in decode thread:" << errbuf;
                }
            }
            continue;
        }
        
        
        const bool likelyFlacNetwork =
            m_networkSource.load() && m_filePath.toLower().contains(".flac");
        const bool localProxySource =
            m_networkSource.load() &&
            (m_filePath.startsWith("http://127.0.0.1", Qt::CaseInsensitive) ||
             m_filePath.startsWith("http://localhost", Qt::CaseInsensitive));
        int readTimeoutMs = kNetworkDefaultReadDeadlineMs;
        if (m_waitingFirstPacketAfterSeek.load()) {
            // HLS 切段后首包 RTT 通常显著高于裸文件 Range，避免误判 EOF。
            if (m_hlsSource.load()) {
                readTimeoutMs = kSeekFirstPacketDeadlineHlsMs;
            } else if (localProxySource) {
                readTimeoutMs = likelyFlacNetwork ? kSeekFirstPacketDeadlineFlacLocalProxyMs
                                                  : kSeekFirstPacketDeadlineLocalProxyMs;
            } else if (likelyFlacNetwork) {
                readTimeoutMs = kSeekFirstPacketDeadlineFlacHttpMs;
            } else {
                readTimeoutMs = 900;
            }
        }
        setIoDeadlineMs(readTimeoutMs);
        int ret = av_read_frame(m_formatCtx, m_packet);
        clearIoDeadline();
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                if (m_networkSource.load() && m_waitingFirstPacketAfterSeek.load()) {
                    const int retryCount = m_seekEofRetryCount.fetch_add(1) + 1;
                    if (shouldKeepWaitingForSeekPacket("eof", retryCount, kSeekEofRetryMax)) {
                        if (retryCount == 1 || retryCount % 4 == 0) {
                            qWarning() << "AudioDecoder: Transient EOF right after seek, retrying"
                                       << retryCount << "/" << kSeekEofRetryMax
                                       << "seek_id=" << m_waitingSeekId.load()
                                       << "seek_generation=" << m_waitingSeekGeneration.load();
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(60));
                        continue;
                    }
                    emit decodeError(QStringLiteral("Seek reached EOF before first audio packet"));
                    av_packet_unref(m_packet);
                    break;
                }

                
                qDebug() << "AudioDecoder: End of stream reached";
                emit decodeCompleted();
                break;
            } else if (ret == AVERROR_EXIT || ret == AVERROR(EINTR)) {
                // 被 seek/stop/超时中断后快速继续循环，优先处理最新请求
                m_ioAbortRequested = false;
                if (m_waitingFirstPacketAfterSeek.load() && !m_seekRequested.load()) {
                    const int retryCount = m_seekReadFailureCount.fetch_add(1) + 1;
                    if (!shouldKeepWaitingForSeekPacket("interrupted", retryCount,
                                                        kSeekReadFailureRetryMax)) {
                        emit decodeError(QStringLiteral("Seek timed out before first audio packet"));
                        av_packet_unref(m_packet);
                        break;
                    }
                }
                const int sleepMs = m_waitingFirstPacketAfterSeek.load()
                                        ? seekFirstPacketRetrySleepMs("interrupted")
                                        : 2;
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
                continue;
            } else if (ret == AVERROR(EAGAIN)) {
                if (m_waitingFirstPacketAfterSeek.load()) {
                    const int retryCount = m_seekReadFailureCount.fetch_add(1) + 1;
                    if (!shouldKeepWaitingForSeekPacket("again", retryCount,
                                                        kSeekReadFailureRetryMax)) {
                        emit decodeError(QStringLiteral("Seek timed out before first audio packet"));
                        av_packet_unref(m_packet);
                        break;
                    }
                }
                const int sleepMs = m_waitingFirstPacketAfterSeek.load()
                                        ? seekFirstPacketRetrySleepMs("again")
                                        : 2;
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
                continue;
            } else {
                
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(ret, errbuf, sizeof(errbuf));
                qDebug() << "AudioDecoder: Read frame error:" << errbuf << ", will retry";
                if (m_waitingFirstPacketAfterSeek.load()) {
                    const int retryCount = m_seekReadFailureCount.fetch_add(1) + 1;
                    if (!shouldKeepWaitingForSeekPacket("read_error", retryCount,
                                                        kSeekReadFailureRetryMax)) {
                        emit decodeError(QStringLiteral("Seek failed before first audio packet"));
                        av_packet_unref(m_packet);
                        break;
                    }
                }
                if (likelyFlacNetwork && m_waitingFirstPacketAfterSeek.load()) {
                    qWarning() << "AudioDecoder: Remote FLAC first packet read failed,"
                               << "bounded retry:" << errbuf
                               << "retry=" << m_seekReadFailureCount.load() << "/"
                               << kSeekReadFailureRetryMax
                               << "seek_id=" << m_waitingSeekId.load()
                               << "seek_generation=" << m_waitingSeekGeneration.load();
                    std::this_thread::sleep_for(std::chrono::milliseconds(80));
                    continue;
                }
                const int sleepMs = m_waitingFirstPacketAfterSeek.load() ? 2 : 10;
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
                continue;  
            }
        }
        m_seekEofRetryCount = 0;
        m_seekReadFailureCount = 0;
        
        
        if (m_packet->stream_index == m_audioStreamIndex) {
            
            if (avcodec_send_packet(m_codecCtx, m_packet) >= 0) {
                
                while (avcodec_receive_frame(m_codecCtx, m_frame) >= 0) {
                    
                    int dst_nb_samples = av_rescale_rnd(
                        swr_get_delay(m_swrCtx, m_codecCtx->sample_rate) + m_frame->nb_samples,
                        m_outSampleRate,
                        m_codecCtx->sample_rate,
                        AV_ROUND_UP
                    );
                    
                    // 计算重采样后的 PCM 缓冲区大小
                    int buffer_size = av_samples_get_buffer_size(
                        nullptr, 
                        m_outChannels, 
                        dst_nb_samples, 
                        m_outSampleFormat, 
                        1
                    );
                    
                    if (buffer_size <= 0) {
                        qDebug() << "Error calculating buffer size:" << buffer_size;
                        continue;
                    }
                    
                    // 为本帧重采样结果分配临时缓冲区
                    uint8_t* buffer = (uint8_t*)malloc(buffer_size);
                    if (!buffer) {
                        qDebug() << "Failed to allocate memory for buffer";
                        continue;
                    }
                    
                    
                    int converted_samples = swr_convert(
                        m_swrCtx, 
                        &buffer,
                        dst_nb_samples,
                        (const uint8_t**)m_frame->data,
                        m_frame->nb_samples
                    );
                    
                    
                    int converted_buffer_size = av_samples_get_buffer_size(
                        nullptr, 
                        m_outChannels,
                        converted_samples,
                        m_outSampleFormat, 
                        1
                    );
                    
                    // 获取时间戳（优先 frame best effort timestamp，回退到 packet pts）。
                    qint64 timestamp_ms = 0;
                    qint64 rawTimestampMs = -1;
                    int64_t pts = AV_NOPTS_VALUE;
                    if (m_frame->best_effort_timestamp != AV_NOPTS_VALUE) {
                        pts = m_frame->best_effort_timestamp;
                    } else if (m_packet->pts != AV_NOPTS_VALUE) {
                        pts = m_packet->pts;
                    }

                    if (pts != AV_NOPTS_VALUE) {
                        AVRational time_base = m_formatCtx->streams[m_audioStreamIndex]->time_base;
                        rawTimestampMs = av_rescale_q(pts, time_base, {1, 1000});

                        // 使用稳定基准换算为“曲目内时间轴”，seek 后不会重置到 0。
                        timestamp_ms = rawTimestampMs - m_firstPts;
                        if (timestamp_ms < 0) {
                            timestamp_ms = 0;
                        }
                        m_currentPts = timestamp_ms;
                    }
                    
                    
                    QByteArray pcmData(reinterpret_cast<const char*>(buffer), converted_buffer_size);
                    free(buffer);
                    
                    
                    if (m_stopRequested) {
                        qDebug() << "AudioDecoder: Stop requested, discarding decoded frame";
                        break;
                    }

                    bool dropFrame = false;
                    QString seekFrameRejectError;
                    if (m_waitingFirstPacketAfterSeek.exchange(false)) {
                        const int waitingSeekId = m_waitingSeekId.load();
                        const int waitingGeneration = m_waitingSeekGeneration.load();
                        const qint64 waitingTargetMs = m_waitingSeekTargetMs.load();
                        qDebug() << "AudioDecoder: First decoded frame arrived after seek"
                                 << "first_decoded_raw_ms=" << rawTimestampMs
                                 << "first_decoded_track_ms=" << timestamp_ms
                                 << "waiting_target_ms=" << waitingTargetMs
                                 << "seek_id=" << waitingSeekId
                                 << "seek_generation=" << waitingGeneration
                                 << "applied_generation=" << m_appliedSeekGeneration.load()
                                 << "validate_generation="
                                 << m_validateSeekGenerationOnFirstPacket.load();
                        if (m_validateSeekGenerationOnFirstPacket.load()) {
                            const qint64 driftMs = qAbs(timestamp_ms - waitingTargetMs);
                            if (driftMs <= kFallbackSeekAcceptToleranceMs) {
                                m_appliedSeekGeneration = waitingGeneration;
                                qWarning()
                                    << "AudioDecoder: FLAC fallback first frame accepted"
                                    << "seek_accept_reason=flac_first_packet_within_tolerance"
                                    << "seek_id=" << waitingSeekId
                                    << "targetMs=" << waitingTargetMs << "ts=" << timestamp_ms
                                    << "driftMs=" << driftMs << "generation=" << waitingGeneration;
                            } else {
                                qWarning()
                                    << "AudioDecoder: FLAC fallback first frame rejected"
                                    << "seek_accept_reason=flac_first_packet_drift_rejected"
                                    << "seek_id=" << waitingSeekId
                                    << "targetMs=" << waitingTargetMs << "ts=" << timestamp_ms
                                    << "driftMs=" << driftMs << "generation=" << waitingGeneration;
                                dropFrame = true;
                                seekFrameRejectError = QStringLiteral(
                                    "FLAC seek failed: decoded timestamp drift too large");
                            }
                        } else {
                            qDebug() << "AudioDecoder: First decoded frame accepted after seek"
                                     << "seek_accept_reason=decoder_seek_applied"
                                     << "first_decoded_raw_ms=" << rawTimestampMs
                                     << "first_decoded_track_ms=" << timestamp_ms
                                     << "waiting_target_ms=" << waitingTargetMs
                                     << "seek_id=" << waitingSeekId
                                     << "seek_generation=" << waitingGeneration;
                        }
                        clearSeekFirstPacketWait();
                    }

                    if (dropFrame) {
                        if (!seekFrameRejectError.isEmpty()) {
                            emit decodeError(seekFrameRejectError);
                        }
                        continue;
                    }
                    
                    emit decodedData(pcmData, timestamp_ms, m_appliedSeekGeneration.load());
                    emit progressUpdated(timestamp_ms);
                }
            }
        }
        
        // 释放当前 packet 引用，准备下一轮读取
        av_packet_unref(m_packet);
    }
    
    
    m_decoding = false;
    qDebug() << "AudioDecoder::decode thread finished, m_decoding reset to false";
}

void AudioDecoder::cleanup()
{
    
    if(m_frame)
        av_frame_free(&m_frame);
    if(m_packet)
    {
        av_packet_unref(m_packet);
        av_packet_free(&m_packet);
    }
    if(m_codecCtx)
        avcodec_free_context(&m_codecCtx);
    if(m_formatCtx) {
        if (m_avioBridge) {
            m_formatCtx->pb = nullptr;
        }
        avformat_close_input(&m_formatCtx);
    }
    delete m_avioBridge;
    m_avioBridge = nullptr;
    if(m_swrCtx)
        swr_free(&m_swrCtx);
}

bool AudioDecoder::shouldUseCustomNetworkInput(const QString& filePath) const
{
    const QUrl url(filePath.trimmed());
    if (!url.isValid()) {
        return false;
    }

    const QString scheme = url.scheme().toLower();
    if (scheme != QStringLiteral("http") && scheme != QStringLiteral("https")) {
        return false;
    }

    const QString lowerPath = url.path().toLower();
    const QString host = url.host().toLower();
    const bool localProxySource =
        host == QStringLiteral("127.0.0.1") || host == QStringLiteral("localhost");
    if (localProxySource) {
        return false;
    }

    if (lowerPath.endsWith(QStringLiteral(".m3u8")) || lowerPath.contains(QStringLiteral("/hls/"))) {
        return false;
    }

    return lowerPath.endsWith(QStringLiteral(".mp3")) ||
           lowerPath.endsWith(QStringLiteral(".flac")) ||
           lowerPath.endsWith(QStringLiteral(".wav")) ||
           lowerPath.endsWith(QStringLiteral(".ogg")) ||
           lowerPath.endsWith(QStringLiteral(".m4a")) ||
           lowerPath.endsWith(QStringLiteral(".aac"));
}

void AudioDecoder::extractAudioTags()
{
    if (!m_formatCtx) return;

    
    AVDictionaryEntry *tag = nullptr;

    
    tag = av_dict_get(m_formatCtx->metadata, "title", nullptr, 0);
    if (tag) {
        m_extractedTitle = QString::fromUtf8(tag->value);
        qDebug() << "AudioDecoder: Extracted title from metadata:" << m_extractedTitle;
    } else {
        m_extractedTitle = "";
    }

    
    tag = av_dict_get(m_formatCtx->metadata, "artist", nullptr, 0);
    if (tag) {
        m_extractedArtist = QString::fromUtf8(tag->value);
        qDebug() << "AudioDecoder: Extracted artist from metadata:" << m_extractedArtist;
    } else {
        m_extractedArtist = "";
    }

    
    emit audioTagsReady(m_extractedTitle, m_extractedArtist);
}

void AudioDecoder::extractAlbumArt()
{
    if (!m_formatCtx) return;
    
    
    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++) {
        if (m_formatCtx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            AVPacket *pkt = &m_formatCtx->streams[i]->attached_pic;
            
            
            QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                             + "/album_cover_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".jpg";
            
            QFile file(tempPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write((const char*)pkt->data, pkt->size);
                file.close();
                
                
                QString fileUrl = QUrl::fromLocalFile(tempPath).toString();
                
                qDebug() << "Album cover extracted to:" << fileUrl;
                emit albumArtReady(fileUrl);
            } else {
                qDebug() << "Failed to save album cover";
                emit albumArtReady("qrc:/new/prefix1/icon/maxresdefault.jpg");
            }
            return;
        }
    }
    
    
    qDebug() << "No album cover found, using default image";
    emit albumArtReady("qrc:/new/prefix1/icon/maxresdefault.jpg");
}

bool AudioDecoder::openFile(const QString& filePath)
{
    auto t0 = std::chrono::high_resolution_clock::now();
    
    QByteArray utf8Data = filePath.toUtf8();
    const char* inputUrl = utf8Data.constData();
    const QString lowered = filePath.trimmed().toLower();
    m_networkSource = lowered.startsWith("http://") || lowered.startsWith("https://");
    m_hlsSource = lowered.contains(".m3u8");

    // 为网络输入注入 interrupt callback，确保 seek/stop 能打断阻塞 I/O
    m_formatCtx = avformat_alloc_context();
    if (!m_formatCtx) {
        qDebug() << "Could not allocate AVFormatContext";
        return false;
    }
    m_formatCtx->interrupt_callback.callback = &AudioDecoder::interruptCallback;
    m_formatCtx->interrupt_callback.opaque = this;

    const bool useCustomNetworkInput = shouldUseCustomNetworkInput(filePath);
    if (useCustomNetworkInput) {
        m_avioBridge = new AudioAvioBridge();
        m_avioBridge->setInterruptCallback([this]() {
            return shouldAbortIo();
        });
        if (m_avioBridge->open(QUrl(filePath)) && m_avioBridge->attachTo(m_formatCtx)) {
            qDebug() << "AudioDecoder: Custom HTTP AVIO enabled for" << filePath;
        } else {
            qWarning() << "AudioDecoder: Custom HTTP AVIO unavailable, fallback to FFmpeg native:"
                       << (m_avioBridge ? m_avioBridge->lastError() : QStringLiteral("unknown"));
            delete m_avioBridge;
            m_avioBridge = nullptr;
            m_formatCtx->pb = nullptr;
            m_formatCtx->flags &= ~AVFMT_FLAG_CUSTOM_IO;
        }
    }
    
    AVDictionary* options = nullptr;
    
    av_dict_set(&options, "timeout", "5000000", 0);
    av_dict_set(&options, "rw_timeout", "5000000", 0);
    av_dict_set(&options, "reconnect", "1", 0);  
    av_dict_set(&options, "reconnect_streamed", "1", 0);  
    av_dict_set(&options, "reconnect_at_eof", "1", 0);
    av_dict_set(&options, "reconnect_on_network_error", "1", 0);
    av_dict_set(&options, "reconnect_on_http_error", "408,429,500,502,503,504", 0);
    av_dict_set(&options, "reconnect_delay_max", "5", 0);  
    if (m_networkSource.load()) {
        // 强制按可随机访问网络流处理，避免 server 已支持 Range 但被判为不可 seek。
        av_dict_set(&options, "seekable", "1", 0);
        av_dict_set(&options, "multiple_requests", "1", 0);
        av_dict_set(&options, "probesize", "65536", 0);
        av_dict_set(&options, "analyzeduration", "500000", 0);
    }
    
    setIoDeadlineMs(kNetworkOpenDeadlineMs);
    int ret = avformat_open_input(&m_formatCtx, m_avioBridge ? nullptr : inputUrl, nullptr, &options);
    clearIoDeadline();
    av_dict_free(&options);
    
    auto t1 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING]   avformat_open_input:" 
             << std::chrono::duration<double, std::milli>(t1 - t0).count() << "ms";
    
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "Could not open input file:" << errbuf << "Error code:" << ret;
        cleanup();
        return false;
    }
    
    setIoDeadlineMs(kNetworkStreamInfoDeadlineMs);
    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        clearIoDeadline();
        qDebug() << "Could not find stream information";
        cleanup();
        return false;
    }
    clearIoDeadline();
    
    auto t2 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING]   avformat_find_stream_info:" 
             << std::chrono::duration<double, std::milli>(t2 - t1).count() << "ms";
    
    
    m_audioStreamIndex = -1;
    for (int i = 0; i < (int)m_formatCtx->nb_streams; ++i) {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioStreamIndex = i;
            break;
        }
    }
    
    if (m_audioStreamIndex == -1) {
        qDebug() << "Could not find an audio stream";
        cleanup();
        return false;
    }
    
    qDebug() << "Audio stream found at index:" << m_audioStreamIndex;
    return true;
}

bool AudioDecoder::setupDecoder()
{
    if (!m_formatCtx || m_audioStreamIndex < 0) return false;
    
    
    AVCodec* codec = avcodec_find_decoder(m_formatCtx->streams[m_audioStreamIndex]->codecpar->codec_id);
    if (!codec) {
        qDebug() << "Codec not found";
        return false;
    }
    
    // 分配解码器上下文
    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) {
        qDebug() << "Could not allocate codec context";
        return false;
    }
    
    
    if (avcodec_parameters_to_context(m_codecCtx, m_formatCtx->streams[m_audioStreamIndex]->codecpar) < 0) {
        qDebug() << "Failed to copy codec parameters to codec context";
        avcodec_free_context(&m_codecCtx);
        return false;
    }
    
    
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        qDebug() << "Could not open codec";
        avcodec_free_context(&m_codecCtx);
        return false;
    }
    
    // 分配 frame 与 packet
    m_frame = av_frame_alloc();
    if (!m_frame) {
        qDebug() << "Could not allocate frame";
        return false;
    }
    
    m_packet = av_packet_alloc();
    if (!m_packet) {
        qDebug() << "Could not allocate packet";
        av_frame_free(&m_frame);
        return false;
    }
    
    qDebug() << "Decoder setup successfully. Sample rate:" << m_codecCtx->sample_rate
             << "Channels:" << m_codecCtx->channels;
    return true;
}

bool AudioDecoder::setupResampler()
{
    if (!m_codecCtx) return false;
    
    
    int64_t input_channel_layout = m_codecCtx->channel_layout;
    if (input_channel_layout == 0) {
        input_channel_layout = av_get_default_channel_layout(m_codecCtx->channels);
    }
    
    
    m_swrCtx = swr_alloc();
    if (!m_swrCtx) {
        qDebug() << "Could not allocate SwrContext";
        return false;
    }
    
    
    int64_t output_channel_layout = AV_CH_LAYOUT_STEREO;
    
    av_opt_set_int(m_swrCtx, "in_channel_layout", input_channel_layout, 0);
    av_opt_set_int(m_swrCtx, "out_channel_layout", output_channel_layout, 0);
    av_opt_set_int(m_swrCtx, "in_sample_rate", m_codecCtx->sample_rate, 0);
    av_opt_set_int(m_swrCtx, "out_sample_rate", m_outSampleRate, 0);
    av_opt_set_sample_fmt(m_swrCtx, "in_sample_fmt", m_codecCtx->sample_fmt, 0);
    av_opt_set_sample_fmt(m_swrCtx, "out_sample_fmt", m_outSampleFormat, 0);
    
    // 初始化重采样器
    if (swr_init(m_swrCtx) < 0) {
        qDebug() << "Could not initialize resampler";
        swr_free(&m_swrCtx);
        return false;
    }
    
    qDebug() << "Resampler setup successfully";
    return true;
}

