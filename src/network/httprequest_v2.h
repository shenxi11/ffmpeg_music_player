#ifndef HTTPREQUEST_V2_H
#define HTTPREQUEST_V2_H

#include "music.h"
#include "network/network_service.h"

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

/**
 * @brief HttpRequest 重构版本，基于统一 Network 层实现。
 *
 * 相比旧实现，这个版本统一了请求构建、重试、缓存和错误处理流程，
 * 用于承载客户端的登录、媒体列表、收藏、历史、推荐等网络能力。
 */
class HttpRequestV2 : public QObject {
    Q_OBJECT

  public:
    explicit HttpRequestV2(QObject* parent = nullptr);

    /**
     * @brief 用户登录
     * @param account 账号
     * @param password 密码
     */
    void login(const QString& account, const QString& password);

    /**
     * @brief 用户注册
     * @param account 账号
     * @param password 密码
     * @param username 用户名
     */
    void registerUser(const QString& account, const QString& password, const QString& username);

    /**
     * @brief 重置密码（当前版本直接重置，不做验证码校验）
     * @param account 账号
     * @param newPassword 新密码
     */
    void resetPassword(const QString& account, const QString& newPassword);

    /**
     * @brief 获取所有文件列表
     * @param useCache 是否使用缓存（默认 true，缓存 5 分钟）
     */
    void getAllFiles(bool useCache = true);

    /**
     * @brief 获取歌词
     * @param url 歌曲 URL
     */
    void getLyrics(const QString& url);

    /**
     * @brief 下载文件（使用 DownloadManager）
     * @param filename 文件名
     * @param downloadFolder 下载目录
     * @param downloadLyrics 是否下载歌词
     * @param coverUrl 封面 URL
     */
    void downloadFile(const QString& filename, const QString& downloadFolder,
                      bool downloadLyrics = true, const QString& coverUrl = QString());

    /**
     * @brief 添加喜欢音乐
     */
    void addFavorite(const QString& userAccount, const QString& path, const QString& title,
                     const QString& artist, const QString& duration, bool isLocal);

    /**
     * @brief 获取喜欢音乐列表
     */
    void getFavorites(const QString& userAccount, bool useCache = true);

    /**
     * @brief 添加播放历史
     */
    void addPlayHistory(const QString& userAccount, const QString& path, const QString& title,
                        const QString& artist, const QString& album, const QString& duration,
                        bool isLocal);

    /**
     * @brief 获取播放历史
     */
    void getPlayHistory(const QString& userAccount, int limit = 50, bool useCache = true);

    /**
     * @brief 获取推荐音乐列表
     * @param userId 用户账号/ID
     * @param scene 场景，默认 home
     * @param limit 数量限制，最大 100
     * @param excludePlayed 是否排除已播放
     * @param cursor 分页游标
     */
    void getAudioRecommendations(const QString& userId,
                                 const QString& scene = QStringLiteral("home"), int limit = 20,
                                 bool excludePlayed = true, const QString& cursor = QString());
    void getSimilarRecommendations(const QString& songId, int limit = 20);

    /**
     * @brief 上报推荐反馈事件
     */
    void postRecommendationFeedback(const QString& userId, const QString& songId,
                                    const QString& eventType,
                                    const QString& scene = QStringLiteral("home"),
                                    const QString& requestId = QString(),
                                    const QString& modelVersion = QString(), qint64 playMs = -1,
                                    qint64 durationMs = -1);

    /**
     * @brief 搜索音乐
     * @param keyword 搜索关键字（标题、歌手、专辑等）
     */
    void getMusic(const QString& keyword);

    /**
     * @brief 删除喜欢音乐
     * @param userAccount 用户账号
     * @param paths 音乐路径列表
     */
    void removeFavorite(const QString& userAccount, const QStringList& paths);

    /**
     * @brief 删除播放历史
     * @param userAccount 用户账号
     * @param paths 音乐路径列表
     */
    void removePlayHistory(const QString& userAccount, const QStringList& paths);

    /**
     * @brief 获取私有歌单列表
     */
    void getPlaylists(const QString& userAccount, int page = 1, int pageSize = 20,
                      bool useCache = true);

    /**
     * @brief 创建私有歌单
     */
    void createPlaylist(const QString& userAccount, const QString& name,
                        const QString& description = QString(),
                        const QString& coverPath = QString());

    /**
     * @brief 获取歌单详情
     */
    void getPlaylistDetail(const QString& userAccount, qint64 playlistId, bool useCache = false);
    void requestPlaylistDetailForCover(const QString& userAccount, qint64 playlistId,
                                       bool useCache = true);

    /**
     * @brief 删除歌单
     */
    void deletePlaylist(const QString& userAccount, qint64 playlistId);

    /**
     * @brief 更新歌单信息
     */
    void updatePlaylist(const QString& userAccount, qint64 playlistId, const QString& name,
                        const QString& description = QString(),
                        const QString& coverPath = QString());

