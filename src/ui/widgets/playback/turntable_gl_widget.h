#ifndef TURNTABLE_GL_WIDGET_H
#define TURNTABLE_GL_WIDGET_H

#include <QElapsedTimer>
#include <QPixmap>
#include <QSize>
#include <QTimer>
#include <QWidget>

class QHideEvent;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;

/*
模块名: TurntableGlWidget
功能概述: 播放页唱片机组件，负责以纯 2D 方式绘制唱片、封面与唱针动画。
对外接口: setImage、setDiscVisualStyle、setCoverScale、setPlaying、onStopRotate
依赖关系: Qt Widgets、QPainter、QTimer、qrc 图片资源
输入输出: 输入封面路径与播放状态，输出为控件自身的唱片机画面
异常与错误: 图片资源缺失时回退到默认封面和简化唱针绘制，不抛出异常
维护说明: 保留原有类名与公开接口，避免播放页其他模块因重构而联动改动
*/
class TurntableGlWidget : public QWidget
{
    Q_OBJECT

public:
    enum class DiscVisualStyle {
        DarkVinyl = 0,
        SilverVinyl = 1,
        GoldVinyl = 2
    };

    explicit TurntableGlWidget(QWidget* parent = nullptr);
    ~TurntableGlWidget() override;

    void setImage(const QString& imagePath);
    void setDiscVisualStyle(DiscVisualStyle style);
    void setCoverScale(qreal scale);
    void setPlaying(bool playing);

public slots:
    void onStopRotate(bool flag);

protected:
    void paintEvent(QPaintEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void advanceAnimation();

private:
    QPixmap preparedCoverForDiameter(int diameter);
    void invalidateCoverCache();
    void invalidateStaticSceneCache();
    void invalidateNeedleCache();
    void ensureStaticSceneCache();
    QPixmap preparedNeedleForHeight(int targetHeight);
    void ensureAnimationTimerState();

private:
    QPixmap m_image;
    QPixmap m_preparedCover;
    QPixmap m_staticSceneCache;
    QPixmap m_scaledNeedlePixmap;
    QPixmap m_needlePixmap;
    int m_preparedCoverDiameter = -1;
    int m_scaledNeedleTargetHeight = -1;
    bool m_needleHasAlpha = false;
    bool m_staticSceneCacheDirty = true;
    QSize m_staticSceneCacheSize;
    DiscVisualStyle m_staticSceneCacheStyle = DiscVisualStyle::DarkVinyl;
    qreal m_staticSceneCacheCoverScale = 0.0;

    QTimer* m_animationTimer = nullptr;
    QElapsedTimer m_frameTimer;

    DiscVisualStyle m_discVisualStyle = DiscVisualStyle::DarkVinyl;
    qreal m_coverScale = 0.70;
    qreal m_discAngle = 0.0;
    qreal m_spinVelocity = 0.0;
    qreal m_needleProgress = 0.0;
    bool m_playing = false;
};

#endif // TURNTABLE_GL_WIDGET_H
