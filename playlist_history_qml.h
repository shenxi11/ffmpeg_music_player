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
        setSource(QUrl("qrc:/qml/components/PlaylistHistory.qml"));
        
        // 不设置窗口标志，作为父窗口的子控件
        setAttribute(Qt::WA_TranslucentBackground);  // 允许圆角
        
        qDebug() << "PlaylistHistoryQml: Created, root object:" << (rootObject() != nullptr);
        
        // 连接QML信号
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
    
    // 添加歌曲到播放列表
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
    
    // 清空列表
    void clearAll()
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "clearAll");
        }
    }
    
    // 设置当前播放路径
    void setCurrentPlayingPath(const QString& path)
    {
        QQuickItem* root = rootObject();
        if (root) {
            qDebug() << "[PlaylistHistory] setCurrentPlayingPath:" << path;
            root->setProperty("currentPlayingPath", path);
        }
    }
    
    // 设置暂停状态
    void setPaused(bool paused)
    {
        QQuickItem* root = rootObject();
        if (root) {
            qDebug() << "[PlaylistHistory] setPaused:" << paused;
            root->setProperty("isPaused", paused);
        }
    }
    
    // 同步播放状态（组合更新）
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
    void pauseToggled();  // 暂停/继续播放信号
};

#endif // PLAYLIST_HISTORY_QML_H
