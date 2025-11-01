#include "music_list_widget_net.h"
#include "httprequest.h"
#include "play_widget.h"
#include "httprequest.h"
#include <QVBoxLayout>
#include <QDebug>

MusicListWidgetNet::MusicListWidgetNet(QWidget *parent) : QWidget(parent)
{
    // 直接使用 QML 版本的在线音乐列表
    listWidget = new MusicListWidgetNetQml(this);
    
    QVBoxLayout* v_layout = new QVBoxLayout(this);
    v_layout->setContentsMargins(0, 0, 0, 0);
    v_layout->addWidget(listWidget);
    setLayout(v_layout);

    // 连接 QML 信号到外部
    connect(listWidget, &MusicListWidgetNetQml::signal_play_click,
            this, &MusicListWidgetNet::on_signal_play_click);
    connect(listWidget, &MusicListWidgetNetQml::signal_remove_click,
            this, &MusicListWidgetNet::on_signal_remove_click);
    connect(listWidget, &MusicListWidgetNetQml::signal_download_click,
            this, &MusicListWidgetNet::on_signal_download_music);
    connect(listWidget, &MusicListWidgetNetQml::signal_choose_download_dir,
            this, &MusicListWidgetNet::signal_choose_download_dir);
    connect(this, &MusicListWidgetNet::signal_next, listWidget, &MusicListWidgetNetQml::signal_next);
    connect(this, &MusicListWidgetNet::signal_last, listWidget, &MusicListWidgetNetQml::signal_last);

    // 监听添加歌曲列表信号
    connect(this, &MusicListWidgetNet::signal_add_songlist, [=](const QStringList songList, const QList<double> duration){
        qDebug() << "MusicListWidgetNet: Received" << songList.size() << "songs with durations:" << duration;
        listWidget->addSongList(songList, duration);
        for(int i = 0; i < songList.size(); i++)
        {
            song_duration[songList.at(i)] = duration.at(i);
        }
    });

    // 监听播放状态变化
    connect(this, &MusicListWidgetNet::signal_play_button_click, [=](bool flag, const QString filename){
        // 通知 listWidget 更新播放状态
        listWidget->setPlayingState(filename, flag);
    });

    request = HttpRequestPool::getInstance().getRequest();
    connect(request, &HttpRequest::signal_streamurl, this, [=](bool flag, QString file){
        emit signal_play_click(file, true);
    });
}

void MusicListWidgetNet::on_signal_play_click(const QString name)
{
    double duration = song_duration[name];
    request->get_music_data(name);
}
void MusicListWidgetNet::on_signal_download_music(QString songName)
{
    request->Download(songName, down_dir);

}
void MusicListWidgetNet::on_signal_remove_click(const QString name)
{

}
void MusicListWidgetNet::on_signal_set_down_dir(QString down_dir)
{
    listWidget->setDownloadDir(down_dir);
    this->down_dir = down_dir;
}
void MusicListWidgetNet::on_signal_play_button_click(bool flag, const QString filename)
{
    if(auto sender_ = dynamic_cast<PlayWidget*>(sender()))
    {
        if(sender_->get_net_flag())

            emit signal_play_button_click(flag, filename);
    }
}

void MusicListWidgetNet::on_signal_translate_button_clicked()
{
    emit signal_translate_button_clicked();
}
