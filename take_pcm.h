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
    void setTranslate(bool flag){this->isTranslate = flag;};
public slots:
    void make_pcm(QString Path);
    void seekToPosition(int newPosition, bool back_flag);
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
    void signal_send_data(uint8_t *buffer, int bufferSize, qint64 timeMap);
    void signal_decodeEnd();
    void signal_seetToPosition(int newPosition);
    void signal_reconnect();
    void signal_worker_play();
    void signal_move();
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

    std::atomic<bool> move_flag = false;
    std::atomic<bool> isTranslate = false;


    bool printF = false;
};

#endif // TAKE_PCM_H
