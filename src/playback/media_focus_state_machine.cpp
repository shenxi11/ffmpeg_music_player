#include "media_focus_state_machine.h"

#include <QDebug>
#include <QState>

MediaFocusStateMachine::MediaFocusStateMachine(QObject* parent)
    : QObject(parent)
    , m_idleState(nullptr)
    , m_audioFocusedState(nullptr)
    , m_videoFocusedState(nullptr)
    , m_currentState(FocusState::Idle)
    , m_pendingIdleFrom(FocusState::Idle)
{
    m_idleDelayTimer.setSingleShot(true);
    m_idleDelayTimer.setInterval(180);
    connect(&m_idleDelayTimer, &QTimer::timeout, this, &MediaFocusStateMachine::onIdleDelayTimeout);

    setupStateMachine();
    m_machine.start();
}

void MediaFocusStateMachine::setupStateMachine()
{
    m_idleState = new QState(&m_machine);
    m_audioFocusedState = new QState(&m_machine);
    m_videoFocusedState = new QState(&m_machine);

    m_machine.setInitialState(m_idleState);

    m_idleState->addTransition(this, &MediaFocusStateMachine::evAudioPlayIntent, m_audioFocusedState);
    m_idleState->addTransition(this, &MediaFocusStateMachine::evVideoPlayIntent, m_videoFocusedState);

    m_audioFocusedState->addTransition(this, &MediaFocusStateMachine::evVideoPlayIntent, m_videoFocusedState);
    m_audioFocusedState->addTransition(this, &MediaFocusStateMachine::evAudioInactive, m_idleState);

    m_videoFocusedState->addTransition(this, &MediaFocusStateMachine::evAudioPlayIntent, m_audioFocusedState);
    m_videoFocusedState->addTransition(this, &MediaFocusStateMachine::evVideoInactive, m_idleState);

    connect(m_idleState, &QState::entered, this, &MediaFocusStateMachine::onEnterIdle);
    connect(m_audioFocusedState, &QState::entered, this, &MediaFocusStateMachine::onEnterAudioFocused);
    connect(m_videoFocusedState, &QState::entered, this, &MediaFocusStateMachine::onEnterVideoFocused);
}

void MediaFocusStateMachine::onAudioPlayIntent()
{
    cancelIdleSchedule();
    emit evAudioPlayIntent();
}

void MediaFocusStateMachine::onVideoPlayIntent()
{
    cancelIdleSchedule();
    emit evVideoPlayIntent();
}

void MediaFocusStateMachine::onAudioInactive()
{
    if (m_currentState == FocusState::AudioFocused) {
        scheduleIdleFrom(FocusState::AudioFocused);
    }
}

void MediaFocusStateMachine::onVideoInactive()
{
    if (m_currentState == FocusState::VideoFocused) {
        scheduleIdleFrom(FocusState::VideoFocused);
    }
}

void MediaFocusStateMachine::onEnterIdle()
{
    applyState(FocusState::Idle);
    cancelIdleSchedule();
}

void MediaFocusStateMachine::onEnterAudioFocused()
{
    cancelIdleSchedule();
    const FocusState oldState = m_currentState;
    applyState(FocusState::AudioFocused);
    if (oldState == FocusState::VideoFocused) {
        qDebug() << "[MediaFocusSM] Audio gained focus, pause video";
        emit pauseVideoRequested();
    }
}

void MediaFocusStateMachine::onEnterVideoFocused()
{
    cancelIdleSchedule();
    const FocusState oldState = m_currentState;
    applyState(FocusState::VideoFocused);
    if (oldState == FocusState::AudioFocused) {
        qDebug() << "[MediaFocusSM] Video gained focus, pause audio";
        emit pauseAudioRequested();
    }
}

void MediaFocusStateMachine::applyState(FocusState newState)
{
    if (m_currentState == newState) {
        return;
    }

    const FocusState oldState = m_currentState;
    m_currentState = newState;
    qDebug() << "[MediaFocusSM] state" << stateToString(oldState) << "->" << stateToString(newState);
    emit focusStateChanged(newState, oldState);
}

const char* MediaFocusStateMachine::stateToString(FocusState state)
{
    switch (state) {
    case FocusState::Idle:
        return "Idle";
    case FocusState::AudioFocused:
        return "AudioFocused";
    case FocusState::VideoFocused:
        return "VideoFocused";
    default:
        return "Unknown";
    }
}

void MediaFocusStateMachine::scheduleIdleFrom(FocusState state)
{
    m_pendingIdleFrom = state;
    if (!m_idleDelayTimer.isActive()) {
        qDebug() << "[MediaFocusSM] schedule idle from" << stateToString(state);
        m_idleDelayTimer.start();
    }
}

void MediaFocusStateMachine::cancelIdleSchedule()
{
    if (m_idleDelayTimer.isActive()) {
        m_idleDelayTimer.stop();
    }
    m_pendingIdleFrom = FocusState::Idle;
}

void MediaFocusStateMachine::onIdleDelayTimeout()
{
    if (m_pendingIdleFrom == FocusState::AudioFocused && m_currentState == FocusState::AudioFocused) {
        emit evAudioInactive();
    } else if (m_pendingIdleFrom == FocusState::VideoFocused && m_currentState == FocusState::VideoFocused) {
        emit evVideoInactive();
    }
    m_pendingIdleFrom = FocusState::Idle;
}
