#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    ,played(false)
    ,loop(false)

{
    ui->setupUi(this);
    ui->play->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/播放_bg.png);"
                "}"
                );

    slider = new QSlider(Qt::Vertical, this);
    slider->close();

    ui->Loop->setStyleSheet(
                "QPushButton {"
                "    border-image: url(:/new/prefix1/icon/循环_bg.png);"
                "}"
                );

    //QThread*a=new QThread();
    //QThread*b=new QThread();
    //QThread*c=new QThread();

    a=new QThread();
    b=new QThread();
    c=new QThread();

    ui->Slider->setRange(0,10000);

    work=std::make_unique<Worker>();
    work->moveToThread(c);

    lrc=std::make_unique<lrc_analyze>();
    //lrc->moveToThread(b);
    //lrc.get()->moveToThread(b);

    take_pcm=std::make_unique<Take_pcm>();
    //take_pcm->start();

    take_pcm->moveToThread(a);


    a->start();
    b->start();
    c->start();
    init_TextEdit();

    qDebug()<<"MainWindow"<<QThread::currentThreadId();

//    connect(this,&MainWindow::play_changed,[=](bool flag){

//    });

    connect(ui->dir,&QPushButton::clicked,this,&MainWindow::openfile);

    connect(this,&MainWindow::filepath,take_pcm.get(),&Take_pcm::make_pcm);

    //    connect(take_pcm.get(),&Take_pcm::begin_to_play,this,&MainWindow::_begin_to_play);
    //    connect(this,&MainWindow::begin_to_play,work.get(),&Worker::begin_play);

    connect(take_pcm.get(),&Take_pcm::begin_to_play,this,&MainWindow::_begin_to_play);
    connect(this,&MainWindow::begin_to_play,work.get(),&Worker::play_pcm);



    connect(work.get(),&Worker::rePlay,this,[=](){
        {
            std::lock_guard<std::mutex>lock(mtx);
            this->played=true;
        }
        if(this->loop&&this->filePath.size()>0){
            rePlay(this->filePath);
        }
    });

    connect(ui->play,&QPushButton::clicked,work.get(),&Worker::stop_play);
    connect(ui->play,&QPushButton::clicked,[=](){
        if(!this->loop&&filePath.size()>0&&played){
            rePlay(this->filePath);
            qDebug()<<"rePlay"<<this->filePath;
        }
    });
    //    connect(take_pcm.get(),&Take_pcm::begin_take_lrc,this,&MainWindow::_begin_take_lrc);
    //    connect(this,&MainWindow::begin_take_lrc,lrc.get(),&lrc_analyze::begin_take_lrc);


    connect(take_pcm.get(),&Take_pcm::begin_take_lrc,this,&MainWindow::_begin_take_lrc);
    connect(this,&MainWindow::begin_take_lrc,lrc.get(),&lrc_analyze::begin_take_lrc);

    connect(lrc.get(),&lrc_analyze::send_lrc,work.get(),&Worker::receive_lrc);
    connect(lrc.get(),&lrc_analyze::send_lrc,this,[=](const std::map<int, std::string> lyrics){
        //        if(textEdit){
        //            textEdit->clear();
        //            delete textEdit;
        //            textEdit=nullptr;
        //        }

        this->textEdit->clear();
        this->textEdit->currentLine=5;
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

        textEdit->disableScrollBar();
    });


    connect(work.get(),&Worker::send_lrc,this,[=](QString str){
        qDebug()<<"textEdit->currentLine"<<textEdit->currentLine<<str;
        QTextDocument *document = textEdit->document();

        // 获取指定行的文本块
        QTextBlock block = document->findBlockByLineNumber(textEdit->currentLine);
        QString lineText;
        // 检查是否找到有效的块
        if (block.isValid()) {
            lineText= block.text();  // 获取该行的文本
        }
        if(str==lineText){
            textEdit->highlightLine(textEdit->currentLine);
            textEdit->scrollOneLine();
            textEdit->currentLine++;
            update();
        }
    });

    connect(work.get(),&Worker::Stop,this,[=](){

        ui->play->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/播放_bg.png);"
                    "}"
                    );
    });
    connect(work.get(),&Worker::Begin,this,[=](){

        ui->play->setStyleSheet(
                    "QPushButton {"
                    "    border-image: url(:/new/prefix1/icon/暂停1_bg.png);"
                    "}"
                    );
    });

    connect(ui->Loop,&QPushButton::clicked,this,[=](){
        if(this->loop){
            ui->Loop->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/循环_bg.png);"
                        "}"
                        );
        }else{
            ui->Loop->setStyleSheet(
                        "QPushButton {"
                        "    border-image: url(:/new/prefix1/icon/正在循环.png);"
                        "}"
                        );
        }
        this->loop=!this->loop;

        qDebug()<<"Loop state"<<this->loop;
    });


    connect(ui->video,&QPushButton::toggled,this,[=](bool checked){
        if(checked){
            qDebug()<<"被选中";
            slider->setMinimum(0);
            slider->setMaximum(100);
            slider->setValue(50);
            slider->setTickPosition(QSlider::TicksRight);  // 在滑条右侧显示刻度
            slider->setTickInterval(10);
            slider->setGeometry(ui->video->pos().x(),ui->video->pos().y()-ui->video->height()*3,ui->video->width(),ui->video->height()*3);
            slider->show();
        }else{
            if(slider){
                slider->close();
            }
        }
    });
    connect(slider,&QSlider::valueChanged,work.get(),&Worker::Set_Volume);

    connect(take_pcm.get(),&Take_pcm::durations,[=](int64_t value){
        this->duration=static_cast<qint64>(value);

    });
    connect(work.get(),&Worker::durations,[=](qint64 value){
        ui->Slider->setValue((value*10000)/this->duration);
        // qDebug()<<value<<' '<<this->duration;
    });



}

void MainWindow::rePlay(QString path){
    emit filepath(path);
    {
        std::lock_guard<std::mutex>lock(mtx);
        this->played=false;
    }

}
void MainWindow::_begin_to_play(QString Path){
    emit begin_to_play(Path);
}

void MainWindow::init_TextEdit(){
    this->textEdit=new LyricTextEdit(this);
    this->textEdit->setFixedSize(450,300);
    QFont font = this->textEdit->font(); // 获取当前字体
    font.setPointSize(16);     // 设置字号为 16
    this->textEdit->setFont(font);       // 应用新字体
    this->textEdit->setStyleSheet("QTextEdit { background: transparent; border: none; }");
    this->textEdit->move(this->width()/2-100, 0);
}



void MainWindow::_begin_take_lrc(QString str){
    this->textEdit->clear();

    emit begin_take_lrc(str);
    QTextCursor cursor = textEdit->textCursor();

    // 将光标移动到文档的开始
    cursor.movePosition(QTextCursor::Start);

    //cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, 5);

    textEdit->setTextCursor(cursor);
}
void MainWindow::openfile(){

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


MainWindow::~MainWindow()
{
    QMetaObject::invokeMethod(work.get(), "stop", Qt::QueuedConnection);

    a->quit();
    b->quit();
    c->quit();

    a->wait();
    b->wait();
    c->wait();

    delete ui;
}
