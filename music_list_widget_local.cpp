#include "music_list_widget_local.h"
#include "play_widget.h"
#include <QVBoxLayout>
#include "httprequest.h"
#include <QFileInfo>
#include <QDebug>

MusicListWidgetLocal::MusicListWidgetLocal(QWidget *parent) : QWidget(parent)
{
    // 直接使用 QML 版本的音乐列表（带内置的标题栏和添加按钮）
    listWidget = new MusicListWidgetQml(false, this);  // false 表示本地音乐
    
    QVBoxLayout* v_layout = new QVBoxLayout(this);
    v_layout->setContentsMargins(0, 0, 0, 0);
    v_layout->addWidget(listWidget);
    setLayout(v_layout);

    request = HttpRequestPool::getInstance().getRequest();

    // 连接 QML 的信号到外部
    connect(listWidget, &MusicListWidgetQml::signal_add_song,
            this, &MusicListWidgetLocal::on_signal_add_button_clicked);
    connect(listWidget, &MusicListWidgetQml::signal_play_button_click,
            this, &MusicListWidgetLocal::on_signal_play_click);
    connect(listWidget, &MusicListWidgetQml::signal_remove_click,
            this, &MusicListWidgetLocal::on_signal_remove_click);
    connect(this, &MusicListWidgetLocal::signal_next, listWidget, &MusicListWidgetQml::signal_next);
    connect(this, &MusicListWidgetLocal::signal_last, listWidget, &MusicListWidgetQml::signal_last);

    // 监听添加歌曲信号
    connect(this, &MusicListWidgetLocal::signal_add_song, [=](QString filename, QString path){
        request->AddMusic(path);
        listWidget->addSong(filename, path, "", "0:00", "", "-");
    });

    // 监听播放状态变化
    connect(this, &MusicListWidgetLocal::signal_play_button_click, [=](bool flag, const QString filename){
        // 通知 listWidget 更新播放状态
        listWidget->setPlayingState(filename, flag);
    });

    // 用户登录后加载音乐列表
    auto user = User::getInstance();
    connect(user, &User::signal_add_songs, this, [=](){
        qDebug() << __FUNCTION__ << "login";
        listWidget->clearAll();
        for(auto i: user->get_music_path())
        {
            QFileInfo info = QFileInfo(i);
            listWidget->addSong(info.fileName(), info.filePath(), "", "0:00", "", "-");
        }
    });
}
void MusicListWidgetLocal::on_signal_add_button_clicked()
{
    emit signal_add_button_clicked();
}
void MusicListWidgetLocal::on_signal_add_song(const QString filename, const QString path)
{
    emit signal_add_song(filename, path);
}
void MusicListWidgetLocal::on_signal_play_button_click(bool flag, const QString filename)
{
    if(auto sender_ = dynamic_cast<PlayWidget*>(sender()))
    {
        if(!sender_->get_net_flag())
            emit signal_play_button_click(flag, filename);
    }
}
void MusicListWidgetLocal::on_signal_play_click(const QString songName)
{
    emit signal_play_click(songName, false);
}
void MusicListWidgetLocal::on_signal_remove_click(const QString songeName)
{
    emit signal_remove_click(songeName);
}

void MusicListWidgetLocal::on_signal_translate_button_clicked()
{
    emit signal_translate_button_clicked();
}
