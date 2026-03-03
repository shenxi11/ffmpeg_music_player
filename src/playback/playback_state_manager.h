#ifndef PLAYBACK_STATE_MANAGER_H
#define PLAYBACK_STATE_MANAGER_H

#include <QObject>
#include <QStateMachine>
#include <QTimer>

class QState;

// 统一管理音频与视频焦点的全局播放状态机。
class PlaybackStateManager : public QObject
{
    Q_OBJECT

public:
    enum class FocusState {
        Idle = 0,
        AudioFocused,
        VideoFocused
    };
    Q_ENUM(FocusState)

    explicit PlaybackStateManager(QObject* parent = nullptr);

    FocusState currentState() const { return m_currentState; }

    void onAudioPlayIntent();
    void onVideoPlayIntent();
    void onAudioInactive();
    void onVideoInactive();

signals:
    void pauseAudioRequested();
    void pauseVideoRequested();
    void focusStateChanged(PlaybackStateManager::FocusState newState,
                           PlaybackStateManager::FocusState oldState);

private slots:
    void onEnterIdle();
    void onEnterAudioFocused();
    void onEnterVideoFocused();
    void onIdleDelayTimeout();

private:
    void setupStateMachine();
    void applyState(FocusState newState);
    static const char* stateToString(FocusState state);
    void scheduleIdleFrom(FocusState state);
    void cancelIdleSchedule();

signals:
    void evAudioPlayIntent();
    void evVideoPlayIntent();
    void evAudioInactive();
    void evVideoInactive();

private:
    QStateMachine m_machine;
    QState* m_idleState;
    QState* m_audioFocusedState;
    QState* m_videoFocusedState;
    FocusState m_currentState;
    FocusState m_pendingIdleFrom;
    QTimer m_idleDelayTimer;
};

#endif // PLAYBACK_STATE_MANAGER_H
