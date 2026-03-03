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
        
        // 涓嶈缃獥鍙ｆ爣蹇楋紝浣滀负鐖剁獥鍙ｇ殑瀛愭帶浠?
        setAttribute(Qt::WA_TranslucentBackground);  // 鍏佽鍦嗚
        
        qDebug() << "PlaylistHistoryQml: Created, root object:" << (rootObject() != nullptr);
        
        // 杩炴帴QML淇″彿
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
    
    // 娣诲姞姝屾洸鍒版挱鏀惧垪琛?
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
    
    // 娓呯┖鍒楄〃
    void clearAll()
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "clearAll");
        }
    }
    
    // 璁剧疆褰撳墠鎾斁璺緞
    void setCurrentPlayingPath(const QString& path)
    {
        QQuickItem* root = rootObject();
        if (root) {
            qDebug() << "[PlaylistHistory] setCurrentPlayingPath:" << path;
            root->setProperty("currentPlayingPath", path);
        }
    }
    
    // 璁剧疆鏆傚仠鐘舵€?
    void setPaused(bool paused)
    {
        QQuickItem* root = rootObject();
        if (root) {
            qDebug() << "[PlaylistHistory] setPaused:" << paused;
            root->setProperty("isPaused", paused);
        }
    }
    
    // 鍚屾鎾斁鐘舵€侊紙缁勫悎鏇存柊锛?
    void updatePlayingState(const QString& currentPath, bool isPlaying)
    {
        QQuickItem* root = rootObject();
        if (root) {
            qDebug() << "[PlaylistHistory] updatePlayingState - path:" << currentPath << "isPlaying:" << isPlaying;
            root->setProperty("currentPlayingPath", currentPath);
            root->setProperty("isPaused", !isPlaying);
        }
    }
    
    // 鍒囨崲鏄剧ず/闅愯棌
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
    void pauseToggled();  // 鏆傚仠/缁х画鎾斁淇″彿
};

#endif // PLAYLIST_HISTORY_QML_H

