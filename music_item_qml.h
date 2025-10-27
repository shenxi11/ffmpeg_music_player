#ifndef MUSIC_ITEM_QML_H
#define MUSIC_ITEM_QML_H

#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlError>
#include <QQmlEngine>
#include <QQuickItem>
#include <QDebug>

class MusicItemQml : public QQuickWidget
{
    Q_OBJECT
public:
    explicit MusicItemQml(const QString &name, const QString &path, const QString &picPath, const QSize &size, QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        setClearColor(Qt::transparent);
        setAttribute(Qt::WA_AlwaysStackOnTop, false);  // 不总是在顶层
        
        // 性能优化：使用软件渲染或共享 OpenGL 上下文
        // setWindowFlag(Qt::FramelessWindowHint);
        
        setSource(QUrl("qrc:/qml/components/MusicItem.qml"));

        if (status() == QQuickWidget::Error) {
            qWarning() << "MusicItemQml: Failed to load MusicItem.qml";
            for (const QQmlError &e : errors()) qWarning() << e.toString();
            return;
        }

        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            rootItem->setProperty("songName", name);
            rootItem->setProperty("filePath", path);
            rootItem->setProperty("cover", picPath);
            rootItem->setProperty("isNet", false);

            // 连接 QML 信号到 C++ 信号
            connect(rootItem, SIGNAL(playRequested(QString)), this, SIGNAL(signal_play_click(QString)));
            connect(rootItem, SIGNAL(removeRequested(QString)), this, SIGNAL(signal_remove_click(QString)));
            connect(rootItem, SIGNAL(downloadRequested(QString)), this, SIGNAL(signal_download_click(QString)));
        }

        setFixedSize(size);
    }

    // 提供与原 MusicItem 兼容的接口
    void button_op(bool flag) {
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            QMetaObject::invokeMethod(rootItem, "setPlayingState", Q_ARG(QVariant, flag));
        }
    }

    void play_to_click() {
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            QMetaObject::invokeMethod(rootItem, "triggerPlay");
        }
    }

signals:
    void signal_play_click(QString songName);
    void signal_remove_click(QString songName);
    void signal_download_click(QString songName);
};

#endif // MUSIC_ITEM_QML_H
