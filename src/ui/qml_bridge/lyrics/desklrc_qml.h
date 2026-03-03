#ifndef DESKLRC_QML_H
#define DESKLRC_QML_H

#include <QObject>
#include <QQuickWidget>
#include <QQuickItem>
#include <QDebug>
#include <QMouseEvent>
#include <QColor>
#include <QFont>
#include <QPoint>
#include <QSize>
#include <QSettings>
#include <QGuiApplication>
#include <QScreen>
#include <QVariant>
#include <QTimer>
#include "process_slider_qml.h"

class DeskLrcQml : public QQuickWidget
{
    Q_OBJECT

signals:
    void signal_play_clicked(int state);
    void signal_next_clicked();
    void signal_last_clicked();
    void signal_forward_clicked();
    void signal_backward_clicked();
    void signal_settings_clicked();
    void signal_close_clicked();

public:
    explicit DeskLrcQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
        , isDragging(false)
        , processSlider(nullptr)
        , qmlSignalsConnected(false)
        , lastLyricText("")
        , lastSongName("")
        , lyricColor("#ffffff")
        , lyricFontSize(18)
        , lyricFontFamily("Microsoft YaHei")
        , pendingPlayingState(false)
        , hasPendingPlayingState(false)
        , lastPlayingState(false)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_AlwaysStackOnTop, true);

        setClearColor(QColor(0, 0, 0, 0));
        setAttribute(Qt::WA_AcceptTouchEvents, true);
        setMouseTracking(true);

        resize(500, 120);
        setMinimumSize(250, 100);
        setMaximumSize(1600, 400);

        loadPersistentSettings();

        connect(this, &QQuickWidget::statusChanged,
                this, &DeskLrcQml::onQmlStatusChanged);

        setSource(QUrl("qrc:/qml/components/lyrics/DeskLyric.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);

        geometrySaveTimer.setSingleShot(true);
        geometrySaveTimer.setInterval(220);
        connect(&geometrySaveTimer, &QTimer::timeout, this, [this]() {
            saveWindowGeometry();
        });

        if (status() == QQuickWidget::Error) {
            qWarning() << "Failed to load DeskLyric.qml:" << errors();
        }

        if (status() == QQuickWidget::Ready) {
            onQmlStatusChanged(status());
        }
    }

    void setLyricText(const QString &text)
    {
        QString normalized = text.trimmed();
        if (normalized.isEmpty()) {
            normalized = QStringLiteral(u"\u2764 \u6682\u65e0\u6b4c\u8bcd \u2764");
        }
        if (normalized == lastLyricText) {
            return;
        }
        lastLyricText = normalized;

        if (rootObject()) {
            QMetaObject::invokeMethod(rootObject(), "setLyricText",
                                      Q_ARG(QVariant, normalized));
        } else {
            pendingLyricText = normalized;
        }
    }

    void setPlayingState(bool isPlaying)
    {
        if (isPlaying == lastPlayingState) {
            return;
        }
        lastPlayingState = isPlaying;

        if (rootObject()) {
            rootObject()->setProperty("isPlaying", isPlaying);
            QMetaObject::invokeMethod(rootObject(), "setPlayButtonState",
                                      Q_ARG(QVariant, isPlaying));
        } else {
            pendingPlayingState = isPlaying;
            hasPendingPlayingState = true;
        }
    }

    void showSettingsDialog()
    {
        if (rootObject()) {
            QMetaObject::invokeMethod(rootObject(), "showSettingsDialog");
        }
    }

    void setSongName(const QString &songName)
    {
        QString normalized = songName.trimmed();
        if (normalized.isEmpty()) {
            normalized = QStringLiteral(u"\u6682\u65e0\u6b4c\u66f2");
        }
        if (normalized == lastSongName) {
            return;
        }
        lastSongName = normalized;

        if (rootObject()) {
            QMetaObject::invokeMethod(rootObject(), "setSongName",
                                      Q_ARG(QVariant, normalized));
        } else {
            pendingSongName = normalized;
        }
    }

    void setLyricStyle(const QColor &color, int fontSize, const QFont &font)
    {
        lyricColor = color.isValid() ? color : QColor("#ffffff");
        lyricFontSize = qBound(12, fontSize, 48);
        lyricFontFamily = font.family().trimmed().isEmpty()
                ? QStringLiteral("Microsoft YaHei")
                : font.family();

        applyPersistentStyle();

        QSettings settings;
        settings.setValue("DeskLrc/color", lyricColor);
        settings.setValue("DeskLrc/fontSize", lyricFontSize);
        settings.setValue("DeskLrc/fontFamily", lyricFontFamily);
    }

    void setProcessSlider(ProcessSliderQml* slider)
    {
        processSlider = slider;
    }

private slots:
    void onQmlStatusChanged(QQuickWidget::Status status)
    {
        if (status != QQuickWidget::Ready) {
            return;
        }
        connectQmlSignals();
        applyPersistentStyle();

        if (!pendingLyricText.isEmpty()) {
            QMetaObject::invokeMethod(rootObject(), "setLyricText",
                                      Q_ARG(QVariant, pendingLyricText));
            pendingLyricText.clear();
        }

        if (!pendingSongName.isEmpty()) {
            QMetaObject::invokeMethod(rootObject(), "setSongName",
                                      Q_ARG(QVariant, pendingSongName));
            pendingSongName.clear();
        }

        if (hasPendingPlayingState) {
            QMetaObject::invokeMethod(rootObject(), "setPlayButtonState",
                                      Q_ARG(QVariant, pendingPlayingState));
            hasPendingPlayingState = false;
        }
    }

    void onPlayClicked(int state)
    {
        emit signal_play_clicked(state);
        if (processSlider) {
            emit processSlider->signal_play_clicked();
        }
    }

    void onNextClicked()
    {
        emit signal_next_clicked();
        if (processSlider) {
            emit processSlider->signal_nextSong();
        }
    }

    void onLastClicked()
    {
        emit signal_last_clicked();
        if (processSlider) {
            emit processSlider->signal_lastSong();
        }
    }

    void onForwardClicked()
    {
        emit signal_forward_clicked();
    }

    void onBackwardClicked()
    {
        emit signal_backward_clicked();
    }

    void onSettingsClicked()
    {
        emit signal_settings_clicked();
    }

    void onCloseClicked()
    {
        emit signal_close_clicked();
        saveWindowGeometry();
        hide();
    }

    void onWindowMoved(const QVariant &deltaVar)
    {
        const QPoint delta = deltaVar.toPoint();
        const QPoint targetPos = pos() + delta;
        move(clampWindowPos(targetPos, size()));
        scheduleSaveWindowGeometry();
    }

    void onWindowResized(const QVariant &requestedSizeVar)
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

    void onLyricStyleChanged(const QVariant& colorVar,
                             int fontSize,
                             const QString& fontFamily)
    {
        const QColor parsedColor = colorVar.value<QColor>();
        const int parsedSize = qBound(12, fontSize, 48);
        const QString parsedFamily = fontFamily.trimmed();

        const QColor nextColor = parsedColor.isValid() ? parsedColor : QColor("#ffffff");
        const QString nextFamily = parsedFamily.isEmpty() ? QStringLiteral("Microsoft YaHei") : parsedFamily;
        if (lyricColor == nextColor && lyricFontSize == parsedSize && lyricFontFamily == nextFamily) {
            return;
        }

        lyricColor = nextColor;
        lyricFontSize = parsedSize;
        lyricFontFamily = nextFamily;

        QSettings settings;
        settings.setValue("DeskLrc/color", lyricColor);
        settings.setValue("DeskLrc/fontSize", lyricFontSize);
        settings.setValue("DeskLrc/fontFamily", lyricFontFamily);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            if (event->y() < 35) {
                QQuickWidget::mousePressEvent(event);
                return;
            }
            isDragging = true;
            dragStartPos = event->globalPos() - frameGeometry().topLeft();
            event->accept();
            return;
        }
        QQuickWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (isDragging && (event->buttons() & Qt::LeftButton)) {
            const QPoint target = event->globalPos() - dragStartPos;
            move(clampWindowPos(target, size()));
            event->accept();
            return;
        }
        QQuickWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            isDragging = false;
            scheduleSaveWindowGeometry();
        }
        QQuickWidget::mouseReleaseEvent(event);
    }

