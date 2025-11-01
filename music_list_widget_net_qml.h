#ifndef MUSIC_LIST_WIDGET_NET_QML_H
#define MUSIC_LIST_WIDGET_NET_QML_H

#include <QQuickWidget>
#include <QQmlContext>
#include <QWidget>
#include <QQuickItem>
#include <QDebug>

class MusicListWidgetNetQml : public QQuickWidget
{
    Q_OBJECT
public:
    explicit MusicListWidgetNetQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        // 设置 QML 源文件
        setSource(QUrl("qrc:/qml/components/MusicListWidgetNet.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        // 连接信号
        QQuickItem* root = rootObject();
        if (root) {
            // 连接 QML 信号到 C++ 信号
            connect(root, SIGNAL(playRequested(QString)), 
                    this, SIGNAL(signal_play_click(QString)));
            connect(root, SIGNAL(removeRequested(QString)), 
                    this, SIGNAL(signal_remove_click(QString)));
            connect(root, SIGNAL(downloadRequested(QString)), 
                    this, SIGNAL(signal_download_click(QString)));
            connect(root, SIGNAL(chooseDownloadDir()), 
                    this, SIGNAL(signal_choose_download_dir()));
        }
        connect(this, &MusicListWidgetNetQml::signal_next, this, &MusicListWidgetNetQml::playNext);
        connect(this, &MusicListWidgetNetQml::signal_last, this, &MusicListWidgetNetQml::playLast);

    }

    // 添加歌曲到列表
    void addSong(const QString& songName, const QString& filePath, 
                 const QString& artist = "", const QString& duration = "0:00",
                 const QString& cover = "")
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "addSong",
                Q_ARG(QVariant, songName),
                Q_ARG(QVariant, filePath),
                Q_ARG(QVariant, artist),
                Q_ARG(QVariant, duration),
                Q_ARG(QVariant, cover));
        }
    }

    // 批量添加歌曲列表
    void addSongList(const QStringList& songNames, const QList<double>& durations)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QVariantList nameList;
            QVariantList durationList;
            
            for (const QString& name : songNames) {
                nameList.append(name);
            }
            
            for (double duration : durations) {
                int minutes = static_cast<int>(duration) / 60;
                int seconds = static_cast<int>(duration) % 60;
                QString formattedDuration = QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
                durationList.append(formattedDuration);
                qDebug() << "Adding song with duration:" << duration << "seconds -> formatted:" << formattedDuration;
            }
            
            QMetaObject::invokeMethod(root, "addSongList",
                Q_ARG(QVariant, nameList),
                Q_ARG(QVariant, durationList));
        }
    }

    // 从列表中移除歌曲
    void removeSong(const QString& filePath)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "removeSong",
                Q_ARG(QVariant, filePath));
        }
    }

    // 清空列表
    void clearAll()
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "clearAll");
        }
    }

    // 获取歌曲数量
    int getCount()
    {
        QQuickItem* root = rootObject();
        if (root) {
            QVariant result;
            QMetaObject::invokeMethod(root, "getCount", 
                Q_RETURN_ARG(QVariant, result));
            return result.toInt();
        }
        return 0;
    }

    // 设置下载路径
    void setDownloadDir(const QString& dir)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setDownloadDir",
                Q_ARG(QVariant, dir));
        }
    }

    // 设置播放状态
    void setPlayingState(const QString& filePath, bool playing)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setPlayingState",
                Q_ARG(QVariant, filePath),
                Q_ARG(QVariant, playing));
        }
    }

    void playNext(const QString& songName){
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "playNext",
                                      Q_ARG(QVariant, songName));
        }
    }
    void playLast(const QString& songName){
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "playLast",
                                      Q_ARG(QVariant, songName));
        }
    }
signals:
    void signal_play_click(QString path);
    void signal_remove_click(QString path);
    void signal_download_click(QString path);
    void signal_choose_download_dir();
    void signal_next(QString songName);
     void signal_last(QString songName);
};

#endif // MUSIC_LIST_WIDGET_NET_QML_H
