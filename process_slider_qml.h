#ifndef PROCESS_SLIDER_QML_H
#define PROCESS_SLIDER_QML_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QWidget>
#include <QDebug>
#include <QElapsedTimer>

class ProcessSliderQml : public QQuickWidget
{
    Q_OBJECT
public:
    // 状态枚举（对应 ControlBar）
    enum State {
        Stop = 0,
        Play = 1,
        Pause = 2
    };
    
    explicit ProcessSliderQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        QElapsedTimer timer;
        timer.start();
        
        // 设置完全透明背景
        setClearColor(Qt::transparent);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_AlwaysStackOnTop, true);
        setAttribute(Qt::WA_NoSystemBackground, true);  // 禁用系统背景
        setAttribute(Qt::WA_OpaquePaintEvent, false);   // 禁用不透明绘制
        setStyleSheet("background: transparent;");      // 确保样式表也透明
        setAutoFillBackground(false);                   // 禁用自动填充背景
        
        qDebug() << "ProcessSliderQml: Loading QML from qrc:/qml/components/ProcessSlider.qml";
        setSource(QUrl("qrc:/qml/components/ProcessSlider.qml"));
        qDebug() << "  QML source set in" << timer.elapsed() << "ms";
        
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        qDebug() << "  Resize mode set in" << timer.elapsed() << "ms";
        
        // 检查 QML 加载状态
        if (status() == QQuickWidget::Error) {
            qDebug() << "ProcessSliderQml: QML loading errors:";
            for (const auto& error : errors()) {
                qDebug() << "  " << error.toString();
            }
        } else {
            qDebug() << "ProcessSliderQml: QML loaded successfully in" << timer.elapsed() << "ms";
        }
        
        QQuickItem* root = rootObject();
        if (root) {
            qDebug() << "ProcessSliderQml: Root object found, connecting signals...";
            // 进度条信号
            connect(root, SIGNAL(seekTo(int)), this, SIGNAL(signal_Slider_Move(int)));
            connect(root, SIGNAL(sliderPressedSignal()), this, SIGNAL(signal_sliderPressed()));
            connect(root, SIGNAL(sliderReleasedSignal()), this, SIGNAL(signal_sliderReleased()));
            connect(root, SIGNAL(upClicked()), this, SIGNAL(signal_up_click()));
            connect(root, SIGNAL(playFinished()), this, SIGNAL(signal_playFinished()));
            
            // 播放控制信号（对应 ControlBar）
            connect(root, SIGNAL(stop()), this, SIGNAL(signal_stop()));
            connect(root, SIGNAL(nextSong()), this, SIGNAL(signal_nextSong()));
            connect(root, SIGNAL(lastSong()), this, SIGNAL(signal_lastSong()));
            connect(root, SIGNAL(volumeChanged(int)), this, SIGNAL(signal_volumeChanged(int)));
            connect(root, SIGNAL(mlistToggled(bool)), this, SIGNAL(signal_mlist_toggled(bool)));
            connect(root, SIGNAL(playClicked()), this, SIGNAL(signal_play_clicked()));
            connect(root, SIGNAL(rePlay()), this, SIGNAL(signal_rePlay()));
            connect(root, SIGNAL(deskToggled(bool)), this, SIGNAL(signal_desk_toggled(bool)));
            connect(root, SIGNAL(loopToggled(bool)), this, SLOT(on_loop_state_changed(bool)));
            qDebug() << "ProcessSliderQml: All signals connected in" << timer.elapsed() << "ms (total)";
        } else {
            qDebug() << "ProcessSliderQml: ERROR - Root object is null!";
        }
        
        qDebug() << "ProcessSliderQml: Constructor completed in" << timer.elapsed() << "ms (total)";
        
        // 确保显示
        show();
    }
    
    // 进度条方法
    void setMaxSeconds(int seconds) {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setMaxTime", Q_ARG(QVariant, seconds));
        }
    }
    
    void setCurrentSeconds(int seconds) {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setCurrentTime", Q_ARG(QVariant, seconds));
        } else {
            qDebug() << "ProcessSliderQml::setCurrentSeconds - root is NULL!";
        }
    }
    
    void setSongName(const QString& name) {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setSongName", Q_ARG(QVariant, name));
        }
    }
    
    void setPicPath(const QString& path) {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setPicPath", Q_ARG(QVariant, path));
        }
    }
    
    // 播放控制方法（对应 ControlBar）
    void setState(State state) {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setPlayState", Q_ARG(QVariant, static_cast<int>(state)));
        }
    }
    
    State getState() {
        QQuickItem* root = rootObject();
        if (root) {
            QVariant result;
            QMetaObject::invokeMethod(root, "getPlayState", Q_RETURN_ARG(QVariant, result));
            return static_cast<State>(result.toInt());
        }
        return Stop;
    }
    
    void setLoopFlag(bool flag) {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setLoopState", Q_ARG(QVariant, flag));
        }
    }
    
    bool getLoopFlag() {
        QQuickItem* root = rootObject();
        if (root) {
            QVariant result;
            QMetaObject::invokeMethod(root, "getLoopState", Q_RETURN_ARG(QVariant, result));
            return result.toBool();
        }
        return false;
    }
    
    void setDeskChecked(bool checked) {
        QQuickItem* root = rootObject();
        if (root) {
            root->setProperty("deskChecked", checked);
            qDebug() << "ProcessSliderQml::setDeskChecked - set to:" << checked;
        } else {
            qDebug() << "ProcessSliderQml::setDeskChecked - root is NULL!";
        }
    }
    
    bool getDeskChecked() {
        QQuickItem* root = rootObject();
        if (root) {
            return root->property("deskChecked").toBool();
        }
        return false;
    }
    
    void slot_isUpChanged(bool isUp) {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setIsUp", Q_ARG(QVariant, isUp));
        }
    }
    
    void slot_playFinished() {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "handlePlayFinished");
        }
    }
    
public slots:
    void on_loop_state_changed(bool loop) {
        qDebug() << "Loop state changed to:" << loop;
        emit signal_loop_change(loop);
    }
    
signals:
    // 进度条信号
    void signal_Slider_Move(int seconds);
    void signal_sliderPressed();
    void signal_sliderReleased();
    void signal_up_click();
    void signal_playFinished();  // 播放完成信号
    
    // 播放控制信号（对应 ControlBar）
    void signal_stop();
    void signal_nextSong();
    void signal_lastSong();
    void signal_volumeChanged(int value);
    void signal_mlist_toggled(bool checked);
    void signal_play_clicked();
    void signal_rePlay();
    void signal_desk_toggled(bool checked);
    void signal_loop_change(bool loop);
};

#endif // PROCESS_SLIDER_QML_H
