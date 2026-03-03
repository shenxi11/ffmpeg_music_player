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
        // 璁剧疆 QML 婧愭枃浠?
        setSource(QUrl("qrc:/qml/components/playback/ControlBar.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        QQuickItem* root = rootObject();
        if (root) {
            // 杩炴帴 QML 淇″彿鍒?C++ 淇″彿
            connect(root, SIGNAL(stop()), this, SIGNAL(signal_stop()));
            connect(root, SIGNAL(nextSong()), this, SIGNAL(signal_nextSong()));
            connect(root, SIGNAL(lastSong()), this, SIGNAL(signal_lastSong()));
            connect(root, SIGNAL(volumeChanged(int)), this, SIGNAL(signal_volumeChanged(int)));
            connect(root, SIGNAL(mlistToggled(bool)), this, SIGNAL(signal_mlist_toggled(bool)));
            connect(root, SIGNAL(playClicked()), this, SIGNAL(signal_play_clicked()));
            connect(root, SIGNAL(rePlay()), this, SIGNAL(signal_rePlay()));
            connect(root, SIGNAL(deskToggled(bool)), this, SIGNAL(signal_desk_toggled(bool)));
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
    void slot_playState(State state_)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setPlayState",
                Q_ARG(QVariant, static_cast<int>(state_)));
        }
    }

    void slot_playFinished()
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "playFinished");
        }
    }

    void slot_isUpChanged(bool flag)
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
        // QML 鍐呴儴宸茬粡鏇存柊浜?loopState锛岃繖閲屽彧闇€璁板綍鏃ュ織鎴栧仛鍏朵粬澶勭悊
        qDebug() << "Loop state changed to:" << loop;
    }

signals:
    void signal_stop();
    void signal_nextSong();
    void signal_lastSong();
    void signal_volumeChanged(int value);
    void signal_mlist_toggled(bool checked);
    void signal_play_clicked();
    void signal_rePlay();
    void signal_desk_toggled(bool checked);
};

#endif // CONTROLBAR_QML_H

