#ifndef HTTPREQUEST_V2_H
#define HTTPREQUEST_V2_H

#include <QObject>
#include "network/network_service.h"
#include "music.h"

/**
 * @brief HttpRequest閲嶆瀯鐗堟湰 - 浣跨敤鏂扮殑Network灞?
 * 
 * 杩欐槸涓€涓ず渚嬬被锛屽睍绀哄浣曚娇鐢ㄦ柊鐨凬etworkService閲嶆瀯鐜版湁HttpRequest绫?
 * 
 * 浼樺娍锛?
 * - 鑷姩杩炴帴澶嶇敤锛堟彁鍗?0%鎬ц兘锛?
 * - 鍐呯疆閲嶈瘯鏈哄埗锛堟彁鍗囨垚鍔熺巼锛?
 * - 鏅鸿兘缂撳瓨锛圙ET璇锋眰锛?
 * - 鏇寸畝娲佺殑浠ｇ爜
 * - 缁熶竴鐨勯敊璇鐞?
 */
class HttpRequestV2 : public QObject {
    Q_OBJECT
    
public:
    explicit HttpRequestV2(QObject* parent = nullptr);
    
    /**
     * @brief 鐢ㄦ埛鐧诲綍
     * @param account 璐﹀彿
     * @param password 瀵嗙爜
     */
    void login(const QString& account, const QString& password);
    
    /**
     * @brief 鐢ㄦ埛娉ㄥ唽
     * @param account 璐﹀彿
     * @param password 瀵嗙爜
     * @param username 鐢ㄦ埛鍚?
     */
    void registerUser(const QString& account, const QString& password, const QString& username);

    /**
     * @brief 閲嶇疆瀵嗙爜锛堝綋鍓嶇増鏈洿鎺ラ噸缃紝涓嶅仛楠岃瘉鐮佹牎楠岋級
     * @param account 璐﹀彿
     * @param newPassword 鏂板瘑鐮?     */
    void resetPassword(const QString& account, const QString& newPassword);
    
    /**
     * @brief 鑾峰彇鎵€鏈夋枃浠跺垪琛?
     * @param useCache 鏄惁浣跨敤缂撳瓨锛堥粯璁rue锛岀紦瀛?鍒嗛挓锛?
     */
    void getAllFiles(bool useCache = true);
    
    /**
     * @brief 鑾峰彇姝岃瘝
     * @param url 姝屾洸URL
     */
    void getLyrics(const QString& url);
    
    /**
     * @brief 涓嬭浇鏂囦欢锛堜娇鐢―ownloadManager锛?
     * @param filename 鏂囦欢鍚?
     * @param downloadFolder 涓嬭浇鐩綍
     * @param downloadLyrics 鏄惁涓嬭浇姝岃瘝
     * @param coverUrl 灏侀潰URL
     */
    void downloadFile(const QString& filename, const QString& downloadFolder, 
                     bool downloadLyrics = true, const QString& coverUrl = QString());
    
    /**
     * @brief 娣诲姞鍠滄闊充箰
     */
    void addFavorite(const QString& userAccount, const QString& path, const QString& title,
                    const QString& artist, const QString& duration, bool isLocal);
    
    /**
     * @brief 鑾峰彇鍠滄闊充箰鍒楄〃
     */
    void getFavorites(const QString& userAccount, bool useCache = true);
    
    /**
     * @brief 娣诲姞鎾斁鍘嗗彶
     */
    void addPlayHistory(const QString& userAccount, const QString& path, const QString& title,
                       const QString& artist, const QString& album, const QString& duration, bool isLocal);
    
    /**
     * @brief 鑾峰彇鎾斁鍘嗗彶
     */
    void getPlayHistory(const QString& userAccount, int limit = 50, bool useCache = true);
    
    /**
     * @brief 鎼滅储闊充箰
     * @param keyword 鎼滅储鍏抽敭瀛楋紙鏍囬銆佹瓕鎵嬨€佷笓杈戠瓑锛?
     */
    void getMusic(const QString& keyword);
    
    /**
     * @brief 鍒犻櫎鍠滄闊充箰
     * @param userAccount 鐢ㄦ埛璐﹀彿
     * @param paths 闊充箰璺緞鍒楄〃
     */
    void removeFavorite(const QString& userAccount, const QStringList& paths);
    
    /**
     * @brief 鍒犻櫎鎾斁鍘嗗彶
     * @param userAccount 鐢ㄦ埛璐﹀彿
     * @param paths 闊充箰璺緞鍒楄〃
     */
    void removePlayHistory(const QString& userAccount, const QStringList& paths);
    
    /**
     * @brief 鑾峰彇瑙嗛鍒楄〃
     */
    void getVideoList();
    
    /**
     * @brief 鑾峰彇瑙嗛娴乁RL
     */
    void getVideoStreamUrl(const QString& videoPath);
    
    /**
     * @brief 鎼滅储姝屾墜
     */
    void searchArtist(const QString& artist);
    
    /**
     * @brief 鏍规嵁姝屾墜鑾峰彇闊充箰
     */
    void getMusicByArtist(const QString& artist);
    
    /**
     * @brief 鑾峰彇闊充箰娴佹暟鎹紙鍏煎鏃PI锛?
     * @param fileName 鏂囦欢鍚?
     */
    void getMusicData(const QString& fileName);
    void get_music_data(const QString& fileName) { getMusicData(fileName); } // 鍒悕锛屽吋瀹规棫浠ｇ爜
    
    /**
     * @brief 娣诲姞闊充箰鍒扮敤鎴峰垪琛?
     * @param musicPath 闊充箰璺緞
     */
    void addMusic(const QString& musicPath);
    void AddMusic(const QString& musicPath) { addMusic(musicPath); } // 鍒悕锛屽吋瀹规棫浠ｇ爜
    
    /**
     * @brief 涓嬭浇锛堝吋瀹规棫API锛?
     */
    void Download(const QString& filename, const QString& downloadFolder, 
                 bool downloadLyrics = true, const QString& coverUrl = QString()) {
        downloadFile(filename, downloadFolder, downloadLyrics, coverUrl);
    }
    
signals:
    // 涓庡師HttpRequest淇濇寔鍏煎鐨勪俊鍙?
    void signal_Loginflag(bool flag);
    void signal_Registerflag(bool flag);
    void signal_RegisterResult(bool success, const QString& message);
    void signal_ResetPasswordResult(bool success, const QString& message);
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
    void signal_streamurl(bool flag, QString url);  // 闊充箰娴乁RL淇″彿
    
private:
    Network::NetworkService& m_networkService;
    QString m_baseUrl;

};

#endif // HTTPREQUEST_V2_H

