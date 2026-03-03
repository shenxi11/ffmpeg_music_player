#include "AudioDecoder.h"
#include <QStandardPaths>
#include <QDateTime>
#include <QFile>
#include <QUrl>
#include <cerrno>
#include <chrono>
#include <limits>
extern "C" {
#include <libavutil/time.h>
}

AudioDecoder::AudioDecoder(QObject *parent)
    : QObject(parent),
      m_formatCtx(nullptr),
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
      m_seekGeneration(0),
      m_appliedSeekGeneration(0),
      m_currentPts(0),
      m_firstPts(0),
      m_firstPtsSet(false),
      m_ioDeadlineUs(0),
      m_ioAbortRequested(false),
      m_networkSource(false),
      m_waitingFirstPacketAfterSeek(false),
      m_hlsSource(false),
      m_seekEofRetryCount(0)
{
    qDebug() << "AudioDecoder created in thread:" << QThread::currentThreadId();
}

AudioDecoder::~AudioDecoder()
{
    stopDecode();
    
    // 绛夊緟瑙ｇ爜绾跨▼缁撴潫
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

    if (self->m_stopRequested.load() || self->m_ioAbortRequested.load()) {
        return 1;
    }

    const qint64 deadlineUs = self->m_ioDeadlineUs.load();
    if (deadlineUs > 0 && av_gettime_relative() > deadlineUs) {
        return 1;
    }

    return 0;
}

void AudioDecoder::setIoDeadlineMs(int timeoutMs)
{
    if (!m_networkSource.load()) {
        m_ioDeadlineUs = 0;
        m_ioAbortRequested = false;
        return;
    }
    m_ioAbortRequested = false;
    m_ioDeadlineUs = av_gettime_relative() + static_cast<qint64>(timeoutMs) * 1000;
}

void AudioDecoder::clearIoDeadline()
{
    m_ioDeadlineUs = 0;
    m_ioAbortRequested = false;
}

bool AudioDecoder::initDecoder(const QString& filePath)
{
    auto t_start = std::chrono::high_resolution_clock::now();
    
    cleanup(); // 娓呯悊鏃ц祫婧?
    
    m_filePath = filePath;
    m_seekGeneration = 0;
    m_appliedSeekGeneration = 0;
    m_seekRequested = false;
    m_seekTarget = 0;
    m_waitingFirstPacketAfterSeek = false;
    m_seekEofRetryCount = 0;
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
    
    // 鑾峰彇闊抽鏃堕暱
    m_duration = m_formatCtx->duration * av_q2d(av_make_q(1, AV_TIME_BASE)) * 1000; // 杞崲涓烘绉?

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
    
    // 鍙戦€佸厓鏁版嵁
    int sampleRate = m_codecCtx->sample_rate;
    int channels = 2; // 閲嶉噰鏍峰悗鐨勯€氶亾鏁?
    emit metadataReady(m_duration, sampleRate, channels);

    // 鎻愬彇闊抽鏍囩锛堟爣棰樸€佽壓鏈锛?
    extractAudioTags();

    // 鎻愬彇灏侀潰
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
        // 妫€鏌ユ槸鍚︾湡鐨勯渶瑕乺esume
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
    m_firstPtsSet = false;  // 閲嶇疆鍩哄噯鏃堕棿鎴虫爣蹇?
    clearIoDeadline();
    
    // 鍚姩鐙珛鐨勮В鐮佺嚎绋?
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
    m_waitingFirstPacketAfterSeek = false;
    m_seekEofRetryCount = 0;
    m_cv.notify_all();
    
    // 绔嬪嵆绛夊緟瑙ｇ爜绾跨▼缁撴潫锛堢‘淇濆悓姝ュ仠姝級
    if (m_decodeThread.joinable()) {
        m_decodeThread.join();
        qDebug() << "AudioDecoder: Decode thread stopped synchronously";
    }
    
    emit decodeStopped();
}

