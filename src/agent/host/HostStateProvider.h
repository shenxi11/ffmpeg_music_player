#ifndef HOST_STATE_PROVIDER_H
#define HOST_STATE_PROVIDER_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

/**
 * @brief Host ??????????????????? Agent ?????????
 */
class HostStateProvider : public QObject
{
    Q_OBJECT

public:
    explicit HostStateProvider(QObject* parent = nullptr);

    void setHostContext(QObject* hostContext);

    QString currentUserAccount() const;
    QVariantMap currentTrackSnapshot() const;
    QVariantMap hostContextSnapshot() const;
    QVariantList convertHistoryList(const QVariantList& history, int limit = -1) const;
    QVariantList convertFavoriteList(const QVariantList& favorites, int limit = -1) const;
    QVariantList convertPlaylistList(const QVariantList& playlists) const;
    QVariantMap convertPlaylistDetail(const QVariantMap& detail) const;
    QVariantList convertLocalMusicList(int limit = -1) const;

private:
    QObject* hostService() const;
    QVariantMap convertHistoryItem(const QVariantMap& raw) const;
    static qint64 normalizeDurationMs(qint64 rawDuration);
    static QString fallbackTrackId(const QString& path, const QString& title, const QString& artist);

private:
    QObject* m_hostContext = nullptr;
};

#endif // HOST_STATE_PROVIDER_H
