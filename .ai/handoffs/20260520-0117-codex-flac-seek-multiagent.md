# Goal
Repair remote FLAC seek/playback behavior without regressing the accepted MP3 low-latency seek path.

# Completed
Implemented the SubAgent-reviewed FLAC fix in `src/audio`.

Stream/AVIO:
- `src/audio/stream/AudioAvioBridge.cpp`
- `src/audio/stream/AudioPrefetchStream.cpp`

Remote FLAC now forces a backed-off byte Range anchor for non-exact seek hints, logs `flac_anchor_byte` / `flac_range_reopen`, rejects far internal AVIO seeks around approximate anchors, and no longer coalesces into already-passed uncached ranges. Curl abort is now issued outside the stream mutex using a retained backend handle.

Decoder/session:
- `src/audio/AudioDecoder.cpp`
- `src/audio/AudioSession.cpp`

Remote FLAC seek generation is not accepted until the first decoded frame lands within tolerance. First-packet waiting is deadline-oriented instead of exhausting retries in a few milliseconds. FLAC seek drift rejection emits a decode error, and session play/resume routes failed FLAC seek through a controlled re-seek recovery gate instead of resuming empty or wrong PCM.

Validation:
- `git diff --check` passed for the touched audio files.
- `cmake --build --preset vs2022-x64-debug --config Debug --target ffmpeg_music_player` passed.
- Build output: `E:\FFmpeg_whisper\ffmpeg_music_player_build\Debug\ffmpeg_music_player.exe`.

# Changed Scope
Touched only the playback/audio module plus governance records:
- `src/audio/stream/AudioAvioBridge.h`
- `src/audio/stream/AudioAvioBridge.cpp`
- `src/audio/stream/AudioPrefetchStream.h`
- `src/audio/stream/AudioPrefetchStream.cpp`
- `src/audio/AudioDecoder.h`
- `src/audio/AudioDecoder.cpp`
- `src/audio/AudioSession.h`
- `src/audio/AudioSession.cpp`
- `src/audio/AudioService.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

# Open Work
Run the freshly built client and validate FLAC with a new `E:\FFmpeg_whisper\ffmpeg_music_player\打印日志.txt`.

Expected fresh-log markers:
- `flac_anchor_byte`
- `flac_range_reopen`
- `curl_upstream_open offset=<anchor byte>`
- `[FLAC_SEEK_ACCEPT]` after a valid first decoded timestamp, or `flac_seek_recovery_state=failed` without wrong-position playback.

# Risks
Runtime FLAC validation has not been performed in this session. The current `打印日志.txt` still contains the old failing run and should be overwritten by a fresh app run before drawing runtime conclusions.

The FLAC approximate anchor is still based on byte/time estimates, not a parsed native FLAC seektable. If some high-bitrate FLAC files have poor byte hint quality, the new gate should fail safely, but user-perceived seek success may still require a real seektable/index implementation later.

# Next Entry Point
Start from:
- `src/audio/stream/AudioAvioBridge.cpp::prepareSeek`
- `src/audio/stream/AudioPrefetchStream.cpp::forceSeekToHint`
- `src/audio/AudioDecoder.cpp::decode`
- `src/audio/AudioSession.cpp::beginRemoteFlacSeekRecovery`

Task board entry:
- `.ai/tasks.yaml` task `optimize-flac-network-seek-vlc-input-model` is in `review`.

Ownership:
- `.ai/ownership.yaml` module `playback-and-audio` is released.
