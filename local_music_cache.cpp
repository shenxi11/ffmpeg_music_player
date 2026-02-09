#include "local_music_cache.h"

QJsonObject LocalMusicInfo::toJson() const
{
    QJsonObject obj;
    obj["filePath"] = filePath;
    obj["fileName"] = fileName;
    obj["coverUrl"] = coverUrl;
    obj["duration"] = duration;
    obj["artist"] = artist;
    return obj;
}

LocalMusicInfo LocalMusicInfo::fromJson(const QJsonObject& obj)
{
    LocalMusicInfo info;
    info.filePath = obj["filePath"].toString();
    info.fileName = obj["fileName"].toString();
    info.coverUrl = obj["coverUrl"].toString();
    info.duration = obj["duration"].toString();
    info.artist = obj["artist"].toString();
    return info;
}

LocalMusicCache::LocalMusicCache()
    : m_settings("FFmpegMusicPlayer", "LocalMusic")
{
    loadMusicList();
    qDebug() << "[LocalMusicCache] Initialized with" << m_musicList.size() << "songs";
}

void LocalMusicCache::addMusic(const LocalMusicInfo& info)
{
    // 检查是否已存在
    for (int i = 0; i < m_musicList.size(); ++i) {
        if (m_musicList[i].filePath == info.filePath) {
            // 更新已有项
            m_musicList[i] = info;
            saveMusicList();
            emit musicListChanged();
            return;
        }
    }
    
    // 添加新项
    m_musicList.append(info);
    saveMusicList();
    emit musicListChanged();
    
    qDebug() << "[LocalMusicCache] Added music:" << info.fileName;
}

void LocalMusicCache::removeMusic(const QString& filePath)
{
    for (int i = 0; i < m_musicList.size(); ++i) {
        if (m_musicList[i].filePath == filePath) {
            m_musicList.removeAt(i);
            saveMusicList();
            emit musicListChanged();
            qDebug() << "[LocalMusicCache] Removed music:" << filePath;
            return;
        }
    }
}

void LocalMusicCache::updateMetadata(const QString& filePath, const QString& coverUrl, const QString& duration)
{
    for (int i = 0; i < m_musicList.size(); ++i) {
        if (m_musicList[i].filePath == filePath) {
            if (!coverUrl.isEmpty()) {
                m_musicList[i].coverUrl = coverUrl;
            }
            if (!duration.isEmpty()) {
                m_musicList[i].duration = duration;
            }
            saveMusicList();
            emit musicListChanged();
            qDebug() << "[LocalMusicCache] Updated metadata:" << filePath;
            return;
        }
    }
}

QList<LocalMusicInfo> LocalMusicCache::getMusicList() const
{
    return m_musicList;
}

void LocalMusicCache::clearAll()
{
    m_musicList.clear();
    saveMusicList();
    emit musicListChanged();
}

void LocalMusicCache::saveMusicList()
{
    QJsonArray array;
    for (const auto& info : m_musicList) {
        array.append(info.toJson());
    }
    
    QJsonDocument doc(array);
    m_settings.setValue("musicList", doc.toJson(QJsonDocument::Compact));
    m_settings.sync();
}

void LocalMusicCache::loadMusicList()
{
    m_musicList.clear();
    
    QByteArray data = m_settings.value("musicList").toByteArray();
    if (data.isEmpty()) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        return;
    }
    
    QJsonArray array = doc.array();
    for (const QJsonValue& value : array) {
        if (value.isObject()) {
            LocalMusicInfo info = LocalMusicInfo::fromJson(value.toObject());
            
            // 检查文件是否仍然存在
            if (QFileInfo::exists(info.filePath)) {
                m_musicList.append(info);
            } else {
                qDebug() << "[LocalMusicCache] File not found, skipping:" << info.filePath;
            }
        }
    }
}
