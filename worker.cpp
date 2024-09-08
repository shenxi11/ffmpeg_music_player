#include "worker.h"

Worker::Worker():

    isPaused(false)
{

}
Worker::~Worker() {

}
void Worker::run() {
    connect(this,&Worker::play,this,&Worker::play_pcm);
    exec();
}
void Worker::receive_lrc(std::map<int, std::string> lyrics){
    this->lyrics=lyrics;
}
void  Worker::begin_play(QString pcmFilePath){
    emit play(pcmFilePath);

}
void Worker::stop_play(){
    emit stopPlay();
}
void Worker::Set_Volume(int value){
    if(audioOutput)
        audioOutput->setVolume(value/100.0);
}
void Worker::play_pcm(QString pcmFilePath ){
    if (file) {
        file->close();
        file.reset();
    }
    if(audioOutput){
        isPaused=false;
        audioOutput->stop();
        audioDevice=nullptr;
        audioOutput.reset();
        disconnect(this,&Worker::stopPlay,this,nullptr);
    }
    file = std::make_unique<QFile>(pcmFilePath);
    if (!file->open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file for reading:" << file->errorString();
        return;
    }

    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    audioOutput =std::make_unique<QAudioOutput>(format, this);
    audioOutput->setBufferSize(8192 * 8);

    audioDevice = audioOutput->start();
    if (!audioDevice) {
        qDebug() << "Failed to start audio device";
        file->close();
        file.reset();
        return;
    }

    audioOutput->setVolume(50/100.0);

    const int bufferSize = 44100 * 2 * 2 * 0.1;
    char* buffer = new char[bufferSize];

    timer = new QTimer(this);
    int currentLyricIndex = 0;  // 用于追踪当前歌词的位置
    connect(timer, &QTimer::timeout, this, [=]() mutable {
        if (audioOutput->bytesFree() < bufferSize) {
            // 缓冲区尚未腾出足够的空间，稍后再试
            return;
        }

        qint64 bytesRead = file->read(buffer, bufferSize);

        if (bytesRead > 0) {
            qint64 bytesWritten = audioDevice->write(buffer, bytesRead);
            if (bytesWritten < 0) {
                qDebug() << "Error writing audio data:" << audioDevice->errorString();
                timer->stop();
                delete[] buffer;
                file->close();
                file.reset();
                return;
            }

            // 获取当前音频播放的微秒数，并将其转换为毫秒
            qint64 currentTimeMS = audioOutput->processedUSecs() / 1000;
            emit durations(audioOutput->processedUSecs());
            // 遍历歌词并同步显示
            if (!lyrics.empty() && currentLyricIndex < (int)lyrics.size()) {
                auto it = std::next(lyrics.begin(), currentLyricIndex);  // 获取当前歌词
                if (currentTimeMS >= it->first) {
                    if(it->second!="")
                        emit send_lrc(QString::fromStdString(it->second));
                    currentLyricIndex++;  // 更新到下一条歌词
                }
            }
        } else {
            // 文件读取完毕，停止定时器
            timer->stop();
            qDebug() << "Playback finished AA";
            delete[] buffer;
            file->close();
            file.reset();
        }
    });

    //    connect(timer, &QTimer::timeout, this, [=]() mutable {
    //        if (audioOutput->bytesFree() < bufferSize) {
    //            // 缓冲区尚未腾出足够的空间，稍后再试
    //            return;
    //        }

    //        qint64 bytesRead = file.read(buffer, bufferSize);
    //        if (bytesRead > 0) {
    //            qint64 bytesWritten = audioDevice->write(buffer, bytesRead);
    //            if (bytesWritten < 0) {
    //                qDebug() << "Error writing audio data:" << audioDevice->errorString();
    //                timer->stop();
    //                delete[] buffer;
    //                file.close();
    //                return;
    //            }
    ////            qDebug() << "Reading bytes:" << bytesRead;
    ////            qDebug() << "Writing bytes:" << bytesWritten;
    //        } else {
    //            // 文件读取完毕，停止定时器
    //            timer->stop();
    //            delete[] buffer;
    //            file.close();
    //            qDebug() << "Playback finished";
    //        }
    //    });

    timer->start(10);  // 每10毫秒检查一次
    emit Begin();

    //    // 等待播放结束
    //    connect(audioOutput, &QAudioOutput::stateChanged, this, [=](QAudio::State state) mutable {
    //        if (state == QAudio::IdleState && audioOutput->processedUSecs()/1000) {
    //            // 音频播放结束，清理资源
    //            timer->stop();
    //            delete[] buffer;
    //            file->close();
    //            audioOutput->deleteLater();
    //            qDebug() << "Playback finished BB";
    //            emit stopped();
    //        }
    //        else if (state == QAudio::SuspendedState) {
    //            qDebug() << "Playback paused.";
    //        } else if (state == QAudio::ActiveState) {
    //            qDebug() << "Playback resumed.";
    //        }
    //    });

    connect(this, &Worker::stopPlay, this, [=]() mutable {
        count++;
        qDebug()<<pcmFilePath;
        if (!isPaused) {
            // 暂停播放
            qDebug() << "暂停";
            audioOutput->suspend();
            timer->stop();
            isPaused = true;
            qDebug() << "Playback paused CC:";
            emit Stop();
        } else if (isPaused) {
            // 恢复播放
            qDebug() << "恢复";
            audioOutput->resume();
            isPaused = false;
            timer->start();
            emit Begin();
            qDebug() << "Playback resumed.";
        }
    });
}

