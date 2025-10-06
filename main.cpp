#include "play_widget.h"
#include <QFontMetrics>
#include <QDebug>
#include <QIcon>
#include <QApplication>
#include <QThread>
#include "main_widget.h"
#include "headers.h"
// 自定义日志处理函数
void customLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (msg.contains("pa_stream_begin_write, error = 无效参数"))
    {
        // 忽略这条日志
        return;
    }

    // 继续处理其他日志
    QTextStream(stderr) << msg << endl;
}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置应用程序信息，用于QSettings
    a.setOrganizationName("MusicPlayer");
    a.setApplicationName("FFmpegMusicPlayer");

    a.setWindowIcon(QIcon("qrc:/new/prefix1/icon/netease.png"));

    setlocale(LC_ALL, "chs");

    qInstallMessageHandler(customLogHandler);
    qRegisterMetaType<std::map<int, std::string>>("std::map<int, std::string>");
    QLoggingCategory::setFilterRules("qt.audio.debug=false");
    qRegisterMetaType<std::vector<std::pair<qint64,qint64>>>("std::vector<std::pair<qint64,qint64> >");

    av_register_all();
    avformat_network_init();

    auto user = User::getInstance();

    MainWidget w;

    w.show();
    w.Update_paint();

    //qDebug()<<"启动";
    return a.exec();
}

