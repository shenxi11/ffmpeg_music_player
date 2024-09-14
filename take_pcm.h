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

signals:
    void begin_take_lrc(QString Path);

    void begin_to_play(QString path);

    void durations(int64_t value);

    void send_pcmMap(std::vector<std::pair<qint64, qint64>> pcmTimeMap);

    void send_totalDuration(qint64 totalAudioDurationInMS);
};

#endif // TAKE_PCM_H
