#include "MediaService.h"

void MediaService::connectMediaSessionSignals(MediaSession* session)
{
    if (!session) {
        return;
    }

    connect(session, &MediaSession::positionChanged,
            this, &MediaService::onMediaSessionPositionChanged);
    connect(session, &MediaSession::metadataReady,
            this, &MediaService::onMediaSessionMetadataReady);
    connect(session, &MediaSession::stateChanged,
            this, &MediaService::onMediaSessionStateChanged);
}
