#include "AudioInputController.h"

#include "AudioService.h"

#include <QDebug>

namespace {
constexpr qint64 kLargeSeekJumpMs = 45 * 1000;
constexpr int kLargeSeekCoalesceMs = 35;
constexpr int kNormalSeekCoalesceMs = 18;
} // namespace

AudioInputController::AudioInputController(AudioService* service, QObject* parent)
    : QObject(parent), m_service(service) {
    m_processTimer.setSingleShot(true);
    m_seekTimer.setSingleShot(true);
    connect(&m_processTimer, &QTimer::timeout, this, &AudioInputController::processNextCommand);
    connect(&m_seekTimer, &QTimer::timeout, this, &AudioInputController::dispatchPendingSeek);
}

bool AudioInputController::postPlayUrl(const QUrl& url) {
    if (!m_service) {
        return false;
    }

    clearAll();
    return m_service->executePlayInternal(url);
}

void AudioInputController::postPause() {
    enqueue(makeCommand(CommandType::Pause));
}

void AudioInputController::postResume() {
    enqueue(makeCommand(CommandType::Resume));
}

void AudioInputController::postStop() {
    clearAll();
    Command command = makeCommand(CommandType::Stop);
    if (m_service) {
        m_service->executeStopInternal();
    }
    Q_UNUSED(command);
}

void AudioInputController::postSeek(const QString& sessionId, qint64 positionMs,
                                    qint64 currentPositionMs) {
    if (sessionId.isEmpty()) {
        return;
    }

    removeQueuedCommands(CommandType::Seek);
    Command command = makeCommand(CommandType::Seek);
    command.sessionId = sessionId;
    command.positionMs = qMax<qint64>(0, positionMs);
    m_pendingSeek = command;
    m_hasPendingSeek = true;
    m_lastSeekSessionId = command.sessionId;
    m_lastSeekPositionMs = command.positionMs;

    const qint64 jumpDistance = qAbs(command.positionMs - qMax<qint64>(0, currentPositionMs));
    const int delayMs = jumpDistance >= kLargeSeekJumpMs ? kLargeSeekCoalesceMs
                                                         : kNormalSeekCoalesceMs;
    m_seekTimer.start(delayMs);
}

void AudioInputController::postPlayNext() {
    clearPendingForSession(m_service ? m_service->currentSessionIdForController() : QString());
    enqueue(makeCommand(CommandType::PlayNext));
}

void AudioInputController::postPlayPrevious() {
    clearPendingForSession(m_service ? m_service->currentSessionIdForController() : QString());
    enqueue(makeCommand(CommandType::PlayPrevious));
}

void AudioInputController::postPlayAtIndex(int index) {
    clearPendingForSession(m_service ? m_service->currentSessionIdForController() : QString());
    Command command = makeCommand(CommandType::PlayAtIndex);
    command.index = index;
    enqueue(command);
}

void AudioInputController::postSetVolume(int volume) {
    removeQueuedCommands(CommandType::SetVolume);
    Command command = makeCommand(CommandType::SetVolume);
    command.volume = volume;
    enqueue(command);
}

void AudioInputController::clearPendingForSession(const QString& sessionId) {
    if (!sessionId.isEmpty()) {
        removeQueuedCommandsForSession(sessionId);
        if (m_hasPendingSeek && m_pendingSeek.sessionId == sessionId) {
            m_hasPendingSeek = false;
            m_seekTimer.stop();
        }
        return;
    }

    if (m_hasPendingSeek) {
        m_hasPendingSeek = false;
        m_seekTimer.stop();
    }
}

void AudioInputController::clearAll() {
    m_queue.clear();
    m_hasPendingSeek = false;
    m_seekTimer.stop();
    m_processTimer.stop();
}

bool AudioInputController::pendingSeekForSession(const QString& sessionId,
                                                 qint64* positionMs) const {
    if (m_hasPendingSeek && m_pendingSeek.sessionId == sessionId) {
        if (positionMs) {
            *positionMs = m_pendingSeek.positionMs;
        }
        return true;
    }

    if (m_lastSeekSessionId != sessionId || m_lastSeekPositionMs < 0) {
        return false;
    }

    if (positionMs) {
        *positionMs = m_lastSeekPositionMs;
    }
    return true;
}

void AudioInputController::processNextCommand() {
    if (!m_service || m_queue.isEmpty()) {
        return;
    }

    const Command command = m_queue.dequeue();
    switch (command.type) {
        case CommandType::PlayUrl:
            m_service->executePlayInternal(command.url);
            break;
        case CommandType::Pause:
            m_service->executePauseInternal();
            break;
        case CommandType::Resume:
            m_service->executeResumeInternal();
            break;
        case CommandType::Stop:
            m_service->executeStopInternal();
            break;
        case CommandType::Seek:
            m_service->executeSeekInternal(command.sessionId, command.positionMs);
            break;
        case CommandType::PlayNext:
            m_service->executePlayNextInternal();
            break;
        case CommandType::PlayPrevious:
            m_service->executePlayPreviousInternal();
            break;
        case CommandType::PlayAtIndex:
            m_service->executePlayAtIndexInternal(command.index);
            break;
        case CommandType::SetVolume:
            m_service->executeSetVolumeInternal(command.volume);
            break;
    }

    if (!m_queue.isEmpty()) {
        scheduleProcess();
    }
}

void AudioInputController::dispatchPendingSeek() {
    if (!m_hasPendingSeek) {
        return;
    }

    Command command = m_pendingSeek;
    m_hasPendingSeek = false;
    enqueue(command);
}

void AudioInputController::enqueue(const Command& command) {
    m_queue.enqueue(command);
    scheduleProcess();
}

void AudioInputController::scheduleProcess(int delayMs) {
    if (m_processTimer.isActive()) {
        return;
    }
    m_processTimer.start(delayMs);
}

void AudioInputController::removeQueuedCommands(CommandType type) {
    QQueue<Command> filtered;
    while (!m_queue.isEmpty()) {
        Command command = m_queue.dequeue();
        if (command.type != type) {
            filtered.enqueue(command);
        }
    }
    m_queue = filtered;
}

void AudioInputController::removeQueuedCommandsForSession(const QString& sessionId) {
    if (sessionId.isEmpty()) {
        return;
    }

    QQueue<Command> filtered;
    while (!m_queue.isEmpty()) {
        Command command = m_queue.dequeue();
        if (command.sessionId != sessionId) {
            filtered.enqueue(command);
        }
    }
    m_queue = filtered;
}

AudioInputController::Command AudioInputController::makeCommand(CommandType type) {
    Command command;
    command.type = type;
    command.generation = ++m_nextGeneration;
    if (m_service) {
        command.sessionId = m_service->currentSessionIdForController();
    }
    return command;
}
