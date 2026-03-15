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
    // 播放控制栏相关信号
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
        
        // 启用透明渲染属性，避免与父窗口背景冲突。
        setClearColor(Qt::transparent);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_NoSystemBackground, true);  // 禁用系统背景填充
        setAttribute(Qt::WA_OpaquePaintEvent, false);   // 设置透明渲染相关属性
        setStyleSheet("background: transparent;");      // 设置透明渲染相关属性
        setAutoFillBackground(false);                   // 禁用自动背景填充
        
        qDebug() << "ProcessSliderQml: Loading QML from qrc:/qml/components/playback/ProcessSlider.qml";
        setSource(QUrl("qrc:/qml/components/playback/ProcessSlider.qml"));
        qDebug() << "  QML source set in" << timer.elapsed() << "ms";
        
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        qDebug() << "  Resize mode set in" << timer.elapsed() << "ms";
        
        // 输出 QML 加载错误，便于启动阶段排查。
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
            // 连接进度拖动和播放控制相关信号。
            connect(root, SIGNAL(seekTo(int)), this, SIGNAL(signalSliderMove(int)));
            connect(root, SIGNAL(sliderPressedSignal()), this, SIGNAL(signalSliderPressed()));
            connect(root, SIGNAL(sliderReleasedSignal()), this, SIGNAL(signalSliderReleased()));
            connect(root, SIGNAL(upClicked()), this, SIGNAL(signalUpClick()));
            connect(root, SIGNAL(playFinished()), this, SIGNAL(signalPlayFinished()));
            
            // 播放控制栏相关信号
            connect(root, SIGNAL(stop()), this, SIGNAL(signalStop()));
            connect(root, SIGNAL(nextSong()), this, SIGNAL(signalNextSong()));
            connect(root, SIGNAL(lastSong()), this, SIGNAL(signalLastSong()));
            connect(root, SIGNAL(volumeChanged(int)), this, SIGNAL(signalVolumeChanged(int)));
            connect(root, SIGNAL(mlistToggled(bool)), this, SIGNAL(signalMlistToggled(bool)));
            connect(root, SIGNAL(playClicked()), this, SIGNAL(signalPlayClicked()));
            connect(root, SIGNAL(rePlay()), this, SIGNAL(signalRePlay()));
            connect(root, SIGNAL(deskToggled(bool)), this, SIGNAL(signalDeskToggled(bool)));
            connect(root, SIGNAL(loopToggled(bool)), this, SLOT(on_loop_state_changed(bool)));
            
            // 播放模式状态同步说明
            connect(root, SIGNAL(playModeChanged()), this, SLOT(on_playMode_changed()));
            
            qDebug() << "ProcessSliderQml: All signals connected in" << timer.elapsed() << "ms (total)";
        } else {
            qDebug() << "ProcessSliderQml: ERROR - Root object is null!";
        }
        
        qDebug() << "ProcessSliderQml: Constructor completed in" << timer.elapsed() << "ms (total)";
        
        // 确保控件可见
        show();
    }
    
    // 设置进度条最大秒数（总时长）。
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

    void setSeekPendingSeconds(int seconds) {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setSeekPending", Q_ARG(QVariant, seconds));
        }
    }

    void clearSeekPending() {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "clearSeekPending");
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

    void setVolume(int value) {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setVolumeValue", Q_ARG(QVariant, value));
        }
    }
    
    // 播放控制栏相关信号
    void setState(State state) {
        QQuickItem* root = rootObject();
        if (root) {
            qDebug() << "[ProcessSliderQml] setState called with state:" << state;
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
    
    void onIsUpChanged(bool isUp) {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setIsUp", Q_ARG(QVariant, isUp));
        }
    }
    
    void onPlayFinished() {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "handlePlayFinished");
        }
    }
    
public slots:
    void on_loop_state_changed(bool loop) {
        qDebug() << "Loop state changed to:" << loop;
        emit signalLoopChange(loop);
    }
    
    void on_playMode_changed() {
        QQuickItem* root = rootObject();
        if (root) {
            int mode = root->property("playMode").toInt();
            qDebug() << "Play mode changed to:" << mode;
            emit signalPlayModeChanged(mode);
        }
    }
    
signals:
    // 用户拖动进度条后的目标秒数。
    void signalSliderMove(int seconds);
    void signalSliderPressed();
    void signalSliderReleased();
    void signalUpClick();
    void signalPlayFinished();  // 播放完成信号
    
    // 播放控制栏相关信号
    void signalStop();
    void signalNextSong();
    void signalLastSong();
    void signalVolumeChanged(int value);
    void signalMlistToggled(bool checked);
    void signalPlayClicked();
    void signalRePlay();
    void signalDeskToggled(bool checked);
    void signalLoopChange(bool loop);
    void signalPlayModeChanged(int mode);  // 播放模式变更信号
};

#endif // PROCESS_SLIDER_QML_H

