#include "play_widget.h"

PlayWidget::PlayWidget(QWidget *parent)
    : QWidget(parent)

{
    qDebug() << __FUNCTION__ << QThread::currentThread()->currentThreadId;
    QWidget* bottom = new QWidget(this);
    bottom->setFixedSize(1000, 100);
    bottom->move(0, 500);

    controlBar = new ControlBar(bottom);

    desk = new DeskLrcWidget();
    desk->raise();
    desk->hide();

    process_slider = new ProcessSlider(this);
    process_slider->setFixedSize(1000,20);
    process_slider->move(0, 500);
    process_slider->setMaxSeconds(0);

    pianWidget = new PianWidget(bottom);
    connect(pianWidget, &PianWidget::signal_up_click, this, [=](bool flag){
        emit signal_big_clicked(flag);
        setPianWidgetEnable(true);
    });

    QHBoxLayout* hlayout__ = new QHBoxLayout(this);
    hlayout__->addWidget(pianWidget);
    hlayout__->addWidget(controlBar);

    bottom->setLayout(hlayout__);

    music = new QPushButton(this);
    music->setFixedSize(30,30);
    music->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/up.png);"
                "}"
                );
    connect(music,&QPushButton::clicked,this,[=]() {
        emit signal_big_clicked(false);
    });
    music->move(10, 10);

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
    init_TextEdit();

    QWidget* rotate_widget = new QWidget(this);
    RotatingCircleImage* rotate = new RotatingCircleImage(rotate_widget);
    rotate_widget->move(100, 100);
    rotate_widget->resize(300, 300);
    rotate->resize(300, 300);

    connect(this, &PlayWidget::signal_stop_rotate, rotate, &RotatingCircleImage::on_signal_stop_rotate);
    connect(take_pcm.get(), &TakePcm::signal_send_pic_path, pianWidget, &PianWidget::on_signal_set_pic_path);
    connect(take_pcm.get(), &TakePcm::begin_take_lrc, work.get(), &Worker::setPATH);
    connect(take_pcm.get(),&TakePcm::begin_to_play,work.get(),&Worker::play_pcm);
    connect(take_pcm.get(),&TakePcm::data,work.get(),&Worker::receive_data);
    connect(take_pcm.get(),&TakePcm::Position_Change,work.get(),&Worker::reset_play);
    connect(work.get(), &Worker::begin_to_decode, take_pcm.get(), &TakePcm::begin_to_decode);
    connect(take_pcm.get(),&TakePcm::send_totalDuration, work.get(),&Worker::receive_totalDuration);
    connect(take_pcm.get(),&TakePcm::durations,[ = ](qint64 value){
        this->duration = static_cast<qint64>(value);
        process_slider->setMaxSeconds(value / 1000000);
    });
    //connect(this,&PlayWidget::signal_process_Change,take_pcm.get(),&TakePcm::seekToPosition);
    connect(this, &PlayWidget::signal_process_Change, take_pcm.get(), &TakePcm::seekToPosition);
    //connect(this, &PlayWidget::signal_process_Change, work.get(), &Worker::reset_play);

    connect(this, &PlayWidget::signal_filepath, [=](QString path) {
        emit take_pcm->signal_begin_make_pcm(path);
    });
    connect(this, &PlayWidget::signal_worker_play, work.get(), &Worker::Pause);
    connect(this, &PlayWidget::signal_remove_click, work.get(), &Worker::reset_status);

    connect(this, &PlayWidget::signal_filepath, this, &PlayWidget::_begin_take_lrc);
    connect(this,&PlayWidget::signal_begin_take_lrc,lrc.get(),&LrcAnalyze::begin_take_lrc);
    connect(lrc.get(),&LrcAnalyze::send_lrc,work.get(),&Worker::receive_lrc);
    connect(lrc.get(),&LrcAnalyze::send_lrc,this, &PlayWidget::slot_Lrc_send_lrc);
    connect(textEdit, &LyricTextEdit::signal_current_lrc, this, &PlayWidget::signal_desk_lrc);
    connect(this, &PlayWidget::signal_desk_lrc, desk, &DeskLrcWidget::slot_receive_lrc);
    connect(work.get(),&Worker::send_lrc,this,[=](int line){
        if(line != textEdit->currentLine)
        {
            textEdit->highlightLine(line);
            textEdit->scrollLines(line);  // 直接传递行号，而不是差值
            textEdit->currentLine = line;
            update();
        }
    });

    nameLabel = new QLabel(this);
    nameLabel->move(400, 50);
    nameLabel->setStyleSheet("QLabel { color: white; font-size: 28px; }");
    nameLabel->setWordWrap(true);
    nameLabel->setFixedSize(550, 30);
    nameLabel->setAlignment(Qt::AlignCenter);

    connect(work.get(),&Worker::Stop,this, &PlayWidget::slot_work_stop);
    connect(work.get(),&Worker::Begin,this, &PlayWidget::slot_work_play);

    connect(desk, &DeskLrcWidget::signal_play_clicked, this, &PlayWidget::slot_play_click);
    connect(this, &PlayWidget::signal_playState, desk, &DeskLrcWidget::signal_play_Clicked);
    connect(desk, &DeskLrcWidget::signal_last_clicked, controlBar, &ControlBar::signal_lastSong);
    connect(desk, &DeskLrcWidget::signal_next_clicked, controlBar, &ControlBar::signal_nextSong);
    connect(desk, &DeskLrcWidget::signal_forward_clicked, this, [=](){
        //emit signal_set_SliderMove(true);
        disconnect(work.get(), &Worker::durations, process_slider, &ProcessSlider::slot_change_duartion);
        work->slot_setMove();

        process_slider->slot_forward();
        int newPosition = std::min(process_slider->maxValue(), process_slider->value() + 1) * this->duration / (1000 * process_slider->maxValue());
       
        work->reset_play();
        
        emit signal_process_Change(newPosition, false);
        //emit signal_set_SliderMove(false);
    });
    connect(desk, &DeskLrcWidget::signal_backward_clicked, this, [=](){
        disconnect(work.get(), &Worker::durations, process_slider, &ProcessSlider::slot_change_duartion);
        work->slot_setMove();

        process_slider->slot_backward();
        int newPosition = std::max(0, process_slider->value() - 1) * this->duration / (1000 * process_slider->maxValue());

        work->reset_play();

        emit signal_process_Change(newPosition, true);
    });
    connect(controlBar, &ControlBar::signal_play_clicked, this, &PlayWidget::slot_play_click);
    connect(this, &PlayWidget::signal_playState, controlBar, &ControlBar::slot_playState);
    connect(controlBar, &ControlBar::signal_volumeChanged, work.get(), &Worker::Set_Volume);
    connect(controlBar, &ControlBar::signal_nextSong, this,[=](){
        emit signal_Next(fileName);
    });
    connect(controlBar, &ControlBar::signal_lastSong, this,[=](){
        emit signal_Last(fileName);
    });
    connect(controlBar, &ControlBar::signal_stop, this, [=](){
        if(fileName.size())
            emit signal_play_button_click(false, fileName);
    });
    connect(controlBar, &ControlBar::signal_mlist_toggled, this, &PlayWidget::signal_list_show);
    connect(controlBar, &ControlBar::signal_rePlay, this, [=](){
        rePlay(this->filePath);
     });
    connect(controlBar, &ControlBar::signal_desk_toggled, this, &PlayWidget::slot_desk_toggled);

    connect(take_pcm.get(), &TakePcm::signal_move, this, [=]() {
        connect(work.get(), &Worker::durations, process_slider, &ProcessSlider::slot_change_duartion);
        work->slot_setMove();
     });
    connect(take_pcm.get(), &TakePcm::signal_worker_play, work.get(), &Worker::Pause);
    connect(this,&PlayWidget::signal_set_SliderMove,work.get(),&Worker::set_SliderMove);
    connect(process_slider,&ProcessSlider::signal_sliderPressed,[=](){
        qDebug() << "拖动进度条";
        
        disconnect(work.get(), &Worker::durations, process_slider, &ProcessSlider::slot_change_duartion);
        work->slot_setMove();
    });
    connect(process_slider,&ProcessSlider::signal_sliderReleased,[=](){
        int press_position = process_slider->getPressPosition();
        int newPosition = process_slider->value() * this->duration / (1000 * process_slider->maxValue());
        qDebug() << __FUNCTION__ << newPosition / 1000;
        
       work->reset_play();

        bool back_flag = false;
        if (process_slider->value() >= press_position) {
            back_flag = true;
        }
        emit signal_process_Change(newPosition, back_flag);
        //take_pcm->seekToPosition(newPosition, back_flag);
       
        process_slider->setPrintF(true);
    });

    connect(process_slider, &ProcessSlider::signal_playFinished, controlBar, &ControlBar::slot_playFinished);
    connect(this, &PlayWidget::signal_isUpChanged, process_slider, &ProcessSlider::slot_isUpChanged);
    connect(this, &PlayWidget::signal_isUpChanged, controlBar, &ControlBar::slot_isUpChanged);
}
void PlayWidget::slot_desk_toggled(bool checked){
    if(checked){
        desk->show();
    }else{
        desk->hide();
    }
}
void PlayWidget::set_isUp(bool flag){
    isUp = flag;
    emit signal_isUpChanged(isUp);
}
void PlayWidget::slot_work_stop(){
    qDebug() << __FUNCTION__ << "暂停";
    emit signal_playState(ControlBar::Pause);
    if(fileName.size())
    {
        emit signal_stop_rotate(false);
        emit signal_play_button_click(false, fileName);
        pianWidget->setName(QFileInfo(fileName).baseName());
        nameLabel->setText(QFileInfo(fileName).baseName());
    }
}
void PlayWidget::slot_work_play(){
    emit signal_playState(ControlBar::Play);
    emit signal_stop_rotate(true);
    emit signal_play_button_click(true, fileName);
    pianWidget->setName(QFileInfo(fileName).baseName());
    nameLabel->setText(QFileInfo(fileName).baseName());
    connect(work.get(), &Worker::durations, process_slider, &ProcessSlider::slot_change_duartion);
}
void PlayWidget::slot_Lrc_send_lrc(const std::map<int, std::string> lyrics){
    this->textEdit->currentLine = 5;  // 初始行设为5，对应第一行歌词
    this->lyrics.clear();
    for(int i = 0;i<5;i++)
    {
        textEdit->append("    ");
    }
    {
        std::lock_guard<std::mutex>lock(mtx);
        for (const auto& [time, text] : lyrics)
        {
            textEdit->append(QString::fromStdString(text));
        }
        this->lyrics = lyrics;
    }
    for(int i = 0;i < 9;i++)
    {
        textEdit->append("    ");
    }
    QTextCursor cursor = textEdit->textCursor();
    cursor.select(QTextCursor::Document);
    QTextBlockFormat blockFormat;
    blockFormat.setAlignment(Qt::AlignCenter);
    cursor.mergeBlockFormat(blockFormat);
    textEdit->setTextCursor(cursor);
    QTextCursor cursor_ = textEdit->textCursor();
    cursor_.movePosition(QTextCursor::Start);
    textEdit->setTextCursor(cursor_);
}
void PlayWidget::slot_play_click(){
    if(!controlBar->getLoopFlag() && process_slider->value() == process_slider->maxValue()){
        rePlay(filePath);
    }else{
        // 根据当前状态立即更新UI
        ControlBar::State currentState = controlBar->getState();
        if(currentState == ControlBar::Pause || currentState == ControlBar::Stop) {
            emit signal_playState(ControlBar::Play);
        } else {
            emit signal_playState(ControlBar::Pause);
        }
        qDebug() << __FUNCTION__ << "播放按钮点击";
        emit signal_worker_play();
        //work->Pause();
    }
}

