Goal
Move remote audio playback closer to VLC-style behavior by reducing unnecessary network/FLAC seek waiting, improving seek observability, and hardening output buffer metrics.

Completed
Implemented with three SubAgents and main-agent review. The stream worker updated `src/audio/stream/AudioPrefetchStream*` and `AudioCurlStreamBackend*` with curl TTFB/throughput metrics, dynamic FLAC read gates, and faster hard-seek abort/reopen behavior. The seek worker updated `src/audio/AudioSession*` and `AudioDecoder*` with `seek_id`, seek accept reason logs, generation-gated resume, and lower remote-FLAC seek PCM thresholds. The buffer worker updated `src/audio/AudioBuffer*` and `AudioPlayer.cpp` with locked stats snapshots, clamped fill percentages, and capacity/underrun diagnostics.

Changed Scope
Primary scope is `src/audio/` and `.ai` governance records. No playback UI signal contract was intentionally changed.

Open Work
Run a fresh runtime playback pass and inspect only `E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt`. Verify MP3 hot seek still stays near-instant, MP3 cold seek avoids multi-second loading, FLAC startup improves from the prior ~5s case, and FLAC seek resume trends below the prior ~2.0-2.4s window.

Risks
FLAC gates are intentionally more aggressive; slow networks may expose underrun sooner, but generation gates should prevent stale PCM from resuming playback. Added diagnostics increase log volume slightly. Existing QQuickWidget shutdown assert is not fixed in this pass.

Next Entry Point
Start with `src/audio/stream/AudioPrefetchStream.cpp`, `src/audio/AudioSession.cpp`, and `src/audio/AudioPlayer.cpp`, then compare fresh `SEEK_METRIC`, `read_gate_*`, `fetch_done`, `buffer_resume_ms`, and buffer capacity fields in `打印日志.txt`.
