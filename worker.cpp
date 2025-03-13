#include "worker.h"

Worker::Worker():

    isPaused(false)
  ,sliderMove(false)
{

    thread_ = std::thread(&Worker::onTimeOut, this);
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
//        timer->stop();
        stopPlayback();
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
        qDebug() << "Playback resumed.";
    }
}

void Worker::reset_play()
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        audioBuffer.clear();
        mp.clear();
    }
    cv.notify_one();
    if(audioOutput)
    {
        this->audioOutput->reset();
        audioDevice = this->audioOutput->start();
    }
}

void Worker::onTimeOut()
{

    //std::lock_guard<std::mutex> lock(mtx);

    while(true)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock,[this]{return !audioBuffer.empty() || !m_stopFlag || m_breakFlag;});
        qDebug()<<__FUNCTION__<<audioBuffer.size();
        if (audioBuffer.isEmpty())
        {
            qDebug()<<"Empty";
//            timer->stop();
//            emit Stop();
//            emit rePlay();
//            return;  // 如果缓冲区为空，退出
            continue;
        }
        if(m_breakFlag){
            break;
        }

        QByteArray pcmData = audioBuffer.front();

        // 检查 audioDevice 的可用空间
        qint64 bytesFree = audioOutput->bytesFree();
        qDebug() << "Audio buffer size:" << pcmData.size() << "audioBytesFree:" << bytesFree<<" number:"<<audioBuffer.size();

        if ( bytesFree < 2 * pcmData.size())
        {
            qDebug() << "Not enough space in audio output buffer. PCM data size:" << pcmData.size() << "bytesFree:" << bytesFree;
            audioBuffer.enqueue(pcmData);  // 将数据重新放回缓冲区
            continue;  // 等待下一次调用
        }

        qint64 bytesWritten = audioDevice->write(pcmData);  // 写入音频设备

        qint64 currentTimeMS = mp[pcmData];

        mp.erase(pcmData);
        audioBuffer.pop_front();
        if (bytesWritten < 0)
        {
            qDebug() << "Error writing audio data:" << audioDevice->errorString();
            //timer->stop();  // 停止定时器
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
        QThread::msleep(5);
    }
}


void Worker::receive_data(const QByteArray &data,qint64 timeMap)
{
    {
        std::lock_guard<std::mutex>lock(mtx);
        this->audioBuffer.enqueue(data);
        this->mp[data]=timeMap;
    }
    cv.notify_one();
}
void Worker::setPATH(QString Path)
{
    this->PATH = Path;

}
void Worker::play_pcm()
{

    if (audioOutput)
    {
//        timer->stop();
        stopPlayback();
        qInfo()<<__FUNCTION__;
        audioOutput->stop();
        audioOutput->reset();

        {
            std::lock_guard<std::mutex> lock(mtx);
            audioBuffer.clear();
            mp.clear();
        }
        cv.notify_one();
        disconnect(this, &Worker::stopPlay, this, nullptr);

    }

    else
    {
        timer = new QTimer(this);

        //connect(timer, &QTimer::timeout, this,&Worker::onTimeOut);

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

    QThread::msleep(100);

    m_stopFlag = false;
    cv.notify_all();
//    timer->setInterval(5);
//    timer->start();


    connect(this, &Worker::stopPlay, this, &Worker::Pause);

    emit Begin();

}

void Worker::reset_status()
{
    if( audioOutput)
    {
        qInfo()<<__FUNCTION__;
        //timer->stop();
        stopPlayback();
        {
            std::lock_guard<std::mutex> lock(mtx);
            audioBuffer.clear();
            mp.clear();
        }
        cv.notify_one();
        audioOutput->stop();
        audioOutput->reset();
        emit Stop();
    }
}
void Worker::onDataReceived(QByteArray data)
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        audioBuffer.enqueue(data);
    }
    cv.notify_one();
}

void Worker::stopPlayback()
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        m_stopFlag= !m_stopFlag;
//        if (thread_.joinable()) {
//                thread_.join();
//            }
    }
    cv.notify_all();
}
void Worker::stopPlayBack()
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        m_breakFlag= true;
        if (thread_.joinable()) {
                thread_.join();
            }
    }
    cv.notify_all();
}
