#ifndef PLAYLIST_HISTORY_QML_H
#define PLAYLIST_HISTORY_QML_H

#include <QObject>
#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlContext>
#include <QDebug>

class PlaylistHistoryQml : public QQuickWidget
{
    Q_OBJECT
public:
    explicit PlaylistHistoryQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        setSource(QUrl("qrc:/qml/components/playback/PlaylistHistory.qml"));
        
        // 允许透明背景以配合圆角样式。
        setAttribute(Qt::WA_TranslucentBackground);
        
        qDebug() << "PlaylistHistoryQml: Created, root object:" << (rootObject() != nullptr);
        
        // 获取根对象并连接播放列表相关交互信号。
        QQuickItem* root = rootObject();
        if (root) {
            connect(root, SIGNAL(playRequested(QString)),
                    this, SIGNAL(playRequested(QString)));
            connect(root, SIGNAL(removeRequested(QString)),
                    this, SIGNAL(removeRequested(QString)));
            connect(root, SIGNAL(clearAllRequested()),
                    this, SIGNAL(clearAllRequested()));
            connect(root, SIGNAL(pauseToggled()),
                    this, SIGNAL(pauseToggled()));
            qDebug() << "PlaylistHistoryQml: Signals connected";
        } else {
            qDebug() << "PlaylistHistoryQml: ERROR - Root object is null!";
        }
    }
    
    // 向最近播放列表追加一首歌曲。
    void addSong(const QString& filePath, const QString& title, 
                 const QString& artist, const QString& cover)
    {
        QQuickItem* root = rootObject();
        if (root) {
            qDebug() << "PlaylistHistory::addSong -" << title << artist;
            QMetaObject::invokeMethod(root, "addSong",
                Q_ARG(QVariant, filePath),
                Q_ARG(QVariant, title),
                Q_ARG(QVariant, artist),
                Q_ARG(QVariant, cover));
        } else {
            qDebug() << "PlaylistHistory::addSong - ERROR: root is null!";
        }
    }

    // 使用真实播放队列快照重建列表。
    void loadPlaylist(const QVariantList& items)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "loadPlaylist",
                Qt::QueuedConnection,
                Q_ARG(QVariant, QVariant::fromValue(items)));
        }
    }
    
    // 清空列表展示项。
    void clearAll()
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "clearAll", Qt::QueuedConnection);
        }
    }
    
    // 设置当前高亮播放路径。
    void setCurrentPlayingPath(const QString& path)
    {
        QQuickItem* root = rootObject();
        if (root) {
            qDebug() << "[PlaylistHistory] setCurrentPlayingPath:" << path;
            root->setProperty("currentPlayingPath", path);
        }
    }
    
    // 同步暂停状态，驱动列表项图标刷新。
    void setPaused(bool paused)
    {
        QQuickItem* root = rootObject();
        if (root) {
            qDebug() << "[PlaylistHistory] setPaused:" << paused;
            root->setProperty("isPaused", paused);
        }
    }
    
    // 一次性更新当前播放路径与播放态。
    void updatePlayingState(const QString& currentPath, bool isPlaying)
    {
        QQuickItem* root = rootObject();
        if (root) {
            qDebug() << "[PlaylistHistory] updatePlayingState - path:" << currentPath << "isPlaying:" << isPlaying;
            root->setProperty("currentPlayingPath", currentPath);
            root->setProperty("isPaused", !isPlaying);
        }
    }
    
    // 切换显示/隐藏
    void toggle()
    {
        if (isVisible()) {
            hide();
        } else {
            show();
            raise();
            activateWindow();
        }
    }

signals:
    void playRequested(const QString& filePath);
    void removeRequested(const QString& filePath);
    void clearAllRequested();
    void pauseToggled();  // 暂停/继续切换信号
};

#endif // PLAYLIST_HISTORY_QML_H

