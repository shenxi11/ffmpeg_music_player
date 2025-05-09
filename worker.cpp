#include "worker.h"

Worker::Worker():

    isPaused(false)
  ,sliderMove(false)
{
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
        audioOutput->suspend();
        stopPlayback();
        isPaused = true;
        emit Stop();
    }
    else if (isPaused)
    {
        audioOutput->resume();
        isPaused = false;
        stopPlayback();
        emit Begin();
        qDebug() << "Playback resumed.";
    }
}

void Worker::reset_play()
{

    if(audioOutput)
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            audioBuffer.clear();
            mp.clear();
        }
        cv.notify_one();
        this->audioOutput->reset();
        audioDevice = this->audioOutput->start();
    }

}

void Worker::onTimeOut()
{

    while(true)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock,[this]{return (!audioBuffer.empty() && !m_stopFlag) || m_breakFlag;});
        if(audioBuffer.empty()){
            continue;
        }
        if(m_breakFlag){
            return;
        }

        QByteArray pcmData = audioBuffer.front();

        qint64 bytesFree = audioOutput->bytesFree();
        if ( bytesFree <  2 * pcmData.size())
        {
            continue;
        }

        qint64 bytesWritten = audioDevice->write(pcmData);
        if (bytesWritten < 0)
        {
            qDebug() << "Error writing audio data:" << audioDevice->errorString();
            continue;
        }
        qint64 currentTimeMS = mp[pcmData];

        mp.erase(pcmData);
        audioBuffer.pop_front();
        pcmData = QByteArray();

        emit durations(currentTimeMS);

        int index=0;

        for (auto it = lyrics.begin(); it != lyrics.end(); ++it,++index)
        {
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
        stopPlayback();
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

    stopPlayback();

    connect(this, &Worker::stopPlay, this, &Worker::Pause);

    emit Begin();

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
