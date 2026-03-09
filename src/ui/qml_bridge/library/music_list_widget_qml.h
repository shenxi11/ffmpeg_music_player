#ifndef MUSIC_LIST_WIDGET_QML_H
#define MUSIC_LIST_WIDGET_QML_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlContext>
#include <QWidget>
#include <QMetaObject>
#include <QVariant>

class MusicListWidgetQml : public QQuickWidget
{
    Q_OBJECT
public:
    explicit MusicListWidgetQml(bool isNetMusic = false, QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        // 加载本地/下载通用音乐列表 QML。
        setSource(QUrl("qrc:/qml/components/library/MusicListWidget.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        // 根对象用于设置模式并转发列表事件。
        QQuickItem* root = rootObject();
        if (root) {
            root->setProperty("isNetMusic", isNetMusic);
            
            // 桥接播放、删除、下载、添加操作信号。
            connect(root, SIGNAL(playRequested(QString)), 
                    this, SIGNAL(signal_play_button_click(QString)));
            connect(root, SIGNAL(removeRequested(QString)), 
                    this, SIGNAL(signal_remove_click(QString)));
            connect(root, SIGNAL(downloadRequested(QString)), 
                    this, SIGNAL(signal_download_click(QString)));
            connect(root, SIGNAL(addButtonClicked()), 
                    this, SLOT(on_add_button_clicked()));
        }
        connect(this, &MusicListWidgetQml::signal_next, this, &MusicListWidgetQml::playNext);
        connect(this, &MusicListWidgetQml::signal_last, this, &MusicListWidgetQml::playLast);

    }

    // 向列表追加单首歌曲及扩展元数据。
    void addSong(const QString& songName, const QString& filePath, 
                 const QString& artist = "", const QString& duration = "0:00",
                 const QString& cover = "", const QString& fileSize = "-")
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "addSong",
                Q_ARG(QVariant, songName),
                Q_ARG(QVariant, filePath),
                Q_ARG(QVariant, artist),
                Q_ARG(QVariant, duration),
                Q_ARG(QVariant, cover),
                Q_ARG(QVariant, fileSize));
        }
    }

    // 按文件路径删除列表项。
    void removeSong(const QString& filePath)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "removeSong",
                Q_ARG(QVariant, filePath));
        }
    }

    // 清空列表全部歌曲。
    void clearAll()
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "clearAll");
        }
    }

    // 获取当前列表项数量。
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

    // 同步单曲播放状态到 QML 列表。
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
    
    // 更新封面与时长等补充信息。
    void updateSongMetadata(const QString& filePath, const QString& coverUrl, const QString& duration)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "updateSongMetadata",
                Q_ARG(QVariant, filePath),
                Q_ARG(QVariant, coverUrl),
                Q_ARG(QVariant, duration));
        }
    }
signals:
    void signal_play_button_click(QString path);
    void signal_remove_click(QString path);
    void signal_download_click(QString path);
    void signal_add_song();
    void signal_next(QString songName);
    void signal_last(QString songName);
private slots:
    void on_add_button_clicked()
    {
        emit signal_add_song();
    }
};

#endif // MUSIC_LIST_WIDGET_QML_H

