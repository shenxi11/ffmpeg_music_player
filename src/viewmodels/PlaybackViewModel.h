#ifndef PLAYBACKVIEWMODEL_H
#define PLAYBACKVIEWMODEL_H

#include "BaseViewModel.h"
#include "AudioService.h"
#include <QUrl>

/**
 * @brief 播放器ViewModel
 * 
 * 职责:
 * - 管理播放器核心状态（播放/暂停/进度）
 * - 提供播放控制命令给UI层
 * - 暴露当前曲目元数据
 * - 与AudioService交互，不直接操作底层音频组件
 * 
 * 设计原则:
 * - 所有状态通过Q_PROPERTY暴露
 * - 所有命令使用Q_INVOKABLE标记
 * - 信号命名统一为xxxChanged
 * - 不包含UI代码，完全与UI分离
 * 
 * 使用示例（QML）:
 * @code
 * Button {
 *     text: viewModel.isPlaying ? "暂停" : "播放"
 *     enabled: viewModel.duration > 0
 *     onClicked: viewModel.togglePlayPause()
 * }
 * 
 * Slider {
 *     from: 0
 *     to: viewModel.duration
 *     value: viewModel.position
 *     onMoved: viewModel.seekTo(value)
 * }
 * @endcode
 */
class PlaybackViewModel : public BaseViewModel
{
    Q_OBJECT
    
    // ========== 播放状态属性 ==========
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY isPausedChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    
    // ========== 当前曲目信息属性 ==========
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY currentTitleChanged)
    Q_PROPERTY(QString currentArtist READ currentArtist NOTIFY currentArtistChanged)
    Q_PROPERTY(QString currentAlbum READ currentAlbum NOTIFY currentAlbumChanged)
    Q_PROPERTY(QString currentAlbumArt READ currentAlbumArt NOTIFY currentAlbumArtChanged)
    Q_PROPERTY(QUrl currentUrl READ currentUrl NOTIFY currentUrlChanged)
    Q_PROPERTY(QString currentFilePath READ currentFilePath NOTIFY currentFilePathChanged)
    
    // ========== 进度格式化属性（方便UI显示） ==========
    Q_PROPERTY(QString positionText READ positionText NOTIFY positionChanged)
    Q_PROPERTY(QString durationText READ durationText NOTIFY durationChanged)
    
public:
    explicit PlaybackViewModel(QObject *parent = nullptr);
    ~PlaybackViewModel() override;
    
    // ========== 播放状态访问器 ==========
    bool isPlaying() const { return m_isPlaying; }
    bool isPaused() const { return m_isPaused; }
    qint64 position() const { return m_position; }
    qint64 duration() const { return m_duration; }
    int volume() const { return m_volume; }
    
    // ========== 曲目信息访问器 ==========
    QString currentTitle() const { return m_currentTitle; }
    QString currentArtist() const { return m_currentArtist; }
    QString currentAlbum() const { return m_currentAlbum; }
    QString currentAlbumArt() const { return m_currentAlbumArt; }
    QUrl currentUrl() const { return m_currentUrl; }
    QString currentFilePath() const { return m_currentFilePath; }
    
    // ========== 格式化文本访问器 ==========
    QString positionText() const { return formatTime(m_position); }
    QString durationText() const { return formatTime(m_duration); }
    
    // ========== 播放控制命令（供QML调用） ==========
    
    /**
     * @brief 播放指定URL的音频
     * @param url 音频文件URL（本地或网络）
     */
    Q_INVOKABLE void play(const QUrl& url);
    
    /**
     * @brief 暂停播放
     */
    Q_INVOKABLE void pause();
    
    /**
     * @brief 恢复播放
     */
    Q_INVOKABLE void resume();
    
    /**
     * @brief 停止播放
     */
    Q_INVOKABLE void stop();
    
    /**
     * @brief 跳转到指定位置
     * @param positionMs 目标位置（毫秒）
     */
    Q_INVOKABLE void seekTo(qint64 positionMs);
    
    /**
     * @brief 切换播放/暂停状态
     */
    Q_INVOKABLE void togglePlayPause();
    
    /**
     * @brief 播放下一曲
     */
    Q_INVOKABLE void playNext();
    
    /**
     * @brief 播放上一曲
     */
    Q_INVOKABLE void playPrevious();
    
    // ========== 音量控制 ==========
    void setVolume(int volume);
    
signals:
    // 播放状态变化信号
    void isPlayingChanged();
    void isPausedChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    
    // 曲目信息变化信号
    void currentTitleChanged();
    void currentArtistChanged();
    void currentAlbumChanged();
    void currentAlbumArtChanged();
    void currentUrlChanged();
    
    // 播放事件信号（通知UI层）
    void playbackStarted();
    void playbackStopped();
    void playbackCompleted();
    void currentFilePathChanged();
    
    // UI相关事件（ViewModel负责协调）
    void shouldStartRotation();  // 通知UI开始旋转动画
    void shouldStopRotation();   // 通知UI停止旋转动画
    void shouldLoadLyrics(const QString& filePath);  // 通知UI加载歌词
    
private slots:
    // AudioService事件处理
    void onAudioServicePlaybackStarted(const QString& sessionId, const QUrl& url);
    void onAudioServicePlaybackPaused();
    void onAudioServicePlaybackResumed();
    void onAudioServicePlaybackStopped();
    void onAudioServicePositionChanged(qint64 position);
    void onAudioServiceDurationChanged(qint64 duration);
    
private:
    // 时间格式化辅助函数
    static QString formatTime(qint64 milliseconds);
    
    // 更新内部状态
    void updatePlayingState(bool playing);
    void updatePausedState(bool paused);
    void updatePosition(qint64 pos);
    void updateDuration(qint64 dur);
    void updateMetadata(const QString& title, const QString& artist, 
                       const QString& album, const QString& albumArt, 
                       const QUrl& url);
    
private:
    // Service层引用
    AudioService* m_audioService;
    
    // 播放状态
    bool m_isPlaying = false;
    bool m_isPaused = false;
    qint64 m_position = 0;
    qint64 m_duration = 0;
    int m_volume = 50;
    
    // 曲目元数据
    QString m_currentTitle;
    QString m_currentArtist;
    QString m_currentAlbum;
    QString m_currentAlbumArt;
    QUrl m_currentUrl;
    QString m_currentFilePath;  // 当前播放文件路径（用于UI显示和歌词加载）
};

#endif // PLAYBACKVIEWMODEL_H