void AudioDecoder::seekTo(qint64 positionMs)
{
    if (!m_formatCtx || m_audioStreamIndex < 0) return;

    // 只在这里投递请求，真正的 av_seek_frame 在解码线程执行，
    // 避免 UI 线程拖动进度条时被阻塞。
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_seekTarget = positionMs;
        m_seekRequested = true;
    }
    const int generation = m_seekGeneration.fetch_add(1) + 1;
    m_ioAbortRequested = true;  // 打断可能阻塞中的 av_read_frame，尽快执行新 seek
    m_cv.notify_all();
    qDebug() << "AudioDecoder: Seek requested at" << positionMs << "ms, generation:" << generation;
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
            const qint64 targetMs = m_seekTarget.load();
            const int targetGeneration = m_seekGeneration.load();
            m_seekRequested = false;

            if (!m_formatCtx || m_audioStreamIndex < 0) {
                continue;
            }

            qDebug() << "AudioDecoder: Processing seek request at" << targetMs << "ms";
            const bool likelyFlacNetwork = m_networkSource.load() && m_filePath.toLower().contains(".flac");
            setIoDeadlineMs(likelyFlacNetwork ? 8000 : 1600);
            int seekFlags = AVSEEK_FLAG_BACKWARD;
            if (m_networkSource.load()) {
                seekFlags |= AVSEEK_FLAG_ANY;
            }
            int seekRet = AVERROR(EINVAL);
            if (m_hlsSource.load()) {
                // HLS 对全局时间轴 seek 更稳定，按 AV_TIME_BASE 进行。
                const qint64 seekTargetUs = targetMs * 1000;
                seekRet = avformat_seek_file(
                    m_formatCtx,
                    -1,
                    std::numeric_limits<int64_t>::min(),
                    seekTargetUs,
                    std::numeric_limits<int64_t>::max(),
                    AVSEEK_FLAG_BACKWARD
                );
                if (seekRet < 0) {
                    seekRet = av_seek_frame(m_formatCtx, -1, seekTargetUs, AVSEEK_FLAG_BACKWARD);
                }
            } else {
                AVRational msBase = {1, 1000};
                const qint64 seekTarget = av_rescale_q(
                    targetMs,
                    msBase,
                    m_formatCtx->streams[m_audioStreamIndex]->time_base
                );
                const qint64 seekTargetUs = targetMs * 1000;
                // 先走 avformat_seek_file（时间窗口更明确），失败再回退 av_seek_frame。
                seekRet = avformat_seek_file(
                    m_formatCtx,
                    m_audioStreamIndex,
                    std::numeric_limits<int64_t>::min(),
                    seekTarget,
                    std::numeric_limits<int64_t>::max(),
                    seekFlags
                );
                if (seekRet < 0) {
                    seekRet = av_seek_frame(m_formatCtx, m_audioStreamIndex, seekTarget, seekFlags);
                }
                // 某些网络容器（尤其 FLAC）在 stream-index seek 上会返回 EPERM，回退到全局时基 seek。
                if (seekRet < 0 && m_networkSource.load()) {
                    seekRet = avformat_seek_file(
                        m_formatCtx,
                        -1,
                        std::numeric_limits<int64_t>::min(),
                        seekTargetUs,
                        std::numeric_limits<int64_t>::max(),
                        AVSEEK_FLAG_BACKWARD
                    );
                }
                if (seekRet < 0 && m_networkSource.load()) {
                    seekRet = av_seek_frame(m_formatCtx, -1, seekTargetUs, AVSEEK_FLAG_BACKWARD);
                }
                if (seekRet < 0 && m_networkSource.load()) {
                    seekRet = av_seek_frame(m_formatCtx, -1, seekTargetUs, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);
                }
            }
            clearIoDeadline();
            if (seekRet >= 0) {
                avcodec_flush_buffers(m_codecCtx);
                m_appliedSeekGeneration = targetGeneration;
                m_waitingFirstPacketAfterSeek = true;
                m_seekEofRetryCount = 0;
                emit progressUpdated(targetMs);
            } else {
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(seekRet, errbuf, sizeof(errbuf));
                qDebug() << "AudioDecoder: Seek failed in decode thread:" << errbuf;
            }
            continue;
        }
        
        // 璇诲彇闊抽鍖?
        const bool likelyFlacNetwork = m_networkSource.load() && m_filePath.toLower().contains(".flac");
        int readTimeoutMs = likelyFlacNetwork ? 4200 : 2600;
        if (m_waitingFirstPacketAfterSeek.load()) {
            // HLS 切段后首包 RTT 通常显著高于裸文件 Range，避免误判 EOF。
            if (m_hlsSource.load()) {
                readTimeoutMs = 4500;
            } else if (likelyFlacNetwork) {
                // 远端 FLAC 大跨度 seek 后首包准备更慢，避免过早打断导致反复重试。
                readTimeoutMs = 5200;
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
                    ++m_seekEofRetryCount;
                    if (m_seekEofRetryCount <= 30) {
                        if (m_seekEofRetryCount == 1 || m_seekEofRetryCount % 5 == 0) {
                            qWarning() << "AudioDecoder: Transient EOF right after seek, retrying"
                                       << m_seekEofRetryCount << "/30";
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(120));
                        continue;
                    }
                }

                // 鍒拌揪鏂囦欢鏈熬
                qDebug() << "AudioDecoder: End of stream reached";
                emit decodeCompleted();
                break;
            } else if (ret == AVERROR_EXIT || ret == AVERROR(EINTR)) {
                // 被 seek/stop/超时中断后快速继续循环，优先处理最新请求
                m_ioAbortRequested = false;
                const int sleepMs = m_waitingFirstPacketAfterSeek.load() ? 1 : 2;
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
                continue;
            } else if (ret == AVERROR(EAGAIN)) {
                const int sleepMs = m_waitingFirstPacketAfterSeek.load() ? 1 : 2;
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
                continue;
            } else {
                // 鍏朵粬閿欒锛堝缃戠粶涓柇锛夛紝璁板綍浣嗙户缁皾璇?
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(ret, errbuf, sizeof(errbuf));
                qDebug() << "AudioDecoder: Read frame error:" << errbuf << ", will retry";
                const int sleepMs = m_waitingFirstPacketAfterSeek.load() ? 2 : 10;
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
                continue;  // 缁х画寰幆锛屼笉閫€鍑?
            }
        }
        m_seekEofRetryCount = 0;
        
        // 鍙鐞嗛煶棰戞祦
        if (m_packet->stream_index == m_audioStreamIndex) {
            // 鍙戦€佹暟鎹寘鍒拌В鐮佸櫒
            if (avcodec_send_packet(m_codecCtx, m_packet) >= 0) {
                // 鎺ユ敹瑙ｇ爜鍚庣殑甯?
                while (avcodec_receive_frame(m_codecCtx, m_frame) >= 0) {
                    // 璁＄畻閲嶉噰鏍峰悗鐨勬牱鏈暟
                    int dst_nb_samples = av_rescale_rnd(
                        swr_get_delay(m_swrCtx, m_codecCtx->sample_rate) + m_frame->nb_samples,
                        m_outSampleRate,
                        m_codecCtx->sample_rate,
                        AV_ROUND_UP
                    );
                    
                    // 璁＄畻缂撳啿鍖哄ぇ灏?
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
                    
                    // 鍒嗛厤缂撳啿鍖?
                    uint8_t* buffer = (uint8_t*)malloc(buffer_size);
                    if (!buffer) {
                        qDebug() << "Failed to allocate memory for buffer";
                        continue;
                    }
                    
                    // 鎵ц閲嶉噰鏍?
                    int converted_samples = swr_convert(
                        m_swrCtx, 
                        &buffer,
                        dst_nb_samples,
                        (const uint8_t**)m_frame->data,
                        m_frame->nb_samples
                    );
                    
                    // 璁＄畻瀹為檯杞崲鍚庣殑鏁版嵁澶у皬
                    int converted_buffer_size = av_samples_get_buffer_size(
                        nullptr, 
                        m_outChannels,
                        converted_samples,
                        m_outSampleFormat, 
                        1
                    );
                    
                    // 获取时间戳（优先 frame best effort timestamp，回退到 packet pts）。
                    qint64 timestamp_ms = 0;
                    int64_t pts = AV_NOPTS_VALUE;
                    if (m_frame->best_effort_timestamp != AV_NOPTS_VALUE) {
                        pts = m_frame->best_effort_timestamp;
                    } else if (m_packet->pts != AV_NOPTS_VALUE) {
                        pts = m_packet->pts;
                    }

                    if (pts != AV_NOPTS_VALUE) {
                        AVRational time_base = m_formatCtx->streams[m_audioStreamIndex]->time_base;
                        qint64 raw_timestamp_ms = av_rescale_q(pts, time_base, {1, 1000});

                        // 使用稳定基准换算为“曲目内时间轴”，seek 后不会重置到 0。
                        timestamp_ms = raw_timestamp_ms - m_firstPts;
                        if (timestamp_ms < 0) {
                            timestamp_ms = 0;
                        }
                        m_currentPts = timestamp_ms;
                    }
                    
                    // 鍒涘缓QByteArray骞跺彂閫佷俊鍙?
                    QByteArray pcmData(reinterpret_cast<const char*>(buffer), converted_buffer_size);
                    free(buffer);
                    
                    // 鍦ㄥ彂閫佹暟鎹墠鍐嶆妫€鏌ユ槸鍚﹀仠姝紙閬垮厤鍙戦€佹棫鏁版嵁鍒版柊Session锛?
                    if (m_stopRequested) {
                        qDebug() << "AudioDecoder: Stop requested, discarding decoded frame";
                        break;
                    }

                    if (m_waitingFirstPacketAfterSeek.exchange(false)) {
                        qDebug() << "AudioDecoder: First decoded frame arrived after seek, ts:" << timestamp_ms << "ms";
                        m_seekEofRetryCount = 0;
                    }
                    
                    emit decodedData(pcmData, timestamp_ms, m_appliedSeekGeneration.load());
                    emit progressUpdated(timestamp_ms);
                    
                    // 姣?00甯ф墦鍗颁竴娆℃棩蹇楋紝閬垮厤鏃ュ織杩囧
                    static int frameCount = 0;
                    if (++frameCount % 100 == 0) {
                        qDebug() << "AudioDecoder: Decoded" << frameCount << "frames, current timestamp:" << timestamp_ms << "ms";
                    }
                    
                    // 缁欎富绾跨▼鏃堕棿澶勭悊淇″彿锛岄伩鍏嶈В鐮佽繃蹇鑷翠俊鍙峰爢绉?
                    // 姣?0甯ield涓€娆★紝璁╀富绾跨▼鏈夋満浼氬鐞哾ecodedData淇″彿
                    if (frameCount % 10 == 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    }
                }
            }
        }
        
        // 閲婃斁鏁版嵁鍖?
        av_packet_unref(m_packet);
    }
    
    // 瑙ｇ爜绾跨▼缁撴潫锛岄噸缃姸鎬佹爣蹇?
    m_decoding = false;
    qDebug() << "AudioDecoder::decode thread finished, m_decoding reset to false";
}

