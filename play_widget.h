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
#include "process_slider.h"
#include "controlbar.h"
#include "desk_lrc_widget.h"

class PlayWidget : public QWidget
{
    Q_OBJECT

public:
    PlayWidget(QWidget *parent = nullptr);
    ~PlayWidget();

    bool isUp = false;
    bool get_net_flag(){return play_net;};
    void set_isUp(bool flag);
    bool checkAndWarnIfPathNotExists(const QString &path);
public slots:

    void _begin_take_lrc(QString str);
    void _play_click(QString songName);
    void _remove_click(QString songName);
    void openfile();
    void setPianWidgetEnable(bool flag);

    void set_play_net(bool flag){play_net = flag;};
    void slot_play_click();
    void slot_Lrc_send_lrc(const std::map<int, std::string> lyrics);
    void slot_work_stop();
    void slot_work_play();
    void slot_desk_toggled(bool checked);
signals:
    void signal_worker_play();
    void signal_filepath(QString filePath);
    void signal_begin_to_play(QString path);
    void signal_begin_take_lrc(QString str);
    void signal_play_changed(bool flag);
    void signal_set_SliderMove(bool flag);
    void signal_process_Change(qint64 newPosition, bool back_flag);
    void signal_big_clicked(bool checked);
    void signal_list_show(bool flag);
    void signal_add_song(const QString fileName,const QString path);
    void signal_play_button_click(bool flag, QString fileName);
    void signal_Next(QString songName);
    void signal_Last(QString songName);
    void signal_remove_click();
    void signal_stop_rotate(bool flag);
    void signal_begin_net_decode();
    void signal_playState(ControlBar::State state);
    void signal_isUpChanged(bool flag);
    void signal_desk_lrc(const QString lrc_);
private:

    void init_TextEdit();
    void rePlay(QString path);

    std::shared_ptr<Worker> work;//音频转化为pcm的线程
    std::shared_ptr<LrcAnalyze> lrc;//解析歌词的线程
    std::shared_ptr<TakePcm> take_pcm;//播放pcm的线程
    std::map<int, std::string> lyrics;

    QString filePath;
    QString fileName;
    LyricTextEdit *textEdit;
    PianWidget* pianWidget;
    QPushButton *music;
    QPushButton* net;
    qint64 duration = 0;// 加载图片
    std::mutex mtx;
    QLabel* nameLabel;
    QThread *a;
    QThread *b;
    QThread *c;

    ProcessSlider* process_slider;
    ControlBar* controlBar;
    DeskLrcWidget* desk;

    bool play_net = false;
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
