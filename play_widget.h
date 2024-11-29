#ifndef PLAY_WIDGET_H
#define PLAY_WIDGET_H

#include <QWidget>
#include <QSlider>
#include <QScrollBar>
#include <QTextBlock>
#include "headers.h"
#include "worker.h"
#include "lrc_analyze.h"
#include "take_pcm.h"
#include "lyrictextedit.h"
#include "httprequest.h"

class Play_Widget : public QWidget
{
    Q_OBJECT

public:
    Play_Widget(QWidget *parent = nullptr);
    ~Play_Widget();

public slots:

    //    void _begin_to_play(QString path);

    void _begin_take_lrc(QString str);

    //    void _play_list_music(QString path);

    void _play_click(QString songName);

    void _remove_click(QString songName);

    void openfile();

signals:
    void filepath(QString filePath);

    void begin_to_play(QString path);

    void begin_take_lrc(QString str);

    void play_changed(bool flag);

    void set_SliderMove(bool flag);

    void process_Change(qint64 newPosition);

    void big_clicked(bool checked);

    void list_show(bool flag);

    void add_song(const QString fileName,const QString path);

    void play_button_click(bool flag, QString fileName);

    void Next(QString songName);

    void Last(QString songName);

    void remove_click();
private:

    void init_TextEdit();

    void rePlay(QString path);

    std::unique_ptr<Worker> work;//音频转化为pcm的线程

    std::unique_ptr<LrcAnalyze> lrc;//解析歌词的线程

    std::unique_ptr<Take_pcm> take_pcm;//播放pcm的线程


    std::map<int, std::string> lyrics;

    HttpRequest* request;

    QString filePath;
    QString fileName;

    LyricTextEdit *textEdit;

    QSlider *slider;

    QPushButton *video;

    QPushButton *play;

    QPushButton *Loop;

    QPushButton *music;

    QPushButton *mlist;

    QPushButton *last;

    QPushButton *next;

    QSlider *Slider;

    QPushButton* net;

    qint64 duration = 0;


    std::mutex mtx;

    QThread *a;
    QThread *b;
    QThread *c;

    bool played;
    bool loop;

protected:

    void paintEvent(QPaintEvent *event) override {
        QWidget::paintEvent(event);

        // 创建画笔对象，用于在窗口上绘图
        QPainter painter(this);

        QLinearGradient gradient(0, 0, width(), height());

        // 设置渐变的起始和结束颜色
        gradient.setColorAt(0, QColor("#101D29"));  // 起始颜色
        gradient.setColorAt(1, QColor("#24435E"));  // 结束颜色

        // 设置渐变为填充色
        painter.setBrush(gradient);

        // 绘制渐变背景
        painter.drawRect(0, 0, width(), height());
    }
};
#endif // MAINWINDOW_H
