#include "play_widget.h"

Play_Widget::Play_Widget(QWidget *parent)
    : QWidget(parent)
    ,played(false)
    ,loop(false)

{



    play=new QPushButton(this);
    video=new QPushButton(this);
    Loop=new QPushButton(this);
    dir=new QPushButton(this);
    music=new QPushButton(this);


    Slider=new QSlider(Qt::Horizontal,this);
    Slider->setMinimum(0);
    Slider->setMaximum(0);

    int space=(800-4*50)/6;

    Loop->setFixedSize(50,50);
    Loop->move(space,400);

    dir->setFixedSize(50,50);
    dir->move(space*2+50,400);

    play->setFixedSize(50,50);
    play->move(50*2+3*space,400);

    video->setFixedSize(50,50);
    video->move(3*50+4*space,400);

    music->setFixedSize(50,50);
    music->move(4*50+5*space,400);

    Slider->setFixedSize(800,20);
    Slider->move((800-Slider->width())/2,350);


    video->setCheckable(true);
    music->setCheckable(true);


    dir->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/上传存盘.png);"
                "}"
                );

    video->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/音量.png);"
                "}"
                );

    play->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/播放.png);"
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
                "    border-image: url(:/new/prefix1/icon/随机播放.png);"
                "}"
                );



    //    QThread*a=new QThread();
    //    QThread*b=new QThread();
    //    QThread*c=new QThread();


    a=new QThread(this);
    b=new QThread(this);
    c=new QThread(this);

    //Slider->setRange(0,10000);



    QTimer*timer = new QTimer();
    work=std::make_unique<Worker>(timer);
    work->moveToThread(c);
    timer->moveToThread(c);

    lrc=std::make_unique<LrcAnalyze>();
    //lrc->moveToThread(b);
    //lrc.get()->moveToThread(b);

    take_pcm=std::make_unique<Take_pcm>();
    //take_pcm->start();

    take_pcm->moveToThread(a);

    connect(c,&QThread::started,work.get(),&Worker::init);
    connect(c,&QThread::finished,work.get(),&Worker::stop);
    a->start();
    b->start();
    c->start();
    init_TextEdit();
    this->textEdit->setCursor(Qt::ArrowCursor);


    qDebug()<<"MainWindow"<<QThread::currentThreadId();

    //    connect(this,&MainWindow::play_changed,[=](bool flag){

    //    });


    connect(music,&QPushButton::toggled,this,[=](bool checked){
        emit big_clicked(checked);
    });



    connect(dir,&QPushButton::clicked,this,&Play_Widget::openfile);

    connect(this,&Play_Widget::filepath,take_pcm.get(),&Take_pcm::make_pcm);
    connect(this,&Play_Widget::filepath,work.get(),&Worker::play_pcm);
    //connect(take_pcm.get(),&Take_pcm::begin_to_play,work.get(),&Worker::play_pcm);
    connect(take_pcm.get(),&Take_pcm::data,work.get(),&Worker::receive_data);
    connect(take_pcm.get(),&Take_pcm::Position_Change,work.get(),&Worker::reset_play);

    //    connect(take_pcm.get(),&Take_pcm::begin_to_play,this,&MainWindow::_begin_to_play);
    //    connect(this,&MainWindow::begin_to_play,work.get(),&Worker::begin_play);

    //    connect(take_pcm.get(),&Take_pcm::begin_to_play,this,&MainWindow::_begin_to_play);
    //    connect(this,&MainWindow::begin_to_play,work.get(),&Worker::play_pcm);



    connect(take_pcm.get(),&Take_pcm::send_totalDuration,work.get(),&Worker::receive_totalDuration);

    connect(work.get(),&Worker::rePlay,this,[=](){
        {
            std::lock_guard<std::mutex>lock(mtx);
            this->played=true;
        }
        if(this->loop&&this->filePath.size()>0){
            rePlay(this->filePath);
        }
    });

    connect(play,&QPushButton::clicked,work.get(),&Worker::stop_play);
    connect(play,&QPushButton::clicked,[=](){
        if(!this->loop&&filePath.size()>0&&played){
            rePlay(this->filePath);
            qDebug()<<"rePlay"<<this->filePath;
        }
    });
    //    connect(take_pcm.get(),&Take_pcm::begin_take_lrc,this,&MainWindow::_begin_take_lrc);
    //    connect(this,&MainWindow::begin_take_lrc,lrc.get(),&lrc_analyze::begin_take_lrc);


    connect(take_pcm.get(),&Take_pcm::begin_take_lrc,this,&Play_Widget::_begin_take_lrc);
    connect(this,&Play_Widget::begin_take_lrc,lrc.get(),&LrcAnalyze::begin_take_lrc);

    connect(lrc.get(),&LrcAnalyze::send_lrc,work.get(),&Worker::receive_lrc);
    connect(lrc.get(),&LrcAnalyze::send_lrc,this,[=](const std::map<int, std::string> lyrics){

        Slider->setRange(0,10000);
        this->textEdit->clear();
        this->textEdit->currentLine=4;
        this->lyrics.clear();
        for(int i=0;i<5;i++){
            textEdit->append("    ");
        }
        {
            std::lock_guard<std::mutex>lock(mtx);
            for (const auto& [time, text] : lyrics){
                textEdit->append(QString::fromStdString(text));
                //qDebug()<<QString::fromStdString(text);
            }
            this->lyrics=lyrics;
        }
        for(int i=0;i<5;i++){
            textEdit->append("    ");
        }


        // 获取 QTextEdit 的光标
        QTextCursor cursor = textEdit->textCursor();

        // 选择整个文档的文本
        cursor.select(QTextCursor::Document);

        // 创建一个 QTextBlockFormat 对象
        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(Qt::AlignCenter); // 设置对齐方式为居中

        // 应用格式到选中的文本
        cursor.mergeBlockFormat(blockFormat);

        // 更新 QTextEdit 的光标位置
        textEdit->setTextCursor(cursor);


    });


    connect(work.get(),&Worker::send_lrc,this,[=](int line){
        // qDebug()<<"textEdit->currentLine"<<textEdit->currentLine<<str;
        //        QTextDocument *document = textEdit->document();

        //        // 获取指定行的文本块
        //        QTextBlock block = document->findBlockByLineNumber(textEdit->currentLine);
        //        QString lineText;
        //        // 检查是否找到有效的块
        //        if (block.isValid()) {
        //            lineText= block.text();  // 获取该行的文本
        //        }
        if(line!=textEdit->currentLine){

            textEdit->highlightLine(line);

            textEdit->scrollLines(line-textEdit->currentLine);

            qDebug()<<"line:"<<line<<"textEdit->currentLine:"<<textEdit->currentLine;

            textEdit->currentLine=line;

            update();
        }
    });

    connect(work.get(),&Worker::Stop,this,[=](){

        play->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/播放.png);"
                    "}"
                    );
    });
    connect(work.get(),&Worker::Begin,this,[=](){

        play->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/暂停.png);"
                    "}"
                    );
    });

    connect(Loop,&QPushButton::clicked,this,[=](){
        if(this->loop){
            Loop->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/随机播放.png);"
                        "}"
                        );
        }else{
            Loop->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/循环播放.png);"
                        "}"
                        );
        }
        this->loop=!this->loop;

        qDebug()<<"Loop state"<<this->loop;
    });


    connect(video,&QPushButton::toggled,this,[=](bool checked){
        if(checked){
            qDebug()<<"被选中";

            slider->show();
        }else{
            if(slider){
                slider->close();
            }
        }
    });
    connect(slider,&QSlider::valueChanged,work.get(),&Worker::Set_Volume);

    connect(take_pcm.get(),&Take_pcm::durations,[=](qint64 value){
        this->duration=static_cast<qint64>(value);
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


        int newPosition=Slider->value()*this->duration/10000000;

        emit process_Change(newPosition);
        emit set_SliderMove(false);

        Slider->setValue((newPosition*10000)/this->duration);
    });

    connect(this,&Play_Widget::process_Change,take_pcm.get(),&Take_pcm::seekToPosition);

}

