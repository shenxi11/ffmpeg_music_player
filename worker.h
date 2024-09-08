#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include"headers.h"

class Worker :public QThread
{
    Q_OBJECT
public:
    explicit Worker();

    ~Worker();

    void run() override;

    void play_pcm(QString pcmFilePath);

    bool getAudioFileInfo(const QString& filePath, int& sampleRate, int& channelCount, int& sampleSize, QAudioFormat::SampleType& sampleType);
public slots:
    void begin_play(QString pcmFilePath);

    void stop_play();

    void receive_lrc(std::map<int, std::string> lyrics);
signals:
    void play(QString pcmFilePath);

    void stopPlay();

    void send_lrc(QString lrc_str);

    void stopped();
private:
    std::map<int, std::string> lyrics;

    std::unique_ptr<QFile>file;

    QIODevice* audioDevice ;

    QTimer* timer;

    std::unique_ptr<QAudioOutput>audioOutput;

    bool isPaused; // 用于区分是否是暂停恢复的情况

    int count=0;

};

#endif // WORKER_H
