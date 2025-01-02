#include "play_widget.h"

PlayWidget::PlayWidget(QWidget *parent)
    : QWidget(parent)
    ,played(false)
    ,loop(false)

{
    qInfo()<<__FUNCTION__<<QThread::currentThreadId();
    QWidget* bottom = new QWidget(this);
    bottom->setFixedSize(1000, 100);
    bottom->move(0, 500);

    button_widget = new QWidget(bottom);

    play  = new QPushButton(button_widget);
    video = new QPushButton(button_widget);
    Loop  = new QPushButton(button_widget);

    mlist = new QPushButton(button_widget);
    last  = new QPushButton(button_widget);
    next  = new QPushButton(button_widget);

    Loop->setFixedSize(30,30);

    last->setFixedSize(30,30);
    play->setFixedSize(30,30);
    next->setFixedSize(30,30);
    video->setFixedSize(30,30);
    mlist->setFixedSize(30,30);

    Slider = new QSlider(Qt::Horizontal,this);
    Slider->setMinimum(0);
    Slider->setMaximum(0);


    QHBoxLayout* hlayout = new QHBoxLayout(this);
    hlayout->addWidget(Loop);
    //hlayout->addWidget(music);
    hlayout->addWidget(last);
    hlayout->addWidget(play);
    hlayout->addWidget(next);
    hlayout->addWidget(video);
    hlayout->addWidget(mlist);


    //button_widget->setFixedSize(1000, 100);
    button_widget->setLayout(hlayout);
    //    button_widget->move(0, 520);
    button_widget->setLayout(hlayout);
    //button_widget->setStyleSheet("background: transparent;");

    pianWidget = new PianWidget(bottom);
    connect(pianWidget, &PianWidget::signal_up_click, this, [=](bool flag){
        emit signal_big_clicked(flag);
        setPianWidgetEnable(true);
    });

    QHBoxLayout* hlayout__ = new QHBoxLayout(this);
    hlayout__->addWidget(pianWidget);
    hlayout__->addWidget(button_widget);

    bottom->setLayout(hlayout__);



    Slider->setFixedSize(1000,20);
    Slider->move((1000-Slider->width())/2, 500);

    Slider->setStyleSheet(
                "QSlider {"
                "    background: transparent;"
                "    border: none;"
                "}"
                "QSlider::handle:horizontal {"
                "    background: #3f8b6d;"
                "    border: 1px solid #5c5c5c;"
                "    width: 15px;"
                "    height: 15px;"
                "    border-radius: 5px;"
                "    margin: -10px 0px;"
                "}"
                "QSlider::groove:horizontal {"
                "    background: #f0f0f0;"
                "    height: 4px;"
                "    border: none;"
                "    margin: 0px;"
                "}"
                "QSlider::add-page:horizontal {"
                "    background: transparent;"
                "}"
                "QSlider::sub-page:horizontal {"
                "    background: #3f8b6d;"
                "}"
                );



    video->setCheckable(true);

    mlist->setCheckable(true);


    video->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/volume.png);"
                "}"
                );

    play->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/play.png);"
                "}"
                );
    mlist->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/playlist.png);"
                "}"
                );
    last->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/last_song.png);"
                "}"
                );
    next->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/next_song.png);"
                "}"
                );

    slider = new QSlider(Qt::Horizontal, this);
    slider->hide();
    slider->setMinimum(0);
    slider->setMaximum(100);
    slider->setValue(50);
    slider->setTickPosition(QSlider::TicksBelow);  // 在滑条右侧显示刻度
    slider->setTickInterval(10);
    slider->resize(100, 30);





    Loop->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/random_play.png);"
                "}"
                );

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


    //music->setCheckable(true);

    a = new QThread(this);
    b = new QThread(this);
    c = new QThread(this);


    work = std::make_shared<Worker>();
    work->moveToThread(c);

    lrc = std::make_shared<LrcAnalyze>();

    take_pcm = std::make_shared<TakePcm>();
    take_pcm->moveToThread(a);



    a->start();
    b->start();
    c->start();
    init_TextEdit();

    //this->textEdit->setCursor(Qt::ArrowCursor);

    QWidget* rotate_widget = new QWidget(this);
    RotatingCircleImage* rotate = new RotatingCircleImage(rotate_widget);
    rotate_widget->move(100, 100);
    rotate_widget->resize(300, 300);
    //rotate->move(100, 100);
    rotate->resize(300, 300);
    rotate->moveToThread(c);
    connect(this, &PlayWidget::signal_stop_rotate, rotate, &RotatingCircleImage::on_signal_stop_rotate);

    //qDebug()<<"MainWindow"<<QThread::currentThreadId();


    connect(next, &QPushButton::clicked, this, [=](){
        emit signal_Next(this->fileName);
    });
    connect(last, &QPushButton::clicked, this, [=](){
        emit signal_Last(this->fileName);
    });

    connect(take_pcm.get(), &TakePcm::signal_send_pic_path, pianWidget, &PianWidget::on_signal_set_pic_path);
    connect(take_pcm.get(), &TakePcm::begin_take_lrc, work.get(), &Worker::setPATH);
    connect(take_pcm.get(),&TakePcm::begin_to_play,work.get(),&Worker::play_pcm);
    connect(take_pcm.get(),&TakePcm::data,work.get(),&Worker::receive_data);
    connect(take_pcm.get(),&TakePcm::Position_Change,work.get(),&Worker::reset_play);
    connect(take_pcm.get(),&TakePcm::send_totalDuration, work.get(),&Worker::receive_totalDuration);
    connect(take_pcm.get(),&TakePcm::durations,[ = ](qint64 value){
        this->duration = static_cast<qint64>(value);
        //qDebug()<<"this->duration"<<this->duration;
    });
    connect(this,&PlayWidget::signal_process_Change,take_pcm.get(),&TakePcm::seekToPosition);

    connect(this, &PlayWidget::signal_filepath, [=](QString path) {
        emit take_pcm->signal_begin_make_pcm(path);
    });
    //connect(this,&Play_Widget::signal_filepath,work.get(),&Worker::reset_play);


    //    connect(take_pcm.get(),&Take_pcm::begin_to_play,this,&MainWindow::_begin_to_play);
    //    connect(this,&MainWindow::begin_to_play,work.get(),&Worker::begin_play);

    //    connect(take_pcm.get(),&Take_pcm::begin_to_play,this,&MainWindow::_begin_to_play);
    //    connect(this,&MainWindow::begin_to_play,work.get(),&Worker::play_pcm);





    connect(this, &PlayWidget::signal_remove_click, work.get(), &Worker::reset_status);
    connect(work.get(),&Worker::rePlay,this,[=](){
        {
            std::lock_guard<std::mutex>lock(mtx);
            this->played = true;
        }
        if(this->loop&&this->filePath.size()>0)
        {
            rePlay(this->filePath);
        }
    });

    connect(play,&QPushButton::clicked,work.get(),&Worker::stop_play);
    connect(play,&QPushButton::clicked,[=](){
        if(!this->loop&&filePath.size()>0&&played)
        {
            rePlay(this->filePath);
            //qDebug()<<"rePlay"<<this->filePath;
        }
    });

    connect(this, &PlayWidget::signal_filepath, this, &PlayWidget::_begin_take_lrc);
    connect(this,&PlayWidget::signal_begin_take_lrc,lrc.get(),&LrcAnalyze::begin_take_lrc);

    connect(lrc.get(),&LrcAnalyze::send_lrc,work.get(),&Worker::receive_lrc);
    connect(lrc.get(),&LrcAnalyze::send_lrc,this,[=](const std::map<int, std::string> lyrics){
        qDebug()<<__FUNCTION__<<"lyrics"<<lyrics.size();
        this->textEdit->currentLine = 4;
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
    });


    connect(work.get(),&Worker::send_lrc,this,[=](int line){
        if(line != textEdit->currentLine)
        {

            textEdit->highlightLine(line);

            textEdit->scrollLines(line-textEdit->currentLine);

            //qDebug()<<"line:"<<line<<"textEdit->currentLine:"<<textEdit->currentLine;

            textEdit->currentLine = line;

            update();
        }
    });

    QLabel* nameLabel = new QLabel(this);
    nameLabel->move(400, 50);
    nameLabel->setStyleSheet("QLabel { color: white; font-size: 28px; }");
    nameLabel->setWordWrap(true);
    nameLabel->setFixedSize(550, 30);
    nameLabel->setAlignment(Qt::AlignCenter);

    connect(work.get(),&Worker::Stop,this,[=](){

        play->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/play.png);"
                    "}"
                    );
        if(fileName.size())
        {
            emit signal_stop_rotate(false);
            emit signal_play_button_click(false, fileName);
            pianWidget->setName(QFileInfo(fileName).baseName());
            nameLabel->setText(QFileInfo(fileName).baseName());
        }
    });
    connect(work.get(),&Worker::Begin,this,[=](){

        play->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/pause.png);"
                    "}"
                    );
        emit signal_stop_rotate(true);
        emit signal_play_button_click(true, fileName);
        pianWidget->setName(QFileInfo(fileName).baseName());
        nameLabel->setText(QFileInfo(fileName).baseName());
    });

    connect(Loop,&QPushButton::clicked,this,[=](){
        if(this->loop)
        {
            Loop->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/random_play.png);"
                        "}"
                        );
        }
        else
        {
            Loop->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/loop.png);"
                        "}"
                        );
        }
        this->loop = !this->loop;

        //qDebug()<<"Loop state"<<this->loop;
    });


    connect(video,&QPushButton::toggled,this,[=](bool checked){
        if(checked)
        {

            auto Position = button_widget->mapToParent(video->pos());
            slider->move(Position.x(), Position.y() + 520);
            qDebug()<<"被选中"<<Position;
            slider->show();
            slider->raise();
        }
        else
        {
            if(slider)
            {
                slider->close();
            }
        }
    });

    connect(mlist,&QPushButton::toggled, this, [=](bool checked){
        if(checked)
        {
            emit signal_list_show(true);
            //qDebug()<<"show";
        }
        else
        {
            emit signal_list_show(false);
        }
    });
    connect(slider,&QSlider::valueChanged,work.get(),&Worker::Set_Volume);


    connect(work.get(),&Worker::durations,[=](qint64 value){
        Slider->setValue((value*10000000)/this->duration);
    });
    connect(this,&PlayWidget::signal_set_SliderMove,work.get(),&Worker::set_SliderMove);

    connect(Slider,&QSlider::sliderPressed,[=](){
        emit signal_set_SliderMove(true);

    });
    connect(Slider,&QSlider::sliderReleased,[=](){


        int newPosition = Slider->value()*this->duration/10000000;

        emit signal_process_Change(newPosition);
        emit signal_set_SliderMove(false);

        Slider->setValue((newPosition*10000)/this->duration);
    });


}


