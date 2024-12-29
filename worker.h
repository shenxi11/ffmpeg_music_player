#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include"headers.h"



class Worker :public QObject
{
    Q_OBJECT
public:
    explicit Worker();

    ~Worker();

public slots:

    void stop_play();

    void receive_lrc(std::map<int, std::string> lyrics);

    void Set_Volume(int value);

    void set_SliderMove(bool flag);

    void receive_totalDuration(qint64 total);

    void play_pcm();

    void receive_data(const QByteArray &data, qint64 timeMap);

    void reset_play();

    void setPATH(QString Path);

    void reset_status();
private slots:
    void onTimeOut();

    void Pause();

signals:
    void durations(qint64 value);

    void stopPlay();

    void send_lrc(int line);

    void Stop();

    void Begin();

    void rePlay();

    void pause();
private:
    std::map<int, std::string> lyrics;

    std::vector<std::pair<qint64, qint64>> pcmTimeMap;

    QIODevice* audioDevice = nullptr;

    QTimer *timer;

    QQueue<QByteArray> audioBuffer;

    QMutex mutex;

    QAudioOutput *audioOutput = nullptr ;

    qint64 totalAudioDurationInMS;

    bool isPaused; // 用于区分是否是暂停恢复的情况

    bool sliderMove;

    std::mutex mtx;
    std::map<QByteArray,qint64> mp;

   QString PATH;
};

#endif // WORKER_H