void Play_Widget::rePlay(QString path){
    emit filepath(path);
    {
        std::lock_guard<std::mutex>lock(mtx);
        this->played=false;
    }

}
void Play_Widget::_begin_to_play(QString Path){
    emit begin_to_play(Path);
}

void Play_Widget::init_TextEdit(){
    this->textEdit=new LyricTextEdit(this);
    this->textEdit->setTextInteractionFlags(Qt::NoTextInteraction);//禁用交互
    this->textEdit->disableScrollBar();
    this->textEdit->setFixedSize(450,300);

    this->textEdit->viewport()->setCursor(Qt::ArrowCursor);

    QFont font = this->textEdit->font(); // 获取当前字体
    font.setPointSize(16);     // 设置字号为 16
    this->textEdit->setFont(font);       // 应用新字体
    this->textEdit->setStyleSheet("QTextEdit { background: transparent; border: none; }");
    this->textEdit->move(400, 0);
}



void Play_Widget::_begin_take_lrc(QString str){
    this->textEdit->clear();

    emit begin_take_lrc(str);
    QTextCursor cursor = textEdit->textCursor();

    // 将光标移动到文档的开始
    cursor.movePosition(QTextCursor::Start);

    //cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, 5);

    textEdit->setTextCursor(cursor);
}
void Play_Widget::openfile(){

    // 创建一个临时的 QWidget 实例，用于显示文件对话框
    QWidget dummyWidget;

    // 打开文件对话框
    QString filePath = QFileDialog::getOpenFileName(
                &dummyWidget,                   // 父窗口（可以是 nullptr）
                "Open File",                    // 对话框标题
                QDir::homePath(),               // 起始目录（可以是任意路径）
                "Audio Files (*.mp3 *.wav *.flac *.ogg);;All Files (*)"  // 文件过滤器
                );

    // 打印选中的文件路径
    if (!filePath.isEmpty()) {
        this->filePath=filePath;
        emit filepath(filePath);

        qDebug() << "Selected file:" << filePath;
    } else {
        qDebug() << "No file selected.";
    }


}
QPushButton*Play_Widget::get_dir(){
    return this->dir;
}

Play_Widget::~Play_Widget()
{

    if(a){
        a->quit();
        a->wait();
    }
    if(b){
        b->quit();
        b->wait();
    }
    if(c){
        c->quit();
        c->wait();
    }

}