void AudioDecoder::cleanup()
{
    // TODO: 娓呯悊FFmpeg璧勬簮
    if(m_frame)
        av_frame_free(&m_frame);
    if(m_packet)
    {
        av_packet_unref(m_packet);
        av_packet_free(&m_packet);
    }
    if(m_codecCtx)
        avcodec_free_context(&m_codecCtx);
    if(m_formatCtx)
        avformat_close_input(&m_formatCtx);
    if(m_swrCtx)
        swr_free(&m_swrCtx);
}

void AudioDecoder::extractAudioTags()
{
    if (!m_formatCtx) return;

    // 浠巉ormat context鐨刴etadata涓彁鍙栨爣棰樺拰鑹烘湳瀹?
    AVDictionaryEntry *tag = nullptr;

    // 鎻愬彇鏍囬
    tag = av_dict_get(m_formatCtx->metadata, "title", nullptr, 0);
    if (tag) {
        m_extractedTitle = QString::fromUtf8(tag->value);
        qDebug() << "AudioDecoder: Extracted title from metadata:" << m_extractedTitle;
    } else {
        m_extractedTitle = "";
    }

    // 鎻愬彇鑹烘湳瀹?
    tag = av_dict_get(m_formatCtx->metadata, "artist", nullptr, 0);
    if (tag) {
        m_extractedArtist = QString::fromUtf8(tag->value);
        qDebug() << "AudioDecoder: Extracted artist from metadata:" << m_extractedArtist;
    } else {
        m_extractedArtist = "";
    }

    // 鍙戦€侀煶棰戞爣绛句俊鍙?
    emit audioTagsReady(m_extractedTitle, m_extractedArtist);
}

