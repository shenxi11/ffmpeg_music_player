#include "take_pcm.h"

Take_pcm::Take_pcm()
{

}
Take_pcm::~Take_pcm(){
    qDebug()<<"Destruct Take_pcm";
}
//将音频文件转化为pcm文件
void Take_pcm::make_pcm(QString Path){
    qDebug()<<"Take_pcm"<<QThread::currentThreadId();
    avformat_network_init();

    const char* inputUrl = Path.toUtf8().constData();

     AVFormatContext* ifmt_ctx=nullptr;
    if (avformat_open_input(&ifmt_ctx, inputUrl, nullptr, nullptr) != 0) {
        qDebug() << "Could not open input fileA";
        return;
    }

    if (avformat_find_stream_info(ifmt_ctx, nullptr) < 0) {
        qDebug() << "Could not find stream information";
        avformat_close_input(&ifmt_ctx);
        return;
    }

    int audioStreamIndex = -1;
    for (int i = 0; i <(int)ifmt_ctx->nb_streams; ++i) {
        if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }

    if (audioStreamIndex == -1) {
        qDebug() << "Could not find an audio stream";
        avformat_close_input(&ifmt_ctx);
        return;
    }

    AVCodec* codec = avcodec_find_decoder(ifmt_ctx->streams[audioStreamIndex]->codecpar->codec_id);
    if (!codec) {
        qDebug() << "Codec not found";
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 初始化 AVCodecContext
    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        qDebug() << "Could not allocate codec context";
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 将 AVCodecParameters 转换为 AVCodecContext
    if (avcodec_parameters_to_context(codec_ctx, ifmt_ctx->streams[audioStreamIndex]->codecpar) < 0) {
        qDebug() << "Failed to copy codec parameters to codec context";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 打开解码器
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
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
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        qDebug() << "Could not allocate frame";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        qDebug() << "Could not allocate packet";
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 初始化 SwrContext
    SwrContext* swr_ctx = swr_alloc();
    if (!swr_ctx) {
        qDebug() << "Could not allocate SwrContext";
        av_packet_free(&pkt);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }

    // 配置 SwrContext
    int output_sample_rate = 44100;  // 目标采样率（可以根据需要调整）
    int64_t output_channel_layout = AV_CH_LAYOUT_STEREO;  // 目标通道布局（例如立体声）
    enum AVSampleFormat output_sample_fmt = AV_SAMPLE_FMT_S16;  // 目标样本格式（例如 16-bit）

    av_opt_set_int(swr_ctx, "in_channel_layout", input_channel_layout, 0);
    av_opt_set_int(swr_ctx, "out_channel_layout", output_channel_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", input_sample_rate, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", output_sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", input_sample_fmt, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", output_sample_fmt, 0);

    if (swr_init(swr_ctx) < 0) {
        qDebug() << "Could not initialize resampler";
        swr_free(&swr_ctx);
        av_packet_free(&pkt);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        return;
    }


    QFileInfo info(inputUrl);
    QString path = QDir::currentPath() + "/" + info.baseName() + ".pcm";



    emit begin_take_lrc(Path);

    FILE* f = fopen(path.toUtf8().constData(), "wb");
    if (!f) {
        qDebug() << "Could not open output fileB";
        av_frame_free(&frame);
        av_packet_free(&pkt);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&ifmt_ctx);
        swr_free(&swr_ctx);
        return;
    }


    while (av_read_frame(ifmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == audioStreamIndex) {
            if (avcodec_send_packet(codec_ctx, pkt) >= 0) {
                while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
                    int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, codec_ctx->sample_rate) + frame->nb_samples, 44100, codec_ctx->sample_rate, AV_ROUND_UP);
                    uint8_t* buffer = nullptr;
                    int buffer_size = av_samples_get_buffer_size(nullptr, 2, dst_nb_samples, AV_SAMPLE_FMT_S16, 1);
                    if (buffer_size < 0) {
                        qDebug() << "Error calculating buffer size:" << buffer_size;
                        continue;
                    }
                    buffer = (uint8_t*)malloc(buffer_size);
                    if (!buffer) {
                        qDebug() << "Could not allocate buffer";
                        continue;
                    }

                    int converted_samples = swr_convert(swr_ctx, &buffer, dst_nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
                    int converted_buffer_size = av_samples_get_buffer_size(nullptr, 2, converted_samples, AV_SAMPLE_FMT_S16, 1);
                    fwrite(buffer, 1, converted_buffer_size, f);
                    free(buffer);
                }
            }
        }
        av_packet_unref(pkt);
    }

    fclose(f);


    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&ifmt_ctx);
    swr_free(&swr_ctx);

    //qDebug()<<"finish taking";

    emit begin_to_play(path);
}
