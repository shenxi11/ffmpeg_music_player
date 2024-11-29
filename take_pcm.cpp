#include "take_pcm.h"

QWaitCondition dataAvailable;
QMutex bufferMutex;
bool endOfStream = false;
QQueue<QByteArray> dataQueue;

static void receiveData(QByteArray newData);
static void finishStream();
static int readPacket(void* opaque, uint8_t* buf, int buf_size);




Take_pcm::Take_pcm():drag(false) ,
    ifmt_ctx(nullptr)

  ,codec_ctx(nullptr)

  ,swr_ctx(nullptr)

  ,frame(nullptr)

  ,pkt(nullptr)

{
    connect(this,&Take_pcm::begin_to_decode,this,&Take_pcm::decode);
}
Take_pcm::~Take_pcm()
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
    //qDebug()<<"Destruct Take_pcm";
}
void Take_pcm::seekToPosition(int newPosition)
{


    // 将毫秒转换为微秒（1毫秒 = 1000微秒）
    int64_t targetTimestamp = static_cast<int64_t>(newPosition) * 1000;

    //qDebug()<<targetTimestamp;

    av_seek_frame(ifmt_ctx, -1, targetTimestamp, AVSEEK_FLAG_BACKWARD);

    //qDebug()<<ret;

    avcodec_flush_buffers(codec_ctx);

    emit Position_Change();

    emit begin_to_decode();



}

void Take_pcm::make_pcm(QString Path)
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

    //qDebug()<<"Take_pcm"<<QThread::currentThreadId();
    avformat_network_init();




    const char* inputUrl = Path.toUtf8().constData();

    ifmt_ctx=nullptr;
    if (avformat_open_input(&ifmt_ctx, inputUrl, nullptr, nullptr) != 0)
    {
        qDebug() << "Could not open input fileA";
        return;
    }

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

    AVCodec* codec = avcodec_find_decoder(ifmt_ctx->streams[audioStreamIndex]->codecpar->codec_id);
    if (!codec)
    {
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

    //    // 打印音频流信息
    //    qDebug() << "Input Sample Rate:" << input_sample_rate;
    //    qDebug() << "Input Channels:" << codec_ctx->channels;
    //    qDebug() << "Input Bitrate:" << codec_ctx->bit_rate;
    //    qDebug() << "Input Sample Format:" << input_sample_fmt;

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

    //qDebug()<<"path is "<<Path;


    //获取音频文件的总时长

    qint64 totalAudioDurationInMS = ifmt_ctx->duration * av_q2d(AV_TIME_BASE_Q) * 1000;


    emit send_totalDuration(totalAudioDurationInMS);
    emit begin_to_play();


    emit begin_to_decode();

}

