#include "rotatingcircleimage.h"
RotatingCircleImage::RotatingCircleImage(QWidget *parent)
        : QWidget(parent), angle(0), rotatedImage(QPixmap()) {

        image = QPixmap(":/new/prefix1/icon/maxresdefault.jpg");

        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &RotatingCircleImage::scheduleRotateTask);
        timer->setInterval(30); // 每30ms旋转一次
    }

QPixmap RotatingCircleImage::rotateImageAsync(const QPixmap &source, int rotationAngle) {
    int diameter = qMin(width(), height());  // 圆的直径为窗口的最小边长
    QPixmap target(diameter, diameter);     // 圆形区域的大小
    target.fill(Qt::transparent);

    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 创建一个圆形裁剪区域
    QPainterPath clipPath;
    clipPath.addEllipse(0, 0, diameter, diameter);
    painter.setClipPath(clipPath);

    // 计算旋转中心为窗口的中心
    QPointF windowCenter(width() / 2.0, height() / 2.0);
    QPointF imageCenter(source.width() / 2.0, source.height() / 2.0);

    // 设置变换：将图像中心移动到窗口中心，旋转，再移回
    QTransform transform;
    transform.translate(diameter / 2.0, diameter / 2.0);  // 将目标圆形的中心对齐
    transform.rotate(rotationAngle);                     // 旋转
    transform.translate(-imageCenter.x(), -imageCenter.y());  // 将图像中心复位

    // 绘制旋转后的图像
    painter.setTransform(transform);
    painter.drawPixmap(0, 0, source);

    return target;
}
void RotatingCircleImage::scheduleRotateTask() {
    // 防止任务重复提交
    if (isRendering) return;

    isRendering = true;
    QPixmap imageCopy = image; // 创建图像副本
    int currentAngle = angle;

    QThreadPool::globalInstance()->start(new LambdaRunnable([this, imageCopy, currentAngle]() {
        QPixmap result = rotateImageAsync(imageCopy, currentAngle);
        {
            QMutexLocker locker(&mutex);
            rotatedImage = result;
        }
        angle = std::fmod(angle + 1, 360.0);
        isRendering = false;

        QTimer::singleShot(0, this, SLOT(update()));
    }));

}
void RotatingCircleImage::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QPoint center(width() / 2, height() / 2);
    int outerRadius = qMin(width(), height()) / 2;

    // 绘制外层黑色圆环
    painter.setBrush(Qt::black);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, outerRadius, outerRadius);

    if (!rotatedImage.isNull()) {
        int innerRadius = outerRadius * 0.7;
        QRect innerRect(center.x() - innerRadius, center.y() - innerRadius, innerRadius * 2, innerRadius * 2);
        painter.drawPixmap(innerRect, rotatedImage);
    }

    QWidget::paintEvent(event);
}
void RotatingCircleImage::on_signal_stop_rotate(bool flag) {
    if (flag) {
        timer->start();
    } else {
        timer->stop();
    }
}
