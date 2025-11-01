#include "rotatingcircleimage.h"
#include <QUrl>

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

    // 先将图片缩放到圆形大小，保持纵横比并填充整个圆形
    QPixmap scaledSource = source.scaled(diameter, diameter, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    
    // 如果缩放后的图片大于圆形，裁剪到中心区域
    int offsetX = (scaledSource.width() - diameter) / 2;
    int offsetY = (scaledSource.height() - diameter) / 2;
    if (offsetX > 0 || offsetY > 0) {
        scaledSource = scaledSource.copy(offsetX, offsetY, diameter, diameter);
    }

    // 设置变换：将图像中心移动到窗口中心，旋转
    QTransform transform;
    transform.translate(diameter / 2.0, diameter / 2.0);  // 移动到圆心
    transform.rotate(rotationAngle);                      // 旋转
    transform.translate(-diameter / 2.0, -diameter / 2.0);  // 移回原点

    // 绘制旋转后的图像
    painter.setTransform(transform);
    painter.drawPixmap(0, 0, scaledSource);

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

void RotatingCircleImage::setImage(const QString &imagePath) {
    // 如果是 file:/// URL 格式，转换为本地路径
    QString localPath = imagePath;
    if (imagePath.startsWith("file:///")) {
        localPath = QUrl(imagePath).toLocalFile();
        qDebug() << "RotatingCircleImage: Converted URL to local path:" << localPath;
    } else if (imagePath.startsWith("qrc:/")) {
        // QRC 资源路径，去掉 qrc: 前缀
        localPath = imagePath.mid(3);  // 移除 "qrc"
        qDebug() << "RotatingCircleImage: Using QRC path:" << localPath;
    }
    
    QPixmap newImage(localPath);
    if (!newImage.isNull()) {
        QMutexLocker locker(&mutex);
        image = newImage;
        // 立即更新显示
        update();
        qDebug() << "RotatingCircleImage: Image loaded successfully, size:" << newImage.size();
    } else {
        qDebug() << "Failed to load image for RotatingCircleImage:" << imagePath;
        qDebug() << "  Tried local path:" << localPath;
    }
}
