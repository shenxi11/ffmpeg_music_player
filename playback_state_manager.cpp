#include "playback_state_manager.h"
#include <QDebug>

PlaybackStateManager& PlaybackStateManager::getInstance()
{
    static PlaybackStateManager instance;
    return instance;
}

PlaybackStateManager::PlaybackStateManager(QObject *parent)
    : QObject(parent)
    , m_stateMachine(new QStateMachine(this))
    , m_currentState(PlaybackState::Stopped)
    , m_seekTimeout(new QTimer(this))
    , m_currentPosition(0)
    , m_duration(0)
{
    setupStateMachine();
    
    // 跳转超时处理
    m_seekTimeout->setSingleShot(true);
    m_seekTimeout->setInterval(5000); // 5秒超时
    connect(m_seekTimeout, &QTimer::timeout, this, &PlaybackStateManager::onSeekTimeout);
}

void PlaybackStateManager::setupStateMachine()
{
    // 创建状态
    m_stoppedState = new QState(m_stateMachine);
    m_loadingState = new QState(m_stateMachine);
    m_playingState = new QState(m_stateMachine);
    m_pausedState = new QState(m_stateMachine);
    m_seekingState = new QState(m_stateMachine);
    m_errorState = new QState(m_stateMachine);
    
    // 设置初始状态
    m_stateMachine->setInitialState(m_stoppedState);
    
    // 状态转换规则
    // 从停止状态
    m_stoppedState->addTransition(this, &PlaybackStateManager::playRequested, m_loadingState);
    
    // 从加载状态
    m_loadingState->addTransition(this, &PlaybackStateManager::pauseRequested, m_pausedState);
    
    // 从播放状态
    m_playingState->addTransition(this, &PlaybackStateManager::pauseRequested, m_pausedState);
    m_playingState->addTransition(this, &PlaybackStateManager::stopRequested, m_stoppedState);
    m_playingState->addTransition(this, &PlaybackStateManager::seekRequested, m_seekingState);
    
    // 从暂停状态
    m_pausedState->addTransition(this, &PlaybackStateManager::playRequested, m_playingState);
    m_pausedState->addTransition(this, &PlaybackStateManager::stopRequested, m_stoppedState);
    m_pausedState->addTransition(this, &PlaybackStateManager::seekRequested, m_seekingState);
    
    // 从跳转状态
    m_seekingState->addTransition(this, &PlaybackStateManager::pauseRequested, m_pausedState);
    m_seekingState->addTransition(this, &PlaybackStateManager::stopRequested, m_stoppedState);
    
    // 状态进入信号
    connect(m_stoppedState, &QState::entered, this, &PlaybackStateManager::onStateEntered);
    connect(m_loadingState, &QState::entered, this, &PlaybackStateManager::onStateEntered);
    connect(m_playingState, &QState::entered, this, &PlaybackStateManager::onStateEntered);
    connect(m_pausedState, &QState::entered, this, &PlaybackStateManager::onStateEntered);
    connect(m_seekingState, &QState::entered, this, &PlaybackStateManager::onStateEntered);
    connect(m_errorState, &QState::entered, this, &PlaybackStateManager::onStateEntered);
    
    // 启动状态机
    m_stateMachine->start();
}

void PlaybackStateManager::onStateEntered()
{
    QState* currentState = qobject_cast<QState*>(sender());
    PlaybackState oldState = m_currentState;
    
    if (currentState == m_stoppedState) {
        m_currentState = PlaybackState::Stopped;
        emit updatePlayButton(false);
    } else if (currentState == m_loadingState) {
        m_currentState = PlaybackState::Loading;
    } else if (currentState == m_playingState) {
        m_currentState = PlaybackState::Playing;
        emit updatePlayButton(true);
    } else if (currentState == m_pausedState) {
        m_currentState = PlaybackState::Paused;
        emit updatePlayButton(false);
    } else if (currentState == m_seekingState) {
        m_currentState = PlaybackState::Seeking;
        m_seekTimeout->start(); // 启动跳转超时
    } else if (currentState == m_errorState) {
        m_currentState = PlaybackState::Error;
        emit updatePlayButton(false);
    }
    
    qDebug() << "状态变化:" << static_cast<int>(oldState) << "->" << static_cast<int>(m_currentState);
    emit stateChanged(m_currentState, oldState);
}

PlaybackState PlaybackStateManager::getCurrentState() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentState;
}

bool PlaybackStateManager::isPlaying() const
{
    return getCurrentState() == PlaybackState::Playing;
}

bool PlaybackStateManager::isPaused() const
{
    return getCurrentState() == PlaybackState::Paused;
}

bool PlaybackStateManager::isSeeking() const
{
    return getCurrentState() == PlaybackState::Seeking;
}

void PlaybackStateManager::requestPlay()
{
    qDebug() << "请求播放, 当前状态:" << static_cast<int>(getCurrentState());
    emit playRequested();
}

void PlaybackStateManager::requestPause()
{
    qDebug() << "请求暂停, 当前状态:" << static_cast<int>(getCurrentState());
    emit pauseRequested();
}

void PlaybackStateManager::requestStop()
{
    qDebug() << "请求停止, 当前状态:" << static_cast<int>(getCurrentState());
    emit stopRequested();
}

void PlaybackStateManager::requestSeek(qint64 position)
{
    qDebug() << "请求跳转到位置:" << position << "ms, 当前状态:" << static_cast<int>(getCurrentState());
    m_currentPosition = position;
    emit seekRequested(position);
}

void PlaybackStateManager::notifyLoadComplete()
{
    if (getCurrentState() == PlaybackState::Loading) {
        // 加载完成，转到播放状态
        m_stateMachine->postEvent(new QEvent(static_cast<QEvent::Type>(1001)));
        transitionToState(PlaybackState::Playing);
    }
}

void PlaybackStateManager::notifySeekComplete()
{
    if (getCurrentState() == PlaybackState::Seeking) {
        m_seekTimeout->stop();
        // 跳转完成，根据之前状态决定转向
        transitionToState(PlaybackState::Playing); // 默认转到播放状态
        qDebug() << "跳转完成，恢复播放";
    }
}

void PlaybackStateManager::notifyError(const QString& error)
{
    qDebug() << "播放错误:" << error;
    transitionToState(PlaybackState::Error);
}

void PlaybackStateManager::onSeekTimeout()
{
    qDebug() << "跳转超时，强制恢复到暂停状态";
    transitionToState(PlaybackState::Paused);
}

void PlaybackStateManager::transitionToState(PlaybackState newState)
{
    // 这里可以添加强制状态转换的逻辑
    // 通常通过发送特定事件来触发状态转换
}