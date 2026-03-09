#ifndef LYRICDISPLAY_QML_H
#define LYRICDISPLAY_QML_H

#include <QDebug>
#include <QObject>
#include <QQuickItem>
#include <QQuickWidget>
#include <QVariantList>
#include <QVariantMap>
#include <map>
#include <string>
#include <vector>

class LyricDisplayQml : public QQuickWidget
{
    Q_OBJECT

public:
    int currentLine = -1;

    explicit LyricDisplayQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        setSource(QUrl("qrc:/qml/components/lyrics/LyricDisplay.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);

        if (status() == QQuickWidget::Error) {
            qWarning() << "Failed to load LyricDisplay.qml:" << errors();
        } else {
            qDebug() << "LyricDisplay.qml loaded successfully";
        }

        connectQmlSignals();
    }

    void clearLyrics()
    {
        QMetaObject::invokeMethod(rootObject(), "clearLyrics");
    }

    void setLyrics(const QStringList &lyrics)
    {
        const QVariant lyricsVar = QVariant::fromValue(lyrics);
        QMetaObject::invokeMethod(rootObject(), "setLyrics", Q_ARG(QVariant, lyricsVar));
    }

    void setLyrics(const std::map<int, std::string> &lyricsMap)
    {
        QVariantList lyricsWithTime;
        lyricTimes.clear();

        for (const auto &[time, text] : lyricsMap) {
            QVariantMap lyricItem;
            lyricItem["text"] = QString::fromStdString(text);
            lyricItem["time"] = formatTime(time);
            lyricItem["timeMs"] = time;
            lyricsWithTime.append(lyricItem);
            lyricTimes.push_back(time);
        }

        const QVariant lyricsVar = QVariant::fromValue(lyricsWithTime);
        QMetaObject::invokeMethod(rootObject(), "setLyrics", Q_ARG(QVariant, lyricsVar));
    }

    void highlightLine(int lineNumber)
    {
        currentLine = lineNumber;
        QMetaObject::invokeMethod(rootObject(), "highlightLine", Q_ARG(QVariant, lineNumber));
    }

    void scrollToLine(int lineNumber)
    {
        QMetaObject::invokeMethod(rootObject(), "scrollToLine", Q_ARG(QVariant, lineNumber));
    }

    void setIsUp(bool up)
    {
        QMetaObject::invokeMethod(rootObject(), "setIsUp", Q_ARG(QVariant, up));
    }

    void setCenterYOffset(double offset)
    {
        if (rootObject()) {
            rootObject()->setProperty("centerYOffset", offset);
        }
    }

    void setSongInfo(const QString &title, const QString &artist = QString())
    {
        QMetaObject::invokeMethod(rootObject(), "setSongInfo",
                                  Q_ARG(QVariant, title),
                                  Q_ARG(QVariant, artist));
    }

    void setSimilarSongs(const QVariantList& items)
    {
        const QVariant itemsVar = QVariant::fromValue(items);
        QMetaObject::invokeMethod(rootObject(), "setSimilarSongs", Q_ARG(QVariant, itemsVar));
    }

    void clearSimilarSongs()
    {
        QMetaObject::invokeMethod(rootObject(), "clearSimilarSongs");
    }

    int getCurrentLine() const
    {
        if (rootObject()) {
            return rootObject()->property("currentLine").toInt();
        }
        return -1;
    }

signals:
    void signal_current_lrc(const QString &lyricText);
    void signal_lyric_seek(int timeMs);
    void signal_lyric_drag_start();
    void signal_lyric_drag_preview(int timeMs);
    void signal_lyric_drag_end();
    void signal_similar_play_requested(const QVariantMap& item);

private slots:
    void onQmlSignal(const QString &lyricText)
    {
        emit signal_current_lrc(lyricText);
    }

    void onLyricClicked(int lineIndex)
    {
        const int actualLyricIndex = lineIndex - 5;
        if (actualLyricIndex >= 0 && actualLyricIndex < static_cast<int>(lyricTimes.size())) {
            const int timeMs = lyricTimes[actualLyricIndex];
            emit signal_lyric_seek(timeMs);
        }
    }

    void onLyricDragStarted()
    {
        emit signal_lyric_drag_start();
    }

    void onLyricDragPreview(int timeMs)
    {
        emit signal_lyric_drag_preview(timeMs);
    }

    void onLyricDragSeek(int timeMs)
    {
        emit signal_lyric_seek(timeMs);
    }

    void onLyricDragEnded()
    {
        emit signal_lyric_drag_end();
    }

    void onSimilarPlayRequested(const QVariant& item)
    {
        emit signal_similar_play_requested(item.toMap());
    }

private:
    std::vector<int> lyricTimes;
    bool m_qmlSignalsConnected = false;

    QString formatTime(int milliseconds) const
    {
        int seconds = milliseconds / 1000;
        const int minutes = seconds / 60;
        seconds %= 60;
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }

    void connectQmlSignals()
    {
        if (m_qmlSignalsConnected || !rootObject()) {
            return;
        }

        connect(rootObject(), SIGNAL(currentLrcChanged(QString)),
                this, SLOT(onQmlSignal(QString)), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(lyricClicked(int)),
                this, SLOT(onLyricClicked(int)), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(lyricDragStarted()),
                this, SLOT(onLyricDragStarted()), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(lyricDragPreview(int)),
                this, SLOT(onLyricDragPreview(int)), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(lyricDragSeek(int)),
                this, SLOT(onLyricDragSeek(int)), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(lyricDragEnded()),
                this, SLOT(onLyricDragEnded()), Qt::UniqueConnection);
        connect(rootObject(), SIGNAL(similarPlayRequested(QVariant)),
                this, SLOT(onSimilarPlayRequested(QVariant)), Qt::UniqueConnection);

        m_qmlSignalsConnected = true;
    }
};

#endif // LYRICDISPLAY_QML_H

