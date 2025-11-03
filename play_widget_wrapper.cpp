#include "play_widget_wrapper.h"
#include <QTime>
#include <QUrl>

PlayWidgetWrapper::PlayWidgetWrapper(PlayWidgetQml *playWidget, QObject *parent)
    : QObject(parent), playWidget(playWidget)
{
    qDebug() << __FUNCTION__ << QThread::currentThread()->currentThreadId;
    
    desk = new DeskLrcWidget();
    desk->raise();
    desk->hide();

    // 创建线程（不要传parent，让线程独立管理生命周期）
    a = new QThread();
    b = new QThread();
    c = new QThread();

    // 创建Worker对象并移动到线程
    work = std::make_shared<Worker>();
    work->moveToThread(c);

    lrc = std::make_shared<LrcAnalyze>();

    // 创建TakePcm对象并移动到线程
    take_pcm = std::make_shared<TakePcm>();
    take_pcm->moveToThread(a);

    // 启动线程
    a->start();
    b->start();
    c->start();
    
    // 验证线程移动是否成功
    qDebug() << "Main thread ID:" << QThread::currentThreadId();
    
    // 通过槽函数验证TakePcm是否在正确线程中运行
    QMetaObject::invokeMethod(take_pcm.get(), [this]() {
        qDebug() << "TakePcm actual thread ID:" << QThread::currentThreadId();
    }, Qt::QueuedConnection);

    // 连接信号
    connect(playWidget, &PlayWidgetQml::signal_big_clicked, this, &PlayWidgetWrapper::signal_big_clicked);
    connect(playWidget, &PlayWidgetQml::signal_current_lrc, this, &PlayWidgetWrapper::signal_desk_lrc);
    
    connect(take_pcm.get(), &TakePcm::signal_send_pic_path, playWidget, &PlayWidgetQml::setPicPath);
    connect(take_pcm.get(), &TakePcm::signal_send_pic_path, this, &PlayWidgetWrapper::slot_updateBackground);
    connect(take_pcm.get(), &TakePcm::begin_take_lrc, work.get(), &Worker::setPATH);
    connect(take_pcm.get(),&TakePcm::begin_to_play,work.get(),&Worker::play_pcm);
    connect(take_pcm.get(),&TakePcm::data,work.get(),&Worker::receive_data);
    connect(take_pcm.get(),&TakePcm::Position_Change,work.get(),&Worker::reset_play);
    connect(work.get(), &Worker::begin_to_decode, take_pcm.get(), &TakePcm::begin_to_decode);
    connect(take_pcm.get(),&TakePcm::send_totalDuration, work.get(),&Worker::receive_totalDuration);
    connect(take_pcm.get(),&TakePcm::durations,[ = ](int64_t value){
        this->duration = static_cast<qint64>(value);
        qDebug() << "TakePcm::durations - Total duration:" << value << "microseconds," << (value/1000000) << "seconds";
        playWidget->setMaxSeconds(value / 1000000);
    });
    
    // 连接进度跳转信号
    connect(this, &PlayWidgetWrapper::signal_process_Change, take_pcm.get(), &TakePcm::seekToPosition);
    connect(playWidget, &PlayWidgetQml::signal_Slider_Move, this, [=](int seconds){
        qDebug() << "Slider moved to:" << seconds << "seconds";
        qint64 milliseconds = static_cast<qint64>(seconds) * 1000;
        work->reset_play();
        emit signal_process_Change(milliseconds, true);
    });
    
    connect(playWidget, &PlayWidgetQml::signal_up_click, this, [=](){
        emit signal_big_clicked(true);
    });

    connect(this, &PlayWidgetWrapper::signal_filepath, [=](QString path) {
        emit take_pcm->signal_begin_make_pcm(path);
    });
    
    connect(this, &PlayWidgetWrapper::signal_worker_play, work.get(), &Worker::Pause);
    connect(this, &PlayWidgetWrapper::signal_remove_click, work.get(), &Worker::reset_status);

    connect(this, &PlayWidgetWrapper::signal_filepath, this, &PlayWidgetWrapper::_begin_take_lrc);
    connect(this,&PlayWidgetWrapper::signal_begin_take_lrc,lrc.get(),&LrcAnalyze::begin_take_lrc);
    connect(lrc.get(),&LrcAnalyze::send_lrc,work.get(),&Worker::receive_lrc);
    connect(lrc.get(),&LrcAnalyze::send_lrc,this, &PlayWidgetWrapper::slot_Lrc_send_lrc);
    
    connect(this, &PlayWidgetWrapper::signal_desk_lrc, desk, &DeskLrcWidget::slot_receive_lrc);
    connect(work.get(),&Worker::send_lrc,this,[=](int line){
        int currentLine = playWidget->getCurrentLine();
        if(line != currentLine)
        {
            playWidget->highlightLine(line);
        }
    });

    connect(work.get(),&Worker::Stop,this, &PlayWidgetWrapper::slot_work_stop);
    connect(work.get(),&Worker::Begin,this, &PlayWidgetWrapper::slot_work_play);

    connect(desk, &DeskLrcWidget::signal_play_Clicked, this, &PlayWidgetWrapper::slot_play_click);
    connect(this, &PlayWidgetWrapper::signal_playState, desk, [=](PlayWidgetQml::State state){
        desk->slot_playState_changed(static_cast<ProcessSliderQml::State>(state));
    });
    connect(desk, &DeskLrcWidget::signal_last_clicked, this, [=](){
        emit signal_Last(fileName, get_net_flag());
    });
    connect(desk, &DeskLrcWidget::signal_next_clicked, this, [=](){
        emit signal_Next(fileName,get_net_flag());
    });
    connect(desk, &DeskLrcWidget::signal_forward_clicked, this, [=](){
        int currentSeconds = playWidget->getState() != PlayWidgetQml::Stop ? 
            static_cast<int>(duration / 1000000) : 0;
        int maxSeconds = static_cast<int>(duration / 1000000);
        int newSeconds = std::min(maxSeconds, currentSeconds + 5);
        emit signal_process_Change(static_cast<qint64>(newSeconds) * 1000000, true);
    });
    connect(desk, &DeskLrcWidget::signal_backward_clicked, this, [=](){
        int currentSeconds = playWidget->getState() != PlayWidgetQml::Stop ? 
            static_cast<int>(duration / 1000000) : 0;
        int newSeconds = std::max(0, currentSeconds - 5);
        emit signal_process_Change(static_cast<qint64>(newSeconds) * 1000000, true);
    });
    
    // PlayWidgetQml 控件连接
    connect(playWidget, &PlayWidgetQml::signal_play_clicked, this, &PlayWidgetWrapper::slot_play_click);
    connect(this, &PlayWidgetWrapper::signal_playState, playWidget, &PlayWidgetQml::setState);
    connect(playWidget, &PlayWidgetQml::signal_volumeChanged, work.get(), &Worker::Set_Volume);
    connect(playWidget, &PlayWidgetQml::signal_nextSong, this,[=](){
        emit signal_Next(fileName,get_net_flag());
    });
    connect(playWidget, &PlayWidgetQml::signal_lastSong, this,[=](){
        emit signal_Last(fileName,get_net_flag());
    });
    connect(playWidget, &PlayWidgetQml::signal_stop, this, [=](){
        if(fileName.size())
            emit signal_play_button_click(false, fileName);
    });
    connect(playWidget, &PlayWidgetQml::signal_mlist_toggled, this, &PlayWidgetWrapper::signal_list_show);
    connect(playWidget, &PlayWidgetQml::signal_rePlay, this, [=](){
        rePlay(this->filePath);
     });
    connect(playWidget, &PlayWidgetQml::signal_desk_toggled, this, &PlayWidgetWrapper::slot_desk_toggled);
    connect(playWidget, &PlayWidgetQml::signal_loop_change, this, [=](bool isLoop){
        qDebug() << "Loop state changed:" << isLoop;
    });

    connect(take_pcm.get(), &TakePcm::signal_move, this, [=]() {
        qDebug() << "TakePcm::signal_move - 重新连接 durations";
          // 先保存指针
        PlayWidgetQml* pw = playWidget;
        durationsConnection = connect(work.get(), &Worker::durations, this, [=](qint64 milliseconds){
            int seconds = static_cast<int>(milliseconds / 1000);
            if (pw) {
                pw->setCurrentSeconds(seconds);
            }
        });
        work->slot_setMove();
     });
     
    connect(take_pcm.get(), &TakePcm::signal_worker_play, work.get(), &Worker::Pause);
    connect(this,&PlayWidgetWrapper::signal_set_SliderMove,work.get(),&Worker::set_SliderMove);
    
    // 更新进度条当前时间显示
    durationsConnection = connect(work.get(), &Worker::durations, this, [&](qint64 milliseconds){
        int seconds = static_cast<int>(milliseconds / 1000);
        if (playWidget) {
            playWidget->setCurrentSeconds(seconds);
        }
    });

    // 连接播放完成信号
    connect(playWidget, &PlayWidgetQml::signal_playFinished, playWidget, &PlayWidgetQml::slot_playFinished);

    connect(this, &PlayWidgetWrapper::signal_isUpChanged, playWidget, &PlayWidgetQml::slot_isUpChanged);
}

