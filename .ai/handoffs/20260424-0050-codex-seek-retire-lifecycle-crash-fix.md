## Goal
Fix the new large-seek APPCRASH where the desktop client died in `Qt5Cored.dll` immediately after the first decoded frame of a reopened recovery session, most likely because a session was being destroyed while queued decoder/player callbacks could still target it.

## Completed
- Updated [AudioSession.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioSession.h) and [AudioSession.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioSession.cpp) to add explicit retire lifecycle state:
  - `beginRetire(const char* reason)`
  - `isRetiring()`
  - runtime signal disconnection from `AudioDecoder` and `AudioPlayer`
  - a common async-callback guard that drops late queued callbacks while retiring
- Updated [AudioSession.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioSession.cpp) so `play`, `resume`, and `seekTo` refuse to run on retiring sessions, `stop()` becomes safer/idempotent for retire cleanup, and seek-path logs now include a `decoded packet entry` marker before the first-packet handler.
- Updated [AudioService.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioService.cpp) so `destroySession()` no longer directly `delete`s runtime sessions. It now removes the session from service maps, calls `beginRetire("destroySession")`, and uses `deleteLater()` during normal runtime while keeping direct delete only for shutdown (`QCoreApplication::closingDown()`).
- Updated `switchToSession()` to retire the old session through `destroySession()` instead of separately calling `oldSession->stop()` followed by a synchronous delete.
- Verified the client builds with `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

## Changed Scope
- This session only touched `src/audio/*` plus `.ai/*` governance records.
- No service code, shell-core routing, QML layout, or settings/theme code was modified.

## Open Work
- Run the client against the remote server again and reproduce one large forward seek on an online MP3.
- Inspect [打印日志.txt](/E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt) after the new run to confirm the log now continues past `AudioDecoder: First decoded frame arrived after seek`.
- Check Windows Event Viewer to confirm the `Qt5Cored.dll / c0000005` APPCRASH no longer appears for the same seek path.

## Risks
- This pass hardens session retirement and queued callback safety, but it does not solve the underlying remote seek precision problem; the app may still buffer too long or land on the wrong timeline even if it no longer crashes.
- `onDecodedData` now logs additional seek-entry markers while active seek latency is in progress, so temporary log volume around one failing seek will increase.

## Next Entry Point
- Start with a fresh reproduction and then read [打印日志.txt](/E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt).
- If the crash is gone but the seek still stalls or lands at the wrong position, inspect [AudioSession.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioSession.cpp) around `onDecodedData()` and `handleSeekLatencyOnDecodedTimestamp()`.
- If a new crash still occurs, compare the final log line against the new `decoded packet entry` marker to determine whether the failure is before queued-slot entry or inside the seek first-packet handling path.