    /**
     * @brief 批量添加歌曲到歌单
     */
    void addPlaylistItems(const QString& userAccount, qint64 playlistId, const QVariantList& items);

    /**
     * @brief 批量移除歌单歌曲
     */
    void removePlaylistItems(const QString& userAccount, qint64 playlistId,
                             const QStringList& musicPaths);

    /**
     * @brief 歌单排序
     */
    void reorderPlaylistItems(const QString& userAccount, qint64 playlistId,
                              const QVariantList& orderedItems);

    /**
     * @brief 获取当前用户资料
     */
    void getUserProfile(const QString& account, const QString& sessionToken);

    /**
     * @brief 更新当前用户名
     */
    void updateUsername(const QString& account, const QString& sessionToken,
                        const QString& username);

    /**
     * @brief 上传并更新用户头像
     */
    void uploadAvatar(const QString& account, const QString& sessionToken, const QString& filePath);

    /**
     * @brief 获取视频列表
     */
    void getVideoList();

    /**
     * @brief 获取视频流 URL
     */
    void getVideoStreamUrl(const QString& videoPath);

    /**
     * @brief 搜索歌手
     */
    void searchArtist(const QString& artist);

    /**
     * @brief 根据歌手获取音乐
     */
    void getMusicByArtist(const QString& artist);

    /**
     * @brief 获取音乐流数据（兼容旧 API）
     * @param fileName 文件名
     */
    void getMusicData(const QString& fileName);

    /**
     * @brief 添加音乐到用户列表
     * @param musicPath 音乐路径
     */
    void addMusic(const QString& musicPath);

    /**
     * @brief 下载（兼容旧 API）
     */
    void download(const QString& filename, const QString& downloadFolder,
                  bool downloadLyrics = true, const QString& coverUrl = QString()) {
        downloadFile(filename, downloadFolder, downloadLyrics, coverUrl);
    }

  signals:
    // 与原 HttpRequest 保持兼容的信号
    void signalLoginFlag(bool flag);
    void signalLoginProfile(const QString& username, const QString& avatarUrl,
                            const QString& onlineSessionToken);
    void signalRegisterFlag(bool flag);
    void signalRegisterResult(bool success, const QString& message);
    void signalResetPasswordResult(bool success, const QString& message);
    void signalGetusername(QString username);
    void signalAddSongList(const QList<Music>& musicList);
    void signalLrc(QStringList content);
    void signalVideoList(const QVariantList& videoList);
    void signalVideoStreamUrl(const QString& videoUrl);
    void signalArtistExists(bool exists, const QString& artist);
    void signalArtistMusicList(const QList<Music>& musicList, const QString& artist);
    void signalAddFavoriteResult(bool success);
    void signalFavoritesList(const QVariantList& favorites);
    void signalRemoveFavoriteResult(bool success);
    void signalAddHistoryResult(bool success);
    void signalHistoryList(const QVariantList& history);
    void signalRemoveHistoryResult(bool success);
    void signalPlaylistsList(const QVariantList& playlists, int page, int pageSize, int total);
    void signalPlaylistDetail(const QVariantMap& detail);
    void signalPlaylistCoverDetail(const QVariantMap& detail);
    void signalCreatePlaylistResult(bool success, qint64 playlistId, const QString& message);
    void signalDeletePlaylistResult(bool success, qint64 playlistId, const QString& message);
    void signalUpdatePlaylistResult(bool success, qint64 playlistId, const QString& message);
    void signalAddPlaylistItemsResult(bool success, qint64 playlistId, int addedCount,
                                      int skippedCount, const QString& message);
    void signalRemovePlaylistItemsResult(bool success, qint64 playlistId, int deletedCount,
                                         const QString& message);
    void signalReorderPlaylistItemsResult(bool success, qint64 playlistId, const QString& message);
    void signalUserProfileResult(bool success, const QVariantMap& profile, const QString& message,
                                 int statusCode);
    void signalUpdateUsernameResult(bool success, const QString& username, const QString& message,
                                    int statusCode);
    void signalUploadAvatarResult(bool success, const QString& avatarUrl, const QString& message,
                                  int statusCode);
    void signalRecommendationList(const QVariantMap& meta, const QVariantList& items);
    void signalSimilarRecommendationList(const QVariantMap& meta, const QVariantList& items,
                                         const QString& anchorSongId);
    void signalRecommendationFeedbackResult(bool success, const QString& eventType,
                                            const QString& songId);
    void signalStreamurl(bool flag, QString url); // 音乐流 URL 信号

  private:
    QVariantMap parsePlaylistDetailPayload(const Network::NetworkResponse& response);
    Network::NetworkService& m_networkService;
    QString m_baseUrl;
};

#endif // HTTPREQUEST_V2_H
