#include "worker.h"

Worker::Worker():

    isPaused(false)
  ,sliderMove(false)
  ,audioOutput(nullptr)
{
    timer = new QTimer(this);
    timer1 = new QTimer(this);


    connect(timer, &QTimer::timeout, this,&Worker::onTimeOut);

    //    connect(timer1, &QTimer::timeout, this, [=]() {
    //        emit durations(audioOutput->processedUSecs());
    //    });//更新进度条


}
Worker::~Worker() {


    timer->stop();
    timer1->stop();

    if(audioOutput){
        qDebug()<<"audioOutput not destruct";
    }
    if(timer||timer1){
        qDebug()<<"timer not destruct";
    }


    qDebug()<<"Destruct Worker";
}

void Worker::stop(){

}
void Worker::receive_totalDuration(qint64 total){
    this->totalAudioDurationInMS=total;
}
qint64 Worker::calculatePlaybackTime(qint64 pcmPosition) {
    qint64 totalPcmSize = file->size();  // PCM 文件的总字节数
    qint64 totalDurationMS = totalAudioDurationInMS;  // 总播放时间（毫秒）
    //qDebug()<<"calculate"<<(pcmPosition * totalDurationMS) / totalPcmSize;
    return (pcmPosition * totalDurationMS) / totalPcmSize;
}
// 获取 PCM 文件中某个字节位置对应的时间戳
qint64 Worker::getPlaybackTimeFromPcmPosition(qint64 pcmPosition) {
    for (size_t i = 0; i < pcmTimeMap.size() - 1; i++) {
        if (pcmPosition >= pcmTimeMap[i].first && pcmPosition < pcmTimeMap[i + 1].first) {
            return pcmTimeMap[i].second;  // 返回毫秒单位的播放时间
        }
    }
    return -1;  // 未找到对应时间
}
void Worker::seekToPosition(int newPosition) {

    //disconnect(timer,&QTimer::timeout,this,nullptr);
    // 计算 PCM 文件中的字节位置
    qint64 bytesPosition = (newPosition / 10000.0) * file->size();

    // 确保 bytesPosition 在文件大小范围内
    if (bytesPosition >= 0 && bytesPosition < file->size()) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            // 移动文件指针到指定位置
            file->seek(bytesPosition);
        }
        //        // 清空当前音频设备的缓冲区
        //        audioDevice->reset();

        //        // 读取数据到缓冲区
        //        qint64 bytesRead = file->read(buffer.get(), BUFFER_SIZE);
        //        if (bytesRead > 0) {
        //            qint64 bytesWritten = audioDevice->write(buffer.get(), bytesRead);
        //            if (bytesWritten < 0) {
        //                qDebug() << "Error writing audio data:" << audioDevice->errorString();
        //            }
        //        }

        //        // 计算并同步新位置的播放时间
        //        qint64 newPlaybackTime = calculatePlaybackTime(bytesPosition);  // 计算跳转后的播放时间
        //        emit durations(newPlaybackTime);  // 通知其他组件同步显示时间

        //        // 恢复播放
        //        audioOutput->resume();
    } else {
        qDebug() << "Invalid position for seek";
    }
    //connect(timer,&QTimer::timeout,this,&Worker::onTimeOut);
}
void Worker::receive_lrc(std::map<int, std::string> lyrics){
    std::lock_guard<std::mutex>lock(mtx);
    this->lyrics=lyrics;
}
void  Worker::begin_play(QString pcmFilePath){
    //qDebug()<<"Worker"<<QThread::currentThreadId();
    emit play(pcmFilePath);

}
void Worker::stop_play(){
    emit stopPlay();
}
void Worker::Set_Volume(int value){
    if(audioOutput)
        audioOutput->setVolume(value/100.0);
}
void Worker::set_SliderMove(bool flag){
    this->sliderMove=flag;
    Pause();
}
void Worker::Pause(){
    if (!isPaused||this->sliderMove) {
        qDebug() << "暂停";
        audioOutput->suspend();
        timer->stop();
        timer1->stop();
        isPaused = true;
        qDebug() << "Playback paused CC:";
        emit Stop();
    } else if (isPaused) {
        qDebug() << "恢复";
        audioOutput->resume();
        isPaused = false;
        timer->start();
        timer1->start();
        emit Begin();
        qDebug() << "Playback resumed.";
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
void Worker::receive_pcmMap(std::vector<std::pair<qint64, qint64>> pcmTimeMap){
    this->pcmTimeMap=pcmTimeMap;
}

void Worker::onTimeOut(){
    std::lock_guard<std::mutex>lock(mtx);
    if (audioOutput->bytesFree() < BUFFER_SIZE) {
        return;
    }

    // 获取当前文件位置
    qint64 bytesPosition = file->pos();

    qint64 bytesRead = file->read(buffer.get(), BUFFER_SIZE);

    if (bytesRead > 0) {
        qint64 bytesWritten = audioDevice->write(buffer.get(), bytesRead);
        if (bytesWritten < 0) {
            qDebug() << "Error writing audio data:" << audioDevice->errorString();
            timer->stop();
            timer1->stop();
            file->close();
            file.reset();
            return;
        }

        // qint64 currentTimeMS = audioOutput->processedUSecs() / 1000;

        // 计算并同步新位置的播放时间
        qint64 newPlaybackTime = calculatePlaybackTime(bytesPosition);
        emit durations(newPlaybackTime);  // 通知其他组件同步显示时间

        // emit durations(currentTimeMS);



        if (!lyrics.empty() && currentLyricIndex < (int)lyrics.size()) {
            auto it = std::next(lyrics.begin(), currentLyricIndex);
            if (newPlaybackTime >= it->first) {
                if (it->second != "")
                    emit send_lrc(QString::fromStdString(it->second));
                currentLyricIndex++;
            }
        }

    } else {
        timer->stop();
        timer1->stop();
        qDebug() << "Playback finished AA";
        file->close();
        file.reset();
        emit Stop();
        emit rePlay();
        disconnect(this, &Worker::stopPlay, this, nullptr);
    }
}
void Worker::play_pcm(QString pcmFilePath) {

    qDebug() << "Worker" << QThread::currentThreadId();

    // 1. 停止并清理当前播放
    if (file) {
        file->close();
        file.reset();
    }

    if (audioOutput) {
        audioOutput->stop();
        audioOutput->reset();
        //        delete audioOutput;
        //        audioOutput = nullptr;

        timer->stop();
        timer1->stop();

        disconnect(this, &Worker::stopPlay, this, nullptr);

        if(buffer){
            qDebug()<<"缓冲区未处理"<<sizeof (buffer)<<"bytes";
        }
    }

    //    if (timer) {
    //        timer->stop();
    //        delete timer;
    //        timer = nullptr;
    //    }

    //    if (timer1) {
    //        timer1->stop();
    //        delete timer1;
    //        timer1 = nullptr;
    //    }

    //    if (buffer) {
    //        delete[] buffer;
    //        buffer = nullptr;
    //    }



    // 2. 重新初始化并开始播放
    file = std::make_unique<QFile>(pcmFilePath);
    if (!file->open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file for reading:" << file->errorString();
        return;
    }

    QAudioFormat format;
    format.setSampleRate(RATE);
    format.setChannelCount(CHANNELS);
    format.setSampleSize(SAMPLE_SIZE);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    if(!audioOutput){
        audioOutput = new QAudioOutput(format, this);
    }
    audioOutput->setBufferSize(AUDIO_BUFFER_SIZE);

    audioDevice = audioOutput->start();
    if (!audioDevice) {
        qDebug() << "Failed to start audio device";
        file->close();
        file.reset();
        return;
    }

    audioOutput->setVolume(50 / 100.0);

    buffer = std::make_unique<char[]>(BUFFER_SIZE);


    this->currentLyricIndex=0;


    timer->start(10);
    timer1->start(1000);
    emit Begin();

    connect(this, &Worker::stopPlay, this, &Worker::Pause);
}

