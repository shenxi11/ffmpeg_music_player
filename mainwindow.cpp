#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)

{
    ui->setupUi(this);

    work=std::make_unique<Worker>();
    work->start();

    lrc=std::make_unique<lrc_analyze>();
    lrc->start();

    take_pcm=std::make_unique<Take_pcm>();
    take_pcm->start();

    QThreadPool *threadPool = QThreadPool::globalInstance();
       qDebug() << "活动线程数:" << threadPool->activeThreadCount();

    connect(ui->pushButton,&QPushButton::clicked,this,&MainWindow::openfile);

    connect(this,&MainWindow::filepath,take_pcm.get(),&Take_pcm::make_pcm);

    connect(take_pcm.get(),&Take_pcm::begin_to_play,this,&MainWindow::_begin_to_play,Qt::QueuedConnection);
    connect(this,&MainWindow::begin_to_play,work.get(),&Worker::begin_play,Qt::QueuedConnection);

    connect(ui->pushButton_2,&QPushButton::clicked,work.get(),&Worker::stop_play,Qt::QueuedConnection);

    connect(take_pcm.get(),&Take_pcm::begin_take_lrc,this,&MainWindow::_begin_take_lrc,Qt::QueuedConnection);
    connect(this,&MainWindow::begin_take_lrc,lrc.get(),&lrc_analyze::begin_take_lrc);


    connect(lrc.get(),&lrc_analyze::send_lrc,work.get(),&Worker::receive_lrc);

    connect(work.get(),&Worker::send_lrc,this,[=](QString lrc_str){
        ui->label->setText(lrc_str);//显示歌词
    });
}
void MainWindow::_begin_to_play(QString Path){
    emit begin_to_play(Path);
}

void MainWindow::_begin_take_lrc(QString str){
    emit begin_take_lrc(str);
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

        emit filepath(filePath);

        qDebug() << "Selected file:" << filePath;
    } else {
        qDebug() << "No file selected.";
    }


}


MainWindow::~MainWindow()
{

    delete ui;
}
