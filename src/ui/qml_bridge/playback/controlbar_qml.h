#ifndef CONTROLBAR_QML_H
#define CONTROLBAR_QML_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QWidget>
#include <QDebug>

class ControlBarQml : public QQuickWidget
{
    Q_OBJECT
public:
    enum State{
      Stop,
      Play,
      Pause
    };

    explicit ControlBarQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        // 加载底部播放控制栏 QML。
        setSource(QUrl("qrc:/qml/components/playback/ControlBar.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        QQuickItem* root = rootObject();
        if (root) {
            // 建立 QML 与 QWidget 的播放控制信号桥接。
            connect(root, SIGNAL(stop()), this, SIGNAL(signalStop()));
            connect(root, SIGNAL(nextSong()), this, SIGNAL(signalNextSong()));
            connect(root, SIGNAL(lastSong()), this, SIGNAL(signalLastSong()));
            connect(root, SIGNAL(volumeChanged(int)), this, SIGNAL(signalVolumeChanged(int)));
            connect(root, SIGNAL(mlistToggled(bool)), this, SIGNAL(signalMlistToggled(bool)));
            connect(root, SIGNAL(playClicked()), this, SIGNAL(signalPlayClicked()));
            connect(root, SIGNAL(rePlay()), this, SIGNAL(signalRePlay()));
            connect(root, SIGNAL(deskToggled(bool)), this, SIGNAL(signalDeskToggled(bool)));
            connect(root, SIGNAL(loopStateChanged(bool)), this, SLOT(on_loop_state_changed(bool)));
        }
    }

    State getState()
    {
        QQuickItem* root = rootObject();
        if (root) {
            QVariant result;
            QMetaObject::invokeMethod(root, "getPlayState", 
                Q_RETURN_ARG(QVariant, result));
            return static_cast<State>(result.toInt());
        }
        return Stop;
    }

    bool getLoopFlag()
    {
        QQuickItem* root = rootObject();
        if (root) {
            QVariant result;
            QMetaObject::invokeMethod(root, "getLoopState", 
                Q_RETURN_ARG(QVariant, result));
            return result.toBool();
        }
        return false;
    }

public slots:
    void onPlayState(State state_)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setPlayState",
                Q_ARG(QVariant, static_cast<int>(state_)));
        }
    }

    void onPlayFinished()
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "playFinished");
        }
    }

    void onIsUpChanged(bool flag)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setIsUp",
                Q_ARG(QVariant, flag));
        }
    }

private slots:
    void on_loop_state_changed(bool loop)
    {
        // 播放模式状态同步说明
        qDebug() << "Loop state changed to:" << loop;
    }

signals:
    void signalStop();
    void signalNextSong();
    void signalLastSong();
    void signalVolumeChanged(int value);
    void signalMlistToggled(bool checked);
    void signalPlayClicked();
    void signalRePlay();
    void signalDeskToggled(bool checked);
};

#endif // CONTROLBAR_QML_H

