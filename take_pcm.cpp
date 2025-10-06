#include "take_pcm.h"
#include <QReadWriteLock>
#include <QtConcurrent>
QMutex bufferMutex;
QReadWriteLock readlock;

static int interrupt_callback(void* ctx) {
    TakePcm* self = static_cast<TakePcm*>(ctx);
    if (self->get_stop_flag()) {
        return 1; // 返回非零值中断操作
    }
    return 0;
}
void TakePcm::initialize() {
    AVIOInterruptCB cb = {interrupt_callback, this};
    ifmt_ctx->interrupt_callback = cb; // 设置回调
}

TakePcm::TakePcm()
    :ifmt_ctx(nullptr)
    ,codec_ctx(nullptr)
    ,swr_ctx(nullptr)
    ,frame(nullptr)
    ,pkt(nullptr)

{
    connect(this,&TakePcm::begin_to_decode,this,&TakePcm::decode);
    connect(this, &TakePcm::signal_begin_make_pcm, this, &TakePcm::make_pcm);
}
TakePcm::~TakePcm()
{

    if(frame)
        av_frame_free(&frame);
    if(pkt)
        av_packet_free(&pkt);
    if(codec_ctx)
        avcodec_free_context(&codec_ctx);
    if(ifmt_ctx)
        avformat_close_input(&ifmt_ctx);
    if(swr_ctx)
        swr_free(&swr_ctx);
}
QString getSubstringAfterLastDot(const QString &inputStr) {
    QStringList parts = inputStr.split('.'); // 按点号分割
    if (parts.isEmpty()) {
        return "";
    }
    return parts.last(); // 返回列表中的最后一个元素
}
void TakePcm::seekToPosition(int newPosition)
{
    int64_t targetTimestamp = static_cast<int64_t>(newPosition) * 1000;

    //av_seek_frame(ifmt_ctx, -1, targetTimestamp, AVSEEK_FLAG_BACKWARD);
    int retries = 3;
    while (retries--) {
        int ret = av_seek_frame(ifmt_ctx, -1, targetTimestamp, AVSEEK_FLAG_BACKWARD);
        if (ret >= 0) {
            break;
        }
        qDebug()<<__FUNCTION__<<"failed"<<retries;
        QThread::msleep(50);
    }
    avcodec_flush_buffers(codec_ctx);

    emit Position_Change();
    emit begin_to_decode();
}
void TakePcm::take_album()
{

}

