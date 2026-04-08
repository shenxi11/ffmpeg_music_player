#include "download_task_model.h"
#include "local_music_cache.h"

DownloadTaskModel::DownloadTaskModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_showCompleted(false)
{
    setupConnections();
}

int DownloadTaskModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_tasks.count();
}

QVariant DownloadTaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_tasks.count())
        return QVariant();

    const DownloadTask& task = m_tasks[index.row()];

    switch (role) {
    case TaskIdRole:
        return task.taskId;
    case FilenameRole:
        return task.filename;
    case SavePathRole:
        return task.savePath;
    case TotalSizeRole:
        return task.totalSize;
    case DownloadedSizeRole:
        return task.downloadedSize;
    case ProgressRole:
        return task.progress();
    case StateRole:
        return static_cast<int>(task.state);
    case StateTextRole:
        switch (task.state) {
        case DownloadState::Waiting:
            return "等待中";
        case DownloadState::Downloading:
            return "下载中";
        case DownloadState::Paused:
            return "已暂停";
        case DownloadState::Completed:
            return "已完成";
        case DownloadState::Failed:
            return "失败";
        case DownloadState::Cancelled:
            return "已取消";
        default:
            return "未知";
        }
    case ErrorMsgRole:
        return task.errorMsg;
    case IsPlayingRole:
        return task.savePath == m_currentPlayingPath;
    case CoverUrlRole:
        return resolveCoverUrl(task);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> DownloadTaskModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TaskIdRole] = "taskId";
    roles[FilenameRole] = "filename";
    roles[SavePathRole] = "savePath";
    roles[TotalSizeRole] = "totalSize";
    roles[DownloadedSizeRole] = "downloadedSize";
    roles[ProgressRole] = "progress";
    roles[StateRole] = "state";
    roles[StateTextRole] = "stateText";
    roles[ErrorMsgRole] = "errorMsg";
    roles[IsPlayingRole] = "isPlaying";
    roles[CoverUrlRole] = "coverUrl";
    return roles;
}

void DownloadTaskModel::refresh(bool showCompleted)
{
    beginResetModel();
    m_showCompleted = showCompleted;
    
    if (showCompleted) {
        m_tasks = DownloadManager::instance().getCompletedTasks();
    } else {
        m_tasks = DownloadManager::instance().getActiveTasks();
    }
    
    endResetModel();
}

void DownloadTaskModel::updateSongMetadata(const QString& savePath, const QString& coverUrl,
                                           const QString& duration)
{
    const QString normalizedPath = savePath.trimmed();
    const QString normalizedCover = coverUrl.trimmed();
    if (normalizedPath.isEmpty()) {
        return;
    }

    if (!normalizedCover.isEmpty()) {
        DownloadManager::instance().updateTaskMetadataBySavePath(normalizedPath, normalizedCover);
    }

    for (int i = 0; i < m_tasks.count(); ++i) {
        DownloadTask& task = m_tasks[i];
        if (task.savePath.trimmed() != normalizedPath) {
            continue;
        }

        bool changed = false;
        if (!normalizedCover.isEmpty() && task.coverUrl.trimmed() != normalizedCover) {
            task.coverUrl = normalizedCover;
            changed = true;
        }

        Q_UNUSED(duration);

        if (changed) {
            const QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {CoverUrlRole});
        }
        return;
    }
}

void DownloadTaskModel::pauseTask(const QString& taskId)
{
    DownloadManager::instance().pauseDownload(taskId);
}

void DownloadTaskModel::resumeTask(const QString& taskId)
{
    DownloadManager::instance().resumeDownload(taskId);
}

void DownloadTaskModel::cancelTask(const QString& taskId)
{
    DownloadManager::instance().cancelDownload(taskId);
}

void DownloadTaskModel::setCurrentPlayingPath(const QString& path)
{
    if (m_currentPlayingPath != path) {
        m_currentPlayingPath = path;
        emit currentPlayingPathChanged();
        // 通知所有行更新isPlaying状态
        if (m_tasks.count() > 0) {
            emit dataChanged(index(0), index(m_tasks.count() - 1), {IsPlayingRole});
        }
    }
}

void DownloadTaskModel::removeTask(const QString& taskId)
{
    qDebug() << "[DownloadTaskModel] Removing task:" << taskId;
    
    // 从模型中移除
    for (int i = 0; i < m_tasks.count(); ++i) {
        if (m_tasks[i].taskId == taskId) {
            beginRemoveRows(QModelIndex(), i, i);
            m_tasks.removeAt(i);
            endRemoveRows();
            break;
        }
    }
    
    // 从下载管理器中删除
    DownloadManager::instance().removeTask(taskId);
}

QString DownloadTaskModel::resolveCoverUrl(const DownloadTask& task) const
{
    const QString taskCover = task.coverUrl.trimmed();
    if (!taskCover.isEmpty()) {
        return taskCover;
    }

    const QString targetPath = task.savePath.trimmed();
    if (targetPath.isEmpty()) {
        return QString();
    }

    const QList<LocalMusicInfo> musicList = LocalMusicCache::instance().getMusicList();
    for (const LocalMusicInfo& info : musicList) {
        if (info.filePath.trimmed() == targetPath && !info.coverUrl.trimmed().isEmpty()) {
            return info.coverUrl.trimmed();
        }
    }

    return QString();
}

