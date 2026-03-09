#ifndef CUSTOM_CONTROL_BAR_QML_H
#define CUSTOM_CONTROL_BAR_QML_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlContext>
#include <QDebug>

class CustomControlBarQml : public QQuickWidget
{
    Q_OBJECT
public:
    explicit CustomControlBarQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        qDebug() << "CustomControlBarQml: Loading QML from qrc:/qml/components/playback/CustomControlBar.qml";
        setSource(QUrl("qrc:/qml/components/playback/CustomControlBar.qml"));
        
        if (status() == QQuickWidget::Error) {
            qCritical() << "CustomControlBarQml: Failed to load QML!";
            qCritical() << "Errors:" << errors();
            return;
        }
        
        // 控件悬浮于播放器上层并保持透明背景。
        setAttribute(Qt::WA_AlwaysStackOnTop);
        setAttribute(Qt::WA_TranslucentBackground);
        setClearColor(Qt::transparent);
        
        QQuickItem* root = rootObject();
        if (root) {
            qDebug() << "CustomControlBarQml: QML loaded successfully, connecting signals";
            // 将 QML 控制栏操作统一转发到 C++ 层。
            connect(root, SIGNAL(playPauseClicked()), this, SIGNAL(playPauseClicked()));
            connect(root, SIGNAL(previousClicked()), this, SIGNAL(previousClicked()));
            connect(root, SIGNAL(nextClicked()), this, SIGNAL(nextClicked()));
            connect(root, SIGNAL(volumeLevelChanged(qreal)), this, SIGNAL(volumeChanged(qreal)));
            connect(root, SIGNAL(seekToPosition(qreal)), this, SIGNAL(seekTo(qreal)));
            connect(root, SIGNAL(loopModeToggled()), this, SIGNAL(loopModeChanged()));
            connect(root, SIGNAL(desktopLrcToggled()), this, SIGNAL(desktopLrcToggled()));
            connect(root, SIGNAL(listToggled()), this, SIGNAL(listToggled()));
        } else {
            qCritical() << "CustomControlBarQml: Failed to get root object!";
        }
    }
    
    void setPlayState(bool playing)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setPlayState",
                Q_ARG(QVariant, playing));
        }
    }
    
    void setProgress(int current, int total)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setProgress",
                Q_ARG(QVariant, current),
                Q_ARG(QVariant, total));
        }
    }
    
    void setSongInfo(const QString& name, const QString& artist, const QString& cover)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setSongInfo",
                Q_ARG(QVariant, name),
                Q_ARG(QVariant, artist),
                Q_ARG(QVariant, cover));
        }
    }
    
    void setVolume(qreal volume)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setVolume",
                Q_ARG(QVariant, volume));
        }
    }
    
    void setLoopMode(int mode)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setLoopMode",
                Q_ARG(QVariant, mode));
        }
    }
    
signals:
    void playPauseClicked();
    void previousClicked();
    void nextClicked();
    void volumeChanged(qreal volume);
    void seekTo(qreal position);
    void loopModeChanged();
    void desktopLrcToggled();
    void listToggled();
};

#endif // CUSTOM_CONTROL_BAR_QML_H

