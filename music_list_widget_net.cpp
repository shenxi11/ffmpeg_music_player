#include "music_list_widget_net.h"
#include "settings_manager.h"
#include "download_manager.h"
#include "httprequest.h"
#include "play_widget.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QMetaObject>

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
    connect(listWidget, &MusicListWidgetNetQml::addToFavorite,
            this, &MusicListWidgetNet::addToFavorite);
    connect(this, &MusicListWidgetNet::signal_next, listWidget, &MusicListWidgetNetQml::signal_next);
    connect(this, &MusicListWidgetNet::signal_last, listWidget, &MusicListWidgetNetQml::signal_last);

    // 连接下载管理器的信号，监听下载状态（新版带taskId）
    connect(&DownloadManager::instance(), &DownloadManager::downloadStarted,
            this, [](const QString& taskId, const QString& filename) {
        qDebug() << "[MusicListWidgetNet] Download started:" << filename << "TaskID:" << taskId;
    });
    
    connect(&DownloadManager::instance(), &DownloadManager::downloadProgress,
            this, [](const QString& taskId, const QString& filename, qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            int progress = (bytesReceived * 100) / bytesTotal;
            if (progress % 10 == 0) {  // 每10%打印一次，减少日志
                qDebug() << "[MusicListWidgetNet] Download progress:" << filename << progress << "%";
            }
        }
    });
    
    connect(&DownloadManager::instance(), &DownloadManager::downloadFinished,
            this, [](const QString& taskId, const QString& filename, const QString& savePath) {
        qDebug() << "[MusicListWidgetNet] Download completed:" << filename;
        qDebug() << "  Saved to:" << savePath;
    });
    
    connect(&DownloadManager::instance(), &DownloadManager::downloadFailed,
            this, [](const QString& taskId, const QString& filename, const QString& error) {
        qWarning() << "[MusicListWidgetNet] Download failed:" << filename;
        qWarning() << "  Error:" << error;
    });

    // 监听添加歌曲列表信号
    connect(this, &MusicListWidgetNet::signal_add_songlist, [=](const QList<Music>& musicList){
        qDebug() << "MusicListWidgetNet: Received" << musicList.size() << "songs";
        
        // 提取Music对象中的信息并传递给QML组件
        QStringList songNames;  // 歌名（从路径提取的文件名）
        QStringList relativePaths;  // 相对路径，用于API调用
        QList<double> durations;
        QStringList coverUrls;
        QStringList artists;
        
        for(const Music& music : musicList) {
            QString fullPath = music.getSongPath();
            QString relativePath;
            
            // 从完整URL中提取相对路径
            // fullPath = "http://slcdut.xyz:8080/uploads/千里之外/千里之外.mp3"
            // relativePath = "千里之外/千里之外.mp3"
            if (fullPath.startsWith("http://") || fullPath.startsWith("https://")) {
                int uploadsPos = fullPath.indexOf("/uploads/");
                if (uploadsPos != -1) {
                    relativePath = fullPath.mid(uploadsPos + 9);  // 9 = length of "/uploads/"
                } else {
                    relativePath = fullPath;  // 降级处理
                }
            } else {
                relativePath = fullPath;  // 已经是相对路径
            }
            
            songNames.append(music.getSongName());      // 歌名
            relativePaths.append(relativePath);          // 相对路径
            durations.append(static_cast<double>(music.getDuration()));
            coverUrls.append(music.getPicPath());
            artists.append(music.getSinger());
            
            // 保存到本地映射（使用相对路径作为key）
            song_duration[relativePath] = static_cast<double>(music.getDuration());
            song_cover[relativePath] = music.getPicPath();
        }
        
        listWidget->addSongList(songNames, relativePaths, durations, coverUrls, artists);
    });

    // 监听播放状态变化
    connect(this, &MusicListWidgetNet::signal_play_button_click, [=](bool flag, const QString filename){
        // 通知 listWidget 更新播放状态
        listWidget->setPlayingState(filename, flag);
    });

    request = HttpRequestPool::getInstance().getRequest();
    connect(request, &HttpRequest::signal_streamurl, this, [=](bool flag, QString file){
        emit signal_play_click(file, currentSongArtist, currentSongCover, true);
    });
}

void MusicListWidgetNet::on_signal_play_click(const QString name, const QString artist, const QString cover)
{
    double duration = song_duration[name];
    // 保存元数据，供后续使用
    currentSongArtist = artist;
    currentSongCover = cover;
    request->get_music_data(name);
}
void MusicListWidgetNet::on_signal_download_music(QString songName)
{
    // 检查登录状态
    if (mainWidget) {
        // 使用QMetaObject调用方法，避免包含main_widget.h
        bool isLoggedIn = false;
        QMetaObject::invokeMethod(mainWidget, "isUserLoggedIn", 
                                  Qt::DirectConnection,
                                  Q_RETURN_ARG(bool, isLoggedIn));
        
        if (!isLoggedIn) {
            qDebug() << "[MusicListWidgetNet] 下载需要登录，显示登录窗口";
            emit loginRequired();
            return;
        }
    }
    
    // 从设置管理器获取下载路径和选项
    QString downloadPath = SettingsManager::instance().downloadPath();
    bool downloadLyrics = SettingsManager::instance().downloadLyrics();
    QString coverUrl = song_cover.value(songName, "");  // 获取封面URL
    
    qDebug() << "[MusicListWidgetNet] Download requested for:" << songName;
    qDebug() << "  Download path:" << downloadPath;
    qDebug() << "  Download lyrics:" << downloadLyrics;
    qDebug() << "  Cover URL:" << coverUrl;
    
    request->Download(songName, downloadPath, downloadLyrics, coverUrl);
}
void MusicListWidgetNet::on_signal_remove_click(const QString name)
{

}
void MusicListWidgetNet::on_signal_play_button_click(bool flag, const QString filename)
{
    qDebug() << "[PLAY_STATE] MusicListWidgetNet::on_signal_play_button_click 收到信号, flag=" << flag << ", filename=" << filename;
    if(auto sender_ = dynamic_cast<PlayWidget*>(sender()))
    {
        qDebug() << "[PLAY_STATE] sender 是 PlayWidget, net_flag=" << sender_->get_net_flag();
        if(sender_->get_net_flag())
        {
            qDebug() << "[PLAY_STATE] 调用 listWidget->setPlayingState(" << filename << "," << flag << ")";
            listWidget->setPlayingState(filename, flag);
            emit signal_play_button_click(flag, filename);
        }
    } else {
        qDebug() << "[PLAY_STATE] sender 不是 PlayWidget";
    }
}

void MusicListWidgetNet::on_signal_translate_button_clicked()
{
    emit signal_translate_button_clicked();
}
