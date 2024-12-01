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
#include "rotatingcircleimage.h"
#include "pianwidget.h"

class Play_Widget : public QWidget
{
    Q_OBJECT

public:
    Play_Widget(QWidget *parent = nullptr);
    ~Play_Widget();

    QWidget* getButtonWidget(){return button_widget;};
    QSlider* getSlider(){return Slider;};


    bool isUp = false;

public slots:

    void _begin_take_lrc(QString str);
    void _play_click(QString songName);
    void _remove_click(QString songName);
    void openfile();
    void setPianWidgetEnable(bool flag);

signals:
    void signal_filepath(QString filePath);
    void signal_begin_to_play(QString path);
    void signal_begin_take_lrc(QString str);
    void signal_play_changed(bool flag);
    void signal_set_SliderMove(bool flag);
    void signal_process_Change(qint64 newPosition);
    void signal_big_clicked(bool checked);
    void signal_list_show(bool flag);
    void signal_add_song(const QString fileName,const QString path);
    void signal_play_button_click(bool flag, QString fileName);
    void signal_Next(QString songName);
    void signal_Last(QString songName);
    void signal_remove_click();
    void signal_stop_rotate(bool flag);
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
    QWidget* button_widget;
    PianWidget* pianWidget;
    QPushButton *video;
    QPushButton *play;
    QPushButton *Loop;
    QPushButton *music;
    QPushButton *mlist;
    QPushButton *last;
    QPushButton *next;
    QSlider *Slider;
    QPushButton* net;
    qint64 duration = 0;// 加载图片
    std::mutex mtx;
    QThread *a;
    QThread *b;
    QThread *c;

    bool played;
    bool loop;

protected:

    void paintEvent(QPaintEvent *event) override {
        QWidget::paintEvent(event);

        QPainter painter(this);

        if(isUp)
        {
            QLinearGradient gradient(0, 0, width(), height());

            gradient.setColorAt(0, QColor("#101D29"));
            gradient.setColorAt(1, QColor("#24435E"));

            painter.setBrush(gradient);
        }
        else
        {
            QColor backgroundColor("#FAFAFA");
            painter.fillRect(rect(), backgroundColor);
        }

        painter.drawRect(0, 0, width(), height());
    }
};
#endif // MAINWINDOW_H
