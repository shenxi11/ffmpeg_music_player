#include "playback_state_manager.h"

#include <QDebug>
#include <QState>

PlaybackStateManager::PlaybackStateManager(QObject* parent)
    : QObject(parent)
    , m_idleState(nullptr)
    , m_audioFocusedState(nullptr)
    , m_videoFocusedState(nullptr)
    , m_currentState(FocusState::Idle)
    , m_pendingIdleFrom(FocusState::Idle)
{
    m_idleDelayTimer.setSingleShot(true);
    m_idleDelayTimer.setInterval(180);
    connect(&m_idleDelayTimer, &QTimer::timeout, this, &PlaybackStateManager::onIdleDelayTimeout);

    setupStateMachine();
    m_machine.start();
}

void PlaybackStateManager::setupStateMachine()
{
    m_idleState = new QState(&m_machine);
    m_audioFocusedState = new QState(&m_machine);
    m_videoFocusedState = new QState(&m_machine);

    m_machine.setInitialState(m_idleState);

    m_idleState->addTransition(this, &PlaybackStateManager::evAudioPlayIntent, m_audioFocusedState);
    m_idleState->addTransition(this, &PlaybackStateManager::evVideoPlayIntent, m_videoFocusedState);

    m_audioFocusedState->addTransition(this, &PlaybackStateManager::evVideoPlayIntent, m_videoFocusedState);
    m_audioFocusedState->addTransition(this, &PlaybackStateManager::evAudioInactive, m_idleState);

    m_videoFocusedState->addTransition(this, &PlaybackStateManager::evAudioPlayIntent, m_audioFocusedState);
    m_videoFocusedState->addTransition(this, &PlaybackStateManager::evVideoInactive, m_idleState);

    connect(m_idleState, &QState::entered, this, &PlaybackStateManager::onEnterIdle);
    connect(m_audioFocusedState, &QState::entered, this, &PlaybackStateManager::onEnterAudioFocused);
    connect(m_videoFocusedState, &QState::entered, this, &PlaybackStateManager::onEnterVideoFocused);
}

void PlaybackStateManager::onAudioPlayIntent()
{
    cancelIdleSchedule();
    emit evAudioPlayIntent();
}

void PlaybackStateManager::onVideoPlayIntent()
{
    cancelIdleSchedule();
    emit evVideoPlayIntent();
}

void PlaybackStateManager::onAudioInactive()
{
    if (m_currentState == FocusState::AudioFocused) {
        scheduleIdleFrom(FocusState::AudioFocused);
    }
}

void PlaybackStateManager::onVideoInactive()
{
    if (m_currentState == FocusState::VideoFocused) {
        scheduleIdleFrom(FocusState::VideoFocused);
    }
}

void PlaybackStateManager::onEnterIdle()
{
    applyState(FocusState::Idle);
    cancelIdleSchedule();
}

void PlaybackStateManager::onEnterAudioFocused()
{
    cancelIdleSchedule();
    const FocusState oldState = m_currentState;
    applyState(FocusState::AudioFocused);
    if (oldState == FocusState::VideoFocused) {
        qDebug() << "[PlaybackSM] Audio gained focus, pause video";
        emit pauseVideoRequested();
    }
}

void PlaybackStateManager::onEnterVideoFocused()
{
    cancelIdleSchedule();
    const FocusState oldState = m_currentState;
    applyState(FocusState::VideoFocused);
    if (oldState == FocusState::AudioFocused) {
        qDebug() << "[PlaybackSM] Video gained focus, pause audio";
        emit pauseAudioRequested();
    }
}

void PlaybackStateManager::applyState(FocusState newState)
{
    if (m_currentState == newState) {
        return;
    }

    const FocusState oldState = m_currentState;
    m_currentState = newState;
    qDebug() << "[PlaybackSM] state" << stateToString(oldState) << "->" << stateToString(newState);
    emit focusStateChanged(newState, oldState);
}

const char* PlaybackStateManager::stateToString(FocusState state)
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

void PlaybackStateManager::scheduleIdleFrom(FocusState state)
{
    m_pendingIdleFrom = state;
    if (!m_idleDelayTimer.isActive()) {
        qDebug() << "[PlaybackSM] schedule idle from" << stateToString(state);
        m_idleDelayTimer.start();
    }
}

void PlaybackStateManager::cancelIdleSchedule()
{
    if (m_idleDelayTimer.isActive()) {
        m_idleDelayTimer.stop();
    }
    m_pendingIdleFrom = FocusState::Idle;
}

void PlaybackStateManager::onIdleDelayTimeout()
{
    if (m_pendingIdleFrom == FocusState::AudioFocused && m_currentState == FocusState::AudioFocused) {
        emit evAudioInactive();
    } else if (m_pendingIdleFrom == FocusState::VideoFocused && m_currentState == FocusState::VideoFocused) {
        emit evVideoInactive();
    }
    m_pendingIdleFrom = FocusState::Idle;
}
