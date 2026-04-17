#include "turntable_gl_widget.h"

#include <QColor>
#include <QHideEvent>
#include <QImage>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QShowEvent>
#include <QUrl>

#include <cmath>

namespace {

constexpr qreal kSpinTargetDegreesPerSecond = 92.0;
constexpr qreal kSpinAccelerationDegreesPerSecond = 260.0;
constexpr qreal kNeedleMoveSpeedPerSecond = 4.3;
constexpr qreal kParkedNeedleDegrees = 0.0;
constexpr qreal kPlayingNeedleDegrees = 30.0;
constexpr qreal kTurntableLayoutScale = 0.70;
constexpr int kAnimationIntervalMs = 16;
constexpr qreal kSpinStopThresholdDegreesPerSecond = 1.0;
constexpr qreal kNeedleStopThreshold = 0.01;

struct DiscPalette {
    QColor sceneColor;
    QColor baseStart;
    QColor baseEnd;
    QColor baseEdge;
    QColor shadowColor;
    QColor discInner;
    QColor discOuter;
    QColor grooveColor;
    QColor ringHighlight;
    QColor labelOuter;
    QColor labelInner;
    QColor spindleOuter;
    QColor spindleInner;
};

struct TurntableLayout {
    qreal side = 0.0;
    qreal baseSide = 0.0;
    qreal baseRadius = 0.0;
    qreal discRadius = 0.0;
    QRectF baseRect;
    QRectF innerGlowRect;
    QRectF discRect;
    QRectF labelRect;
    QPointF discCenter;
    QPointF needlePivot;
};

qreal lerp(qreal a, qreal b, qreal t)
{
    return a + (b - a) * t;
}

DiscPalette paletteForStyle(TurntableGlWidget::DiscVisualStyle style)
{
    switch (style) {
    case TurntableGlWidget::DiscVisualStyle::SilverVinyl:
        return {QColor("#EEF6F9"), QColor("#FFFFFF"), QColor("#E4EBF0"), QColor(215, 224, 232, 220),
                QColor(52, 76, 92, 34), QColor("#F9FBFD"), QColor("#B7C6D2"),
                QColor(255, 255, 255, 34), QColor(255, 255, 255, 160), QColor("#48535C"),
                QColor("#2F363D"), QColor("#FFFFFF"), QColor("#CCD6DE")};
    case TurntableGlWidget::DiscVisualStyle::GoldVinyl:
        return {QColor("#F5F1EA"), QColor("#FFFDF9"), QColor("#F0E4D0"), QColor(225, 206, 171, 220),
                QColor(82, 60, 28, 30), QColor("#F9E7BC"), QColor("#B88D33"),
                QColor(255, 248, 220, 28), QColor(255, 243, 188, 160), QColor("#6A4D1B"),
                QColor("#4F3813"), QColor("#FFF8E5"), QColor("#D9BE84")};
    case TurntableGlWidget::DiscVisualStyle::DarkVinyl:
    default:
        return {QColor("#EEF5F8"), QColor("#FBFCFD"), QColor("#E3EAF0"), QColor(214, 222, 230, 220),
                QColor(34, 56, 74, 28), QColor("#767B82"), QColor("#262B31"),
                QColor(255, 255, 255, 20), QColor(220, 230, 240, 100), QColor("#4C382D"),
                QColor("#35241C"), QColor("#F3F5F7"), QColor("#C8D0D8")};
    }
}

QColor softened(const QColor& color, qreal alphaFactor)
{
    QColor result = color;
    result.setAlphaF(qBound<qreal>(0.0, color.alphaF() * alphaFactor, 1.0));
    return result;
}

TurntableLayout buildTurntableLayout(const QSize& widgetSize, qreal coverScale)
{
    TurntableLayout layout;
    layout.side = qMin(widgetSize.width(), widgetSize.height());
    if (layout.side <= 2.0) {
        return layout;
    }

    const qreal outerPadding = qMax<qreal>(12.0, layout.side * 0.035);
    const QRectF frameRect(QPointF(outerPadding, outerPadding),
                           QSizeF(qMax<qreal>(0.0, widgetSize.width() - outerPadding * 2.0),
                                  qMax<qreal>(0.0, widgetSize.height() - outerPadding * 2.0)));
    if (frameRect.width() <= 0.0 || frameRect.height() <= 0.0) {
        layout.side = 0.0;
        return layout;
    }

    const QSizeF scaledBaseSize(frameRect.width() * kTurntableLayoutScale,
                                frameRect.height() * kTurntableLayoutScale);
    layout.baseRect = QRectF(frameRect.center().x() - scaledBaseSize.width() / 2.0,
                             frameRect.center().y() - scaledBaseSize.height() / 2.0,
                             scaledBaseSize.width(), scaledBaseSize.height());
    layout.baseSide = qMin(layout.baseRect.width(), layout.baseRect.height());
    if (layout.baseSide <= 0.0) {
        layout.side = 0.0;
        return layout;
    }
    layout.baseRadius = layout.baseSide * 0.11;
    layout.innerGlowRect =
        layout.baseRect.adjusted(layout.baseSide * 0.015, layout.baseSide * 0.015,
                                 -layout.baseSide * 0.015, -layout.baseSide * 0.015);
    layout.discCenter = layout.baseRect.center();
    layout.discRadius = layout.baseSide * 0.45;
    layout.discRect = QRectF(layout.discCenter.x() - layout.discRadius,
                             layout.discCenter.y() - layout.discRadius,
                             layout.discRadius * 2.0, layout.discRadius * 2.0);

    const qreal labelRadius = layout.discRadius * coverScale;
    layout.labelRect = QRectF(layout.discCenter.x() - labelRadius, layout.discCenter.y() - labelRadius,
                              labelRadius * 2.0, labelRadius * 2.0);
    layout.needlePivot = QPointF(layout.baseRect.left() + layout.baseRect.width() * 0.82,
                                 layout.baseRect.top() + layout.baseRect.height() * 0.09);
    return layout;
}

void drawFallbackNeedle(QPainter& painter, const QPointF& pivot, qreal angleDegrees, qreal side)
{
    painter.save();
    painter.translate(pivot);
    painter.rotate(angleDegrees);

    const qreal armHeight = side * 0.52;
    const QRectF pivotCap(-side * 0.040, -side * 0.040, side * 0.080, side * 0.080);
    const QRectF armRect(-side * 0.010, 0.0, side * 0.020, armHeight * 0.82);
    const QRectF headRect(-side * 0.030, armHeight * 0.74, side * 0.060, side * 0.110);

    painter.setPen(Qt::NoPen);

    painter.setBrush(QColor(235, 239, 242));
    painter.drawEllipse(pivotCap);

    QLinearGradient armGradient(armRect.topLeft(), armRect.bottomRight());
    armGradient.setColorAt(0.0, QColor(250, 251, 252));
    armGradient.setColorAt(0.5, QColor(188, 193, 198));
    armGradient.setColorAt(1.0, QColor(248, 249, 250));
    painter.setBrush(armGradient);
    painter.drawRoundedRect(armRect, side * 0.012, side * 0.012);

    QLinearGradient headGradient(headRect.topLeft(), headRect.bottomRight());
    headGradient.setColorAt(0.0, QColor(246, 247, 249));
    headGradient.setColorAt(1.0, QColor(198, 201, 206));
    painter.setBrush(headGradient);
    painter.drawRoundedRect(headRect, side * 0.014, side * 0.014);

    painter.setBrush(QColor("#2E343A"));
    painter.drawRect(QRectF(-side * 0.008, armHeight * 0.83, side * 0.016, side * 0.028));

    painter.restore();
}

} // namespace

