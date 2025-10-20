#ifndef PLAYBACK_STATE_MANAGER_H
#define PLAYBACK_STATE_MANAGER_H

#include <QObject>
#include <QStateMachine>
#include <QState>
#include <QTimer>
#include <QMutex>

// 全局播放状态枚举
enum class PlaybackState {
    Stopped,        // 停止状态
    Loading,        // 加载中
    Playing,        // 播放中
    Paused,         // 暂停状态
    Seeking,        // 跳转中
    Error           // 错误状态
};

// 播放事件枚举
enum class PlaybackEvent {
    Play,           // 播放事件
    Pause,          // 暂停事件
    Stop,           // 停止事件
    Seek,           // 跳转事件
    LoadComplete,   // 加载完成
    SeekComplete,   // 跳转完成
    Error           // 错误事件
};

/**
 * 全局播放状态管理器
 * 负责统一管理所有播放相关状态，确保状态一致性
 */
class PlaybackStateManager : public QObject
{
    Q_OBJECT

public:
    static PlaybackStateManager& getInstance();
    
    // 状态查询
    PlaybackState getCurrentState() const;
    bool isPlaying() const;
    bool isPaused() const;
    bool isSeeking() const;
    
    // 状态控制
    void requestPlay();
    void requestPause();
    void requestStop();
    void requestSeek(qint64 position);
    
    // 内部状态更新（由各组件调用）
    void notifyLoadComplete();
    void notifySeekComplete();
    void notifyError(const QString& error);

signals:
    // 状态变化通知
    void stateChanged(PlaybackState newState, PlaybackState oldState);
    
    // 控制指令（发送给各组件）
    void playRequested();
    void pauseRequested();
    void stopRequested();
    void seekRequested(qint64 position);
    
    // UI更新通知
    void updatePlayButton(bool playing);
    void updateProgress(qint64 position);

private slots:
    void onStateEntered();
    void onSeekTimeout();

private:
    explicit PlaybackStateManager(QObject *parent = nullptr);
    
    void setupStateMachine();
    void transitionToState(PlaybackState newState);
    
    QStateMachine *m_stateMachine;
    QState *m_stoppedState;
    QState *m_loadingState;
    QState *m_playingState;
    QState *m_pausedState;
    QState *m_seekingState;
    QState *m_errorState;
    
    PlaybackState m_currentState;
    QTimer *m_seekTimeout;
    mutable QMutex m_mutex;
    
    QString m_currentFile;
    qint64 m_currentPosition;
    qint64 m_duration;
};

#endif // PLAYBACK_STATE_MANAGER_H