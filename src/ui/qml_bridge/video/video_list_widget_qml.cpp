#include "video_list_widget_qml.h"

VideoListWidgetQml::VideoListWidgetQml(QWidget *parent)
    : QQuickWidget(parent)
{
    // 璁剧疆 QML 婧愭枃浠?
    setSource(QUrl("qrc:/qml/components/video/VideoListWidget.qml"));
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    // 杩炴帴淇″彿
    QQuickItem* root = rootObject();
    if (root) {
        // 杩炴帴 QML 淇″彿鍒?C++ 淇″彿
        connect(root, SIGNAL(videoSelected(QString, QString)), 
                this, SIGNAL(signal_video_selected(QString, QString)));
        connect(root, SIGNAL(refreshRequested()), 
                this, SIGNAL(signal_refresh_requested()));
    } else {
        qWarning() << "VideoListWidgetQml: Failed to load QML root object";
    }
}

void VideoListWidgetQml::addVideo(const QString& name, const QString& path, qint64 size)
{
    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "addVideo",
            Q_ARG(QVariant, name),
            Q_ARG(QVariant, path),
            Q_ARG(QVariant, size));
    }
}

void VideoListWidgetQml::addVideoList(const QVariantList& videoList)
{
    for (const QVariant& video : videoList) {
        QVariantMap videoMap = video.toMap();
        QString name = videoMap["name"].toString();
        QString path = videoMap["path"].toString();
        qint64 size = videoMap["size"].toLongLong();
        addVideo(name, path, size);
    }
}

void VideoListWidgetQml::clearAll()
{
    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "clearAll");
    }
}

int VideoListWidgetQml::getCount()
{
    QQuickItem* root = rootObject();
    if (root) {
        QVariant result;
        QMetaObject::invokeMethod(root, "getCount", Q_RETURN_ARG(QVariant, result));
        return result.toInt();
    }
    return 0;
}

