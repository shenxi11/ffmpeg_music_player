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
        // 设置 QML 源文件
        setSource(QUrl("qrc:/qml/components/ControlBar.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        QQuickItem* root = rootObject();
        if (root) {
            // 连接 QML 信号到 C++ 信号
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
        // QML 内部已经更新了 loopState，这里只需记录日志或做其他处理
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