void PlayWidget::rePlay(QString path)
{
    if(path.size() == 0)
        return;
    emit signal_filepath(path);
}


void PlayWidget::init_TextEdit()
{
    textEdit = new LyricTextEdit(this);
    textEdit->disableScrollBar();
    textEdit->resize(550, 350);
    textEdit->setReadOnly(true);
    textEdit->setFocusPolicy(Qt::NoFocus);
    textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textEdit->setTextInteractionFlags(Qt::NoTextInteraction);
    textEdit->viewport()->setCursor(Qt::ArrowCursor);

    QFont font = textEdit->font();
    font.setPointSize(16);

    textEdit->setFont(font);
    textEdit->setStyleSheet("QTextEdit { background: transparent; border: none; }");
    textEdit->move(400, 100);

    QPalette palette = this->textEdit->palette();
    palette.setColor(QPalette::Text, QColor("#FAFAFA"));
    this->textEdit->setPalette(palette);
}


void PlayWidget::_begin_take_lrc(QString str)
{
    this->textEdit->clear();
    lrc->begin_take_lrc(str);
}
void PlayWidget::openfile()
{

    // 创建一个临时的 QWidget 实例，用于显示文件对话框
    QWidget dummyWidget;

    // 打开文件对话框
    QString filePath_ = QFileDialog::getOpenFileName(
                &dummyWidget,                   // 父窗口（可以是 nullptr）
                "Open File",                    // 对话框标题
                QDir::homePath(),               // 起始目录（可以是任意路径）
                "Audio Files (*.mp3 *.wav *.flac *.ogg);;All Files (*)"  // 文件过滤器
                );

    // 打印选中的文件路径
    if (!filePath_.isEmpty())
    {
        QFileInfo fileInfo(filePath_);
        QString filename = fileInfo.fileName();
        emit signal_add_song(filename,filePath_);
    }
}

