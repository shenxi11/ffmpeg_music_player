#ifndef AGENT_TOOL_EXECUTOR_H
#define AGENT_TOOL_EXECUTOR_H

#include <QObject>
#include <QHash>
#include <QQueue>
#include <QUrl>
#include <QVariantMap>

class MainShellViewModel;
class HostStateProvider;
class ToolRegistry;
class Music;
class HttpRequestV2;

/**
 * @brief Agent 工具执行器，负责将 tool_call 映射为客户端业务调用并回传结构化结果。
 */
class AgentToolExecutor : public QObject
{
    Q_OBJECT

public:
    explicit AgentToolExecutor(HostStateProvider* hostStateProvider, QObject* parent = nullptr);
    ~AgentToolExecutor() override;

    void setMainShellViewModel(MainShellViewModel* shellViewModel);
    ToolRegistry* toolRegistry() const { return m_toolRegistry; }
    bool executeToolCall(const QString& toolCallId,
                         const QString& tool,
                         const QVariantMap& args);

signals:
    void toolResultReady(const QString& toolCallId,
                         bool ok,
                         const QVariantMap& result,
                         const QVariantMap& error);
    void toolProgress(const QString& message);

private slots:
    void onSearchResultsReady(const QList<Music>& musicList);
    void onHistoryListReady(const QVariantList& history);
    void onFavoritesListReady(const QVariantList& favorites);
    void onPlaylistsListReady(const QVariantList& playlists, int page, int pageSize, int total);
    void onPlaylistDetailReady(const QVariantMap& detail);

    void onAddFavoriteResultReady(bool success);
    void onRemoveFavoriteResultReady(bool success);
    void onAddHistoryResultReady(bool success);
    void onRemoveHistoryResultReady(bool success);
    void onCreatePlaylistResultReady(bool success, qint64 playlistId, const QString& message);
    void onDeletePlaylistResultReady(bool success, qint64 playlistId, const QString& message);
    void onUpdatePlaylistResultReady(bool success, qint64 playlistId, const QString& message);
    void onAddPlaylistItemsResultReady(bool success, qint64 playlistId, int addedCount, int skippedCount, const QString& message);
    void onRemovePlaylistItemsResultReady(bool success, qint64 playlistId, int deletedCount, const QString& message);
    void onReorderPlaylistItemsResultReady(bool success, qint64 playlistId, const QString& message);

    void onRecommendationListReady(const QVariantMap& meta, const QVariantList& items);
    void onSimilarRecommendationListReady(const QVariantMap& meta,
                                          const QVariantList& items,
                                          const QString& anchorSongId);
    void onRecommendationFeedbackResultReady(bool success,
                                             const QString& eventType,
                                             const QString& songId);
    void onLyricsReady(const QStringList& lines);
    void onVideoListReady(const QVariantList& videoList);
    void onVideoStreamUrlReady(const QString& videoUrl);
    void onArtistSearchReady(bool exists, const QString& artist);
    void onArtistTracksReady(const QList<Music>& musicList, const QString& artist);

private:
    struct PendingTask
    {
        QString toolCallId;
        QString tool;
        QVariantMap args;
    };

    void clearPendingTasks();
    bool requireLogin(const QString& toolCallId);
    void failTool(const QString& toolCallId, const QString& code, const QString& message, bool retryable = false);
    void succeedTool(const QString& toolCallId, const QVariantMap& result);

    static QVariantMap makeError(const QString& code, const QString& message, bool retryable = false);
    static qint64 parsePlaylistId(const QVariantMap& args);
    static QUrl toPlayableUrl(const QString& rawPath);
    static QStringList variantToStringList(const QVariant& value);
    static QString playModeName(int playMode);
    static int parsePlayModeValue(const QVariant& value);

    void rememberTracks(const QVariantList& tracks);
    void rememberPlaylistMeta(const QVariantList& playlists);
    void rememberPlaylistDetail(const QVariantMap& detail);

    QVariantMap resolveTrackById(const QString& trackId) const;
    QVariantMap resolveTrackByPath(const QString& path) const;
    QVariantList convertQueueToItems(const QList<QUrl>& urls) const;
    QList<QUrl> buildQueueUrls(const QVariantList& trackIds, QString* errorMessage) const;
    QVariantMap buildQueueSnapshot() const;

    QVariantList buildPlaylistItemsFromTrackIds(const QVariantList& trackIds, QString* errorMessage) const;
    QStringList buildMusicPathsFromTrackIds(const QVariantList& trackIds) const;

private:
    MainShellViewModel* m_shellViewModel = nullptr;
    HostStateProvider* m_hostStateProvider = nullptr;
    ToolRegistry* m_toolRegistry = nullptr;
    HttpRequestV2* m_requestGateway = nullptr;

    QQueue<PendingTask> m_pendingSearch;
    QQueue<PendingTask> m_pendingLyricsFetch;
    QQueue<PendingTask> m_pendingVideoListFetch;
    QQueue<PendingTask> m_pendingVideoStreamFetch;
    QQueue<PendingTask> m_pendingArtistSearch;
    QQueue<PendingTask> m_pendingArtistTracks;
    QQueue<PendingTask> m_pendingHistoryFetch;
    QQueue<PendingTask> m_pendingHistoryAdd;
    QQueue<PendingTask> m_pendingHistoryRemove;

    QQueue<PendingTask> m_pendingFavoritesFetch;
    QQueue<PendingTask> m_pendingFavoriteAdd;
    QQueue<PendingTask> m_pendingFavoriteRemove;

    QQueue<PendingTask> m_pendingPlaylistsFetch;
    QQueue<PendingTask> m_pendingPlaylistDetail;
    QQueue<PendingTask> m_pendingPlaylistPlay;

    QQueue<PendingTask> m_pendingCreatePlaylist;
    QQueue<PendingTask> m_pendingDeletePlaylist;
    QQueue<PendingTask> m_pendingUpdatePlaylist;
    QQueue<PendingTask> m_pendingAddPlaylistItems;
    QQueue<PendingTask> m_pendingRemovePlaylistItems;
    QQueue<PendingTask> m_pendingReorderPlaylistItems;

    QQueue<PendingTask> m_pendingRecommendationList;
    QQueue<PendingTask> m_pendingSimilarRecommendationList;
    QQueue<PendingTask> m_pendingRecommendationFeedback;

    QHash<QString, QVariantMap> m_trackCacheById;
    QHash<qint64, QVariantMap> m_playlistMetaById;
    QHash<qint64, QVariantMap> m_playlistDetailById;
};

#endif // AGENT_TOOL_EXECUTOR_H