void Take_pcm::decodeFromStream()
{
    if (frame)
        av_frame_free(&frame);
    if (pkt)
        av_packet_free(&pkt);
    if (codec_ctx)
        avcodec_free_context(&codec_ctx);
    if (ifmt_ctx)
        avformat_close_input(&ifmt_ctx);
    if (swr_ctx)
        swr_free(&swr_ctx);

    avformat_network_init(); // 初始化网络协议支持
    ifmt_ctx = avformat_alloc_context(); // 创建 AVFormatContext
    if (!ifmt_ctx) {
        qDebug() << "Failed to allocate AVFormatContext.";
        return;
    }

    // 为 AVPacket 分配内存
    pkt = av_packet_alloc();
    if (!pkt) {
        qDebug() << "Failed to allocate AVPacket.";
        avformat_free_context(ifmt_ctx);
        return;
    }

    // 创建 AVIOContext，传入 readPacket 回调
    AVIOContext *avio_ctx = nullptr;
    uint8_t *buffer = static_cast<uint8_t*>(av_malloc(BUFFER_SIZE)); // 分配缓冲区
    if (!buffer) {
        qDebug() << "Failed to allocate buffer.";
        av_packet_free(&pkt);
        avformat_free_context(ifmt_ctx);
        return;
    }

    avio_ctx = avio_alloc_context(buffer, 4096, 0, nullptr, readPacket, nullptr, nullptr);
    if (!avio_ctx) {
        qDebug() << "Failed to allocate AVIOContext.";
        if (buffer)
            av_free(buffer);
        av_packet_free(&pkt);
        avformat_free_context(ifmt_ctx);
        return;
    }

    ifmt_ctx->pb = avio_ctx;
    ifmt_ctx->flags = AVFMT_FLAG_CUSTOM_IO;

    // 打开输入流，这里是从 QByteArray 或网络流读取
    if (avformat_open_input(&ifmt_ctx, nullptr, nullptr, nullptr) < 0) {
        qDebug() << "Failed to open input.";
        avio_context_free(&avio_ctx);
        av_free(buffer);
        av_packet_free(&pkt);
        avformat_free_context(ifmt_ctx);
        return;
    }

    // 获取流信息
    if (avformat_find_stream_info(ifmt_ctx, nullptr) < 0) {
        qDebug() << "Failed to find stream info.";
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 获取音频流索引
    audioStreamIndex = -1;
    AVCodecParameters* codec_params = nullptr;
    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
        codec_params = ifmt_ctx->streams[i]->codecpar;
        if (codec_params->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }

    if (audioStreamIndex == -1) {
        qDebug() << "Audio stream not found.";
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 查找解码器
    AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        qDebug() << "Audio codec not found.";
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 创建解码器上下文
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        qDebug() << "Failed to allocate codec context.";
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 将流参数复制到解码器上下文
    if (avcodec_parameters_to_context(codec_ctx, codec_params) < 0) {
        qDebug() << "Failed to copy codec parameters to codec context.";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 打开解码器
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        qDebug() << "Failed to open codec.";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 打印音频流的参数
    qDebug() << "Input Sample Rate:" << codec_ctx->sample_rate;
    qDebug() << "Input Channel Layout:" << codec_ctx->channel_layout;
    qDebug() << "Input Sample Format:" << codec_ctx->sample_fmt;

    // 检查解码器参数是否有效
    if (codec_ctx->sample_rate <= 0 || codec_ctx->channel_layout == 0 || codec_ctx->sample_fmt == AV_SAMPLE_FMT_NONE) {
        qDebug() << "Invalid codec context parameters!";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 为解码帧分配内存
    frame = av_frame_alloc();
    if (!frame) {
        qDebug() << "Failed to allocate frame.";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 为音频转换分配 SwrContext
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        qDebug() << "Failed to allocate SwrContext.";
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    int output_sample_rate = RATE;  // 目标采样率（可以根据需要调整）
    int64_t output_channel_layout = AV_CH_LAYOUT_STEREO;  // 目标通道布局（例如立体声）
    enum AVSampleFormat output_sample_fmt = AV_SAMPLE_FMT_S16;  // 目标样本格式（例如 16-bit）

    // 检查并设置 SwrContext 参数
    if (av_opt_set_int(swr_ctx, "in_channel_layout", codec_ctx->channel_layout, 0) < 0) {
        qDebug() << "Failed to set input channel layout.";
    }
    if (av_opt_set_int(swr_ctx, "out_channel_layout", output_channel_layout, 0) < 0) {
        qDebug() << "Failed to set output channel layout.";
    }
    if (av_opt_set_int(swr_ctx, "in_sample_rate", codec_ctx->sample_rate, 0) < 0) {
        qDebug() << "Failed to set input sample rate.";
    }
    if (av_opt_set_int(swr_ctx, "out_sample_rate", output_sample_rate, 0) < 0) {
        qDebug() << "Failed to set output sample rate.";
    }
    if (av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", codec_ctx->sample_fmt, 0) < 0) {
        qDebug() << "Failed to set input sample format.";
    }
    if (av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", output_sample_fmt, 0) < 0) {
        qDebug() << "Failed to set output sample format.";
    }

    // 初始化音频重采样器
    int ret = swr_init(swr_ctx);
    if (ret < 0) {
        qDebug() << "Could not initialize resampler:";
        swr_free(&swr_ctx);
        av_packet_free(&pkt);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    qint64 totalAudioDurationInMS = ifmt_ctx->duration * av_q2d(AV_TIME_BASE_Q) * 1000;


    emit send_totalDuration(totalAudioDurationInMS);

    qInfo()<<__FUNCTION__<<totalAudioDurationInMS;

    emit begin_to_play();
    emit begin_to_decode();
}

void Take_pcm::decode()
{

    while (av_read_frame(ifmt_ctx, pkt) >= 0)
    {
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

                    //                    QByteArray pcmData;
                    //                    pcmData.resize(buffer_size);
                    //                    uint8_t* buffer = (uint8_t*)pcmData.data();
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

                        // 可以将时间戳与对应的数据一起发送
                        send_data(buffer, converted_buffer_size, timestamp_ms);
                    }

                }
            }
        }
        av_packet_unref(pkt);
    }
}

void Take_pcm::send_data(uint8_t *buffer, int bufferSize,qint64 timeMap)
{


    QByteArray byteArray(reinterpret_cast<const char*>(buffer), bufferSize);

    free(buffer);

    emit data(byteArray,timeMap);


}


int readPacket(void* opaque, uint8_t* buf, int buf_size) {
    QMutexLocker locker(&bufferMutex);

    // 等待数据可用
    while (dataQueue.isEmpty() && !endOfStream) {
        dataAvailable.wait(&bufferMutex);
    }

    // 如果流结束且没有数据，返回 EOF
    if (dataQueue.isEmpty() && endOfStream) {
        return AVERROR_EOF;
    }

    // 从队列中取出数据
    QByteArray chunk = dataQueue.dequeue();
    int toCopy = qMin(buf_size, chunk.size());
    memcpy(buf, chunk.data(), toCopy);

    // 如果还有剩余数据，将剩余部分放回队列
    if (toCopy < chunk.size()) {
        dataQueue.enqueue(chunk.mid(toCopy));
    }

    return toCopy;
}
void Take_pcm::Receivedata(QByteArray chunk)
{
    QMutexLocker locker(&bufferMutex);
    receiveData(chunk);
}

void receiveData(QByteArray newData) {

    // 将新数据添加到队列
    dataQueue.enqueue(newData);
    dataAvailable.wakeOne(); // 通知等待的读取线程有新数据可用
}

void finishStream() {
    QMutexLocker locker(&bufferMutex);
    endOfStream = true;
    dataAvailable.wakeAll(); // 通知所有等待线程，流已经结束
}
