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

    void stopPlay();

    void receiveLrc(std::map<int, std::string> lyrics);

    void setVolume(int value);

    void setSliderMove(bool flag);

    void receiveTotalDuration(qint64 total);

    void playPcm();

    void receiveData(const QByteArray &data, qint64 timeMap);

    void resetPlay();

    void setPATH(QString Path);

    void resetStatus();

    void onDataReceived(QByteArray data);

    void stopPlayback();
    void stopPlayBack();

    void pausePlayback();
    void onSetMove();
private slots:
    void onTimeOut();


signals:
    void durations(qint64 value);
    void signalStopPlay();
    void send_lrc(int line);
    void Stop();
    void Begin();
    void rePlay();
    void pause();
    void begin_to_decode();
    void signalReconnect();
private:
    std::map<int, std::string> lyrics;

    QIODevice* audioDevice = nullptr;

    QTimer *timer;

    QQueue<PCM> audioBuffer;

    QMutex mutex;

    QAudioOutput *audioOutput = nullptr ;

    qint64 totalAudioDurationInMS;

    bool isPaused; // 用于区分是否是暂停恢复的情况

    bool sliderMove;

    std::mutex mtx;

   QString PATH;

   std::condition_variable cv;
   std::atomic<bool> m_stopFlag = true;
   std::atomic<bool> m_breakFlag = false;
   std::atomic<bool> m_moveFlag = false;

   std::thread thread_;
   bool flag_ = true;
   bool first_flag = true;
};

#endif // WORKER_H