void TakePcm::make_pcm(QString Path)
{

    if(frame)
        av_frame_free(&frame);
    if(pkt)
    {
        av_packet_unref(pkt);
        av_packet_free(&pkt);
    }
    if(codec_ctx)
        avcodec_free_context(&codec_ctx);
    if(ifmt_ctx)
        avformat_close_input(&ifmt_ctx);
    if(swr_ctx)
        swr_free(&swr_ctx);

    qDebug()<<"Take_pcm"<<QThread::currentThreadId();

    QByteArray utf8Data = Path.toUtf8();
    const char* inputUrl = utf8Data.constData();
    //const char* inputUrl = stdStr.c_str();

    ifmt_ctx=nullptr;
    AVDictionary* options = nullptr;
    av_dict_set(&options, "listen_timeout", "5000000", 0);

    QString back_ = getSubstringAfterLastDot(inputUrl);
    //stdStr = back_.toStdString();
    //AVInputFormat *ifmt = av_find_input_format(stdStr.c_str());
    qDebug() << __FUNCTION__ << inputUrl << Path;
    int ret = avformat_open_input(&ifmt_ctx, inputUrl, nullptr, &options);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << __FUNCTION__ << inputUrl << Path;
        qDebug() << "Could not open input file:" << errbuf << "Error code:" << ret;
        return;
    }

    av_dict_free(&options);
    if (avformat_find_stream_info(ifmt_ctx, nullptr) < 0)
    {
        qDebug() << "Could not find stream information";
        avformat_close_input(&ifmt_ctx);
        return;
    }

    audioStreamIndex = -1;
    for (int i = 0; i <(int)ifmt_ctx->nb_streams; ++i)
    {
        if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioStreamIndex = i;
            break;
        }
    }

    if (audioStreamIndex == -1)
    {
        qDebug() << "Could not find an audio stream";
        avformat_close_input(&ifmt_ctx);
        return;
    }

    //    AVCodec* codec = avcodec_find_decoder(ifmt_ctx->streams[audioStreamIndex]->codecpar->codec_id);
    //    //AVCodec* codec = avcodec_find_decoder_by_name("aac");
    //    if (!codec)
    //    {
    //        qDebug() << "Codec not found";
    //        avformat_close_input(&ifmt_ctx);
    //        return;
    //    }
    AVCodec* codec = nullptr;
    if (ifmt_ctx->streams[audioStreamIndex]->codecpar->codec_id == AV_CODEC_ID_H264) {
        codec = avcodec_find_decoder_by_name("h264_cuvid"); // 尝试 NVIDIA CUVID 解码器
        if (!codec) {
            qDebug() << "Hardware decoder for H.264 not found, fallback to software.";
        }
    }


    if (!codec) {
        codec = avcodec_find_decoder(ifmt_ctx->streams[audioStreamIndex]->codecpar->codec_id);
    }
    if (!codec) {
        qDebug() << "Codec not found";
        avformat_close_input(&ifmt_ctx);
        return;
    }
    // 初始化 AVCodecContext
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx)
    {
        qDebug() << "Could not allocate codec context";
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 将 AVCodecParameters 转换为 AVCodecContext
    if (avcodec_parameters_to_context(codec_ctx, ifmt_ctx->streams[audioStreamIndex]->codecpar) < 0)
    {
        qDebug() << "Failed to copy codec parameters to codec context";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 打开解码器
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0)
    {
        qDebug() << "Could not open codec";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 获取音频流信息
    int input_sample_rate = codec_ctx->sample_rate;  // 输入采样率
    int64_t input_channel_layout = codec_ctx->channel_layout;  // 输入通道布局
    enum AVSampleFormat input_sample_fmt = codec_ctx->sample_fmt;  // 输入样本格式

    emit durations(ifmt_ctx->duration);


    // 初始化 AVFrame 和 AVPacket
    frame = av_frame_alloc();
    if (!frame)
    {
        qDebug() << "Could not allocate frame";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    pkt = av_packet_alloc();
    if (!pkt)
    {
        qDebug() << "Could not allocate packet";
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 初始化 SwrContext
    swr_ctx = swr_alloc();
    if (!swr_ctx)
    {
        qDebug() << "Could not allocate SwrContext";
        av_packet_free(&pkt);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 配置 SwrContext
    int output_sample_rate = RATE;  // 目标采样率（可以根据需要调整）
    int64_t output_channel_layout = AV_CH_LAYOUT_STEREO;  // 目标通道布局（例如立体声）
    enum AVSampleFormat output_sample_fmt = AV_SAMPLE_FMT_S16;  // 目标样本格式（例如 16-bit）

    av_opt_set_int(swr_ctx, "in_channel_layout", input_channel_layout, 0);
    av_opt_set_int(swr_ctx, "out_channel_layout", output_channel_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", input_sample_rate, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", output_sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", input_sample_fmt, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", output_sample_fmt, 0);

    if (swr_init(swr_ctx) < 0)
    {
        qDebug() << "Could not initialize resampler";
        swr_free(&swr_ctx);
        av_packet_free(&pkt);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    emit begin_take_lrc(Path);


    qint64 totalAudioDurationInMS = ifmt_ctx->duration * av_q2d(AV_TIME_BASE_Q) * 1000;


    emit send_totalDuration(totalAudioDurationInMS);
    emit begin_to_play();
    emit begin_to_decode();

}

void TakePcm::decode()
{
    while(1)
    {

        int ret = av_read_frame(ifmt_ctx, pkt);
        if(ret < 0)
            break;
        if (pkt->stream_index == audioStreamIndex)
        {
            if (avcodec_send_packet(codec_ctx, pkt) >= 0)
            {
                while (avcodec_receive_frame(codec_ctx, frame) >= 0)
                {

                    // 计算目标缓冲区大小
                    int dst_nb_samples = av_rescale_rnd(
                        swr_get_delay(swr_ctx, codec_ctx->sample_rate) + frame->nb_samples,
                        44100,
                        codec_ctx->sample_rate,
                        AV_ROUND_UP
                        );
                    int buffer_size = av_samples_get_buffer_size(nullptr, 2, dst_nb_samples, AV_SAMPLE_FMT_S16, 1);

                    // 确保 buffer_size 不小于0
                    if (buffer_size <= 0)
                    {
                        qDebug() << "Error calculating buffer size:" << buffer_size;
                        continue;
                    }
                    uint8_t* buffer = nullptr;
                    buffer = (uint8_t*)malloc(buffer_size);

                    if(!buffer)
                    {
                        qDebug()<<"Failed to allocate memory for buffer.";
                        return;
                    }
                    int converted_samples = swr_convert(swr_ctx, &buffer
                                                        , dst_nb_samples
                                                        , (const uint8_t**)frame->data
                                                        , frame->nb_samples);

                    int converted_buffer_size = av_samples_get_buffer_size(nullptr, 2
                                                                           , converted_samples
                                                                           , AV_SAMPLE_FMT_S16, 1);


                    // 获取时间戳（以毫秒为单位）
                    AVRational time_base = ifmt_ctx->streams[audioStreamIndex]->time_base;
                    if (pkt->pts != AV_NOPTS_VALUE)
                    {
                        int64_t pts = pkt->pts;
                        qint64 timestamp_ms = av_rescale_q(pts, time_base, {1, 1000});
                        //qDebug() << "Timestamp (ms):" << timestamp_ms;
                        if(isTranslate.load()){
                            emit signal_send_data(buffer, converted_buffer_size, timestamp_ms);
                        }else{
                            send_data(buffer, converted_buffer_size, timestamp_ms);
                        }
                    }

                }
            }
        }
        av_packet_unref(pkt);

    }
    emit signal_decodeEnd();
}


void TakePcm::send_data(uint8_t *buffer, int bufferSize,qint64 timeMap)
{


    QByteArray byteArray(reinterpret_cast<const char*>(buffer), bufferSize);
    free(buffer);

    emit data(byteArray,timeMap);

}

