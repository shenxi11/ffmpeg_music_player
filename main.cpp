#include "play_widget.h"
#include <QFontMetrics>
#include <QDebug>
#include <QIcon>
#include <QApplication>
#include <QThread>
#include "main_widget.h"
#include "headers.h"
#include "cplaywidget.h"
#include "movie_decoder.h"
#include "translate_widget.h"
// 自定义日志处理函数
void customLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (msg.contains("pa_stream_begin_write, error = 无效参数"))
    {
        return;
    }

    QTextStream(stderr) << msg << endl;
}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setWindowIcon(QIcon("qrc:/new/prefix1/icon/netease.png"));

    setlocale(LC_ALL, "chs");

    qInstallMessageHandler(customLogHandler);
    qRegisterMetaType<std::map<int, std::string>>("std::map<int, std::string>");
    QLoggingCategory::setFilterRules("qt.audio.debug=false");
    qRegisterMetaType<std::vector<std::pair<qint64,qint64>>>("std::vector<std::pair<qint64,qint64> >");

    auto user = User::getInstance();

    MainWidget *w = new MainWidget;

    w->show();
    w->Update_paint();

    QObject::connect(w, &MainWidget::signal_close, [=](){
        w->deleteLater();
        exit(0);
    });
    qDebug()<<"启动";

//    MovieDecoder* decoder = new MovieDecoder;
//    QThread* c = new QThread;
//    decoder->moveToThread(c);
//    c->start();

//    CPlayWidget w;
//    w.show();


//    QObject::connect(decoder, &MovieDecoder::signal_resize, &w, &CPlayWidget::slot_resize);
//    QObject::connect(decoder, &MovieDecoder::signal_resize, [&w](int width, int height){
//        w.resize(width, height);
//    });
//    QObject::connect(decoder, &MovieDecoder::signal_frameDecoded, &w, &CPlayWidget::slot_receiveFrame);

//    if(decoder->open("/home/shen/shared/video/视频.mp4"))
//        decoder->start();

//    std::thread thread_ = std::thread(&CPlayWidget::thread_func, &w);

//    TranslateWidget t;
//    t.show();


    return a.exec();
}

