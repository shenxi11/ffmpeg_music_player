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

    bool getAudioFileInfo(const QString& filePath, int& sampleRate, int& channelCount, int& sampleSize, QAudioFormat::SampleType& sampleType);


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

    void updatePlaybackTime(qint64 newPlaybackTime);

    void pause();
private:
    std::map<int, std::string> lyrics;

    std::unique_ptr<QFile> file;

    std::vector<std::pair<qint64, qint64>> pcmTimeMap;

    QIODevice* audioDevice = nullptr;

    QTimer *timer;

    QQueue<QByteArray> audioBuffer;

    QMutex mutex;

    QWaitCondition *cond;

    QAudioOutput *audioOutput = nullptr ;

    qint64 totalAudioDurationInMS;

    bool isPaused; // 用于区分是否是暂停恢复的情况

    bool sliderMove;

    std::mutex mtx;
    std::mutex mtx1;

    std::unique_ptr<char[]> buffer;

    std::map<QByteArray,qint64> mp;

   QString PATH;
};

#endif // WORKER_H
