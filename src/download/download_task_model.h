#ifndef DOWNLOAD_TASK_MODEL_H
#define DOWNLOAD_TASK_MODEL_H

#include <QAbstractListModel>
#include <QHash>
#include "download_manager.h"
#include "local_music_cache.h"

/**
 * @brief 下载任务列表模型，用于QML显示
 */
class DownloadTaskModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString currentPlayingPath READ currentPlayingPath WRITE setCurrentPlayingPath NOTIFY currentPlayingPathChanged)

public:
    enum TaskRoles {
        TaskIdRole = Qt::UserRole + 1,
        FilenameRole,
        SavePathRole,
        TotalSizeRole,
        DownloadedSizeRole,
        ProgressRole,
        StateRole,
        StateTextRole,
        ErrorMsgRole,
        IsPlayingRole,
        CoverUrlRole,
        ArtistRole,
        DurationRole
    };

    explicit DownloadTaskModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief 刷新模型数据
     * @param showCompleted 是否显示已完成的任务
     */
    Q_INVOKABLE void refresh(bool showCompleted = false);

    /**
     * @brief 暂停下载
     */
    Q_INVOKABLE void pauseTask(const QString& taskId);

    /**
     * @brief 恢复下载
     */
    Q_INVOKABLE void resumeTask(const QString& taskId);

    /**
     * @brief 取消下载
     */
    Q_INVOKABLE void cancelTask(const QString& taskId);

    /**
     * @brief 删除已完成的任务记录
     */
    Q_INVOKABLE void removeTask(const QString& taskId);

    /**
     * @brief 获取当前播放路径
     */
    QString currentPlayingPath() const { return m_currentPlayingPath; }

    /**
     * @brief 设置当前播放路径
     */
    void setCurrentPlayingPath(const QString& path);

    /**
     * @brief 按下载保存路径更新已完成歌曲元数据
     */
    void updateSongMetadata(const QString& savePath, const QString& coverUrl, const QString& duration,
                            const QString& artist = QString());

signals:
    void currentPlayingPathChanged();

private slots:
    void onDownloadAdded(const QString& taskId, const QString& filename);
    void onDownloadStarted(const QString& taskId, const QString& filename);
    void onDownloadProgress(const QString& taskId, const QString& filename, qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished(const QString& taskId, const QString& filename, const QString& savePath);
    void onDownloadFailed(const QString& taskId, const QString& filename, const QString& error);
    void onDownloadPaused(const QString& taskId, const QString& filename);
    void onDownloadCancelled(const QString& taskId, const QString& filename);
    void onTaskRemoved(const QString& taskId, const QString& filename);

private:
    // 连接拆分：集中管理下载管理器到模型层的事件绑定。
    void setupConnections();

    QList<DownloadTask> m_tasks;
    bool m_showCompleted;
    QString m_currentPlayingPath;
    QHash<QString, LocalMusicInfo> m_localMetadataCache;

    QString resolveCoverUrl(const DownloadTask& task) const;
    QString resolveArtist(const DownloadTask& task) const;
    QString resolveDuration(const DownloadTask& task) const;
    void rebuildLocalMetadataCache();
    void updateLocalMetadataCacheEntry(const QString& savePath, const QString& coverUrl,
                                       const QString& duration, const QString& artist);
};

#endif // DOWNLOAD_TASK_MODEL_H
