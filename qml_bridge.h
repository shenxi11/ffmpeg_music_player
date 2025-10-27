#ifndef UI_BRIDGE_H
#define UI_BRIDGE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <memory>
#include "worker.h"
#include "take_pcm.h"
#include "lrc_analyze.h"
#include "controlbar.h"
#include "httprequest.h"

/**
 * @brief UI桥接类 - 连接C++业务逻辑和QML界面
 * 保留所有原有信号槽，提供QML可调用的接口
 */
class UIBridge : public QObject
{
    Q_OBJECT
    
    // ===== 播放控制属性 =====
    Q_PROPERTY(QString currentSongName READ currentSongName NOTIFY currentSongNameChanged)
    Q_PROPERTY(QString currentSongPath READ currentSongPath NOTIFY currentSongPathChanged)
    Q_PROPERTY(QString currentAlbumArt READ currentAlbumArt NOTIFY currentAlbumArtChanged)
    Q_PROPERTY(int currentPosition READ currentPosition NOTIFY currentPositionChanged)
    Q_PROPERTY(int totalDuration READ totalDuration NOTIFY totalDurationChanged)
    Q_PROPERTY(int playState READ playState NOTIFY playStateChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(bool isLooping READ isLooping NOTIFY isLoopingChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    
    // ===== 歌词属性 =====
    Q_PROPERTY(QString currentLyric READ currentLyric NOTIFY currentLyricChanged)
    Q_PROPERTY(int currentLyricLine READ currentLyricLine NOTIFY currentLyricLineChanged)
    Q_PROPERTY(QStringList allLyrics READ allLyrics NOTIFY allLyricsChanged)
    
    // ===== 音乐列表属性 =====
    Q_PROPERTY(QVariantList localMusicList READ localMusicList NOTIFY localMusicListChanged)
    Q_PROPERTY(QVariantList netMusicList READ netMusicList NOTIFY netMusicListChanged)
    
    // ===== UI状态属性 =====
    Q_PROPERTY(bool isMinimized READ isMinimized WRITE setIsMinimized NOTIFY isMinimizedChanged)
    Q_PROPERTY(bool showMusicList READ showMusicList WRITE setShowMusicList NOTIFY showMusicListChanged)
    Q_PROPERTY(bool showDeskLyric READ showDeskLyric WRITE setShowDeskLyric NOTIFY showDeskLyricChanged)
    Q_PROPERTY(bool isNetMode READ isNetMode WRITE setIsNetMode NOTIFY isNetModeChanged)
    
    // ===== 用户信息属性 =====
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY isLoggedInChanged)
    Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
    
public:
    explicit UIBridge(QObject *parent = nullptr);
    ~UIBridge();
    
    // ===== 属性访问器 =====
    QString currentSongName() const { return m_currentSongName; }
    QString currentSongPath() const { return m_currentSongPath; }
    QString currentAlbumArt() const { return m_currentAlbumArt; }
    int currentPosition() const { return m_currentPosition; }
    int totalDuration() const { return m_totalDuration; }
    int playState() const { return m_playState; }
    bool isPlaying() const { return m_playState == 1; }
    bool isLooping() const { return m_isLooping; }
    int volume() const { return m_volume; }
    
    QString currentLyric() const { return m_currentLyric; }
    int currentLyricLine() const { return m_currentLyricLine; }
    QStringList allLyrics() const { return m_allLyrics; }
    
    QVariantList localMusicList() const { return m_localMusicList; }
    QVariantList netMusicList() const { return m_netMusicList; }
    
    bool isMinimized() const { return m_isMinimized; }
    bool showMusicList() const { return m_showMusicList; }
    bool showDeskLyric() const { return m_showDeskLyric; }
    bool isNetMode() const { return m_isNetMode; }
    
    bool isLoggedIn() const { return m_isLoggedIn; }
    QString username() const { return m_username; }
    
    // ===== 属性设置器 =====
    void setVolume(int vol);
    void setIsMinimized(bool mini);
    void setShowMusicList(bool show);
    void setShowDeskLyric(bool show);
    void setIsNetMode(bool netMode);
    
    // ===== QML可调用的方法 =====
    // 播放控制
    Q_INVOKABLE void playSong(const QString& path, const QString& name);
    Q_INVOKABLE void playPause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void nextSong();
    Q_INVOKABLE void previousSong();
    Q_INVOKABLE void seekTo(int position);
    Q_INVOKABLE void toggleLoop();
    Q_INVOKABLE void forward(); // 快进
    Q_INVOKABLE void backward(); // 快退
    
    // 音乐列表管理
    Q_INVOKABLE void addLocalMusic(const QString& filePath);
    Q_INVOKABLE void removeMusic(const QString& songName);
    Q_INVOKABLE QStringList openFileDialog();
    Q_INVOKABLE void refreshLocalMusicList();
    Q_INVOKABLE void refreshNetMusicList();
    Q_INVOKABLE void searchMusic(const QString& keyword);
    Q_INVOKABLE void downloadMusic(const QString& songName);
    
    // 用户相关
    Q_INVOKABLE void login(const QString& account, const QString& password);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void registerAccount(const QString& account, const QString& password, const QString& username);
    
    // 歌词相关
    Q_INVOKABLE QString getLyricAt(int line);
    Q_INVOKABLE void scrollLyricTo(int line);
    
    // 插件相关
    Q_INVOKABLE void openTranslatePlugin();
    Q_INVOKABLE void openAudioConverter();
    
signals:
    // ===== 属性变化信号 =====
    void currentSongNameChanged(QString name);
    void currentSongPathChanged(QString path);
    void currentAlbumArtChanged(QString artPath);
    void currentPositionChanged(int position);
    void totalDurationChanged(int duration);
    void playStateChanged(int state);
    void isPlayingChanged(bool playing);
    void isLoopingChanged(bool looping);
    void volumeChanged(int volume);
    
    void currentLyricChanged(QString lyric);
    void currentLyricLineChanged(int line);
    void allLyricsChanged(QStringList lyrics);
    
    void localMusicListChanged(QVariantList list);
    void netMusicListChanged(QVariantList list);
    
    void isMinimizedChanged(bool minimized);
    void showMusicListChanged(bool show);
    void showDeskLyricChanged(bool show);
    void isNetModeChanged(bool netMode);
    
    void isLoggedInChanged(bool loggedIn);
    void usernameChanged(QString username);
    
    // ===== 原有业务信号（保留） =====
    void signal_worker_play();
    void signal_filepath(QString filePath);
    void signal_begin_to_play(QString path);
    void signal_begin_take_lrc(QString str);
    void signal_play_changed(bool flag);
    void signal_set_SliderMove(bool flag);
    void signal_process_Change(qint64 newPosition, bool back_flag);
    void signal_add_song(const QString fileName, const QString path);
    void signal_remove_click();
    void signal_stop_rotate(bool flag);
    
private slots:
    // ===== 连接原有C++组件的槽函数 =====
    void onDurationChanged(qint64 duration);
    void onPositionChanged(qint64 position);
    void onWorkStop();
    void onWorkPlay();
    void onLrcReceived(const std::map<int, std::string>& lyrics);
    void onLrcLineChanged(int line);
    void onAlbumArtChanged(QString picPath);
    void onLoginSuccess(QString username);
    void onMusicListReceived(const QStringList& songList, const QList<double>& durations);
    
private:
    void setupConnections();
    void initializeComponents();
    
    // ===== 核心业务组件（保留原有） =====
    std::shared_ptr<Worker> m_worker;
    std::shared_ptr<TakePcm> m_takePcm;
    std::shared_ptr<LrcAnalyze> m_lrcAnalyze;
    HttpRequest* m_httpRequest;
    
    QThread* m_workerThread;
    QThread* m_decoderThread;
    QThread* m_lrcThread;
    
    // ===== 状态变量 =====
    QString m_currentSongName;
    QString m_currentSongPath;
    QString m_currentAlbumArt;
    int m_currentPosition;
    int m_totalDuration;
    int m_playState; // 0=Stop, 1=Play, 2=Pause
    bool m_isLooping;
    int m_volume;
    
    QString m_currentLyric;
    int m_currentLyricLine;
    QStringList m_allLyrics;
    std::map<int, std::string> m_lyricsMap;
    
    QVariantList m_localMusicList;
    QVariantList m_netMusicList;
    
    bool m_isMinimized;
    bool m_showMusicList;
    bool m_showDeskLyric;
    bool m_isNetMode;
    
    bool m_isLoggedIn;
    QString m_username;
    
    qint64 m_duration; // 原始duration（微秒）
};

#endif // UI_BRIDGE_H
