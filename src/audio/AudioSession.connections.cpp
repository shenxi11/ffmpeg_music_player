#include "AudioSession.h"

/*
模块名称: AudioSession 信号连接
功能概述: 集中管理解码器与播放器到会话层的连接关系，保证跨线程回调统一使用队列连接。
对外接口: AudioSession::connectSignals()
维护说明: 仅维护连接拓扑，不承载播放业务逻辑。
*/

void AudioSession::connectSignals()
{
    // 解码器信号来自工作线程，使用队列连接切回会话线程处理。
    connect(m_decoder, &AudioDecoder::decodedData, this, &AudioSession::onDecodedData, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::metadataReady, this, &AudioSession::onMetadataReady, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::audioTagsReady, this, &AudioSession::onAudioTagsReady, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::albumArtReady, this, &AudioSession::onAlbumArtReady, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodeError, this, &AudioSession::onDecodeError, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodeCompleted, this, &AudioSession::onDecodeCompleted, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodeStarted, this, &AudioSession::onDecodeStarted, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodePaused, this, &AudioSession::onDecodePaused, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodeStopped, this, &AudioSession::onDecodeStopped, Qt::QueuedConnection);

    // 播放器信号同样使用队列连接，避免跨线程直接回调。
    connect(m_player, &AudioPlayer::positionChanged, this, &AudioSession::onPositionChanged, Qt::QueuedConnection);
    connect(m_player, &AudioPlayer::playbackError, this, &AudioSession::onPlaybackError, Qt::QueuedConnection);
    connect(m_player, &AudioPlayer::bufferStatusChanged, this, &AudioSession::onBufferStatusChanged, Qt::QueuedConnection);
    connect(m_player, &AudioPlayer::bufferUnderrun, this, &AudioSession::onBufferUnderrun, Qt::QueuedConnection);
    connect(m_player, &AudioPlayer::playbackStarted, this, &AudioSession::onPlaybackStarted, Qt::QueuedConnection);
    connect(m_player, &AudioPlayer::playbackPaused, this, &AudioSession::onPlaybackPaused, Qt::QueuedConnection);
    connect(m_player, &AudioPlayer::playbackResumed, this, &AudioSession::onPlaybackResumed, Qt::QueuedConnection);
    connect(m_player, &AudioPlayer::playbackStopped, this, &AudioSession::onPlaybackStopped, Qt::QueuedConnection);
}
