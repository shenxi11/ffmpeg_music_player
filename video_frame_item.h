#ifndef VIDEO_FRAME_ITEM_H
#define VIDEO_FRAME_ITEM_H

#include <QQuickPaintedItem>
#include <QImage>
#include <QPainter>
#include <QMutex>

class VideoFrameItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QImage frame READ frame WRITE setFrame NOTIFY frameChanged)
    
public:
    explicit VideoFrameItem(QQuickItem *parent = nullptr);
    
    QImage frame() const { return currentFrame; }
    void setFrame(const QImage& frame);
    
    void paint(QPainter *painter) override;
    
signals:
    void frameChanged();
    
private:
    QImage currentFrame;
    QMutex frameMutex;
};

#endif // VIDEO_FRAME_ITEM_H
