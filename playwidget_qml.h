#ifndef PLAYWIDGET_QML_H
#define PLAYWIDGET_QML_H

#include <QObject>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <memory>
#include "worker.h"
#include "lrc_analyze.h"
#include "take_pcm.h"
#include "process_slider_qml.h"
#include "desk_lrc_widget.h"

class PlayWidgetQml : public QQuickWidget
{
    Q_OBJECT

public:
    explicit PlayWidgetQml(QWidget *parent = nullptr);
    ~PlayWidgetQml();

    // 公共属性
    bool isUp = false;
    bool get_net_flag() { return play_net; }
    void set_isUp(bool flag);
    bool checkAndWarnIfPathNotExists(const QString &path);

public slots:
    void _begin_take_lrc(QString str);
    void _play_click(QString songName);
    void _remove_click(QString songName);
    void openfile();
    void setPianWidgetEnable(bool flag);
    void set_play_net(bool flag) { 
        play_net = flag; 
        emit signal_netFlagChanged(flag); 
    }
    void slot_play_click();
    void slot_Lrc_send_lrc(const std::map<int, std::string> lyrics);
    void slot_work_stop();
    void slot_work_play();
    void slot_desk_toggled(bool checked);
    void slot_updateBackground(QString picPath);

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
    void signal_add_song(const QString fileName, const QString path);
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

private slots:
    void onSliderMoved(int seconds);
    void onUpClicked();

private:
    void initializeComponents();
    void setupConnections();
    void setupQmlConnections();
    void rePlay(QString path);

    // 业务逻辑组件
    std::shared_ptr<Worker> work;
    std::shared_ptr<LrcAnalyze> lrc;
    std::shared_ptr<TakePcm> take_pcm;
    std::map<int, std::string> lyrics;

    // 线程管理
    QThread *workerThread;
    QThread *lrcThread;
    QThread *pcmThread;

    // 状态变量
    QString filePath;
    QString fileName;
    qint64 duration = 0;
    std::mutex mtx;
    bool play_net = false;
    QMetaObject::Connection durationsConnection;

    // 外部组件引用
    DeskLrcWidget* desk;

    // QML 接口方法
    Q_INVOKABLE void setIsUpQml(bool flag);
    Q_INVOKABLE void setBackgroundImageQml(const QString &imagePath);
    Q_INVOKABLE void setSongNameQml(const QString &name);
    Q_INVOKABLE void setLyricsQml(const QVariantList &lyricsData);
    Q_INVOKABLE void highlightLyricLineQml(int lineNumber);
    Q_INVOKABLE void setPlayStateQml(int state);
    Q_INVOKABLE void setPositionQml(double position);
    Q_INVOKABLE void setDurationQml(double duration);
    Q_INVOKABLE void startRotationQml();
    Q_INVOKABLE void stopRotationQml();
};

#endif // PLAYWIDGET_QML_H