void PlayWidget::_play_click(QString songPath)
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
        emit controlBar->signal_play_clicked();
    }

}
void PlayWidget::_remove_click(QString songName)
{
    if(songName == this->fileName)
    {
        this->fileName.clear();
        this->filePath.clear();

        emit signal_remove_click();
    }
}
void PlayWidget::setPianWidgetEnable(bool flag)
{
    if(flag)
        this->pianWidget->hide();
    else
        this->pianWidget->show();
}
bool PlayWidget::checkAndWarnIfPathNotExists(const QString &path) {

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
PlayWidget::~PlayWidget()
{
    qDebug() << "PlayWidget::~PlayWidget() - Starting cleanup...";
 
    
    // 2. 等待线程完成当前工作
    QThread::msleep(200);
    
    // 3. 退出并等待所有 QThread
    if(a)
    {
        qDebug() << "PlayWidget: Stopping thread a...";
        a->quit();
        if(!a->wait(2000)) {  // 等待最多2秒
            qDebug() << "PlayWidget: Thread a did not finish, terminating...";
            a->terminate();
            a->wait();
        }
    }
    if(b)
    {
        qDebug() << "PlayWidget: Stopping thread b...";
        b->quit();
        if(!b->wait(2000)) {
            qDebug() << "PlayWidget: Thread b did not finish, terminating...";
            b->terminate();
            b->wait();
        }
    }
    if(c)
    {
        qDebug() << "PlayWidget: Stopping thread c...";
        c->quit();
        if(!c->wait(2000)) {
            qDebug() << "PlayWidget: Thread c did not finish, terminating...";
            c->terminate();
            c->wait();
        }
    }
    
    // 4. 清理 shared_ptr（会自动调用析构函数）
    work.reset();
    lrc.reset();
    take_pcm.reset();
    
    qDebug() << "PlayWidget::~PlayWidget() - Cleanup complete";
}
