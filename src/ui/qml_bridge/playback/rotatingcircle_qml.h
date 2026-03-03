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
        // 璁剧疆瀹屽叏閫忔槑鑳屾櫙
        setClearColor(Qt::transparent);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_AlwaysStackOnTop, false);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_OpaquePaintEvent, false);   // 绂佺敤涓嶉€忔槑缁樺埗
        setStyleSheet("background: transparent;");
        setAutoFillBackground(false);                   // 绂佺敤鑷姩濉厖鑳屾櫙
        
        // 鍔犺浇 QML 鏂囦欢
        setSource(QUrl("qrc:/qml/components/playback/RotatingCircle.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        if (status() == QQuickWidget::Error) {
            qWarning() << "Failed to load RotatingCircle.qml:" << errors();
        }
    }

    // 寮€濮嬫棆杞?
    void startRotation() {
        QMetaObject::invokeMethod(rootObject(), "startRotation");
    }

    // 鍋滄鏃嬭浆
    void stopRotation() {
        QMetaObject::invokeMethod(rootObject(), "stopRotation");
    }

    // 璁剧疆鍥剧墖
    void setImage(const QString &imagePath) {
        QMetaObject::invokeMethod(rootObject(), "setImage",
                                  Q_ARG(QVariant, imagePath));
    }

    // 璁剧疆鍥剧墖璺緞锛堝吋瀹规€ф柟娉曪級
    void setImagePath(const QString &imagePath) {
        setImage(imagePath);
    }

public slots:
    // 鍏煎鍘熸湁鎺ュ彛
    void on_signal_stop_rotate(bool flag) {
        if (flag) {
            startRotation();
        } else {
            stopRotation();
        }
    }
};

#endif // ROTATINGCIRCLE_QML_H

