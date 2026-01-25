#include "video_frame_item.h"
#include <QDebug>

VideoFrameItem::VideoFrameItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setFillColor(Qt::black);
}

void VideoFrameItem::setFrame(const QImage& frame)
{
    QMutexLocker locker(&frameMutex);
    
    if (frame.isNull()) {
        return;
    }
    
    currentFrame = frame;
    emit frameChanged();
    
    // 触发重绘
    update();
}

void VideoFrameItem::paint(QPainter *painter)
{
    QMutexLocker locker(&frameMutex);
    
    if (currentFrame.isNull()) {
        // 绘制黑色背景
        painter->fillRect(boundingRect(), Qt::black);
        return;
    }
    
    // 计算居中显示的矩形（保持宽高比）
    QRectF targetRect = boundingRect();
    QSizeF imageSize = currentFrame.size();
    
    // 计算缩放比例（保持宽高比）
    qreal scale = qMin(targetRect.width() / imageSize.width(),
                      targetRect.height() / imageSize.height());
    
    QSizeF scaledSize = imageSize * scale;
    
    // 居中位置
    qreal x = (targetRect.width() - scaledSize.width()) / 2.0;
    qreal y = (targetRect.height() - scaledSize.height()) / 2.0;
    
    QRectF drawRect(x, y, scaledSize.width(), scaledSize.height());
    
    // 绘制黑色背景
    painter->fillRect(targetRect, Qt::black);
    
    // 绘制视频帧
    painter->drawImage(drawRect, currentFrame);
}
