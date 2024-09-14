#include "mainwindow.h"
#include <QFontMetrics>
#include <QDebug>
#include <QApplication>
#include"test_widget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //    int fontSize = 20;
    //    int lineHeight = getLineHeight(fontSize);
    //    qDebug() << "Font Size:" << fontSize << "Line Height:" << lineHeight;

    qRegisterMetaType<std::map<int, std::string>>("std::map<int, std::string>");

    qRegisterMetaType<std::vector<std::pair<qint64,qint64> >>("std::vector<std::pair<qint64,qint64> >");
    MainWindow w;
    w.show();



    qDebug()<<"启动";
    return a.exec();
}

