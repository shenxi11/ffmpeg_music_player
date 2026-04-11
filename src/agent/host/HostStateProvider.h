#ifndef HOST_STATE_PROVIDER_H
#define HOST_STATE_PROVIDER_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class MainShellViewModel;
class Music;

/**
 * @brief Host 状态提供器，负责把客户端业务状态转换为 Agent 可消费的数据结构。
 */
class HostStateProvider : public QObject
{
    Q_OBJECT

public:
    explicit HostStateProvider(QObject* parent = nullptr);

    void setMainShellViewModel(MainShellViewModel* shellViewModel);
    MainShellViewModel* mainShellViewModel() const { return m_shellViewModel; }

    QString currentUserAccount() const;
    QVariantMap currentTrackSnapshot() const;
    QVariantList convertMusicList(const QList<Music>& musics, int limit = -1) const;
    QVariantList convertHistoryList(const QVariantList& history, int limit = -1) const;
    QVariantList convertFavoriteList(const QVariantList& favorites, int limit = -1) const;
    QVariantList convertPlaylistList(const QVariantList& playlists) const;
    QVariantMap convertPlaylistDetail(const QVariantMap& detail) const;
    QVariantList convertLocalMusicList(int limit = -1) const;

private:
    QVariantMap convertMusicItem(const Music& music) const;
    QVariantMap convertHistoryItem(const QVariantMap& raw) const;
    static qint64 normalizeDurationMs(qint64 rawDuration);
    static QString fallbackTrackId(const QString& path, const QString& title, const QString& artist);

private:
    MainShellViewModel* m_shellViewModel = nullptr;
};

#endif // HOST_STATE_PROVIDER_H