void PlayWidgetWrapper::slot_desk_toggled(bool checked){
    if(checked){
        desk->show();
    }else{
        desk->hide();
    }
}

void PlayWidgetWrapper::set_isUp(bool flag){
    isUp = flag;
    qDebug() << "PlayWidgetWrapper::set_isUp called with flag:" << flag;
    playWidget->setIsUp(flag);
    emit signal_isUpChanged(isUp);
}

void PlayWidgetWrapper::slot_work_stop(){
    qDebug() << __FUNCTION__ << "暂停";
    emit signal_playState(PlayWidgetQml::Pause);
    if(fileName.size())
    {
        playWidget->setRotating(false);
        emit signal_play_button_click(false, fileName);
        playWidget->setSongName(QFileInfo(fileName).baseName());
    }
}

void PlayWidgetWrapper::slot_work_play(){
    emit signal_playState(PlayWidgetQml::Play);
    playWidget->setRotating(true);
    emit signal_play_button_click(true, fileName);
    playWidget->setSongName(QFileInfo(fileName).baseName());
}

void PlayWidgetWrapper::slot_Lrc_send_lrc(std::map<int, std::string> lyrics){
    QStringList lyricsList;
    
    {
        std::lock_guard<std::mutex>lock(mtx);
        for (const auto& [time, text] : lyrics)
        {
            lyricsList.append(QString::fromStdString(text));
        }
        this->lyrics = lyrics;
    }
    
    playWidget->setLyrics(lyricsList);
}