void PlayWidget::rePlay(QString path)
{
    Slider->setMinimum(0);
    Slider->setMaximum(0);

    emit signal_filepath(path);
    {
        std::lock_guard<std::mutex>lock(mtx);
        this->played = false;
    }

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
    Slider->setRange(0,10000);
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

        //request->Upload(filePath_);

        //qDebug() << "Selected file:" << filePath;
    }
    else
    {
        //qDebug() << "No file selected.";
    }


}

void PlayWidget::_play_click(QString songPath)
{

    if(songPath != this->filePath)
    {

        qDebug()<<__FUNCTION__<<sender()<<songPath<<this->filePath<<take_pcm.get();
        this->filePath = songPath;

        QFileInfo fileInfo(songPath);
        fileName = fileInfo.fileName();

        Slider->setMinimum(0);
        Slider->setMaximum(0);

        emit signal_filepath(songPath);

    }
    else
    {
        this->play->click();
    }

}
void PlayWidget::_remove_click(QString songName)
{
    if(songName == this->fileName)
    {
        Slider->setMinimum(0);
        Slider->setMaximum(0);

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
PlayWidget::~PlayWidget()
{

    if(a)
    {
        a->quit();
        a->wait();
    }
    if(b)
    {
        b->quit();
        b->wait();
    }
    if(c)
    {
        c->quit();
        c->wait();
    }

}
