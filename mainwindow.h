#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QWidget>
#include<QSlider>
#include<QScrollBar>
#include<QTextBlock>
#include"headers.h"
#include"worker.h"
#include"lrc_analyze.h"
#include"take_pcm.h"
#include"lyrictextedit.h"
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

    void play_changed(bool flag);
private:
    Ui::MainWindow *ui;

    void rePlay(QString path);

    std::unique_ptr<Worker>work;//音频转化为pcm的线程

    std::unique_ptr<lrc_analyze>lrc;//解析歌词的线程

    std::unique_ptr<Take_pcm>take_pcm;//播放pcm的线程


    std::map<QString,int>lrc_check;

    std::map<int, std::string> lyrics;

    QString filePath;

    LyricTextEdit*textEdit;

    QSlider *slider;

    qint64 duration=0;

    void init_TextEdit();

    std::mutex mtx;

    QThread*a;
    QThread*b;
    QThread*c;

    bool played;

    bool loop;
};
#endif // MAINWINDOW_H
