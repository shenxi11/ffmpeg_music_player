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
        // 设置完全透明背景
        setClearColor(Qt::transparent);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_AlwaysStackOnTop, false);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_OpaquePaintEvent, false);   // 禁用不透明绘制
        setStyleSheet("background: transparent;");
        setAutoFillBackground(false);                   // 禁用自动填充背景
        
        // 加载 QML 文件
        setSource(QUrl("qrc:/qml/components/RotatingCircle.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        if (status() == QQuickWidget::Error) {
            qWarning() << "Failed to load RotatingCircle.qml:" << errors();
        }
    }

    // 开始旋转
    void startRotation() {
        QMetaObject::invokeMethod(rootObject(), "startRotation");
    }

    // 停止旋转
    void stopRotation() {
        QMetaObject::invokeMethod(rootObject(), "stopRotation");
    }

    // 设置图片
    void setImage(const QString &imagePath) {
        QMetaObject::invokeMethod(rootObject(), "setImage",
                                  Q_ARG(QVariant, imagePath));
    }

    // 设置图片路径（兼容性方法）
    void setImagePath(const QString &imagePath) {
        setImage(imagePath);
    }

public slots:
    // 兼容原有接口
    void on_signal_stop_rotate(bool flag) {
        if (flag) {
            startRotation();
        } else {
            stopRotation();
        }
    }
};

#endif // ROTATINGCIRCLE_QML_H
