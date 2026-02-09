#ifndef MUSICLISTWIDGETNET_H
#define MUSICLISTWIDGETNET_H

#include <QObject>
#include <QWidget>
#include <QStringList>
#include <QMap>
#include "music_list_widget_net_qml.h"
#include "httprequest.h"
#include "music.h"

// 前向声明，避免循环依赖
class MainWidget;

class MusicListWidgetNet : public QWidget
{
    Q_OBJECT
public:
    explicit MusicListWidgetNet(QWidget *parent = nullptr);
    
    // 获取内部的QML列表控件
    MusicListWidgetNetQml* getListWidget() const { return listWidget; }
    
    // 清空音乐列表
    void clearList() {
        if (listWidget) {
            listWidget->clearAll();
        }
    }
    
    void on_signal_play_click(const QString name, const QString artist, const QString cover);
    void on_signal_remove_click(const QString name);
    void on_signal_play_button_click(bool flag, const QString filename);
    void on_signal_download_music(QString songName);
    void on_signal_translate_button_clicked();
signals:
    void signal_add_songlist(const QList<Music>& musicList);
    void signal_play_click(const QString songName, const QString artist, const QString cover, bool net);
    void signal_play_button_click(bool flag, const QString filename);
    void signal_last(QString songName);
    void signal_next(QString songName);
    void signal_translate_button_clicked();
    void loginRequired();  // 下载时需要登录的信号
    void addToFavorite(const QString& path, const QString& title, const QString& artist, const QString& duration);
    //void signal_netFlag
public:
    void setMainWidget(QWidget* widget) { mainWidget = widget; }
    
private:
    MusicListWidgetNetQml* listWidget;

    QMap<QString, double> song_duration;
    QMap<QString, QString> song_cover;  // 存储歌曲封面URL

    HttpRequest* request;
    QString currentSongArtist;  // 当前歌曲的艺术家
    QString currentSongCover;   // 当前歌曲的封面
    QWidget* mainWidget;  // MainWidget指针，用于检查登录状态
};

#endif // MUSICLISTWIDGETNET_H
