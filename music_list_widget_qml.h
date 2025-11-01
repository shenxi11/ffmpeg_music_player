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
        // 设置 QML 源文件
        setSource(QUrl("qrc:/qml/components/MusicListWidget.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        // 设置是否为网络音乐
        QQuickItem* root = rootObject();
        if (root) {
            root->setProperty("isNetMusic", isNetMusic);
            
            // 连接 QML 信号到 C++ 信号
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

    // 添加歌曲到列表
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
