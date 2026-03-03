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
        setAttribute(Qt::WA_AlwaysStackOnTop, false);  // 涓嶆€绘槸鍦ㄩ《灞?
        
        // 鎬ц兘浼樺寲锛氫娇鐢ㄨ蒋浠舵覆鏌撴垨鍏变韩 OpenGL 涓婁笅鏂?
        // setWindowFlag(Qt::FramelessWindowHint);
        
        setSource(QUrl("qrc:/qml/components/library/MusicItem.qml"));

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

            // 杩炴帴 QML 淇″彿鍒?C++ 淇″彿
            connect(rootItem, SIGNAL(playRequested(QString)), this, SIGNAL(signal_play_click(QString)));
            connect(rootItem, SIGNAL(removeRequested(QString)), this, SIGNAL(signal_remove_click(QString)));
            connect(rootItem, SIGNAL(downloadRequested(QString)), this, SIGNAL(signal_download_click(QString)));
        }

        setFixedSize(size);
    }

    // 鎻愪緵涓庡師 MusicItem 鍏煎鐨勬帴鍙?
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

