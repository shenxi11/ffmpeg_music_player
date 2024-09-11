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
public slots:
    void begin_play(QString pcmFilePath);

    void stop_play();

    void receive_lrc(std::map<int, std::string> lyrics);

    void Set_Volume(int value);

    void stop();
signals:
    void durations(qint64 value);

    void play(QString pcmFilePath);

    void stopPlay();

    void send_lrc(QString str);

    void Stop();

    void Begin();

    void rePlay();
private:
    std::map<int, std::string> lyrics;

    std::unique_ptr<QFile>file;

    QIODevice* audioDevice ;

    QTimer* timer;
    QTimer* timer1;

    QAudioOutput*audioOutput;

    bool isPaused; // 用于区分是否是暂停恢复的情况

    std::mutex mtx;

    char* buffer;

    const int bufferSize = 44100 * 2 * 2 * 0.1;
};

#endif // WORKER_H
