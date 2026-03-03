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

    void onDataReceived(QByteArray data);

    void stopPlayback();
    void stopPlayBack();

    void Pause();
    void slot_setMove();
private slots:
    void onTimeOut();


signals:
    void durations(qint64 value);
    void stopPlay();
    void send_lrc(int line);
    void Stop();
    void Begin();
    void rePlay();
    void pause();
    void begin_to_decode();
    void signal_reconnect();
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