TurntableGlWidget::TurntableGlWidget(QWidget* parent)
    : QWidget(parent), m_image(":/new/prefix1/icon/maxresdefault.jpg")
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    setFocusPolicy(Qt::NoFocus);

    m_needlePixmap = QPixmap(":/qml/assets/turntable/needle.png");
    if (!m_needlePixmap.isNull()) {
        m_needleHasAlpha = m_needlePixmap.toImage().hasAlphaChannel();
    }

    m_animationTimer = new QTimer(this);
    m_animationTimer->setTimerType(Qt::PreciseTimer);
    m_animationTimer->setInterval(kAnimationIntervalMs);
    connect(m_animationTimer, &QTimer::timeout, this, &TurntableGlWidget::advanceAnimation);
}

TurntableGlWidget::~TurntableGlWidget() = default;

QPixmap TurntableGlWidget::preparedCoverForDiameter(int diameter)
{
    if (m_image.isNull() || diameter <= 0) {
        return QPixmap();
    }

    if (!m_preparedCover.isNull() && m_preparedCoverDiameter == diameter) {
        return m_preparedCover;
    }

    QPixmap scaledSource = m_image.scaled(diameter, diameter, Qt::KeepAspectRatioByExpanding,
                                          Qt::SmoothTransformation);
    const int offsetX = qMax(0, (scaledSource.width() - diameter) / 2);
    const int offsetY = qMax(0, (scaledSource.height() - diameter) / 2);
    if (offsetX > 0 || offsetY > 0) {
        scaledSource = scaledSource.copy(offsetX, offsetY, diameter, diameter);
    }

    m_preparedCover = scaledSource;
    m_preparedCoverDiameter = diameter;
    return m_preparedCover;
}

