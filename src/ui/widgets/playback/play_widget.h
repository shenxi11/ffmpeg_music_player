#ifndef PLAY_WIDGET_H
#define PLAY_WIDGET_H

#include <QWidget>
#include <QSlider>
#include <QScrollBar>
#include <QTextBlock>
#include <QVariantMap>
#include <QResizeEvent>
#include "headers.h"
// 旧版线程解码入口已下线，保留 include 位置用于历史兼容。
//#include "worker.h"
//#include "take_pcm.h"
// 音频播放核心服务（会话、解码、渲染由该服务统一调度）。
#include "AudioService.h"
#include "viewmodels/PlaybackViewModel.h"  // 播放页面 ViewModel（UI 层入口）
#include "lrc_analyze.h"
#include "lyricdisplay_qml.h"
#include "rotatingcircleimage.h"
#include "process_slider_qml.h"
#include "controlbar_qml.h"
#include "desklrc_qml.h"
#include "playlist_history_qml.h"

class PlayWidget : public QWidget
{
    Q_OBJECT
    
    // MVVM 架构说明
    Q_PROPERTY(PlaybackViewModel* playbackViewModel READ playbackViewModel CONSTANT)

public:
    PlayWidget(QWidget *parent = nullptr, QWidget *mainWidget = nullptr);
    ~PlayWidget();

    // 获取 ViewModel 指针，供 QML 绑定使用
    PlaybackViewModel* playbackViewModel() const;

    bool isUp = false;
    bool get_net_flag();
    void set_isUp(bool flag);
    bool checkAndWarnIfPathNotExists(const QString &path);
    int collapsedPlaybackHeight() const;
public slots:

    void _begin_take_lrc(QString str);
    void _play_click(QString songName);
    void _remove_click(QString songName);
    void openfile();
    void setPianWidgetEnable(bool flag);

    void set_play_net(bool flag);
    void setNetworkMetadata(const QString& artist, const QString& cover);
    void setNetworkMetadata(const QString& title, const QString& artist, const QString& cover);
    void slot_play_click();
    void slot_Lrc_send_lrc(const std::map<int, std::string> lyrics);
    void slot_work_stop();
    void slot_work_play();
    void slot_desk_toggled(bool checked);
    void slot_updateBackground(QString picPath);  // 根据封面更新播放页背景
    void slot_lyric_seek(int timeMs);  // 歌词点击跳转到指定时间
    void slot_lyric_preview(int timeMs);  // 歌词拖动时预览目标时间
    void slot_lyric_drag_start();  // 歌词开始拖动，进入预览态
    void slot_lyric_drag_end();
    void setSimilarRecommendations(const QVariantList& items);
    void clearSimilarRecommendations();
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
    void signal_similarSongSelected(const QVariantMap& item);
private:
    void updateAdaptiveLayout();

    void init_LyricDisplay();
    void rePlay(QString path);

    // 播放页内部遵循“会话驱动 + UI被动同步”模式：
    // 1) AudioSession 提供时间轴与状态；2) PlayWidget 只负责展示与交互回传。
    // 这样可以减少播放状态在多列表间分叉。
    
    // MVVM 架构说明
    PlaybackViewModel* m_playbackViewModel;  // 播放页面 ViewModel（UI 层入口）
    
    // 当前音频会话句柄，切歌时会替换并重新绑定信号。
    AudioSession* currentSession;  // 当前播放会话（用于歌词与进度同步）
    
    std::shared_ptr<LrcAnalyze> lrc;// 歌词解析模块
    std::map<int, std::string> lyrics;

    QString filePath;
    QString fileName;
    QString currentSongTitle;   // 当前显示的歌曲标题
    QString currentSongArtist;  // 当前显示的歌手名
    QString networkSongArtist;  // 在线歌曲元数据中的歌手名
    QString networkSongCover;   // 在线歌曲元数据中的封面地址
    
    LyricDisplayQml *lyricDisplay;
    QPushButton *music;
    QPushButton* net;
    qint64 duration = 0;  // 当前曲目时长（微秒）
    std::mutex mtx;
    QLabel* nameLabel;
    QLabel* backgroundLabel;  // 背景图层（用于封面模糊或默认渐变）
    RotatingCircleImage* rotatingCircle;  // 唱片旋转控件
    QWidget* rotatingCircleHost = nullptr;
    
    // 歌词滚动信号连接，切歌时需断开旧连接防止重复触发。
    QMetaObject::Connection lyricUpdateConnection;
    // 历史线程字段已废弃，保留注释以便追踪旧实现。
    //QThread *a;
    //QThread *b;
    //QThread *c;
    // 歌词解析任务使用独立线程，避免阻塞 UI 渲染。
    
    // 解析线程与播放进度连接句柄。
    QThread *b;  // 歌词解析线程
    QMetaObject::Connection positionUpdateConnection;  // 播放位置更新连接
    qint64 lastSeekPosition;  // 最近一次 seek 的目标位置（微秒）

    ProcessSliderQml* process_slider;
    ProcessSliderQml* controlBar;  // controlBar 当前复用 process_slider 组件
    DeskLrcQml* desk;
    PlaylistHistoryQml* playlistHistory;  // 播放历史列表组件

    bool play_net = false;
protected:
    void resizeEvent(QResizeEvent *event) override;

    void paintEvent(QPaintEvent *event) override {
        QWidget::paintEvent(event);

        QPainter painter(this);

        if(isUp)
        {
            // 展开态优先使用封面背景，没有封面时退化为浅色渐变。
            if (!backgroundLabel->pixmap() || backgroundLabel->pixmap()->isNull()) {
                // 默认背景渐变，避免纯白闪烁。
                QLinearGradient gradient(0, 0, width(), height());
                gradient.setColorAt(0, QColor("#F5F5F7"));   // 设置浅色渐变背景
                gradient.setColorAt(0.5, QColor("#FAFAFA")); // 设置浅色渐变背景
                gradient.setColorAt(1, QColor("#F0F0F2"));   // 设置浅色渐变背景
                painter.setBrush(gradient);
                painter.drawRect(0, 0, width(), height());
            } else {
                // 使用当前背景图覆盖整个播放页。
                painter.drawPixmap(rect(), *backgroundLabel->pixmap());
            }
        }
        else
        {
            // 收起态统一使用浅色渐变背景。
            QLinearGradient gradient(0, 0, width(), height());
            gradient.setColorAt(0, QColor("#F5F5F7"));   // 设置浅色渐变背景
            gradient.setColorAt(0.5, QColor("#FAFAFA")); // 设置浅色渐变背景
            gradient.setColorAt(1, QColor("#F0F0F2"));   // 设置浅色渐变背景
            painter.setBrush(gradient);
            painter.drawRect(rect());
        }
    }
};
#endif // MAINWINDOW_H

