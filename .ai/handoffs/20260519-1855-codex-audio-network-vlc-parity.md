# Goal
Reduce remote audio startup and seek latency by coordinating SubAgents across stream/AVIO, decoder, and session buffering layers.

# Completed
- Worker 1 updated src/audio/stream/AudioPrefetchStream* and src/audio/stream/AudioAvioBridge* for smaller read gates, exact-only forced byte seek, bounded seek guard, and generation-safe stale response handling.
- Worker 2 updated src/audio/AudioDecoder* for deadline-aware network seek/read behavior, HTTPS custom AVIO coverage, bounded first-packet wait, FLAC seek failure handling, and removal of decode-thread artificial sleep.
- Worker 3 updated src/audio/AudioSession* so buffering only finishes after decoded data is actually written, seek metrics finalize after decoded-write readiness, and remote seek retry is bounded/single-flight.
- Main agent reviewed combined diffs, updated .ai/ownership.yaml and .ai/tasks.yaml, and verified cmake --build build/vs2022-x64-debug --target ffmpeg_music_player --config Debug --parallel 4 succeeds.

# Changed Scope
- src/audio/stream/AudioPrefetchStream.cpp
- src/audio/stream/AudioPrefetchStream.h
- src/audio/stream/AudioAvioBridge.cpp
- src/audio/stream/AudioAvioBridge.h
- src/audio/AudioDecoder.cpp
- src/audio/AudioDecoder.h
- src/audio/AudioSession.cpp
- src/audio/AudioSession.h
- .ai/ownership.yaml
- .ai/tasks.yaml

# Open Work
- Runtime test remote MP3 seek and remote FLAC seek in the app.
- Analyze only E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt; do not use 旧打印日志.txt.
- Confirm seek windows show buffering_decoded_write_ready, seek_to_first_decoded_ms, buffer_resume_ms, and SEEK_METRIC seek_to_decoded_write_threshold_resume_ms in the expected order.

# Risks
- Remote FLAC without a precise seek index still cannot fully match VLC; this change reduces client-side waiting and repeated reseek but does not add a server-side FLAC SEEKTABLE/format seek index.
- Current worktree contains many unrelated dirty files; avoid reverting or formatting outside the changed scope.
- windeployqt still warns that VCINSTALLDIR is not set, but target build succeeds.

# Next Entry Point
- Start with src/audio/AudioSession.cpp if loading state still sticks.
- Start with src/audio/stream/AudioPrefetchStream.cpp if logs show delayed prefetch_first_byte_ms or repeated stale response loops.
- Start with src/audio/AudioDecoder.cpp if logs show first packet wait exhaustion or FLAC seek decode errors.
