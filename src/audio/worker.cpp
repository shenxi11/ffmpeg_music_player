#include "worker.h"
#include <QTime>

Worker::Worker():

    isPaused(false)
  ,sliderMove(false)
{
    //qDebug() << __FUNCTION__ << "worker线程" << QThread::currentThreadId();
    QAudioFormat format;

    format.setSampleRate(RATE);
    format.setChannelCount(CHANNELS);
    format.setSampleSize(SAMPLE_SIZE);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    audioOutput = new QAudioOutput(format, this);
    audioOutput->setBufferSize(AUDIO_BUFFER_SIZE  * 1024);
    audioDevice = audioOutput->start();
    audioOutput->setVolume(75 / 100.0);

    thread_ = std::thread(&Worker::onTimeOut, this);

}

Worker::~Worker()
{
    {
        std::lock_guard<std::mutex>lock(mtx);
        m_breakFlag = true;
    }
    cv.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
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

void Worker::stop_play()
{
    //qDebug() << __FUNCTION__ << "播放按钮点击";
    //emit stopPlay();
    Pause();
}
void Worker::slot_setMove() {
    //qDebug() << __FUNCTION__ << "停止";
    {
        std::lock_guard<std::mutex> lock(mtx);
        m_moveFlag = !m_moveFlag;
    }cv.notify_one();
}
void Worker::Set_Volume(int value)
{
    if(audioOutput)
        audioOutput->setVolume(value/100.0);
}
void Worker::set_SliderMove(bool flag)
{
    this->sliderMove=flag;
    qDebug() << __FUNCTION__ << "set slider move" << flag;
    emit stopPlay();
}
void Worker::Pause()
{
    qDebug() << "[TIMING] Worker::Pause START, isPaused=" << isPaused << QTime::currentTime().toString("hh:mm:ss.zzz");
    if (!isPaused||this->sliderMove)
    {
        isPaused = true;
        stopPlayback();
        emit Stop();
    }
    else if (isPaused)
    {
        isPaused = false;
        stopPlayback();
        emit Begin();
    }
    qDebug() << "[TIMING] Worker::Pause END" << QTime::currentTime().toString("hh:mm:ss.zzz");
}

void Worker::reset_play()
{
    qDebug() << __FUNCTION__ << "重置播放";
    {
        std::lock_guard<std::mutex> lock(mtx);
        audioBuffer.clear();  
    }
    cv.notify_one(); 
    emit signal_reconnect();
    qDebug() << __FUNCTION__ << "重置播放完成";
}

void Worker::onTimeOut()
{
    qDebug() << "[TIMING] Worker::onTimeOut thread started" << QTime::currentTime().toString("hh:mm:ss.zzz");
    bool firstAudioWrite = true;

    while(true)
    {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] {return (!audioBuffer.empty() && !m_stopFlag) || m_breakFlag; });
            if (audioBuffer.empty()) {
                continue;
            }
            if (m_breakFlag) {
                return;
            }
            if (flag_) {
                qDebug() << __FUNCTION__ << "后台线程" << QThread::currentThreadId();
                flag_ = false;
            }
            PCM pcmData = audioBuffer.front();

            qint64 bytesFree = audioOutput->bytesFree();
            if (bytesFree < 2 * pcmData.data_.size())
            {
                continue;
            }
            
            if (firstAudioWrite) {
                qDebug() << "[TIMING] Worker::onTimeOut 首次写入音频数据" << QTime::currentTime().toString("hh:mm:ss.zzz");
                firstAudioWrite = false;
            }

            audioBuffer.pop_front();

            qint64 bytesWritten = audioDevice->write(pcmData.data_);
            if (bytesWritten < 0)
            {
                qDebug() << "Error writing audio data:" << audioDevice->errorString();
                continue;
            }
            qint64 currentTimeMS = pcmData.timeMp;

            pcmData.data_ = QByteArray();

            emit durations(currentTimeMS);

            int index = 0;

            for (auto it = lyrics.begin(); it != lyrics.end(); ++it, ++index)
            {
                auto nextIt = std::next(it);

                if (nextIt != lyrics.end())
                {

                    if ((it->first <= static_cast<int>(currentTimeMS)
                        && nextIt->first > static_cast<int>(currentTimeMS))
                        || (it == lyrics.begin() && it->first >= static_cast<int>(currentTimeMS)))
                    {
                        emit send_lrc(index + 5);  // 加上前面5行空行的偏移

                        break;
                    }

                }
                else
                {
                    emit send_lrc(index + 5);  // 加上前面5行空行的偏移

                    break;
                }
            }
        }
        QThread::msleep(10);
    }
}


void Worker::receive_data(const QByteArray &data,qint64 timeMap)
{
    {
        std::lock_guard<std::mutex>lock(mtx);
        PCM pcm;
        pcm.data_ = data;
        pcm.timeMp = timeMap;
        this->audioBuffer.enqueue(pcm);
    }
    cv.notify_one();
}
void Worker::setPATH(QString Path)
{
    this->PATH = Path;

}
void Worker::play_pcm()
{
    qDebug()<<__FUNCTION__<<"准备开始播放1"<<QTime::currentTime().toString("hh:mm:ss.zzz");
    if (!first_flag)
    {
        stopPlayback();
        {
            std::lock_guard<std::mutex> lock(mtx);
            audioBuffer.clear();
        }
        cv.notify_one();
        disconnect(this, &Worker::stopPlay, this, nullptr);

    }
    else
    {
        first_flag = false;
    }
    stopPlayback();

    connect(this, &Worker::stopPlay, this, &Worker::Pause);

    emit Begin();
    qDebug()<<__FUNCTION__<<"开始播放"<<QTime::currentTime().toString("hh:mm:ss.zzz");

}

void Worker::reset_status()
{
    if( audioOutput)
    {
        qInfo()<<__FUNCTION__;
        stopPlayback();
        {
            std::lock_guard<std::mutex> lock(mtx);
            audioBuffer.clear();
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
       // audioBuffer.enqueue(data);
    }
    cv.notify_one();
}

void Worker::stopPlayback()
{

    {
        std::lock_guard<std::mutex> lock(mtx);
        m_stopFlag= !m_stopFlag;
    }
    cv.notify_one();

}
void Worker::stopPlayBack()
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        m_breakFlag= true;
        /*if (thread_.joinable()) {
            thread_.join();
        }*/
    }
    cv.notify_all();
}