void AudioDecoder::extractAlbumArt()
{
    if (!m_formatCtx) return;
    
    // 鏌ユ壘闄勫姞鍥剧墖娴侊紙灏侀潰锛?
    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++) {
        if (m_formatCtx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            AVPacket *pkt = &m_formatCtx->streams[i]->attached_pic;
            
            // 淇濆瓨灏侀潰鍒颁复鏃舵枃浠?
            QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                             + "/album_cover_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".jpg";
            
            QFile file(tempPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write((const char*)pkt->data, pkt->size);
                file.close();
                
                // 杞崲涓?file:/// URL 鏍煎紡渚?QML 浣跨敤
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
    
    // 娌℃湁鎵惧埌灏侀潰锛屼娇鐢ㄩ粯璁ゅ浘鐗?
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
    
    AVDictionary* options = nullptr;
    // 缃戠粶瓒呮椂璁剧疆锛?0绉掞級锛屽簲瀵圭綉缁滀笉绋冲畾
    av_dict_set(&options, "timeout", "30000000", 0);  // 30绉掕秴鏃?
    av_dict_set(&options, "rw_timeout", "3000000", 0); // 单次I/O约3秒超时
    av_dict_set(&options, "reconnect", "1", 0);  // 鍚敤鑷姩閲嶈繛
    av_dict_set(&options, "reconnect_streamed", "1", 0);  // 娴佸紡閲嶈繛
    av_dict_set(&options, "reconnect_delay_max", "5", 0);  // 鏈€澶ч噸杩炲欢杩?绉?
    if (m_networkSource.load()) {
        // 强制按可随机访问网络流处理，避免 server 已支持 Range 但被判为不可 seek。
        av_dict_set(&options, "seekable", "1", 0);
    }
    
    setIoDeadlineMs(9000);
    int ret = avformat_open_input(&m_formatCtx, inputUrl, nullptr, &options);
    clearIoDeadline();
    av_dict_free(&options);
    
    auto t1 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING]   avformat_open_input:" 
             << std::chrono::duration<double, std::milli>(t1 - t0).count() << "ms";
    
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "Could not open input file:" << errbuf << "Error code:" << ret;
        avformat_close_input(&m_formatCtx);
        return false;
    }
    
    setIoDeadlineMs(9000);
    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        clearIoDeadline();
        qDebug() << "Could not find stream information";
        avformat_close_input(&m_formatCtx);
        return false;
    }
    clearIoDeadline();
    
    auto t2 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING]   avformat_find_stream_info:" 
             << std::chrono::duration<double, std::milli>(t2 - t1).count() << "ms";
    
    // 鏌ユ壘闊抽娴?
    m_audioStreamIndex = -1;
    for (int i = 0; i < (int)m_formatCtx->nb_streams; ++i) {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioStreamIndex = i;
            break;
        }
    }
    
    if (m_audioStreamIndex == -1) {
        qDebug() << "Could not find an audio stream";
        avformat_close_input(&m_formatCtx);
        return false;
    }
    
    qDebug() << "Audio stream found at index:" << m_audioStreamIndex;
    return true;
}

