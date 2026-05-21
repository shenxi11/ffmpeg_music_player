# Goal
Reduce ordinary remote MP3 startup/seek latency by moving closer to a VLC-style continuous input model: demux-led positioning, non-exact seek hints, bounded buffering gates, and no session-layer waiting for MP3 timestamps to naturally catch up.

# Completed
SubAgents performed read-only analysis of `E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt`, `src/audio/stream`, and `src/audio/AudioDecoder.cpp` / `src/audio/AudioSession.cpp`.

Main-agent integration completed:
- `src/audio/AudioDecoder.h` and `src/audio/AudioDecoder.cpp`: seek requests now bind target/mode to a pending seek generation before waking the decode thread, preventing new target / old generation mismatch during repeated slider drags.
- `src/audio/AudioCacheManager.cpp`: direct seek hint status now reports real `seekIndexHit` separately from `exact`, so server seek-index hits are not treated as byte-exact by default.
- `src/audio/AudioSession.cpp`: MP3 first decoded packet no longer waits for natural timestamp catch-up; early landing can rebase the playback clock, otherwise the first packet is accepted as non-exact recovery and written to the audio buffer.
- `src/audio/stream/AudioAvioBridge.cpp`: `prepareSeek()` now registers seek hints without pre-forcing byte seek, leaving FFmpeg demux seek as the final positioning owner.
- `src/audio/stream/AudioPrefetchStream.cpp`: ordinary fast seek read gate was reduced to return first data sooner.

# Changed Scope
Changed files:
- `src/audio/AudioCacheManager.cpp`
- `src/audio/AudioDecoder.cpp`
- `src/audio/AudioDecoder.h`
- `src/audio/AudioSession.cpp`
- `src/audio/stream/AudioAvioBridge.cpp`
- `src/audio/stream/AudioPrefetchStream.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

# Open Work
Run the freshly built client and reproduce MP3 seek against an online MP3 such as `/uploads/枫/枫.mp3`. Then inspect only `E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt`.

Expected runtime signals:
- Ordinary remote MP3 should not show `forced_byte_seek`.
- `AudioDecoder` should log a stable `seek_generation` that matches `decodedData` generation.
- MP3 seek should log `[MP3_SEEK_ACCEPT]` or `[MP3_SEEK_ACCEPT_NON_EXACT]` near the first decoded packet instead of many `[FLAC_SEEK_VERIFY] remote_flac=false` lines.
- `SEEK_METRIC` should track first decoded/write readiness rather than several seconds of timestamp catch-up.

# Risks
This change intentionally prioritizes low-latency MP3 resume over strict timestamp landing. If an MP3 seek lands very far from target, playback may resume at a non-exact decoded position while logging the drift for future hint correction.

FLAC remains on the stricter path. A separate FLAC seektable/refine implementation is still needed for VLC-like FLAC seeking.

# Next Entry Point
Start with:
- `src/audio/AudioSession.cpp::handleSeekLatencyOnDecodedTimestamp`
- `src/audio/AudioDecoder.cpp::seekTo`
- `src/audio/stream/AudioAvioBridge.cpp::prepareSeek`
- `src/audio/stream/AudioPrefetchStream.cpp::read` and `seek`

Use `E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt` only for the next runtime validation pass.