void DownloadTaskModel::onDownloadAdded(const QString& taskId, const QString& filename)
{
    Q_UNUSED(filename)
    qDebug() << "[DownloadTaskModel] Download added signal received:" << taskId;
    
    // 任务添加时立即插入到模型中
    if (!m_showCompleted) {
        DownloadTask newTask = DownloadManager::instance().getTask(taskId);
        if (newTask.taskId.isEmpty()) {
            qWarning() << "[DownloadTaskModel] Failed to get task:" << taskId;
            return;
        }
        
        // 检查是否已存在
        for (int i = 0; i < m_tasks.count(); ++i) {
            if (m_tasks[i].taskId == taskId) {
                qDebug() << "[DownloadTaskModel] Task already exists in model:" << taskId;
                return;
            }
        }
        
        // 插入到列表开头
        beginInsertRows(QModelIndex(), 0, 0);
        m_tasks.prepend(newTask);
        endInsertRows();
        
        qDebug() << "[DownloadTaskModel] Task added to model, total tasks:" << m_tasks.count();
    }
}

void DownloadTaskModel::onDownloadStarted(const QString& taskId, const QString& filename)
{
    Q_UNUSED(filename)
    
    // 如果是活跃任务列表，更新任务状态
    if (!m_showCompleted) {
        for (int i = 0; i < m_tasks.count(); ++i) {
            if (m_tasks[i].taskId == taskId) {
                DownloadTask updatedTask = DownloadManager::instance().getTask(taskId);
                m_tasks[i] = updatedTask;
                QModelIndex idx = index(i);
                emit dataChanged(idx, idx, {StateRole, StateTextRole});
                return;
            }
        }
        // 如果没找到，说明是新任务，需要刷新
        refresh(false);
    }
}

void DownloadTaskModel::onDownloadProgress(const QString& taskId, const QString& filename, 
                                            qint64 bytesReceived, qint64 bytesTotal)
{
    Q_UNUSED(filename)
    
    // 查找对应的任务并更新
    for (int i = 0; i < m_tasks.count(); ++i) {
        if (m_tasks[i].taskId == taskId) {
            // 从管理器获取最新数据
            DownloadTask updatedTask = DownloadManager::instance().getTask(taskId);
            m_tasks[i] = updatedTask;
            
            // 只通知相关字段变化
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {ProgressRole, DownloadedSizeRole, TotalSizeRole});
            break;
        }
    }
}

void DownloadTaskModel::onDownloadFinished(const QString& taskId, const QString& filename, const QString& savePath)
{
    Q_UNUSED(filename)
    Q_UNUSED(savePath)
    
    // 如果是活跃任务列表，需要移除已完成的任务
    if (!m_showCompleted) {
        for (int i = 0; i < m_tasks.count(); ++i) {
            if (m_tasks[i].taskId == taskId) {
                beginRemoveRows(QModelIndex(), i, i);
                m_tasks.removeAt(i);
                endRemoveRows();
                return;
            }
        }
    } else {
        // 如果是已完成列表，刷新以显示新完成的任务
        refresh(true);
    }
}

void DownloadTaskModel::onDownloadFailed(const QString& taskId, const QString& filename, const QString& error)
{
    Q_UNUSED(filename)
    Q_UNUSED(error)
    
    // 更新任务状态显示
    if (!m_showCompleted) {
        for (int i = 0; i < m_tasks.count(); ++i) {
            if (m_tasks[i].taskId == taskId) {
                DownloadTask updatedTask = DownloadManager::instance().getTask(taskId);
                m_tasks[i] = updatedTask;
                QModelIndex idx = index(i);
                emit dataChanged(idx, idx);
                return;
            }
        }
    }
}

void DownloadTaskModel::onDownloadPaused(const QString& taskId, const QString& filename)
{
    Q_UNUSED(filename)
    
    // 更新任务状态
    if (!m_showCompleted) {
        for (int i = 0; i < m_tasks.count(); ++i) {
            if (m_tasks[i].taskId == taskId) {
                DownloadTask updatedTask = DownloadManager::instance().getTask(taskId);
                m_tasks[i] = updatedTask;
                QModelIndex idx = index(i);
                emit dataChanged(idx, idx, {StateRole, StateTextRole});
                return;
            }
        }
    }
}

void DownloadTaskModel::onDownloadCancelled(const QString& taskId, const QString& filename)
{
    Q_UNUSED(filename)
    
    // 从活跃列表移除已取消的任务
    if (!m_showCompleted) {
        for (int i = 0; i < m_tasks.count(); ++i) {
            if (m_tasks[i].taskId == taskId) {
                beginRemoveRows(QModelIndex(), i, i);
                m_tasks.removeAt(i);
                endRemoveRows();
                return;
            }
        }
    }
}

void DownloadTaskModel::onTaskRemoved(const QString& taskId, const QString& filename)
{
    Q_UNUSED(filename)
    qDebug() << "[DownloadTaskModel] Task removed signal received:" << taskId;
    
    // 无论哪个列表，都移除该任务
    for (int i = 0; i < m_tasks.count(); ++i) {
        if (m_tasks[i].taskId == taskId) {
            beginRemoveRows(QModelIndex(), i, i);
            m_tasks.removeAt(i);
            endRemoveRows();
            return;
        }
    }
}
