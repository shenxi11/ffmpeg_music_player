#include "worker.h"

Worker::Worker():

    isPaused(false)
  ,sliderMove(false)
{


}


Worker::~Worker()
{
    //qDebug()<<"desruct Worker";
}

void Worker::receive_totalDuration(qint64 total)
{
    this->totalAudioDurationInMS=total;
}
void Worker::receive_lrc(std::map<int, std::string> lyrics)
{
    std::lock_guard<std::mutex>lock(mtx);
    this->lyrics=lyrics;
}
//void  Worker::begin_play(QString pcmFilePath){
//    //qDebug()<<"Worker"<<QThread::currentThreadId();
//    emit play(pcmFilePath);

//}
void Worker::stop_play()
{
    emit stopPlay();
}
void Worker::Set_Volume(int value)
{
    if(audioOutput)
        audioOutput->setVolume(value/100.0);
}
void Worker::set_SliderMove(bool flag)
{
    this->sliderMove=flag;

    emit stopPlay();
}
void Worker::Pause()
{

    if (!isPaused||this->sliderMove)
    {
        //qDebug() << "暂停";
        audioOutput->suspend();
        timer->stop();

        isPaused = true;
        //qDebug() << "Playback paused CC:";
        emit Stop();
    }
    else if (isPaused)
    {
        //qDebug() << "恢复";
        audioOutput->resume();
        isPaused = false;
        timer->start();

        emit Begin();
        //qDebug() << "Playback resumed.";
    }
}
//void Worker::play_pcm(QString pcmFilePath ){

//    qDebug()<<"Worker"<<QThread::currentThreadId();
//    if (file) {
//        file->close();
//        file.reset();
//    }
//    if(audioOutput){
//        isPaused=false;
//        audioOutput->stop();
//        audioOutput->reset();
//        delete audioOutput;
//        qDebug()<<"重播";
//        if(buffer){
//            delete [] buffer;
//        }
//        disconnect(this,&Worker::stopPlay,this,nullptr);
//    }
//    file = std::make_unique<QFile>(pcmFilePath);
//    if (!file->open(QIODevice::ReadOnly)) {
//        qDebug() << "Failed to open file for reading:" << file->errorString();
//        return;
//    }

//    QAudioFormat format;
//    format.setSampleRate(44100);
//    format.setChannelCount(2);
//    format.setSampleSize(16);
//    format.setCodec("audio/pcm");
//    format.setByteOrder(QAudioFormat::LittleEndian);
//    format.setSampleType(QAudioFormat::SignedInt);

//    audioOutput =new QAudioOutput(format, this);
//    audioOutput->setBufferSize(8192 * 8);

//    audioDevice = audioOutput->start();
//    if (!audioDevice) {
//        qDebug() << "Failed to start audio device";
//        file->close();
//        file.reset();
//        return;
//    }

//    audioOutput->setVolume(50/100.0);


//    buffer = new char[bufferSize];

//    timer = new QTimer(this);
//    timer1= new QTimer(this);
//    int currentLyricIndex = 0;  // 用于追踪当前歌词的位置
//    connect(timer, &QTimer::timeout, this, [=]() mutable {
//        if (audioOutput->bytesFree() < bufferSize) {
//            // 缓冲区尚未腾出足够的空间，稍后再试
//            return;
//        }

//        qint64 bytesRead = file->read(buffer, bufferSize);

//        if (bytesRead > 0) {
//            qint64 bytesWritten = audioDevice->write(buffer, bytesRead);
//            if (bytesWritten < 0) {
//                qDebug() << "Error writing audio data:" << audioDevice->errorString();
//                timer->stop();
//                timer1->stop();
//                delete[] buffer;
//                file->close();
//                file.reset();
//                return;
//            }

//            // 获取当前音频播放的微秒数，并将其转换为毫秒
//            qint64 currentTimeMS = audioOutput->processedUSecs() / 1000;

//            // 遍历歌词并同步显示
//            if (!lyrics.empty() && currentLyricIndex < (int)lyrics.size()) {
//                auto it = std::next(lyrics.begin(), currentLyricIndex);  // 获取当前歌词
//                if (currentTimeMS >= it->first) {
//                    if(it->second!="")
//                        emit send_lrc(QString::fromStdString(it->second));
//                    currentLyricIndex++;  // 更新到下一条歌词
//                }
//            }
//        } else {
//            // 文件读取完毕，停止定时器
//            timer->stop();
//            timer1->stop();
//            qDebug() << "Playback finished AA";
//            delete[] buffer;
//            file->close();
//            file.reset();
//            emit Stop();
//            emit rePlay();
//            disconnect(this,&Worker::stopPlay,this,nullptr);
//        }
//    });


//    timer->start(10);  // 每10毫秒检查一次
//    timer1->start(1000);
//    emit Begin();

