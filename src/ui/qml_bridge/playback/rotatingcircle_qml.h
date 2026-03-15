#ifndef ROTATINGCIRCLE_QML_H
#define ROTATINGCIRCLE_QML_H

#include <QObject>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>
#include <QDebug>

class RotatingCircleQml : public QQuickWidget
{
    Q_OBJECT

public:
    explicit RotatingCircleQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        // 透明渲染配置，避免唱片区域出现底色块。
        setClearColor(Qt::transparent);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_AlwaysStackOnTop, false);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_OpaquePaintEvent, false);   // 设置透明渲染相关属性
        setStyleSheet("background: transparent;");
        setAutoFillBackground(false);                   // 禁用自动背景填充
        
        // 加载唱片旋转 QML。
        setSource(QUrl("qrc:/qml/components/playback/RotatingCircle.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        if (status() == QQuickWidget::Error) {
            qWarning() << "Failed to load RotatingCircle.qml:" << errors();
        }
    }

    // 启动唱片旋转动画。
    void startRotation() {
        QMetaObject::invokeMethod(rootObject(), "startRotation");
    }

    // 停止唱片旋转动画。
    void stopRotation() {
        QMetaObject::invokeMethod(rootObject(), "stopRotation");
    }

    // 设置封面图路径并下发到 QML。
    void setImage(const QString &imagePath) {
        QMetaObject::invokeMethod(rootObject(), "setImage",
                                  Q_ARG(QVariant, imagePath));
    }

    // 兼容旧接口命名。
    void setImagePath(const QString &imagePath) {
        setImage(imagePath);
    }

public slots:
    // 根据布尔状态控制旋转动画启停。
    void onStopRotate(bool flag) {
        if (flag) {
            startRotation();
        } else {
            stopRotation();
        }
    }
};

#endif // ROTATINGCIRCLE_QML_H

