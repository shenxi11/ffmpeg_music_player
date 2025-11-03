#ifndef DESKLRC_QML_H
#define DESKLRC_QML_H

#include <QObject>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>
#include <QDebug>
#include <QMouseEvent>
#include <QColor>
#include <QFont>
#include <QPoint>
#include <QSize>
#include "process_slider_qml.h"

class DeskLrcQml : public QQuickWidget
{
    Q_OBJECT

signals:
    // 控制信号
    void signal_play_clicked(int state);
    void signal_next_clicked();
    void signal_last_clicked();
    void signal_forward_clicked();
    void signal_backward_clicked();
    void signal_settings_clicked();
    void signal_close_clicked();

public:
    explicit DeskLrcQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        // 设置窗口属性 - 无边框，置顶，透明背景
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_AlwaysStackOnTop, true);
        
        // 设置透明背景
        setClearColor(QColor(0, 0, 0, 0));  // 完全透明
        
        // 关键：允许鼠标事件穿透和处理
        setAttribute(Qt::WA_AcceptTouchEvents, true);
        setMouseTracking(true);
        
        // 设置初始大小
        resize(500, 120);
        setMinimumSize(250, 100);
        setMaximumSize(1600, 400);
        
        // 加载 QML 文件
        setSource(QUrl("qrc:/qml/components/DeskLyric.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        // 初始化拖拽变量
        isDragging = false;
        processSlider = nullptr;
        pendingLyricText = "";
        pendingSongName = "";
        
        if (status() == QQuickWidget::Error) {
            qWarning() << "Failed to load DeskLyric.qml:" << errors();
        } else {
            qDebug() << "DeskLyric.qml loaded successfully";
        }
        
        // 连接 QML 信号到 C++ 槽
        connectQmlSignals();
    }

    // 设置歌词文本
    void setLyricText(const QString &text) {
        qDebug() << "DeskLrcQml::setLyricText called with:" << text;
        if (rootObject()) {
            QMetaObject::invokeMethod(rootObject(), "setLyricText",
                                      Q_ARG(QVariant, text));
        } else {
            qDebug() << "DeskLrcQml: rootObject is null, storing text for later";
            // 存储文本，等QML加载完成后设置
            pendingLyricText = text;
        }
    }

    // 设置播放状态
    void setPlayingState(bool isPlaying) {
        if (rootObject()) {
            rootObject()->setProperty("isPlaying", isPlaying);
        }
    }

    // 显示设置对话框
    void showSettingsDialog() {
        QMetaObject::invokeMethod(rootObject(), "showSettingsDialog");
    }

    // 设置歌曲名字
    void setSongName(const QString &songName) {
        qDebug() << "DeskLrcQml::setSongName called with:" << songName;
        if (rootObject()) {
            QMetaObject::invokeMethod(rootObject(), "setSongName",
                                      Q_ARG(QVariant, songName));
        } else {
            qDebug() << "DeskLrcQml: rootObject is null, storing song name for later";
            pendingSongName = songName;
        }
    }

    // 设置歌词样式
    void setLyricStyle(const QColor &color, int fontSize, const QFont &font) {
        QMetaObject::invokeMethod(rootObject(), "setLyricStyle",
                                  Q_ARG(QVariant, color),
                                  Q_ARG(QVariant, fontSize),
                                  Q_ARG(QVariant, font.family()));
    }

    // 设置 ProcessSlider 引用，用于模拟按钮点击
    void setProcessSlider(ProcessSliderQml* processSlider) {
        this->processSlider = processSlider;
    }

private slots:
    void onPlayClicked(int state) {
        qDebug() << "DeskLrcQml: Play clicked, state:" << state;
        // 发出 ProcessSlider 的 playClicked 信号，完全参照 ControlBar 逻辑
        if (processSlider) {
            qDebug() << "DeskLrcQml: Emitting ProcessSlider playClicked signal";
            emit processSlider->signal_play_clicked();
        } else {
            qDebug() << "DeskLrcQml: ProcessSlider is null!";
        }
    }

    void onNextClicked() {
        qDebug() << "DeskLrcQml: Next clicked";
        // 发出 ProcessSlider 的 nextSong 信号
        if (processSlider) {
            qDebug() << "DeskLrcQml: Emitting ProcessSlider nextSong signal";
            emit processSlider->signal_nextSong();
        } else {
            qDebug() << "DeskLrcQml: ProcessSlider is null!";
        }
    }

    void onLastClicked() {
        qDebug() << "DeskLrcQml: Last clicked";
        // 发出 ProcessSlider 的 lastSong 信号
        if (processSlider) {
            qDebug() << "DeskLrcQml: Emitting ProcessSlider lastSong signal";
            emit processSlider->signal_lastSong();
        } else {
            qDebug() << "DeskLrcQml: ProcessSlider is null!";
        }
    }

    void onForwardClicked() {
        qDebug() << "DeskLrcQml: Forward clicked";
        emit signal_forward_clicked();
    }

    void onBackwardClicked() {
        qDebug() << "DeskLrcQml: Backward clicked";
        emit signal_backward_clicked();
    }

    void onSettingsClicked() {
        qDebug() << "DeskLrcQml: Settings clicked";
        emit signal_settings_clicked();
    }

    void onCloseClicked() {
        qDebug() << "DeskLrcQml: Close clicked";
        emit signal_close_clicked();
        hide(); // 隐藏桌面歌词
    }

    void onWindowMoved(const QPoint &newPos) {
        // 处理窗口移动
        QPoint currentPos = pos();
        QPoint targetPos = currentPos + newPos;
        move(targetPos);
        qDebug() << "DeskLrcQml: Window moved to:" << targetPos;
    }

    void onWindowResized(const QSize &newSize) {
        // 处理窗口大小调整
        resize(newSize);
        qDebug() << "DeskLrcQml: Window resized to:" << newSize;
    }

protected:
    void showEvent(QShowEvent *event) override {
        QQuickWidget::showEvent(event);
        // 信号只在构造函数中连接一次，不需要重复连接
    }

private:
    void connectQmlSignals() {
        if (rootObject()) {
            // 连接播放控制信号
            connect(rootObject(), SIGNAL(playClicked(int)),
                    this, SLOT(onPlayClicked(int)));
            connect(rootObject(), SIGNAL(nextClicked()),
                    this, SLOT(onNextClicked()));
            connect(rootObject(), SIGNAL(lastClicked()),
                    this, SLOT(onLastClicked()));
            connect(rootObject(), SIGNAL(forwardClicked()),
                    this, SLOT(onForwardClicked()));
            connect(rootObject(), SIGNAL(backwardClicked()),
                    this, SLOT(onBackwardClicked()));
            connect(rootObject(), SIGNAL(settingsClicked()),
                    this, SLOT(onSettingsClicked()));
            connect(rootObject(), SIGNAL(closeClicked()),
                    this, SLOT(onCloseClicked()));
            
            // 连接窗口操作信号 - 使用函数指针方式避免类型问题
            if (rootObject()) {
                QObject::connect(rootObject(), &QQuickItem::xChanged, this, [this]() {
                    // 窗口位置变化时的处理
                });
                QObject::connect(rootObject(), &QQuickItem::yChanged, this, [this]() {
                    // 窗口位置变化时的处理  
                });
            }
                    
            qDebug() << "DeskLrcQml: QML signals connected";
            
            // 设置待处理的文本（如果有的话）
            if (!pendingLyricText.isEmpty()) {
                qDebug() << "Setting pending lyric text:" << pendingLyricText;
                QMetaObject::invokeMethod(rootObject(), "setLyricText",
                                          Q_ARG(QVariant, pendingLyricText));
                pendingLyricText.clear();
            }
            
            if (!pendingSongName.isEmpty()) {
                qDebug() << "Setting pending song name:" << pendingSongName;
                QMetaObject::invokeMethod(rootObject(), "setSongName",
                                          Q_ARG(QVariant, pendingSongName));
                pendingSongName.clear();
            }
        }
    }

private:
    // 拖拽相关变量
    bool isDragging;
    QPoint dragStartPos;
    
    // ProcessSlider 引用
    ProcessSliderQml* processSlider;
    
    // 待设置的文本（当QML还未加载完成时）
    QString pendingLyricText;
    QString pendingSongName;

protected:
    // 重写鼠标事件处理
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            // 检查是否在控制栏区域（顶部35像素）
            if (event->y() < 35) {
                // 在控制栏区域，不处理拖拽，让QML处理
                QQuickWidget::mousePressEvent(event);
                return;
            }
            
            isDragging = true;
            dragStartPos = event->globalPos() - frameGeometry().topLeft();
            event->accept();
            return;
        }
        QQuickWidget::mousePressEvent(event);
    }
    
    void mouseMoveEvent(QMouseEvent *event) override {
        if (isDragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - dragStartPos);
            event->accept();
            return;
        }
        QQuickWidget::mouseMoveEvent(event);
    }
    
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            isDragging = false;
        }
        QQuickWidget::mouseReleaseEvent(event);
    }

};

#endif // DESKLRC_QML_H