//    connect(timer1,&QTimer::timeout,this,[=](){
//        emit durations(audioOutput->processedUSecs());
//    });


//    connect(this, &Worker::stopPlay, this, [=]() mutable {

//        //qDebug()<<pcmFilePath;
//        if (!isPaused) {
//            // 暂停播放
//            qDebug() << "暂停";
//            audioOutput->suspend();
//            timer->stop();
//            timer1->stop();
//            isPaused = true;
//            qDebug() << "Playback paused CC:";
//            emit Stop();
//        } else if (isPaused) {
//            // 恢复播放
//            qDebug() << "恢复";
//            audioOutput->resume();
//            isPaused = false;
//            timer->start();
//            timer1->start();
//            emit Begin();
//            qDebug() << "Playback resumed.";
//        }

//    });
//}
void Worker::reset_play()
{
//    std::lock_guard<std::mutex>lock(mtx);
    this->mp.clear();
    this->audioBuffer.clear();
    this->audioOutput->reset();
    audioDevice = this->audioOutput->start();

}

void Worker::onTimeOut()
{

//    std::lock_guard<std::mutex> lock(mtx);


    if (audioBuffer.isEmpty())
    {
        qDebug()<<"Empty";
        timer->stop();
        emit Stop();
        emit rePlay();
        return;  // 如果缓冲区为空，退出
    }

    QByteArray pcmData = audioBuffer.front();

    // 检查 audioDevice 的可用空间
    qint64 bytesFree = audioOutput->bytesFree();
    //qDebug() << "Audio buffer size:" << pcmData.size() << "audioBytesFree:" << bytesFree<<" number:"<<audioBuffer.size();

    if ( bytesFree < 2 * pcmData.size())
    {
        //qDebug() << "Not enough space in audio output buffer. PCM data size:" << pcmData.size() << "bytesFree:" << bytesFree;
        //audioBuffer.enqueue(pcmData);  // 将数据重新放回缓冲区
        return;  // 等待下一次调用
    }

    qint64 bytesWritten = audioDevice->write(pcmData);  // 写入音频设备

    qint64 currentTimeMS = mp[pcmData];



    mp.erase(pcmData);
    audioBuffer.pop_front();
    if (bytesWritten < 0)
    {
        qDebug() << "Error writing audio data:" << audioDevice->errorString();
        timer->stop();  // 停止定时器
        return;
    }



    emit durations(currentTimeMS);  // 通知其他组件同步显示时间

    int index=0;

    for (auto it = lyrics.begin(); it != lyrics.end(); ++it,++index)
    {

        // 访问下一个元素
        auto nextIt = std::next(it);

        if (nextIt != lyrics.end())
        {

            if ((it->first <= static_cast<int>(currentTimeMS)
                 && nextIt->first > static_cast<int>(currentTimeMS))
                    ||(it == lyrics.begin()&&it->first >= static_cast<int>(currentTimeMS)))
            {
                emit send_lrc(index+5);

                break;
            }

        }
        else
        {
            emit send_lrc(index+5);

            break;
        }
    }


    //    if (!lyrics.empty() && currentLyricIndex < (int)lyrics.size()) {
    //        auto it = std::next(lyrics.begin(), currentLyricIndex);
    //        if (currentTimeMS >= it->first) {
    //            if (!it->second.empty())
    //                emit send_lrc(QString::fromStdString(it->second));
    //                qDebug()<<QString::fromStdString(it->second);
    //            currentLyricIndex++;
    //        }
    //    }


}


void Worker::receive_data(const QByteArray &data,qint64 timeMap)
{
//    std::lock_guard<std::mutex>lock(mtx);
    this->audioBuffer.enqueue(data);
    this->mp[data]=timeMap;

}
void Worker::setPATH(QString Path)
{
    this->PATH = Path;

}
void Worker::play_pcm()
{



    if (audioOutput)
    {
        timer->stop();

        audioOutput->stop();
        audioOutput->reset();

        this->audioBuffer.clear();

        disconnect(this, &Worker::stopPlay, this, nullptr);

    }

    else
    {
        timer = new QTimer(this);

        connect(timer, &QTimer::timeout, this,&Worker::onTimeOut);

        QAudioFormat format;

        format.setSampleRate(RATE);
        format.setChannelCount(CHANNELS);
        format.setSampleSize(SAMPLE_SIZE);
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::SignedInt);

        audioOutput = new QAudioOutput(format, this);

        audioOutput->setBufferSize(AUDIO_BUFFER_SIZE);

    }

    audioDevice = audioOutput->start();

    audioOutput->setVolume(75 / 100.0);


    timer->setInterval(5);
    timer->start();


    connect(this, &Worker::stopPlay, this, &Worker::Pause);

    emit Begin();

}

void Worker::reset_status()
{
    timer->stop();
    this->mp.clear();
    this->audioBuffer.clear();
    audioOutput->stop();
    audioOutput->reset();

    emit Stop();
}
