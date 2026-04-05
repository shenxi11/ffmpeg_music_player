#include "rotatingcircleimage.h"

#include <QUrl>

RotatingCircleImage::RotatingCircleImage(QWidget *parent)
    : QWidget(parent), angle(0.0)
{
    image = QPixmap(":/new/prefix1/icon/maxresdefault.jpg");
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    setStyleSheet("background: transparent;");

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &RotatingCircleImage::scheduleRotateTask);
    timer->setInterval(30); // 保持现有转盘转速，避免播放页动效节奏突变
}

QPixmap RotatingCircleImage::preparedCoverForDiameter(int diameter)
{
    if (image.isNull() || diameter <= 0) {
        return QPixmap();
    }

    if (!m_preparedCover.isNull() && m_preparedCoverDiameter == diameter) {
        return m_preparedCover;
    }

    QPixmap scaledSource =
        image.scaled(diameter, diameter, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const int offsetX = qMax(0, (scaledSource.width() - diameter) / 2);
    const int offsetY = qMax(0, (scaledSource.height() - diameter) / 2);
    if (offsetX > 0 || offsetY > 0) {
        scaledSource = scaledSource.copy(offsetX, offsetY, diameter, diameter);
    }

    m_preparedCover = scaledSource;
    m_preparedCoverDiameter = diameter;
    return m_preparedCover;
}

void RotatingCircleImage::invalidateCoverCache()
{
    m_preparedCover = QPixmap();
    m_preparedCoverDiameter = -1;
}

void RotatingCircleImage::setDiscVisualStyle(DiscVisualStyle style)
{
    if (m_discVisualStyle == style) {
        return;
    }
    m_discVisualStyle = style;
    update();
}

void RotatingCircleImage::setCoverScale(qreal scale)
{
    const qreal normalized = qBound<qreal>(0.42, scale, 0.88);
    if (qFuzzyCompare(m_coverScale, normalized)) {
        return;
    }
    m_coverScale = normalized;
    update();
}

void RotatingCircleImage::scheduleRotateTask()
{
    if (!isVisible()) {
        return;
    }

    angle = std::fmod(angle + 1.0, 360.0);
    update();
}

void RotatingCircleImage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    const QPoint center(width() / 2, height() / 2);
    const int outerRadius = qMin(width(), height()) / 2;
    const QRect discRect(center.x() - outerRadius, center.y() - outerRadius, outerRadius * 2, outerRadius * 2);

    QRadialGradient discGradient(center, outerRadius);
    QColor grooveColor("#141414");
    QColor edgeColor("#050505");
    QColor centerFill("#232323");

    switch (m_discVisualStyle) {
    case DiscVisualStyle::SilverVinyl:
        discGradient.setColorAt(0.0, QColor(250, 252, 255, 245));
        discGradient.setColorAt(0.40, QColor(214, 226, 232, 228));
        discGradient.setColorAt(0.78, QColor(173, 191, 200, 212));
        discGradient.setColorAt(1.0, QColor(142, 158, 168, 224));
        grooveColor = QColor(255, 255, 255, 22);
        edgeColor = QColor(244, 249, 252, 180);
        centerFill = QColor(54, 73, 81, 255);
        break;
    case DiscVisualStyle::GoldVinyl:
        discGradient.setColorAt(0.0, QColor(255, 244, 188, 245));
        discGradient.setColorAt(0.40, QColor(244, 213, 97, 222));
        discGradient.setColorAt(0.78, QColor(220, 182, 55, 200));
        discGradient.setColorAt(1.0, QColor(176, 139, 28, 215));
        grooveColor = QColor(152, 112, 0, 24);
        edgeColor = QColor(255, 245, 189, 170);
        centerFill = QColor(103, 78, 15, 255);
        break;
    case DiscVisualStyle::DarkVinyl:
    default:
        discGradient.setColorAt(0.0, QColor(60, 60, 60, 255));
        discGradient.setColorAt(0.42, QColor(23, 23, 23, 255));
        discGradient.setColorAt(0.82, QColor(12, 12, 12, 255));
        discGradient.setColorAt(1.0, QColor(5, 5, 5, 255));
        grooveColor = QColor(255, 255, 255, 16);
        edgeColor = QColor(18, 18, 18, 255);
        centerFill = QColor(29, 33, 40, 255);
        break;
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(discGradient);
    painter.drawEllipse(discRect);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(edgeColor, qMax(2, outerRadius / 34)));
    painter.drawEllipse(discRect.adjusted(2, 2, -2, -2));

    painter.setPen(QPen(grooveColor, 1));
    for (int grooveIndex = 0; grooveIndex < 14; ++grooveIndex) {
        const int grooveRadius = outerRadius - 10 - grooveIndex * qMax(3, outerRadius / 28);
        if (grooveRadius <= outerRadius * 0.42) {
            break;
        }
        painter.drawEllipse(center, grooveRadius, grooveRadius);
    }

    if (!image.isNull()) {
        const int innerRadius = static_cast<int>(outerRadius * m_coverScale);
        const QRect innerRect(center.x() - innerRadius, center.y() - innerRadius, innerRadius * 2, innerRadius * 2);
        const QPixmap coverPixmap = preparedCoverForDiameter(innerRect.width());
        if (!coverPixmap.isNull()) {
            painter.save();
            QPainterPath coverClipPath;
            coverClipPath.addEllipse(innerRect);
            painter.setClipPath(coverClipPath);
            painter.translate(innerRect.center());
            painter.rotate(angle);
            painter.translate(-innerRect.width() / 2.0, -innerRect.height() / 2.0);
            painter.drawPixmap(0, 0, coverPixmap);
            painter.restore();
        }
    }

    painter.setBrush(centerFill);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, qMax(8, outerRadius / 10), qMax(8, outerRadius / 10));

    painter.setBrush(QColor(255, 255, 255, 178));
    painter.drawEllipse(center, qMax(2, outerRadius / 28), qMax(2, outerRadius / 28));
}

void RotatingCircleImage::onStopRotate(bool flag)
{
    if (flag) {
        timer->start();
    } else {
        timer->stop();
    }
}

void RotatingCircleImage::setImage(const QString &imagePath)
{
    QString localPath = imagePath;
    if (imagePath.startsWith("file:///")) {
        localPath = QUrl(imagePath).toLocalFile();
        qDebug() << "RotatingCircleImage: Converted URL to local path:" << localPath;
    } else if (imagePath.startsWith("qrc:/")) {
        localPath = imagePath.mid(3);
        qDebug() << "RotatingCircleImage: Using QRC path:" << localPath;
    }

    QPixmap newImage(localPath);
    if (!newImage.isNull()) {
        image = newImage;
        invalidateCoverCache();
        update();
        qDebug() << "RotatingCircleImage: Image loaded successfully, size:" << newImage.size();
    } else {
        qDebug() << "Failed to load image for RotatingCircleImage:" << imagePath;
        qDebug() << "  Tried local path:" << localPath;
    }
}
