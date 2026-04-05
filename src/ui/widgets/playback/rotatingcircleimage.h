#ifndef ROTATINGCIRCLEIMAGE_H
#define ROTATINGCIRCLEIMAGE_H

#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <cmath>
#include <QDebug>
#include <QPainterPath>

class RotatingCircleImage : public QWidget {
    Q_OBJECT

public:
    enum class DiscVisualStyle {
        DarkVinyl,
        SilverVinyl,
        GoldVinyl
    };

    explicit RotatingCircleImage(QWidget *parent = nullptr);
    void setImage(const QString &imagePath);  // 设置新的图片
    void setDiscVisualStyle(DiscVisualStyle style);
    void setCoverScale(qreal scale);
public slots:
    void onStopRotate(bool flag);

protected:
    void paintEvent(QPaintEvent *event);

private slots:
    void scheduleRotateTask();

private:
    QPixmap preparedCoverForDiameter(int diameter);
    void invalidateCoverCache();

private:
    QPixmap image;         // 原始图像
    qreal angle;           // 当前角度
    QTimer *timer;         // 定时器
    DiscVisualStyle m_discVisualStyle = DiscVisualStyle::DarkVinyl;
    qreal m_coverScale = 0.70;
    QPixmap m_preparedCover;
    int m_preparedCoverDiameter = -1;
};

#endif // ROTATINGCIRCLEIMAGE_H
