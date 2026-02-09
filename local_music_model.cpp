#include "local_music_model.h"
#include <QFileInfo>

LocalMusicModel::LocalMusicModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // 连接缓存管理器的信号
    connect(&LocalMusicCache::instance(), &LocalMusicCache::musicListChanged,
            this, &LocalMusicModel::onMusicListChanged);
    
    // 初始加载
    refresh();
}

int LocalMusicModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_musicList.count();
}

QVariant LocalMusicModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_musicList.count())
        return QVariant();

    const LocalMusicInfo& info = m_musicList[index.row()];

    switch (role) {
    case FilePathRole:
        return info.filePath;
    case FileNameRole:
        return info.fileName;
    case CoverUrlRole:
        return info.coverUrl;
    case DurationRole:
        return info.duration;
    case ArtistRole:
        return info.artist;
    case IsPlayingRole:
        return info.filePath == m_currentPlayingPath;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> LocalMusicModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[FilePathRole] = "filePath";
    roles[FileNameRole] = "fileName";
    roles[CoverUrlRole] = "coverUrl";
    roles[DurationRole] = "duration";
    roles[ArtistRole] = "artist";
    roles[IsPlayingRole] = "isPlaying";
    return roles;
}

void LocalMusicModel::refresh()
{
    beginResetModel();
    m_musicList = LocalMusicCache::instance().getMusicList();
    endResetModel();
}

void LocalMusicModel::addMusic(const QString& filePath)
{
    emit addMusicRequested();
}

void LocalMusicModel::removeMusic(const QString& filePath)
{
    LocalMusicCache::instance().removeMusic(filePath);
}

void LocalMusicModel::setCurrentPlayingPath(const QString& path)
{
    if (m_currentPlayingPath != path) {
        m_currentPlayingPath = path;
        emit currentPlayingPathChanged();
        // 通知所有行更新isPlaying状态
        if (m_musicList.count() > 0) {
            emit dataChanged(index(0), index(m_musicList.count() - 1), {IsPlayingRole});
        }
    }
}

void LocalMusicModel::onMusicListChanged()
{
    refresh();
}
