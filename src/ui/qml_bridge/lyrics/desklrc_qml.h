#ifndef DESKLRC_QML_H
#define DESKLRC_QML_H

#include "process_slider_qml.h"
#include "settings_manager.h"

#include <QColor>
#include <QDebug>
#include <QFont>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QObject>
#include <QPoint>
#include <QQuickItem>
#include <QQuickWidget>
#include <QScreen>
#include <QSettings>
#include <QSize>
#include <QTimer>
#include <QVariant>

class DeskLrcQml : public QQuickWidget
{
    Q_OBJECT

signals:
    void signalPlayClicked(int state);
    void signalNextClicked();
    void signalLastClicked();
    void signalForwardClicked();
    void signalBackwardClicked();
    void signalSettingsClicked();
    void signalCloseClicked();

public:
    explicit DeskLrcQml(QWidget* parent = nullptr)
        : QQuickWidget(parent)
        , processSlider(nullptr)
        , qmlSignalsConnected(false)
        , lastCurrentLyricText(QStringLiteral(u"\u2764 \u6682\u65e0\u6b4c\u8bcd \u2764"))
        , lastNextLyricText(QString())
        , lastSongName(QStringLiteral(u"\u6682\u65e0\u6b4c\u66f2"))
        , lastPlayingState(false)
        , playedLyricColor(QStringLiteral("#ded93e"))
        , unplayedLyricColor(QStringLiteral("#4ca6e8"))
        , strokeLyricColor(QStringLiteral("#000000"))
        , lyricFontSize(57)
        , lyricFontFamily(QStringLiteral("Microsoft YaHei"))
        , lyricLineMode(QStringLiteral("double"))
        , lyricAlignment(QStringLiteral("split"))
        , lyricOpacity(100)
        , lyricBold(false)
        , currentLineProgress(0.0)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_AlwaysStackOnTop, true);

        setClearColor(QColor(0, 0, 0, 0));
        setAttribute(Qt::WA_AcceptTouchEvents, true);
        setMouseTracking(true);

        resize(760, 190);
        setMinimumSize(360, 132);
        setMaximumSize(1800, 420);

        loadPersistentStyle();
        loadPersistentGeometry();

        connect(this, &QQuickWidget::statusChanged, this, &DeskLrcQml::onQmlStatusChanged);
        connect(&geometrySaveTimer, &QTimer::timeout, this, &DeskLrcQml::onGeometrySaveTimeout);
        connect(&SettingsManager::instance(), &SettingsManager::settingsCenterChanged, this,
                &DeskLrcQml::onSettingsCenterChanged);

        geometrySaveTimer.setSingleShot(true);
        geometrySaveTimer.setInterval(220);

        setSource(QUrl(QStringLiteral("qrc:/qml/components/lyrics/DeskLyric.qml")));
        setResizeMode(QQuickWidget::SizeRootObjectToView);

        if (status() == QQuickWidget::Error) {
            qWarning() << "Failed to load DeskLyric.qml:" << errors();
        }

        if (status() == QQuickWidget::Ready) {
            onQmlStatusChanged(status());
        }
    }

    void setLyricText(const QString& text)
    {
        setLyricLines(text, QString());
    }

    void setLyricLines(const QString& currentText, const QString& nextText)
    {
        const QString normalizedCurrent = normalizePrimaryLyric(currentText);
        const QString normalizedNext = normalizeSecondaryLyric(nextText);
        if (normalizedCurrent == lastCurrentLyricText && normalizedNext == lastNextLyricText) {
            return;
        }

        lastCurrentLyricText = normalizedCurrent;
        lastNextLyricText = normalizedNext;
        applyLyricContent();
    }

    void setPlayingState(bool isPlaying)
    {
        if (isPlaying == lastPlayingState) {
            return;
        }
        lastPlayingState = isPlaying;
        applyRuntimeState();
    }

    void setCurrentLineProgress(qreal progress)
    {
        const qreal normalized = qBound<qreal>(0.0, progress, 1.0);
        if (qFuzzyCompare(currentLineProgress, normalized)) {
            return;
        }
        currentLineProgress = normalized;
        applyLyricContent();
    }

    void showSettingsDialog()
    {
        if (rootObject()) {
            QMetaObject::invokeMethod(rootObject(), "showSettingsDialog");
        }
    }

    void setSongName(const QString& songName)
    {
        const QString normalized = normalizeSongName(songName);
        if (normalized == lastSongName) {
            return;
        }
        lastSongName = normalized;
        applyRuntimeState();
    }

    void setLyricStyle(const QColor& primaryColor, const QColor& secondaryColor,
                       const QColor& outlineColor, int fontSize, const QFont& font,
                       const QString& lineMode = QString(), const QString& alignment = QString(),
                       int opacity = -1, bool bold = false)
    {
        playedLyricColor = primaryColor.isValid() ? primaryColor : QColor(QStringLiteral("#ded93e"));
        unplayedLyricColor =
            secondaryColor.isValid() ? secondaryColor : QColor(QStringLiteral("#4ca6e8"));
        strokeLyricColor =
            outlineColor.isValid() ? outlineColor : QColor(QStringLiteral("#000000"));
        lyricFontSize = qBound(24, fontSize, 64);
        lyricFontFamily = normalizeFontFamily(font.family());
        lyricLineMode = normalizeLineMode(lineMode.isEmpty() ? lyricLineMode : lineMode);
        lyricAlignment = normalizeAlignment(alignment.isEmpty() ? lyricAlignment : alignment);
        lyricOpacity = opacity < 0 ? lyricOpacity : qBound(0, opacity, 100);
        lyricBold = bold;

        applyPersistentStyle();

        SettingsManager& settings = SettingsManager::instance();
        settings.setDesktopLyricColor(playedLyricColor);
        settings.setDesktopLyricFontSize(lyricFontSize);
        settings.setDesktopLyricFontFamily(lyricFontFamily);
        settings.setDesktopLyricsPlayedColor(playedLyricColor);
        settings.setDesktopLyricsUnplayedColor(unplayedLyricColor);
        settings.setDesktopLyricsStrokeColor(strokeLyricColor);
        settings.setDesktopLyricsLineMode(lyricLineMode);
        settings.setDesktopLyricsAlignment(lyricAlignment);
        settings.setDesktopLyricsOpacity(lyricOpacity);
        settings.setDesktopLyricsBold(lyricBold);
    }

    void setProcessSlider(ProcessSliderQml* slider)
    {
        processSlider = slider;
    }

    QString lyricText() const { return lastCurrentLyricText; }
    QString currentLyricText() const { return lastCurrentLyricText; }
    QString nextLyricText() const { return lastNextLyricText; }
    QString songName() const { return lastSongName; }
    QColor lyricStyleColor() const { return playedLyricColor; }
    QColor playedColor() const { return playedLyricColor; }
    QColor unplayedColor() const { return unplayedLyricColor; }
    QColor strokeColor() const { return strokeLyricColor; }
    int lyricStyleFontSize() const { return lyricFontSize; }
    QString lyricStyleFontFamily() const { return lyricFontFamily; }
    QString lyricLineModeValue() const { return lyricLineMode; }
    QString lyricAlignmentValue() const { return lyricAlignment; }
    int lyricOpacityValue() const { return lyricOpacity; }
    bool lyricBoldValue() const { return lyricBold; }
    qreal currentLineProgressValue() const { return currentLineProgress; }

