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


    void play_pcm(QString pcmFilePath);

    bool getAudioFileInfo(const QString& filePath, int& sampleRate, int& channelCount, int& sampleSize, QAudioFormat::SampleType& sampleType);

    qint64 getPlaybackTimeFromPcmPosition(qint64 pcmPosition);

    qint64 calculatePlaybackTime(qint64 pcmPosition);
public slots:
    void begin_play(QString pcmFilePath);

    void stop_play();

    void receive_lrc(std::map<int, std::string> lyrics);

    void Set_Volume(int value);

    void stop();

    void set_SliderMove(bool flag);

    void seekToPosition(int newPosition);

    void receive_pcmMap(std::vector<std::pair<qint64, qint64>> pcmTimeMap);

    void receive_totalDuration(qint64 total);

private slots:
    void onTimeOut();

    void Pause();

signals:
    void durations(qint64 value);

    void play(QString pcmFilePath);

    void stopPlay();

    void send_lrc(QString str);

    void Stop();

    void Begin();

    void rePlay();

    void updatePlaybackTime(qint64 newPlaybackTime);
private:
    std::map<int, std::string> lyrics;

    std::unique_ptr<QFile>file;

    std::vector<std::pair<qint64, qint64>> pcmTimeMap;

    QIODevice* audioDevice ;

    QTimer* timer;
    QTimer* timer1;

    QAudioOutput*audioOutput;

    qint64 totalAudioDurationInMS;

    bool isPaused; // 用于区分是否是暂停恢复的情况

    bool sliderMove;

    std::mutex mtx;

    std::unique_ptr<char[]>buffer;


    int currentLyricIndex = 0;
};

#endif // WORKER_H
