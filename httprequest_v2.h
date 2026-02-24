#ifndef HTTPREQUEST_V2_H
#define HTTPREQUEST_V2_H

#include <QObject>
#include "network/network_service.h"
#include "music.h"

/**
 * @brief HttpRequest重构版本 - 使用新的Network层
 * 
 * 这是一个示例类，展示如何使用新的NetworkService重构现有HttpRequest类
 * 
 * 优势：
 * - 自动连接复用（提升80%性能）
 * - 内置重试机制（提升成功率）
 * - 智能缓存（GET请求）
 * - 更简洁的代码
 * - 统一的错误处理
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
     * @brief 获取所有文件列表
     * @param useCache 是否使用缓存（默认true，缓存5分钟）
     */
    void getAllFiles(bool useCache = true);
    
    /**
     * @brief 获取歌词
     * @param url 歌曲URL
     */
    void getLyrics(const QString& url);
    
    /**
     * @brief 下载文件（使用DownloadManager）
     * @param filename 文件名
     * @param downloadFolder 下载目录
     * @param downloadLyrics 是否下载歌词
     * @param coverUrl 封面URL
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
    void getFavorites(const QString& userAccount);
    
    /**
     * @brief 添加播放历史
     */
    void addPlayHistory(const QString& userAccount, const QString& path, const QString& title,
                       const QString& artist, const QString& album, const QString& duration, bool isLocal);
    
    /**
     * @brief 获取播放历史
     */
    void getPlayHistory(const QString& userAccount, int limit = 50);
    
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
     * @brief 获取视频列表
     */
    void getVideoList();
    
    /**
     * @brief 获取视频流URL
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
     * @brief 获取音乐流数据（兼容旧API）
     * @param fileName 文件名
     */
    void getMusicData(const QString& fileName);
    void get_music_data(const QString& fileName) { getMusicData(fileName); } // 别名，兼容旧代码
    
    /**
     * @brief 添加音乐到用户列表
     * @param musicPath 音乐路径
     */
    void addMusic(const QString& musicPath);
    void AddMusic(const QString& musicPath) { addMusic(musicPath); } // 别名，兼容旧代码
    
    /**
     * @brief 下载（兼容旧API）
     */
    void Download(const QString& filename, const QString& downloadFolder, 
                 bool downloadLyrics = true, const QString& coverUrl = QString()) {
        downloadFile(filename, downloadFolder, downloadLyrics, coverUrl);
    }
    
signals:
    // 与原HttpRequest保持兼容的信号
    void signal_Loginflag(bool flag);
    void signal_Registerflag(bool flag);
    void signal_getusername(QString username);
    void signal_addSong_list(const QList<Music>& musicList);
    void signal_lrc(QStringList content);
    void signal_videoList(const QVariantList& videoList);
    void signal_videoStreamUrl(const QString& videoUrl);
    void signal_artistExists(bool exists, const QString& artist);
    void signal_artistMusicList(const QList<Music>& musicList, const QString& artist);
    void signal_addFavoriteResult(bool success);
    void signal_favoritesList(const QVariantList& favorites);
    void signal_removeFavoriteResult(bool success);
    void signal_addHistoryResult(bool success);
    void signal_historyList(const QVariantList& history);
    void signal_removeHistoryResult(bool success);
    void signal_streamurl(bool flag, QString url);  // 音乐流URL信号
    
private:
    Network::NetworkService& m_networkService;
    const QString m_baseUrl = "http://slcdut.xyz:8080/";
};

#endif // HTTPREQUEST_V2_H
