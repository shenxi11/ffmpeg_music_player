#ifndef PLAY_WIDGET_H
#define PLAY_WIDGET_H

#include <QWidget>
#include <QSlider>
#include <QScrollBar>
#include <QTextBlock>
#include "headers.h"
// ========== 旧架构（已注释，保留供参考） ==========
//#include "worker.h"
//#include "take_pcm.h"
// ========== 新架构 ==========
#include "AudioService.h"
#include "lrc_analyze.h"
#include "lyricdisplay_qml.h"
#include "httprequest.h"
#include "rotatingcircleimage.h"
#include "process_slider_qml.h"
#include "controlbar_qml.h"
#include "desklrc_qml.h"
#include "playlist_history_qml.h"

class PlayWidget : public QWidget
{
    Q_OBJECT

public:
    PlayWidget(QWidget *parent = nullptr, QWidget *mainWidget = nullptr);
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
    void signal_metadata_updated(QString filePath, QString coverUrl, QString duration);
private:

    void init_LyricDisplay();
    void rePlay(QString path);

    // ========== 旧架构（已注释，保留供参考） ==========
    //std::shared_ptr<Worker> work;//音频转化为pcm的线程
    //std::shared_ptr<TakePcm> take_pcm;//播放pcm的线程
    
    // ========== 新架构 ==========
    AudioService* audioService;  // 音频服务（单例）
    AudioSession* currentSession;  // 当前播放会话
    
    std::shared_ptr<LrcAnalyze> lrc;//解析歌词的线程
    std::map<int, std::string> lyrics;

    QString filePath;
    QString fileName;
    QString currentSongTitle;   // 当前歌曲标题
    QString currentSongArtist;  // 当前歌曲艺术家
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
    // ========== 旧架构线程（已注释） ==========
    //QThread *a;
    //QThread *b;
    //QThread *c;
    //QMetaObject::Connection durationsConnection; // 保存 Worker::durations 连接句柄
    
    // ========== 新架构 ==========
    QThread *b;  // 歌词解析线程（保留）
    QMetaObject::Connection positionUpdateConnection;  // 位置更新连接
    qint64 lastSeekPosition;  // 记录最后一次跳转位置

    ProcessSliderQml* process_slider;
    ProcessSliderQml* controlBar;  // controlBar 现在指向 process_slider
    DeskLrcQml* desk;
    PlaylistHistoryQml* playlistHistory;  // 播放历史列表

    bool play_net = false;
protected:

    void paintEvent(QPaintEvent *event) override {
        QWidget::paintEvent(event);

        QPainter painter(this);

        if(isUp)
        {
            // 展开状态：绘制模糊的专辑封面背景
            if (!backgroundLabel->pixmap() || backgroundLabel->pixmap()->isNull()) {
                // 如果还没有加载专辑封面，使用QQ音乐风格的浅灰白色渐变
                QLinearGradient gradient(0, 0, width(), height());
                gradient.setColorAt(0, QColor("#F5F5F7"));   // 浅灰白色
                gradient.setColorAt(0.5, QColor("#FAFAFA")); // 几乎白色
                gradient.setColorAt(1, QColor("#F0F0F2"));   // 浅灰色
                painter.setBrush(gradient);
                painter.drawRect(0, 0, width(), height());
            } else {
                // 绘制模糊的专辑封面
                painter.drawPixmap(0, 0, *backgroundLabel->pixmap());
            }
        }
        else
        {
            // 收起状态：使用QQ音乐风格的浅灰白色渐变
            QLinearGradient gradient(0, 0, width(), height());
            gradient.setColorAt(0, QColor("#F5F5F7"));   // 浅灰白色
            gradient.setColorAt(0.5, QColor("#FAFAFA")); // 几乎白色
            gradient.setColorAt(1, QColor("#F0F0F2"));   // 浅灰色
            painter.setBrush(gradient);
            painter.drawRect(rect());
        }
    }
};
#endif // MAINWINDOW_H