bool AudioDecoder::setupDecoder()
{
    if (!m_formatCtx || m_audioStreamIndex < 0) return false;
    
    // 鏌ユ壘瑙ｇ爜鍣?
    AVCodec* codec = avcodec_find_decoder(m_formatCtx->streams[m_audioStreamIndex]->codecpar->codec_id);
    if (!codec) {
        qDebug() << "Codec not found";
        return false;
    }
    
    // 鍒嗛厤瑙ｇ爜鍣ㄤ笂涓嬫枃
    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) {
        qDebug() << "Could not allocate codec context";
        return false;
    }
    
    // 澶嶅埗鍙傛暟
    if (avcodec_parameters_to_context(m_codecCtx, m_formatCtx->streams[m_audioStreamIndex]->codecpar) < 0) {
        qDebug() << "Failed to copy codec parameters to codec context";
        avcodec_free_context(&m_codecCtx);
        return false;
    }
    
    // 鎵撳紑瑙ｇ爜鍣?
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        qDebug() << "Could not open codec";
        avcodec_free_context(&m_codecCtx);
        return false;
    }
    
    // 鍒嗛厤甯у拰鍖?
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
    
    // 鑾峰彇杈撳叆鏍煎紡
    int64_t input_channel_layout = m_codecCtx->channel_layout;
    if (input_channel_layout == 0) {
        input_channel_layout = av_get_default_channel_layout(m_codecCtx->channels);
    }
    
    // 鍒嗛厤閲嶉噰鏍蜂笂涓嬫枃
    m_swrCtx = swr_alloc();
    if (!m_swrCtx) {
        qDebug() << "Could not allocate SwrContext";
        return false;
    }
    
    // 閰嶇疆閲嶉噰鏍峰弬鏁?
    int64_t output_channel_layout = AV_CH_LAYOUT_STEREO;
    
    av_opt_set_int(m_swrCtx, "in_channel_layout", input_channel_layout, 0);
    av_opt_set_int(m_swrCtx, "out_channel_layout", output_channel_layout, 0);
    av_opt_set_int(m_swrCtx, "in_sample_rate", m_codecCtx->sample_rate, 0);
    av_opt_set_int(m_swrCtx, "out_sample_rate", m_outSampleRate, 0);
    av_opt_set_sample_fmt(m_swrCtx, "in_sample_fmt", m_codecCtx->sample_fmt, 0);
    av_opt_set_sample_fmt(m_swrCtx, "out_sample_fmt", m_outSampleFormat, 0);
    
    // 鍒濆鍖栭噸閲囨牱鍣?
    if (swr_init(m_swrCtx) < 0) {
        qDebug() << "Could not initialize resampler";
        swr_free(&m_swrCtx);
        return false;
    }
    
    qDebug() << "Resampler setup successfully";
    return true;
}

