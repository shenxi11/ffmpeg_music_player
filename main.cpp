#include "mainwindow.h"
#include <QFontMetrics>
#include <QDebug>
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    int fontSize = 20;
//    int lineHeight = getLineHeight(fontSize);
//    qDebug() << "Font Size:" << fontSize << "Line Height:" << lineHeight;

    MainWindow w;
    w.show();


    return a.exec();
}

