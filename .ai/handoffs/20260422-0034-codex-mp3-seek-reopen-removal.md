Goal

Remove MP3/AAC/M4A reopen-on-seek as the primary path for server-local online audio and replace it with an in-session indexed seek flow that reuses the active proxy handle, coalesces duplicate seek requests earlier, and avoids the `Operation not permitted` failures seen when FFmpeg tries to reopen a mid-file MP3 HTTP range.

Completed

- Updated [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp) and [AudioService.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.h) so seek requests are coalesced at the service layer before dispatch, using a 60ms window and a duplicate guard for near-identical targets.
- Removed the MP3/AAC/M4A reopen-on-seek branch from `AudioService::seekTo()`. Indexed compressed HTTP audio now stays on the current session and logs `seek_execution_mode=in_session_indexed` instead of creating a fresh seek session.
- Updated [AudioCacheManager.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.h) and [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp) to add `primeSeekHintForPlaybackUrl()`, which writes a one-shot `pendingSeekByteOffset` onto the active proxy handle instead of building a new playback URL.
- Changed proxy request handling so a primed seek hint overrides the next upstream request as an open-ended `Range: bytes=<offset>-` request, logs `seek_hint_applied=true`, and clears the hint after first use.
- Updated [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp) and [AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h) so indexed in-session seek uses a 3s first-packet tolerance, still caps corrective reseek at one retry, and no longer exposes the old `prepareReopenSeek()` path.
- Verified local build success with `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

Changed Scope

- [AudioCacheManager.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.h)
- [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp)
- [AudioService.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.h)
- [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp)
- [AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h)
- [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp)
- [ownership.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml)
- [tasks.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml)

Open Work

- Run the desktop client against `122.51.73.110` and capture a fresh large-jump seek log for the same MP3 used in the prior analysis.
- Confirm the log no longer shows `Could not open input file: Operation not permitted` for MP3/AAC/M4A seek operations.
- Confirm the log no longer shows `Reopen seek handle request` and instead shows `Primed seek hint`, `seek_hint_applied=true`, and `seek_execution_mode=in_session_indexed`.
- Measure whether remote large-jump seek recovery is materially better than before and whether playback still lands early enough to need a second precision pass.

Risks

- This pass removes the reopen path for compressed HTTP audio, but it does not change FFmpeg’s underlying in-session seek accuracy for VBR MP3; if landing is still noticeably early, another correction pass may still be needed.
- The open-ended seek hint deliberately biases the next proxy request to `bytes=<offset>-`; this should be more FFmpeg-friendly than reopening a bounded mid-file window, but runtime verification is still required to confirm it behaves well on repeated large jumps.
- `git diff --check` still reports a pre-existing trailing-whitespace issue in [打印日志.txt](E:\FFmpeg_whisper\ffmpeg_music_player\打印日志.txt); this session did not normalize that log file.

Next Entry Point

- Start with [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp) `seekTo()` and `onSeekDispatchTimeout()` while reproducing on the remote server to confirm only one seek dispatch happens per slider release and that no fresh seek session is created.
- Then watch [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp) around `primeSeekHintForPlaybackUrl()` and `handleProxyRequest()` to verify the active handle receives the correct hint byte offset and clears it after the first overridden request.
- If landing precision is still poor after the reopen removal, continue in [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp) `handleSeekLatencyOnDecodedTimestamp()` to tune indexed MP3 correction thresholds using the new remote runtime logs.
