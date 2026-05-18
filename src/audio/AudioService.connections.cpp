#include "AudioService.h"

#include "AudioCacheManager.h"
#include "settings_manager.h"

void AudioService::setupServiceConnections() {
    connect(&SettingsManager::instance(), &SettingsManager::audioCachePathChanged, this,
            &AudioService::onAudioCachePathChanged);

    connect(&AudioCacheManager::instance(), &AudioCacheManager::fullFileCacheReady, this,
            &AudioService::onFullFileCacheReady);
}

void AudioService::connectSessionSignals(AudioSession* session) {
    if (!session) {
        return;
    }

    connect(session, &AudioSession::sessionFinished, this, &AudioService::onSessionFinished);
    connect(session, &AudioSession::sessionError, this, &AudioService::onSessionError);
    connect(session, &AudioSession::metadataReady, this, &AudioService::onMetadataReady);
    connect(session, &AudioSession::albumArtReady, this, &AudioService::onAlbumArtReady);
    connect(session, &AudioSession::positionChanged, this, &AudioService::onPositionChanged);
    connect(session, &AudioSession::durationChanged, this, &AudioService::onDurationChanged);
    connect(session, &AudioSession::bufferingStarted, this, &AudioService::onBufferingStarted);
    connect(session, &AudioSession::bufferingProgress, this, &AudioService::onBufferingProgress);
    connect(session, &AudioSession::bufferingFinished, this, &AudioService::onBufferingFinished);
    connect(session, &AudioSession::sessionStarted, this, &AudioService::onSessionStarted);
    connect(session, &AudioSession::sessionPaused, this, &AudioService::onSessionPaused);
    connect(session, &AudioSession::sessionResumed, this, &AudioService::onSessionResumed);
}