private slots:
    void onQmlStatusChanged(QQuickWidget::Status status)
    {
        if (status != QQuickWidget::Ready) {
            return;
        }

        connectQmlSignals();
        applyRuntimeState();
        applyLyricContent();
        applyPersistentStyle();
    }

    void onPlayClicked(int state)
    {
        emit signalPlayClicked(state);
        if (processSlider) {
            emit processSlider->signalPlayClicked();
        }
    }

    void onNextClicked()
    {
        emit signalNextClicked();
        if (processSlider) {
            emit processSlider->signalNextSong();
        }
    }

    void onLastClicked()
    {
        emit signalLastClicked();
        if (processSlider) {
            emit processSlider->signalLastSong();
        }
    }

    void onForwardClicked()
    {
        emit signalForwardClicked();
    }

    void onBackwardClicked()
    {
        emit signalBackwardClicked();
    }

    void onSettingsClicked()
    {
        emit signalSettingsClicked();
    }

    void onCloseClicked()
    {
        emit signalCloseClicked();
        saveWindowGeometry();
        hide();
    }

    void onWindowMoved(const QVariant& deltaVar)
    {
        const QPoint delta = deltaVar.toPoint();
        const QPoint targetPos = pos() + delta;
        move(clampWindowPos(targetPos, size()));
        scheduleSaveWindowGeometry();
    }

    void onWindowResized(const QVariant& requestedSizeVar)
    {
        QSize requestedSize = requestedSizeVar.toSize();
        if (!requestedSize.isValid()) {
            const QSizeF asSizeF = requestedSizeVar.toSizeF();
            if (asSizeF.isValid()) {
                requestedSize = asSizeF.toSize();
            }
        }
        if (!requestedSize.isValid()) {
            return;
        }

        QSize target = requestedSize;
        target.setWidth(qBound(minimumWidth(), target.width(), maximumWidth()));
        target.setHeight(qBound(minimumHeight(), target.height(), maximumHeight()));
        resize(target);
        move(clampWindowPos(pos(), target));
        scheduleSaveWindowGeometry();
    }

    void onLyricStyleChanged(const QVariant& styleVar)
    {
        const QVariantMap styleMap = styleVar.toMap();
        if (styleMap.isEmpty()) {
            return;
        }

        const QColor nextPrimary = colorFromMap(styleMap, QStringLiteral("playedColor"),
                                                colorFromMap(styleMap, QStringLiteral("color"),
                                                             playedLyricColor));
        const QColor nextSecondary = colorFromMap(styleMap, QStringLiteral("unplayedColor"),
                                                  unplayedLyricColor);
        const QColor nextStroke =
            colorFromMap(styleMap, QStringLiteral("strokeColor"), strokeLyricColor);
        const int nextFontSize =
            qBound(24, styleMap.value(QStringLiteral("fontSize"), lyricFontSize).toInt(), 64);
        const QString nextFontFamily =
            normalizeFontFamily(styleMap.value(QStringLiteral("fontFamily"), lyricFontFamily)
                                    .toString());
        const QString nextLineMode =
            normalizeLineMode(styleMap.value(QStringLiteral("lineMode"), lyricLineMode).toString());
        const QString nextAlignment = normalizeAlignment(
            styleMap.value(QStringLiteral("alignment"), lyricAlignment).toString());
        const int nextOpacity =
            qBound(0, styleMap.value(QStringLiteral("opacity"), lyricOpacity).toInt(), 100);
        const bool nextBold = styleMap.value(QStringLiteral("bold"), lyricBold).toBool();

        if (playedLyricColor == nextPrimary && unplayedLyricColor == nextSecondary &&
            strokeLyricColor == nextStroke && lyricFontSize == nextFontSize &&
            lyricFontFamily == nextFontFamily && lyricLineMode == nextLineMode &&
            lyricAlignment == nextAlignment && lyricOpacity == nextOpacity &&
            lyricBold == nextBold) {
            return;
        }

        playedLyricColor = nextPrimary;
        unplayedLyricColor = nextSecondary;
        strokeLyricColor = nextStroke;
        lyricFontSize = nextFontSize;
        lyricFontFamily = nextFontFamily;
        lyricLineMode = nextLineMode;
        lyricAlignment = nextAlignment;
        lyricOpacity = nextOpacity;
        lyricBold = nextBold;

        SettingsManager& settings = SettingsManager::instance();
        settings.setDesktopLyricColor(playedLyricColor);
        settings.setDesktopLyricFontSize(lyricFontSize);
        settings.setDesktopLyricFontFamily(lyricFontFamily);
        settings.setDesktopLyricsPlayedColor(playedLyricColor);
        settings.setDesktopLyricsUnplayedColor(unplayedLyricColor);
        settings.setDesktopLyricsStrokeColor(strokeLyricColor);
        settings.setDesktopLyricsLineMode(lyricLineMode);
        settings.setDesktopLyricsAlignment(lyricAlignment);
        settings.setDesktopLyricsOpacity(lyricOpacity);
        settings.setDesktopLyricsBold(lyricBold);

        applyPersistentStyle();
    }

    void onGeometrySaveTimeout()
    {
        saveWindowGeometry();
    }

    void onSettingsCenterChanged()
    {
        const QString previousLineMode = lyricLineMode;
        const QString previousAlignment = lyricAlignment;
        const QColor previousPlayed = playedLyricColor;
        const QColor previousUnplayed = unplayedLyricColor;
        const QColor previousStroke = strokeLyricColor;
        const int previousFontSize = lyricFontSize;
        const QString previousFontFamily = lyricFontFamily;
        const int previousOpacity = lyricOpacity;
        const bool previousBold = lyricBold;

        loadPersistentStyle();

        if (previousLineMode != lyricLineMode || previousAlignment != lyricAlignment ||
            previousPlayed != playedLyricColor || previousUnplayed != unplayedLyricColor ||
            previousStroke != strokeLyricColor || previousFontSize != lyricFontSize ||
            previousFontFamily != lyricFontFamily || previousOpacity != lyricOpacity ||
            previousBold != lyricBold) {
            applyPersistentStyle();
        }
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        QQuickWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        QQuickWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        QQuickWidget::mouseReleaseEvent(event);
    }

