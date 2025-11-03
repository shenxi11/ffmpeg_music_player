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
#include "lyricdisplay_qml.h"
#include "httprequest.h"
#include "rotatingcircleimage.h"
#include "process_slider_qml.h"
#include "controlbar_qml.h"
#include "desklrc_qml.h"

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

    void set_play_net(bool flag){play_net = flag; emit signal_netFlagChanged(flag);};
    void slot_play_click();
    void slot_Lrc_send_lrc(const std::map<int, std::string> lyrics);
    void slot_work_stop();
    void slot_work_play();
    void slot_desk_toggled(bool checked);
    void slot_updateBackground(QString picPath);  // 更新背景图片（模糊效果）
    void slot_lyric_seek(int timeMs);  // 歌词点击跳转
    void slot_lyric_preview(int timeMs);  // 歌词拖拽预览
    void slot_lyric_drag_start();  // 歌词拖拽开始
    void slot_lyric_drag_end();
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
    void signal_Next(QString songName, bool net_flag);
    void signal_Last(QString songName, bool net_flag);
    void signal_remove_click();
    void signal_stop_rotate(bool flag);
    void signal_begin_net_decode();
    void signal_playState(ProcessSliderQml::State state);
    void signal_isUpChanged(bool flag);
    void signal_desk_lrc(const QString lrc_);
    void signal_netFlagChanged(bool net_flag);
private:

    void init_LyricDisplay();
    void rePlay(QString path);

    std::shared_ptr<Worker> work;//音频转化为pcm的线程
    std::shared_ptr<LrcAnalyze> lrc;//解析歌词的线程
    std::shared_ptr<TakePcm> take_pcm;//播放pcm的线程
    std::map<int, std::string> lyrics;

    QString filePath;
    QString fileName;
    LyricDisplayQml *lyricDisplay;
    QPushButton *music;
    QPushButton* net;
    qint64 duration = 0;// 加载图片
    std::mutex mtx;
    QLabel* nameLabel;
    QLabel* backgroundLabel;  // 背景图片标签（用于显示模糊的专辑封面）
    RotatingCircleImage* rotatingCircle;  // 旋转唱片
    
    // 歌词更新信号连接管理
    QMetaObject::Connection lyricUpdateConnection;
    QThread *a;
    QThread *b;
    QThread *c;
    QMetaObject::Connection durationsConnection; // 保存 Worker::durations 连接句柄

    ProcessSliderQml* process_slider;
    ProcessSliderQml* controlBar;  // controlBar 现在指向 process_slider
    DeskLrcQml* desk;

    bool play_net = false;
protected:

    void paintEvent(QPaintEvent *event) override {
        QWidget::paintEvent(event);

        QPainter painter(this);

        if(isUp)
        {
            // 展开状态：绘制模糊的专辑封面背景
            if (!backgroundLabel->pixmap() || backgroundLabel->pixmap()->isNull()) {
                // 如果还没有加载专辑封面，使用渐变色作为后备
                QLinearGradient gradient(0, 0, width(), height());
                gradient.setColorAt(0, QColor("#101D29"));
                gradient.setColorAt(1, QColor("#24435E"));
                painter.setBrush(gradient);
                painter.drawRect(0, 0, width(), height());
            } else {
                // 绘制模糊的专辑封面
                painter.drawPixmap(0, 0, *backgroundLabel->pixmap());
            }
        }
        else
        {
            // 收起状态：浅色背景
            QColor backgroundColor("#FAFAFA");
            painter.fillRect(rect(), backgroundColor);
        }
    }
};
#endif // MAINWINDOW_H
