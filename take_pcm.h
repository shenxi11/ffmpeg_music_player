#ifndef TAKE_PCM_H
#define TAKE_PCM_H

#include <QObject>
#include<QThreadPool>
#include <QtConcurrent>

#include"headers.h"
class TakePcm : public QObject
{
    Q_OBJECT
public:
    explicit TakePcm();

    ~TakePcm();
public slots:
    void make_pcm(QString Path);

    void seekToPosition(int newPosition);
    void initialize();
    std::atomic<bool>& get_stop_flag(){return stop_flag;}
signals:
    void begin_take_lrc(QString Path);
    void begin_to_play();
    void durations(int64_t value);
    void send_pcmMap(std::vector<std::pair<qint64, qint64>> pcmTimeMap);
    void send_totalDuration(qint64 totalAudioDurationInMS);
    void data(const QByteArray &data, qint64 timeMap);
    void Position_Change();
    void begin_to_decode();
    void signal_begin_make_pcm(QString path);
    void signal_send_pic_path(QString picPath);


private:
    void send_data(uint8_t *buffer, int bufferSize, qint64 timeMap);
    void take_album();
    void decode();
    void run_async() {
            QtConcurrent::run(std::bind(&TakePcm::take_album, this));
        }

    AVFormatContext *ifmt_ctx;
    AVCodecContext *codec_ctx ;
    SwrContext *swr_ctx;
    AVFrame *frame;
    AVPacket *pkt;
    AVIOContext *avio_ctx = nullptr;

    int audioStreamIndex;

    std::atomic<bool> stop_flag = false;
    std::atomic<bool> end_flag = false;
};

#endif // TAKE_PCM_H