private:
    static QString normalizeFontFamily(const QString& family)
    {
        const QString trimmed = family.trimmed();
        return trimmed.isEmpty() ? QStringLiteral("Microsoft YaHei") : trimmed;
    }

    static QString normalizeLineMode(const QString& mode)
    {
        return mode.compare(QStringLiteral("single"), Qt::CaseInsensitive) == 0
                   ? QStringLiteral("single")
                   : QStringLiteral("double");
    }

    static QString normalizeAlignment(const QString& alignment)
    {
        const QString normalized = alignment.trimmed().toLower();
        if (normalized == QStringLiteral("left") || normalized == QStringLiteral("right") ||
            normalized == QStringLiteral("split")) {
            return normalized;
        }
        return QStringLiteral("center");
    }

    static QColor colorFromMap(const QVariantMap& styleMap, const QString& key,
                               const QColor& fallback)
    {
        const QVariant rawValue = styleMap.value(key);
        QColor parsed = rawValue.value<QColor>();
        if (!parsed.isValid()) {
            parsed = QColor(rawValue.toString().trimmed());
        }
        return parsed.isValid() ? parsed : fallback;
    }

    QString normalizePrimaryLyric(const QString& text) const
    {
        const QString trimmed = text.trimmed();
        return trimmed.isEmpty() ? QStringLiteral(u"\u2764 \u6682\u65e0\u6b4c\u8bcd \u2764")
                                 : trimmed;
    }

    QString normalizeSecondaryLyric(const QString& text) const
    {
        return text.trimmed();
    }

    QString normalizeSongName(const QString& text) const
    {
        const QString trimmed = text.trimmed();
        return trimmed.isEmpty() ? QStringLiteral(u"\u6682\u65e0\u6b4c\u66f2") : trimmed;
    }

    void connectQmlSignals()
    {
        if (qmlSignalsConnected || !rootObject()) {
            return;
        }

        connect(rootObject(), SIGNAL(playClicked(int)), this, SLOT(onPlayClicked(int)),
                Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(nextClicked()), this, SLOT(onNextClicked()),
                Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(lastClicked()), this, SLOT(onLastClicked()),
                Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(forwardClicked()), this, SLOT(onForwardClicked()),
                Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(backwardClicked()), this, SLOT(onBackwardClicked()),
                Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(settingsClicked()), this, SLOT(onSettingsClicked()),
                Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(closeClicked()), this, SLOT(onCloseClicked()),
                Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(windowMoved(QVariant)), this, SLOT(onWindowMoved(QVariant)),
                Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(windowResized(QVariant)), this,
                SLOT(onWindowResized(QVariant)), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(lyricStyleChanged(QVariant)), this,
                SLOT(onLyricStyleChanged(QVariant)), Qt::UniqueConnection);

        qmlSignalsConnected = true;
        qDebug() << "DeskLrcQml: QML signals connected";
    }

    void loadPersistentStyle()
    {
        SettingsManager& settings = SettingsManager::instance();
        playedLyricColor = settings.desktopLyricsPlayedColor().isValid()
                               ? settings.desktopLyricsPlayedColor()
                               : QColor(QStringLiteral("#ded93e"));
        unplayedLyricColor = settings.desktopLyricsUnplayedColor().isValid()
                                 ? settings.desktopLyricsUnplayedColor()
                                 : QColor(QStringLiteral("#4ca6e8"));
        strokeLyricColor = settings.desktopLyricsStrokeColor().isValid()
                               ? settings.desktopLyricsStrokeColor()
                               : QColor(QStringLiteral("#000000"));
        lyricFontSize = qBound(24, settings.desktopLyricFontSize(), 64);
        lyricFontFamily = normalizeFontFamily(settings.desktopLyricFontFamily());
        lyricLineMode = normalizeLineMode(settings.desktopLyricsLineMode());
        lyricAlignment = normalizeAlignment(settings.desktopLyricsAlignment());
        lyricOpacity = qBound(0, settings.desktopLyricsOpacity(), 100);
        lyricBold = settings.desktopLyricsBold();
    }

    void loadPersistentGeometry()
    {
        QSettings settings;
        QSize savedSize = settings.value(QStringLiteral("DeskLrc/windowSize"), QSize(760, 190))
                              .toSize();
        savedSize.setWidth(qBound(minimumWidth(), savedSize.width(), maximumWidth()));
        savedSize.setHeight(qBound(minimumHeight(), savedSize.height(), maximumHeight()));
        resize(savedSize);

        const QPoint savedPos =
            settings.value(QStringLiteral("DeskLrc/windowPos"), QPoint(120, 120)).toPoint();
        move(clampWindowPos(savedPos, savedSize));
    }

    void applyRuntimeState()
    {
        if (!rootObject()) {
            return;
        }
        rootObject()->setProperty("songName", lastSongName);
        rootObject()->setProperty("isPlaying", lastPlayingState);
    }

    void applyLyricContent()
    {
        if (!rootObject()) {
            return;
        }
        rootObject()->setProperty("currentLyricText", lastCurrentLyricText);
        rootObject()->setProperty("nextLyricText", lastNextLyricText);
        rootObject()->setProperty("currentLineProgress", currentLineProgress);
        QMetaObject::invokeMethod(rootObject(), "scheduleAutoResize");
    }

    void applyPersistentStyle()
    {
        if (!rootObject()) {
            return;
        }
        rootObject()->setProperty("playedLyricColor", playedLyricColor);
        rootObject()->setProperty("unplayedLyricColor", unplayedLyricColor);
        rootObject()->setProperty("strokeColor", strokeLyricColor);
        rootObject()->setProperty("lyricFontSize", lyricFontSize);
        rootObject()->setProperty("lyricFontFamily", lyricFontFamily);
        rootObject()->setProperty("lineMode", lyricLineMode);
        rootObject()->setProperty("alignment", lyricAlignment);
        rootObject()->setProperty("lyricOpacity", lyricOpacity);
        rootObject()->setProperty("lyricBold", lyricBold);
        QMetaObject::invokeMethod(rootObject(), "scheduleAutoResize");
    }

    void saveWindowGeometry()
    {
        QSettings settings;
        settings.setValue(QStringLiteral("DeskLrc/windowPos"), pos());
        settings.setValue(QStringLiteral("DeskLrc/windowSize"), size());
    }

    void scheduleSaveWindowGeometry()
    {
        geometrySaveTimer.start();
    }

    QPoint clampWindowPos(const QPoint& desiredPos, const QSize& widgetSize) const
    {
        QScreen* screen = QGuiApplication::primaryScreen();
        if (!screen) {
            return desiredPos;
        }

        const QRect area = screen->availableGeometry();
        const int maxX = qMax(area.left(), area.right() - widgetSize.width());
        const int maxY = qMax(area.top(), area.bottom() - widgetSize.height());

        QPoint clamped = desiredPos;
        clamped.setX(qBound(area.left(), clamped.x(), maxX));
        clamped.setY(qBound(area.top(), clamped.y(), maxY));
        return clamped;
    }

private:
    ProcessSliderQml* processSlider;
    bool qmlSignalsConnected;
    QString lastCurrentLyricText;
    QString lastNextLyricText;
    QString lastSongName;
    bool lastPlayingState;
    QColor playedLyricColor;
    QColor unplayedLyricColor;
    QColor strokeLyricColor;
    int lyricFontSize;
    QString lyricFontFamily;
    QString lyricLineMode;
    QString lyricAlignment;
    int lyricOpacity;
    bool lyricBold;
    qreal currentLineProgress;
    QTimer geometrySaveTimer;
};

#endif // DESKLRC_QML_H
