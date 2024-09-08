#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QWidget>
#include"headers.h"
#include"worker.h"
#include"lrc_analyze.h"
#include"take_pcm.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:

    void _begin_to_play(QString path);

    void _begin_take_lrc(QString str);

private slots:

    void openfile();

signals:
    void filepath(QString filepath);

    void begin_to_play(QString path);

    void begin_take_lrc(QString str);
private:
    Ui::MainWindow *ui;

    std::unique_ptr<Worker>work;//音频转化为pcm的线程

    std::unique_ptr<lrc_analyze>lrc;//解析歌词的线程

    std::unique_ptr<Take_pcm>take_pcm;//播放pcm的线程

    QThreadPool *threadPool;

};
#endif // MAINWINDOW_H
