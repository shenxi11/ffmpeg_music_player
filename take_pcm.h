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

public slots:
    void make_pcm(QString Path);

signals:
    void begin_take_lrc(QString Path);

    void begin_to_play(QString path);

    void durations(int64_t value);
private:
     AVFormatContext* ifmt_ctx;
};

#endif // TAKE_PCM_H