void TurntableGlWidget::invalidateCoverCache()
{
    m_preparedCover = QPixmap();
    m_preparedCoverDiameter = -1;
}

void TurntableGlWidget::invalidateStaticSceneCache()
{
    m_staticSceneCacheDirty = true;
    m_staticSceneCache = QPixmap();
    m_staticSceneCacheSize = QSize();
}

void TurntableGlWidget::invalidateNeedleCache()
{
    m_scaledNeedlePixmap = QPixmap();
    m_scaledNeedleTargetHeight = -1;
}

void TurntableGlWidget::ensureStaticSceneCache()
{
    if (size().isEmpty()) {
        invalidateStaticSceneCache();
        return;
    }

    const bool cacheMatchesCurrentState =
        !m_staticSceneCacheDirty && !m_staticSceneCache.isNull() && m_staticSceneCacheSize == size() &&
        m_staticSceneCacheStyle == m_discVisualStyle &&
        qFuzzyCompare(m_staticSceneCacheCoverScale, m_coverScale);
    if (cacheMatchesCurrentState) {
        return;
    }

    const TurntableLayout layout = buildTurntableLayout(size(), m_coverScale);
    if (layout.side <= 2.0) {
        invalidateStaticSceneCache();
        return;
    }

    const DiscPalette palette = paletteForStyle(m_discVisualStyle);
    const qreal dpr = devicePixelRatioF();
    QPixmap cache(qMax(1, qRound(width() * dpr)), qMax(1, qRound(height() * dpr)));
    cache.setDevicePixelRatio(dpr);
    cache.fill(Qt::transparent);

    QPainter painter(&cache);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPainterPath shadowPath;
    shadowPath.addRoundedRect(layout.baseRect.translated(0.0, layout.baseSide * 0.055),
                              layout.baseRadius, layout.baseRadius);
    painter.fillPath(shadowPath, softened(palette.shadowColor, 0.85));

    QLinearGradient baseGradient(layout.baseRect.topLeft(), layout.baseRect.bottomRight());
    baseGradient.setColorAt(0.0, palette.baseStart);
    baseGradient.setColorAt(1.0, palette.baseEnd);

    painter.setPen(QPen(palette.baseEdge, qMax<qreal>(1.0, layout.baseSide * 0.004)));
    painter.setBrush(baseGradient);
    painter.drawRoundedRect(layout.baseRect, layout.baseRadius, layout.baseRadius);

    painter.setPen(
        QPen(QColor(255, 255, 255, 124), qMax<qreal>(1.0, layout.baseSide * 0.0035)));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(layout.innerGlowRect, layout.baseRadius * 0.92,
                            layout.baseRadius * 0.92);

    QRadialGradient discShadow(layout.discCenter + QPointF(0.0, layout.baseSide * 0.030),
                               layout.discRadius * 1.08);
    discShadow.setColorAt(0.0, QColor(34, 52, 68, 28));
    discShadow.setColorAt(1.0, QColor(34, 52, 68, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(discShadow);
    painter.drawEllipse(layout.discRect.adjusted(-layout.baseSide * 0.018, -layout.baseSide * 0.010,
                                                 layout.baseSide * 0.018,
                                                 layout.baseSide * 0.026));

    QRadialGradient discGradient(layout.discCenter -
                                     QPointF(layout.discRadius * 0.18, layout.discRadius * 0.22),
                                 layout.discRadius * 1.08);
    discGradient.setColorAt(0.0, palette.discInner);
    discGradient.setColorAt(0.60, palette.discOuter.lighter(112));
    discGradient.setColorAt(1.0, palette.discOuter);
    painter.setBrush(discGradient);
    painter.drawEllipse(layout.discRect);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(palette.ringHighlight, qMax<qreal>(1.2, layout.baseSide * 0.006)));
    painter.drawEllipse(layout.discRect.adjusted(layout.baseSide * 0.012, layout.baseSide * 0.012,
                                                 -layout.baseSide * 0.012,
                                                 -layout.baseSide * 0.012));

    painter.setPen(QPen(palette.grooveColor, qMax<qreal>(0.8, layout.baseSide * 0.0022)));
    constexpr int grooveCount = 15;
    for (int grooveIndex = 0; grooveIndex < grooveCount; ++grooveIndex) {
        const qreal grooveRatio = static_cast<qreal>(grooveIndex) / grooveCount;
        const qreal grooveRadius = layout.discRadius * (0.96 - grooveRatio * 0.50);
        if (grooveRadius <= layout.discRadius * m_coverScale * 1.06) {
            break;
        }
        painter.drawEllipse(layout.discCenter, grooveRadius, grooveRadius);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(palette.labelOuter);
    painter.drawEllipse(layout.labelRect.adjusted(-layout.baseSide * 0.010, -layout.baseSide * 0.010,
                                                  layout.baseSide * 0.010,
                                                  layout.baseSide * 0.010));
    painter.setBrush(palette.labelInner);
    painter.drawEllipse(layout.labelRect);

    painter.end();

    m_staticSceneCache = cache;
    m_staticSceneCacheSize = size();
    m_staticSceneCacheStyle = m_discVisualStyle;
    m_staticSceneCacheCoverScale = m_coverScale;
    m_staticSceneCacheDirty = false;
}

QPixmap TurntableGlWidget::preparedNeedleForHeight(int targetHeight)
{
    if (m_needlePixmap.isNull() || !m_needleHasAlpha || targetHeight <= 0) {
        return QPixmap();
    }

    if (!m_scaledNeedlePixmap.isNull() && m_scaledNeedleTargetHeight == targetHeight) {
        return m_scaledNeedlePixmap;
    }

    m_scaledNeedlePixmap =
        m_needlePixmap.scaledToHeight(targetHeight, Qt::SmoothTransformation);
    m_scaledNeedleTargetHeight = targetHeight;
    return m_scaledNeedlePixmap;
}

void TurntableGlWidget::setImage(const QString& imagePath)
{
    QString localPath = imagePath.trimmed();
    if (localPath.startsWith("file:///")) {
        localPath = QUrl(localPath).toLocalFile();
    } else if (localPath.startsWith("qrc:/")) {
        localPath = localPath.mid(3);
    }

    QPixmap newImage(localPath);
    if (newImage.isNull()) {
        newImage = QPixmap(":/new/prefix1/icon/maxresdefault.jpg");
    }

    if (newImage.cacheKey() == m_image.cacheKey()) {
        return;
    }

    m_image = newImage;
    invalidateCoverCache();
    update();
}

void TurntableGlWidget::setDiscVisualStyle(DiscVisualStyle style)
{
    if (m_discVisualStyle == style) {
        return;
    }

    m_discVisualStyle = style;
    invalidateStaticSceneCache();
    update();
}

void TurntableGlWidget::setCoverScale(qreal scale)
{
    const qreal normalized = qBound<qreal>(0.42, scale, 0.88);
    if (qFuzzyCompare(m_coverScale, normalized)) {
        return;
    }

    m_coverScale = normalized;
    invalidateStaticSceneCache();
    invalidateCoverCache();
    update();
}

void TurntableGlWidget::setPlaying(bool playing)
{
    if (m_playing == playing) {
        ensureAnimationTimerState();
        return;
    }

    m_playing = playing;
    m_frameTimer.restart();
    ensureAnimationTimerState();
    update();
}

void TurntableGlWidget::onStopRotate(bool flag)
{
    setPlaying(flag);
}

void TurntableGlWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const TurntableLayout layout = buildTurntableLayout(size(), m_coverScale);
    if (layout.side <= 2.0) {
        return;
    }

    ensureStaticSceneCache();
    if (!m_staticSceneCache.isNull()) {
        painter.drawPixmap(0, 0, m_staticSceneCache);
    }

    const QPixmap coverPixmap = preparedCoverForDiameter(qMax(1, qRound(layout.labelRect.width())));
    if (!coverPixmap.isNull()) {
        painter.save();
        QPainterPath clipPath;
        clipPath.addEllipse(layout.labelRect);
        painter.setClipPath(clipPath);
        painter.translate(layout.labelRect.center());
        painter.rotate(m_discAngle);
        painter.translate(-layout.labelRect.width() / 2.0, -layout.labelRect.height() / 2.0);
        painter.drawPixmap(QRectF(0.0, 0.0, layout.labelRect.width(), layout.labelRect.height()), coverPixmap,
                           QRectF(0.0, 0.0, coverPixmap.width(), coverPixmap.height()));
        painter.restore();
    }

    painter.setBrush(QColor(255, 255, 255, 236));
    painter.setPen(Qt::NoPen);
    const DiscPalette palette = paletteForStyle(m_discVisualStyle);
    painter.drawEllipse(layout.discCenter, layout.discRadius * 0.115, layout.discRadius * 0.115);
    painter.setBrush(palette.spindleOuter);
    painter.drawEllipse(layout.discCenter, layout.discRadius * 0.058, layout.discRadius * 0.058);
    painter.setBrush(palette.spindleInner);
    painter.drawEllipse(layout.discCenter, layout.discRadius * 0.020, layout.discRadius * 0.020);

    const qreal needleAngle = lerp(kParkedNeedleDegrees, kPlayingNeedleDegrees, m_needleProgress);
    const QPixmap scaledNeedle =
        preparedNeedleForHeight(qMax(1, qRound(layout.baseRect.height() * 0.80)));
    if (!scaledNeedle.isNull()) {
        painter.save();
        const QPointF imagePivot(scaledNeedle.width() * 0.50, scaledNeedle.height() * 0.06);
        painter.translate(layout.needlePivot);
        painter.rotate(needleAngle);
        painter.translate(-imagePivot);
        painter.drawPixmap(0, 0, scaledNeedle);
        painter.restore();
    } else {
        drawFallbackNeedle(painter, layout.needlePivot, needleAngle, layout.baseSide);
    }
}

void TurntableGlWidget::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    if (m_animationTimer) {
        m_animationTimer->stop();
    }
}

void TurntableGlWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    invalidateCoverCache();
    invalidateStaticSceneCache();
    invalidateNeedleCache();
}

void TurntableGlWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_frameTimer.restart();
    ensureAnimationTimerState();
    update();
}

void TurntableGlWidget::advanceAnimation()
{
    const qint64 elapsedMs = m_frameTimer.isValid() ? m_frameTimer.restart() : kAnimationIntervalMs;
    const qreal deltaSeconds = qBound<qreal>(0.008, static_cast<qreal>(elapsedMs) / 1000.0, 0.050);

    if (m_playing) {
        m_spinVelocity =
            qMin<qreal>(kSpinTargetDegreesPerSecond,
                        m_spinVelocity + kSpinAccelerationDegreesPerSecond * deltaSeconds);
        m_needleProgress =
            qMin<qreal>(1.0, m_needleProgress + kNeedleMoveSpeedPerSecond * deltaSeconds);
    } else {
        m_spinVelocity =
            qMax<qreal>(0.0, m_spinVelocity - kSpinAccelerationDegreesPerSecond * 1.20 * deltaSeconds);
        m_needleProgress =
            qMax<qreal>(0.0, m_needleProgress - kNeedleMoveSpeedPerSecond * deltaSeconds);
    }

    m_discAngle = std::fmod(m_discAngle + m_spinVelocity * deltaSeconds, 360.0);
    if (m_discAngle < 0.0) {
        m_discAngle += 360.0;
    }

    update();
    ensureAnimationTimerState();
}

void TurntableGlWidget::ensureAnimationTimerState()
{
    if (!m_animationTimer) {
        return;
    }

    const bool shouldAnimate = isVisible() &&
                               (m_playing || m_spinVelocity > kSpinStopThresholdDegreesPerSecond ||
                                m_needleProgress > kNeedleStopThreshold);
    if (shouldAnimate) {
        if (!m_animationTimer->isActive()) {
            m_frameTimer.restart();
            m_animationTimer->start();
        }
    } else {
        m_animationTimer->stop();
    }
}
