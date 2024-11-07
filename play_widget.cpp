#include "play_widget.h"

Play_Widget::Play_Widget(QWidget *parent)
    : QWidget(parent)
    ,played(false)
    ,loop(false)

{

    play  = new QPushButton(this);
    video = new QPushButton(this);
    Loop  = new QPushButton(this);
    music = new QPushButton(this);
    mlist = new QPushButton(this);
    last  = new QPushButton(this);
    next  = new QPushButton(this);

    Slider = new QSlider(Qt::Horizontal,this);
    Slider->setMinimum(0);
    Slider->setMaximum(0);

    int space = (800-8*50)/9;

    Loop->setFixedSize(50,50);
    Loop->move(space,400);

    music->setFixedSize(50,50);
    music->move(50+2*space,400);

    last->setFixedSize(50,50);
    last->move(2*50+3*space,400);

    play->setFixedSize(50,50);
    play->move(50*3+4*space,400);

    next->setFixedSize(50,50);
    next->move(4*50+5*space,400);

    video->setFixedSize(50,50);
    video->move(5*50+6*space,400);

    mlist->setFixedSize(50,50);
    mlist->move(6*50+7*space,400);

    Slider->setFixedSize(800,20);
    Slider->move((800-Slider->width())/2,350);


    video->setCheckable(true);
    music->setCheckable(true);
    mlist->setCheckable(true);

    video->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/volume.png);"
                "}"
                );
    music->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/up.png);"
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

    slider = new QSlider(Qt::Vertical, this);
    slider->close();
    slider->setMinimum(0);
    slider->setMaximum(100);
    slider->setValue(50);
    slider->setTickPosition(QSlider::TicksRight);  // 在滑条右侧显示刻度
    slider->setTickInterval(10);
    slider->setGeometry(video->pos().x(),video->pos().y()-video->height()*3,video->width(),video->height()*3);


    Loop->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/random_play.png);"
                "}"
                );


    a = new QThread(this);
    b = new QThread(this);
    c = new QThread(this);


    work = std::make_unique<Worker>();
    work->moveToThread(c);

    lrc = std::make_unique<LrcAnalyze>();

    take_pcm = std::make_unique<Take_pcm>();
    take_pcm->moveToThread(a);

    a->start();
    b->start();
    c->start();
    init_TextEdit();
    this->textEdit->setCursor(Qt::ArrowCursor);


    //qDebug()<<"MainWindow"<<QThread::currentThreadId();


    connect(music,&QPushButton::toggled,this,[=](bool checked) {
        emit big_clicked(checked);
    });

    connect(next, &QPushButton::clicked, this, [=](){
        emit Next(this->fileName);
    });
    connect(last, &QPushButton::clicked, this, [=](){
        emit Last(this->fileName);
    });

    connect(take_pcm.get(), &Take_pcm::begin_take_lrc, work.get(), &Worker::setPATH);


    connect(this,&Play_Widget::filepath,take_pcm.get(),&Take_pcm::make_pcm);
    //connect(this,&Play_Widget::filepath,work.get(),&Worker::play_pcm);
    connect(take_pcm.get(),&Take_pcm::begin_to_play,work.get(),&Worker::play_pcm);
    connect(take_pcm.get(),&Take_pcm::data,work.get(),&Worker::receive_data);
    connect(take_pcm.get(),&Take_pcm::Position_Change,work.get(),&Worker::reset_play);

    //    connect(take_pcm.get(),&Take_pcm::begin_to_play,this,&MainWindow::_begin_to_play);
    //    connect(this,&MainWindow::begin_to_play,work.get(),&Worker::begin_play);

    //    connect(take_pcm.get(),&Take_pcm::begin_to_play,this,&MainWindow::_begin_to_play);
    //    connect(this,&MainWindow::begin_to_play,work.get(),&Worker::play_pcm);



    connect(take_pcm.get(),&Take_pcm::send_totalDuration,work.get(),&Worker::receive_totalDuration);

    connect(this, &Play_Widget::remove_click, work.get(), &Worker::reset_status);
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
    //    connect(take_pcm.get(),&Take_pcm::begin_take_lrc,this,&MainWindow::_begin_take_lrc);
    //    connect(this,&MainWindow::begin_take_lrc,lrc.get(),&lrc_analyze::begin_take_lrc);


    //    connect(take_pcm.get(),&Take_pcm::begin_take_lrc,this,&Play_Widget::_begin_take_lrc);
    //    connect(this,&Play_Widget::begin_take_lrc,lrc.get(),&LrcAnalyze::begin_take_lrc);
    connect(this, &Play_Widget::filepath, this, &Play_Widget::_begin_take_lrc);
    connect(this,&Play_Widget::begin_take_lrc,lrc.get(),&LrcAnalyze::begin_take_lrc);

    //connect(take_pcm.get(),&Take_pcm::begin_take_lrc,lrc.get(),&LrcAnalyze::begin_take_lrc);


    connect(lrc.get(),&LrcAnalyze::send_lrc,work.get(),&Worker::receive_lrc);
    connect(lrc.get(),&LrcAnalyze::send_lrc,this,[=](const std::map<int, std::string> lyrics){



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
                //qDebug()<<QString::fromStdString(text);
            }
            this->lyrics = lyrics;
        }
        for(int i = 0;i<5;i++)
        {
            textEdit->append("    ");
        }


        // 获取 QTextEdit 的光标
        QTextCursor cursor = textEdit->textCursor();

        // 选择整个文档的文本
        cursor.select(QTextCursor::Document);

        // 创建一个 QTextBlockFormat 对象
        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(Qt::AlignCenter); // 设置对齐方式为居中

        cursor.mergeBlockFormat(blockFormat);

        // 更新 QTextEdit 的光标位置
        textEdit->setTextCursor(cursor);


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

    connect(work.get(),&Worker::Stop,this,[=](){

        play->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/play.png);"
                    "}"
                    );
        if(fileName.size())
            emit play_button_click(false, fileName);

    });
    connect(work.get(),&Worker::Begin,this,[=](){

        play->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/pause.png);"
                    "}"
                    );
        emit play_button_click(true, fileName);
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
            //qDebug()<<"被选中";

            slider->show();
        }
        else
        {
            if(slider)
            {
                slider->close();
            }
        }
    });

    connect(mlist,&QPushButton::toggled,this,[=](bool checked){
        if(checked)
        {
            emit list_show(true);
            //qDebug()<<"show";
        }
        else
        {
            emit list_show(false);
        }
    });
    connect(slider,&QSlider::valueChanged,work.get(),&Worker::Set_Volume);

    connect(take_pcm.get(),&Take_pcm::durations,[ = ](qint64 value){
        this->duration = static_cast<qint64>(value);
        //qDebug()<<"this->duration"<<this->duration;
    });
    connect(work.get(),&Worker::durations,[=](qint64 value){
        Slider->setValue((value*10000000)/this->duration);
        // qDebug()<<Slider->value()*this->duration/10000000;
    });
    connect(this,&Play_Widget::set_SliderMove,work.get(),&Worker::set_SliderMove);

    connect(Slider,&QSlider::sliderPressed,[=](){
        emit set_SliderMove(true);

    });

    connect(Slider,&QSlider::sliderReleased,[=](){


        int newPosition = Slider->value()*this->duration/10000000;

        emit process_Change(newPosition);
        emit set_SliderMove(false);

        Slider->setValue((newPosition*10000)/this->duration);
    });

    connect(this,&Play_Widget::process_Change,take_pcm.get(),&Take_pcm::seekToPosition);

}


