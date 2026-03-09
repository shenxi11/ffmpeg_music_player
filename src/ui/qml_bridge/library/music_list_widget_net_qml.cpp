#include "music_list_widget_net_qml.h"
#include <cmath>

namespace {
QString formatDurationForDisplay(double rawDuration)
{
    if (!std::isfinite(rawDuration) || rawDuration <= 0.0) {
        return "0:00";
    }

    // Search API currently mixes milliseconds and seconds across endpoints.
    // Normalize to seconds first, then render as m:ss.
    double secondsValue = rawDuration;
    if (rawDuration > 10000.0) {
        secondsValue = rawDuration / 1000.0;
    }

    qint64 totalSeconds = static_cast<qint64>(secondsValue + 0.5);
    if (totalSeconds < 0) {
        totalSeconds = 0;
    }

    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}
}

MusicListWidgetNetQml::MusicListWidgetNetQml(QWidget *parent)
    : QQuickWidget(parent)
    , m_currentPlayingPath("")
    , m_isPlaying(false)
    , m_songCount(0)
{
    // 加载在线音乐列表 QML。
    setSource(QUrl("qrc:/qml/components/library/MusicListWidgetNet.qml"));
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    // 根对象用于桥接在线列表交互信号。
    QQuickItem* root = rootObject();
    if (root) {
        // 透传播放、删除、下载、收藏等操作。
        connect(root, SIGNAL(playRequested(QString,QString,QString)), 
                this, SIGNAL(signal_play_click(QString,QString,QString)));
        connect(root, SIGNAL(removeRequested(QString)), 
                this, SIGNAL(signal_remove_click(QString)));
        connect(root, SIGNAL(downloadRequested(QString)), 
                this, SIGNAL(signal_download_click(QString)));
        connect(root, SIGNAL(addToFavorite(QString,QString,QString,QString)), 
                this, SIGNAL(addToFavorite(QString,QString,QString,QString)));
    }
    connect(this, &MusicListWidgetNetQml::signal_next, this, &MusicListWidgetNetQml::playNext);
    connect(this, &MusicListWidgetNetQml::signal_last, this, &MusicListWidgetNetQml::playLast);
}

void MusicListWidgetNetQml::setCurrentPlayingPath(const QString& path)
{
    if (m_currentPlayingPath == path) return;
    m_currentPlayingPath = path;
    emit currentPlayingPathChanged();
}

void MusicListWidgetNetQml::setIsPlaying(bool playing)
{
    if (m_isPlaying == playing) return;
    m_isPlaying = playing;
    emit isPlayingChanged();
}

void MusicListWidgetNetQml::setPlayingState(const QString& filePath, bool playing)
{
    if (m_currentPlayingPath != filePath) {
        m_currentPlayingPath = filePath;
        emit currentPlayingPathChanged();
    }
    
    if (m_isPlaying != playing) {
        m_isPlaying = playing;
        emit isPlayingChanged();
    }
    
    // Always forward to QML to keep list row state in sync after model refreshes.
    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "setPlayingState",
            Q_ARG(QVariant, filePath),
            Q_ARG(QVariant, playing));
    }
}

void MusicListWidgetNetQml::addSong(const QString& songName, const QString& filePath, 
             const QString& artist, const QString& duration,
             const QString& cover)
{
    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "addSong",
            Q_ARG(QVariant, songName),
            Q_ARG(QVariant, filePath),
            Q_ARG(QVariant, artist),
            Q_ARG(QVariant, duration),
            Q_ARG(QVariant, cover));
        
        // 每次增项后刷新缓存的歌曲数量。
        updateSongCount();
    }
}

void MusicListWidgetNetQml::addSongList(const QStringList& songNames, const QStringList& relativePaths, const QList<double>& durations, const QStringList& coverUrls, const QStringList& artists)
{
    QQuickItem* root = rootObject();
    if (root) {
        QVariantList nameList;
        QVariantList pathList;
        QVariantList durationList;
        QVariantList coverList;
        QVariantList artistList;
        
        for (const QString& name : songNames) {
            nameList.append(name);
        }
        
        for (const QString& path : relativePaths) {
            pathList.append(path);
        }
        
        for (double duration : durations) {
            durationList.append(formatDurationForDisplay(duration));
        }
        
        for (const QString& cover : coverUrls) {
            coverList.append(cover);
        }
        
        for (const QString& artist : artists) {
            artistList.append(artist);
        }
        
        QMetaObject::invokeMethod(root, "addSongList",
            Q_ARG(QVariant, nameList),
            Q_ARG(QVariant, pathList),
            Q_ARG(QVariant, durationList),
            Q_ARG(QVariant, coverList),
            Q_ARG(QVariant, artistList));
        
        // 批量写入后刷新歌曲总数。
        updateSongCount();
    }
}

void MusicListWidgetNetQml::removeSong(const QString& filePath)
{
    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "removeSong",
            Q_ARG(QVariant, filePath));
        
        // 删除后同步歌曲数量。
        updateSongCount();
    }
}

void MusicListWidgetNetQml::clearAll()
{
    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "clearAll");
        
        // 清空后重置内部计数并通知 UI。
        if (m_songCount != 0) {
            m_songCount = 0;
            emit songCountChanged();
        }
    }
}

int MusicListWidgetNetQml::getCount()
{
    QQuickItem* root = rootObject();
    if (root) {
        QVariant result;
        QMetaObject::invokeMethod(root, "getCount", 
            Q_RETURN_ARG(QVariant, result));
        int count = result.toInt();
        
        // 读数后同步到缓存，避免信号重复发送。
        if (m_songCount != count) {
            m_songCount = count;
            emit songCountChanged();
        }
        
        return count;
    }
    return 0;
}

void MusicListWidgetNetQml::playNext(const QString& songName)
{
    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "playNext",
                                  Q_ARG(QVariant, songName));
    }
}

void MusicListWidgetNetQml::playLast(const QString& songName)
{
    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "playLast",
                                  Q_ARG(QVariant, songName));
    }
}

void MusicListWidgetNetQml::updateSongCount()
{
    int count = getCount();
    if (m_songCount != count) {
        m_songCount = count;
        emit songCountChanged();
    }
}

