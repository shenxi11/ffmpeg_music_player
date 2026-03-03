#ifndef LOCAL_MUSIC_CACHE_H
#define LOCAL_MUSIC_CACHE_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

/**
 * @brief 本地音乐信息
 */
struct LocalMusicInfo {
    QString filePath;    // 文件完整路径
    QString fileName;    // 文件名
    QString coverUrl;    // 封面URL（从播放时提取）
    QString duration;    // 时长
    QString artist;      // 艺术家
    
    LocalMusicInfo() : duration("0:00"), artist("未知艺术家") {}
    
    QJsonObject toJson() const;
    static LocalMusicInfo fromJson(const QJsonObject& obj);
};

/**
 * @brief 本地音乐缓存管理器（单例模式）
 * 负责保存和读取本地音乐列表信息
 */
class LocalMusicCache : public QObject
{
    Q_OBJECT

public:
    static LocalMusicCache& instance()
    {
        static LocalMusicCache instance;
        return instance;
    }

    /**
     * @brief 添加音乐到缓存
     */
    void addMusic(const LocalMusicInfo& info);

    /**
     * @brief 移除音乐
     */
    void removeMusic(const QString& filePath);

    /**
     * @brief 更新音乐元数据
     */
    void updateMetadata(const QString& filePath, const QString& coverUrl, const QString& duration);

    /**
     * @brief 获取所有音乐列表
     */
    QList<LocalMusicInfo> getMusicList() const;

    /**
     * @brief 清空音乐列表
     */
    void clearAll();

signals:
    void musicListChanged();

private:
    LocalMusicCache();
    ~LocalMusicCache() = default;

    // 禁用拷贝和赋值
    LocalMusicCache(const LocalMusicCache&) = delete;
    LocalMusicCache& operator=(const LocalMusicCache&) = delete;

    /**
     * @brief 保存音乐列表到配置文件
     */
    void saveMusicList();

    /**
     * @brief 从配置文件加载音乐列表
     */
    void loadMusicList();

    QSettings m_settings;
    QList<LocalMusicInfo> m_musicList;
};

#endif // LOCAL_MUSIC_CACHE_H
