#ifndef MEDIA_FOCUS_STATE_MACHINE_H
#define MEDIA_FOCUS_STATE_MACHINE_H

#include <QObject>
#include <QStateMachine>
#include <QTimer>

class QState;

class MediaFocusStateMachine : public QObject
{
    Q_OBJECT

public:
    enum class FocusState {
        Idle = 0,
        AudioFocused,
        VideoFocused
    };
    Q_ENUM(FocusState)

    explicit MediaFocusStateMachine(QObject* parent = nullptr);

    FocusState currentState() const { return m_currentState; }

    void onAudioPlayIntent();
    void onVideoPlayIntent();
    void onAudioInactive();
    void onVideoInactive();

signals:
    void pauseAudioRequested();
    void pauseVideoRequested();
    void focusStateChanged(MediaFocusStateMachine::FocusState newState,
                           MediaFocusStateMachine::FocusState oldState);

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

#endif // MEDIA_FOCUS_STATE_MACHINE_H
