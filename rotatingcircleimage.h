#ifndef ROTATINGCIRCLEIMAGE_H
#define ROTATINGCIRCLEIMAGE_H

#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QMutexLocker>
#include <cmath>
#include <QPainterPath>
class LambdaRunnable : public QRunnable {
public:
    explicit LambdaRunnable(std::function<void()> lambda) : lambdaFunc(std::move(lambda)) {}

    void run() override {
        lambdaFunc();
    }

private:
    std::function<void()> lambdaFunc;
};

class RotatingCircleImage : public QWidget {
    Q_OBJECT

public:
    explicit RotatingCircleImage(QWidget *parent = nullptr);
public slots:
    void on_signal_stop_rotate(bool flag);

protected:
    void paintEvent(QPaintEvent *event);

private slots:
    void scheduleRotateTask();

private:
    QPixmap rotateImageAsync(const QPixmap &source, int rotationAngle);

private:
    QPixmap image;         // 原始图像
    QPixmap rotatedImage;  // 绘制后的图像
    qreal angle;           // 当前角度
    QTimer *timer;         // 定时器
    QMutex mutex;          // 用于保护 rotatedImage 的互斥锁
    bool isRendering = false; // 渲染任务是否在进行中
};

#endif // ROTATINGCIRCLEIMAGE_H