void PlayWidgetWrapper::slot_play_click(){
    qDebug() << "[TIMING] slot_play_click START" << QTime::currentTime().toString("hh:mm:ss.zzz");
    
    PlayWidgetQml::State currentState = playWidget->getState();
    
    if(currentState == PlayWidgetQml::Pause || currentState == PlayWidgetQml::Stop) {
        emit signal_playState(PlayWidgetQml::Play);
    } else {
        emit signal_playState(PlayWidgetQml::Pause);
    }
    
    qDebug() << "[TIMING] slot_play_click 发出 signal_worker_play" << QTime::currentTime().toString("hh:mm:ss.zzz");
    emit signal_worker_play();
    qDebug() << "[TIMING] slot_play_click END" << QTime::currentTime().toString("hh:mm:ss.zzz");
}

void PlayWidgetWrapper::rePlay(QString path)
{
    if(path.size() == 0)
        return;
    emit signal_filepath(path);
}

void PlayWidgetWrapper::_begin_take_lrc(QString str)
{
    playWidget->clearLyrics();
    lrc->begin_take_lrc(str);
}

void PlayWidgetWrapper::openfile()
{
    QWidget dummyWidget;
    QString filePath_ = QFileDialog::getOpenFileName(
                &dummyWidget,
                "Open File",
                QDir::homePath(),
                "Audio Files (*.mp3 *.wav *.flac *.ogg);;All Files (*)"
                );

    if (!filePath_.isEmpty())
    {
        QFileInfo fileInfo(filePath_);
        QString filename = fileInfo.fileName();
        emit signal_add_song(filename,filePath_);
    }
}

void PlayWidgetWrapper::_play_click(QString songPath)
{
    if(songPath != this->filePath)
    {
        this->filePath = songPath;

        QFileInfo fileInfo(songPath);
        fileName = fileInfo.fileName();
        if(!checkAndWarnIfPathNotExists(songPath))
            return;
        emit signal_filepath(songPath);
    }
    else
    {
        slot_play_click();
    }
}

void PlayWidgetWrapper::_remove_click(QString songName)
{
    if(songName == this->fileName)
    {
        this->fileName.clear();
        this->filePath.clear();
        emit signal_remove_click();
    }
}

void PlayWidgetWrapper::setPianWidgetEnable(bool flag)
{
    // 不需要实现
}

bool PlayWidgetWrapper::checkAndWarnIfPathNotExists(const QString &path) {
    if (path.startsWith("http", Qt::CaseInsensitive)) {
        qDebug() << "检测到网络路径，跳过存在性检查:" << path;
        return true;
    }

    QFileInfo fileInfo(path);

    if (fileInfo.exists()) {
        return true;
    } else {
        QMessageBox::warning(nullptr,
                             QObject::tr("路径不存在"),
                             QObject::tr("路径不存在或无法访问：\n%1").arg(path));
        return false;
    }
}

PlayWidgetWrapper::~PlayWidgetWrapper()
{
    qDebug() << "PlayWidgetWrapper::~PlayWidgetWrapper() - Starting cleanup...";
    
    QThread::msleep(200);
    
    if(a)
    {
        qDebug() << "PlayWidgetWrapper: Stopping thread a...";
        a->quit();
        if(!a->wait(2000)) {
            qDebug() << "PlayWidgetWrapper: Thread a did not finish, terminating...";
            a->terminate();
            a->wait();
        }
    }
    if(b)
    {
        qDebug() << "PlayWidgetWrapper: Stopping thread b...";
        b->quit();
        if(!b->wait(2000)) {
            qDebug() << "PlayWidgetWrapper: Thread b did not finish, terminating...";
            b->terminate();
            b->wait();
        }
    }
    if(c)
    {
        qDebug() << "PlayWidgetWrapper: Stopping thread c...";
        c->quit();
        if(!c->wait(2000)) {
            qDebug() << "PlayWidgetWrapper: Thread c did not finish, terminating...";
            c->terminate();
            c->wait();
        }
    }
    
    work.reset();
    lrc.reset();
    take_pcm.reset();
    
    qDebug() << "PlayWidgetWrapper::~PlayWidgetWrapper() - Cleanup complete";
}

void PlayWidgetWrapper::slot_updateBackground(QString picPath) {
    qDebug() << "slot_updateBackground called with:" << picPath;
    // 模糊背景已经在 playwidget_qml.h 中的 setPicPath 处理
}
