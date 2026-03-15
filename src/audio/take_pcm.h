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
    void makePcm(QString Path);
    void seekToPosition(int newPosition, bool back_flag);
    void decodeThread();
signals:
    void begin_take_lrc(QString Path);
    void begin_to_play();
    void durations(int64_t value);
    void send_pcmMap(std::vector<std::pair<qint64, qint64>> pcmTimeMap);
    void send_totalDuration(qint64 totalAudioDurationInMS);
    void data(const QByteArray &data, qint64 timeMap);
    void Position_Change();
    void begin_to_decode();
    void signalBeginMakePcm(QString path);
    void signalSendPicPath(QString picPath);
    void signalSendData(uint8_t *buffer, int bufferSize, qint64 timeMap);
    void signalDecodeEnd();
    void signalSeekToPosition(int newPosition);
    void signalReconnect();
    void signalWorkerPlay();
    void signalMove();
private:
    void sendData(uint8_t *buffer, int bufferSize, qint64 timeMap);
    void takeAlbum();
    void decode();
    void run_async() {
            QtConcurrent::run(std::bind(&TakePcm::takeAlbum, this));
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

    std::thread thread_;
    std::mutex mtx;
    std::atomic<bool> decode_flag = false;
    std::atomic<bool> decoding = false;
    std::atomic<bool> break_flag = false;
    std::condition_variable cv;
};

#endif // TAKE_PCM_H
