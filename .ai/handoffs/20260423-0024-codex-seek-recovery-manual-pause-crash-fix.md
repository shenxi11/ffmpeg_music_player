## Goal
Fix the drag-seek crash where a reopened recovery session inherited an internal paused state as a manual pause, then hit the `manualPaused && buffering.active` assertion inside `AudioSession::onSeekRetryTimeout`.

## Completed
- Updated [AudioSession.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioSession.h) and [AudioSession.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioSession.cpp) to expose `isManuallyPaused()` as the explicit user-intent pause query instead of reusing the broader `isPaused()` semantics.
- Updated [AudioService.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioService.cpp) so all three recovery/reopen paths (`direct/proxy -> HLS`, `HLS -> fallback`, and `direct/proxy reopen`) preserve only real manual pauses across rebuilt sessions.
- Added recovery logs with `old_is_paused`, `old_manual_paused`, and `new_session_resume_mode` so fresh runtime logs can confirm whether reopened sessions are correctly classified as `auto_recover` versus `manual_paused`.
- Added a final race guard in `AudioSession::onSeekRetryTimeout()` that clears `buffering.active` and returns if an async manual pause arrives before the state-machine finalize step, preventing the assertion crash from firing on that edge.
- Verified the client builds with `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

## Changed Scope
- The fix stays inside `src/audio/*` plus `.ai/*` governance records.
- No service code, shell-core hosts, QML, or settings/theme files were changed in this session.

## Open Work
- Run the desktop client against the remote server again and inspect [打印日志.txt](/E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt) for one large-seek recovery flow.
- Confirm the recovery log now shows `old_is_paused=true old_manual_paused=false new_session_resume_mode=auto_recover` for the previously crashing path.
- Check whether the crash is gone and whether the remaining issue is now only seek latency / seek accuracy, not state-machine failure.

## Risks
- This fix hardens the reopen/recovery pause semantics and the retry-timeout finalize path, but it does not by itself solve remote direct-HTTP seek accuracy or public-network latency.
- The extra `onSeekRetryTimeout()` race guard prevents the assert crash, but if another async path still mutates pause/buffering state incorrectly, the next symptom may be a recoverable stalled seek rather than a crash.

## Next Entry Point
- Start with [打印日志.txt](/E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt) after one new remote playback run.
- If the crash is gone but recovery still stalls, inspect [AudioService.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioService.cpp) around the recovery logs and [AudioSession.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioSession.cpp) around `onSeekRetryTimeout()`.
