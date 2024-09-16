#include "mainwindow.h"
#include <QFontMetrics>
#include <QDebug>
#include <QApplication>
#include"test_widget.h"
// 自定义日志处理函数
void customLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (msg.contains("pa_stream_begin_write, error = 无效参数")) {
        // 忽略这条日志
        return;
    }

    // 继续处理其他日志
    QTextStream(stderr) << msg << endl;
}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //    int fontSize = 20;
    //    int lineHeight = getLineHeight(fontSize);
    //    qDebug() << "Font Size:" << fontSize << "Line Height:" << lineHeight;

    qInstallMessageHandler(customLogHandler);
    qRegisterMetaType<std::map<int, std::string>>("std::map<int, std::string>");
    QLoggingCategory::setFilterRules("qt.audio.debug=false");
    qRegisterMetaType<std::vector<std::pair<qint64,qint64> >>("std::vector<std::pair<qint64,qint64> >");
    MainWindow w;
    w.show();



    qDebug()<<"启动";
    return a.exec();
}

