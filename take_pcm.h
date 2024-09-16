#ifndef TAKE_PCM_H
#define TAKE_PCM_H

#include <QObject>
#include<QThreadPool>
#include"headers.h"

class Take_pcm : public QObject
{
    Q_OBJECT
public:
    explicit Take_pcm();

    ~Take_pcm();
public slots:
    void make_pcm(QString Path);

    void seekToPosition(int newPosition);

signals:
    void begin_take_lrc(QString Path);

    void begin_to_play();

    void durations(int64_t value);

    void send_pcmMap(std::vector<std::pair<qint64, qint64>> pcmTimeMap);

    void send_totalDuration(qint64 totalAudioDurationInMS);

    void data(const QByteArray &data,qint64 timeMap);

    void Position_Change();

    void begin_to_decode();
private:
    void send_data(uint8_t *buffer, int bufferSize,qint64 timeMap);

    void decode();

    AVFormatContext *ifmt_ctx;

    AVCodecContext*codec_ctx ;

    SwrContext*swr_ctx;

    AVFrame* frame;

    AVPacket* pkt;

    int audioStreamIndex;

    bool drag;
};

#endif // TAKE_PCM_H
