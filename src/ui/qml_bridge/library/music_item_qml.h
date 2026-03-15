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
        setAttribute(Qt::WA_AlwaysStackOnTop, false);  // 设置透明渲染相关属性
        
        // 预留：如需无边框独立弹窗可启用无边框窗口标志。
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

            // 转发单行歌曲项的播放/删除/下载操作。
            connect(rootItem, SIGNAL(playRequested(QString)), this, SIGNAL(signalPlayClick(QString)));
            connect(rootItem, SIGNAL(removeRequested(QString)), this, SIGNAL(signalRemoveClick(QString)));
            connect(rootItem, SIGNAL(downloadRequested(QString)), this, SIGNAL(signalDownloadClick(QString)));
        }

        setFixedSize(size);
    }

    // 同步单条目播放态（用于显示播放图标）。
    void buttonOp(bool flag) {
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            QMetaObject::invokeMethod(rootItem, "setPlayingState", Q_ARG(QVariant, flag));
        }
    }

    void playToClick() {
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            QMetaObject::invokeMethod(rootItem, "triggerPlay");
        }
    }

signals:
    void signalPlayClick(QString songName);
    void signalRemoveClick(QString songName);
    void signalDownloadClick(QString songName);
};

#endif // MUSIC_ITEM_QML_H

