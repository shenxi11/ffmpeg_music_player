#ifndef LOCAL_MUSIC_MODEL_H
#define LOCAL_MUSIC_MODEL_H

#include <QAbstractListModel>
#include "local_music_cache.h"

/**
 * @brief 本地音乐列表模型，用于QML显示
 */
class LocalMusicModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString currentPlayingPath READ currentPlayingPath WRITE setCurrentPlayingPath NOTIFY currentPlayingPathChanged)

public:
    enum MusicRoles {
        FilePathRole = Qt::UserRole + 1,
        FileNameRole,
        CoverUrlRole,
        DurationRole,
        ArtistRole,
        IsPlayingRole
    };

    explicit LocalMusicModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief 刷新模型数据
     */
    Q_INVOKABLE void refresh();

    /**
     * @brief 添加音乐
     */
    Q_INVOKABLE void addMusic(const QString& filePath);

    /**
     * @brief 删除音乐
     */
    Q_INVOKABLE void removeMusic(const QString& filePath);

    /**
     * @brief 获取当前播放路径
     */
    QString currentPlayingPath() const { return m_currentPlayingPath; }

    /**
     * @brief 设置当前播放路径
     */
    void setCurrentPlayingPath(const QString& path);

signals:
    void currentPlayingPathChanged();
    void addMusicRequested();

private slots:
    void onMusicListChanged();

private:
    QList<LocalMusicInfo> m_musicList;
    QString m_currentPlayingPath;
};

#endif // LOCAL_MUSIC_MODEL_H