private:
    void connectQmlSignals()
    {
        if (qmlSignalsConnected || !rootObject()) {
            return;
        }

        connect(rootObject(), SIGNAL(playClicked(int)),
                this, SLOT(onPlayClicked(int)), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(nextClicked()),
                this, SLOT(onNextClicked()), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(lastClicked()),
                this, SLOT(onLastClicked()), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(forwardClicked()),
                this, SLOT(onForwardClicked()), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(backwardClicked()),
                this, SLOT(onBackwardClicked()), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(settingsClicked()),
                this, SLOT(onSettingsClicked()), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(closeClicked()),
                this, SLOT(onCloseClicked()), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(windowMoved(QVariant)),
                this, SLOT(onWindowMoved(QVariant)), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(windowResized(QVariant)),
                this, SLOT(onWindowResized(QVariant)), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(lyricStyleChanged(QVariant,int,QString)),
                this, SLOT(onLyricStyleChanged(QVariant,int,QString)), Qt::UniqueConnection);

        qmlSignalsConnected = true;
        qDebug() << "DeskLrcQml: QML signals connected";
    }

    void loadPersistentSettings()
    {
        QSettings settings;
        lyricColor = settings.value("DeskLrc/color", QColor("#ffffff")).value<QColor>();
        lyricFontSize = qBound(12, settings.value("DeskLrc/fontSize", 18).toInt(), 48);
        lyricFontFamily = settings.value("DeskLrc/fontFamily", QStringLiteral("Microsoft YaHei")).toString().trimmed();
        if (lyricFontFamily.isEmpty()) {
            lyricFontFamily = QStringLiteral("Microsoft YaHei");
        }

        QSize savedSize = settings.value("DeskLrc/windowSize", QSize(500, 120)).toSize();
        savedSize.setWidth(qBound(minimumWidth(), savedSize.width(), maximumWidth()));
        savedSize.setHeight(qBound(minimumHeight(), savedSize.height(), maximumHeight()));
        resize(savedSize);

        QPoint savedPos = settings.value("DeskLrc/windowPos", QPoint(120, 120)).toPoint();
        move(clampWindowPos(savedPos, savedSize));
    }

    void applyPersistentStyle()
    {
        if (!rootObject()) {
            return;
        }
        QMetaObject::invokeMethod(rootObject(), "setLyricStyle",
                                  Q_ARG(QVariant, lyricColor),
                                  Q_ARG(QVariant, lyricFontSize),
                                  Q_ARG(QVariant, lyricFontFamily));
    }

    void saveWindowGeometry()
    {
        QSettings settings;
        settings.setValue("DeskLrc/windowPos", pos());
        settings.setValue("DeskLrc/windowSize", size());
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
    bool isDragging;
    QPoint dragStartPos;
    ProcessSliderQml* processSlider;

    bool qmlSignalsConnected;
    QString pendingLyricText;
    QString pendingSongName;
    bool pendingPlayingState;
    bool hasPendingPlayingState;
    QString lastLyricText;
    QString lastSongName;
    bool lastPlayingState;

    QColor lyricColor;
    int lyricFontSize;
    QString lyricFontFamily;
    QTimer geometrySaveTimer;
};

#endif // DESKLRC_QML_H