void Play_Widget::rePlay(QString path)
{
    Slider->setMinimum(0);
    Slider->setMaximum(0);

    emit filepath(path);
    {
        std::lock_guard<std::mutex>lock(mtx);
        this->played = false;
    }

}


void Play_Widget::init_TextEdit()
{
    this->textEdit = new LyricTextEdit(this);
    this->textEdit->setTextInteractionFlags(Qt::NoTextInteraction);//禁用交互
    this->textEdit->disableScrollBar();
    this->textEdit->setFixedSize(450,300);

    this->textEdit->viewport()->setCursor(Qt::ArrowCursor);

    QFont font = this->textEdit->font();
    font.setPointSize(16);
    this->textEdit->setFont(font);
    this->textEdit->setStyleSheet("QTextEdit { background: transparent; border: none; }");
    this->textEdit->move(400, 0);

}



void Play_Widget::_begin_take_lrc(QString str)
{
    Slider->setRange(0,10000);
    this->textEdit->clear();

    emit begin_take_lrc(str);

    QTextCursor cursor = textEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    textEdit->setTextCursor(cursor);
}
void Play_Widget::openfile()
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
        emit add_song(filename,filePath_);



        //qDebug() << "Selected file:" << filePath;
    }
    else
    {
        //qDebug() << "No file selected.";
    }


}

void Play_Widget::_play_click(QString songPath)
{
    if(songPath != this->filePath)
    {
        this->filePath = songPath;

        QFileInfo fileInfo(songPath);
        fileName = fileInfo.fileName();

        Slider->setMinimum(0);
        Slider->setMaximum(0);

        emit filepath(songPath);


    }
    else
    {
        this->play->click();
    }
}
void Play_Widget::_remove_click(QString songName)
{
    if(songName == this->fileName)
    {
        Slider->setMinimum(0);
        Slider->setMaximum(0);

        this->fileName.clear();
        this->filePath.clear();

        emit remove_click();
    }
}
Play_Widget::~Play_Widget()
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
