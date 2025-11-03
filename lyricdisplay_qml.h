#ifndef LYRICDISPLAY_QML_H
#define LYRICDISPLAY_QML_H

#include <QObject>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>
#include <QDebug>
#include <map>
#include <string>
#include <vector>

class LyricDisplayQml : public QQuickWidget
{
    Q_OBJECT

public:
    int currentLine = -1;  // 当前高亮行号
    explicit LyricDisplayQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        // 设置完全透明背景 - 这是最关键的一句
        
        // 设置透明相关属性
        // setAttribute(Qt::WA_TranslucentBackground, true);
        // setAttribute(Qt::WA_AlwaysStackOnTop, false);
        // setAttribute(Qt::WA_NoSystemBackground, true);
        // setAttribute(Qt::WA_OpaquePaintEvent, false);
        // setAutoFillBackground(false);
        
        // 加载 QML 文件
        setSource(QUrl("qrc:/qml/components/LyricDisplay.qml"));
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        if (status() == QQuickWidget::Error) {
            qWarning() << "Failed to load LyricDisplay.qml:" << errors();
        } else {
            qDebug() << "LyricDisplay.qml loaded successfully";
        }
    }

    // 清空歌词
    void clearLyrics() {
        QMetaObject::invokeMethod(rootObject(), "clearLyrics");
    }

    // 设置歌词列表
    void setLyrics(const QStringList &lyrics) {
        QVariant lyricsVar = QVariant::fromValue(lyrics);
        QMetaObject::invokeMethod(rootObject(), "setLyrics",
                                  Q_ARG(QVariant, lyricsVar));
    }

    // 设置歌词列表（从 std::map 转换，包含时间）
    void setLyrics(const std::map<int, std::string> &lyricsMap) {
        QVariantList lyricsWithTime;
        lyricTimes.clear();
        
        for (const auto& [time, text] : lyricsMap) {
            QVariantMap lyricItem;
            lyricItem["text"] = QString::fromStdString(text);
            lyricItem["time"] = formatTime(time);
            lyricsWithTime.append(lyricItem);
            lyricTimes.push_back(time);
        }
        
        QVariant lyricsVar = QVariant::fromValue(lyricsWithTime);
        QMetaObject::invokeMethod(rootObject(), "setLyrics",
                                  Q_ARG(QVariant, lyricsVar));
    }

    // 高亮指定行
    void highlightLine(int lineNumber) {
        currentLine = lineNumber;
        QMetaObject::invokeMethod(rootObject(), "highlightLine",
                                  Q_ARG(QVariant, lineNumber));
    }

    // 滚动到指定行
    void scrollToLine(int lineNumber) {
        QMetaObject::invokeMethod(rootObject(), "scrollToLine",
                                  Q_ARG(QVariant, lineNumber));
    }

    // 设置展开/收起状态
    void setIsUp(bool up) {
        QMetaObject::invokeMethod(rootObject(), "setIsUp",
                                  Q_ARG(QVariant, up));
    }

    // 获取当前行号
    int getCurrentLine() const {
        if (rootObject()) {
            return rootObject()->property("currentLine").toInt();
        }
        return -1;
    }

private:
    std::vector<int> lyricTimes;  // 存储歌词时间映射
    
    // 将时间（毫秒）格式化为 MM:SS 格式
    QString formatTime(int milliseconds) const {
        int seconds = milliseconds / 1000;
        int minutes = seconds / 60;
        seconds = seconds % 60;
        return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    }

signals:
    // 当前歌词改变信号
    void signal_current_lrc(const QString &lyricText);
    // 歌词跳转信号
    void signal_lyric_seek(int timeMs);

private slots:
    void onQmlSignal(const QString &lyricText) {
        qDebug() << "LyricDisplayQml::onQmlSignal called with:" << lyricText;
        emit signal_current_lrc(lyricText);
    }
    
    void onLyricClicked(int lineIndex) {
        qDebug() << "Lyric clicked at index:" << lineIndex;
        
        // 将行索引转换为实际的歌词行（减去前面5行空行）
        int actualLyricIndex = lineIndex - 5;
        
        if (actualLyricIndex >= 0 && actualLyricIndex < static_cast<int>(lyricTimes.size())) {
            int timeMs = lyricTimes[actualLyricIndex];
            qDebug() << "Lyric clicked at line" << actualLyricIndex << "time:" << timeMs << "ms";
            emit signal_lyric_seek(timeMs);
        }
    }

protected:
    void showEvent(QShowEvent *event) override {
        QQuickWidget::showEvent(event);
        
        // 连接 QML 信号到 C++ 槽
        if (rootObject()) {
            connect(rootObject(), SIGNAL(currentLrcChanged(QString)),
                    this, SLOT(onQmlSignal(QString)));
            connect(rootObject(), SIGNAL(lyricClicked(int)),
                    this, SLOT(onLyricClicked(int)));
        }
    }
};

#endif // LYRICDISPLAY_QML_H
