## Goal

Adjust the desktop playback seek-timeout strategy so a seek timeout no longer maps directly to immediate user-visible failure. The intended behavior is: keep buffering and retrying longer, request HLS-to-direct fallback before failing, and only surface a final seek error after the extended buffering and fallback path are both exhausted.

## Completed

- Updated [AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h) and [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp):
  - expanded `SeekRetryAction` with `KeepBuffering` and `RequestFallbackRoute`
  - added per-seek timeout extension state in `SeekState`
  - changed `decideSeekRetryAction()` to treat HLS and non-HLS timeouts as staged recovery instead of immediate failure
  - HLS seek timeout now requests a fallback route once before final failure
  - direct/indexed seek timeout now gets extra buffering-extension windows before final failure
  - final failure logs now include the timeout-extension count
- Updated [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp):
  - `onSessionFallbackRouteRequested()` no longer emits an immediate UI error when fallback switching cannot happen instantly
  - instead it logs the failed switch attempt and lets the current session continue the extended buffering/retry flow
- Local validation passed:
  - `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`

## Changed Scope

- [AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h)
- [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp)
- [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp)
- [.ai/ownership.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml)
- [.ai/tasks.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml)

## Open Work

- Run the desktop client against `122.51.73.110:8080` and capture a fresh [打印日志.txt](E:\FFmpeg_whisper\ffmpeg_music_player\打印日志.txt) for:
  - cold-start first seek on a never-cached remote mp3
  - repeated large forward seeks
  - large reverse seek after several successful jumps
- Verify the logs now show:
  - `SEEK_DEGRADE` continuing beyond the old first give-up point
  - `SEEK_FALLBACK request fallback route after buffering timeout` for HLS seek timeout
  - no immediate `Seek entered failed state` at the first timeout threshold
  - final failure only after timeout extensions and fallback attempts are exhausted
- If fallback switching still fails repeatedly in runtime, inspect whether the service-level route state should explicitly record “fallback switch already attempted” to avoid repeated no-op requests.

## Risks

- The timeout-extension counts are heuristic: HLS currently gets one fallback-request stage plus two extra buffering extensions, while non-HLS gets two to three extra extensions depending on seek-index status. Runtime logs may still show these numbers need tuning.
- If fallback switching is unavailable but the route remains unstable, the user may now see a longer buffering window before the final error. That is intentional, but it shifts the tradeoff toward patience over fast failure.
- `onSessionFallbackRouteRequested()` now suppresses the previous immediate `serviceError()` path. If the session somehow stops retrying after a failed switch, the user could remain buffering longer than before; this must be verified in runtime logs.

## Next Entry Point

- Start from [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp) around:
  - `onSeekRetryTimeout()`
  - `decideSeekRetryAction()`
  - `handleSeekRetryKeepBuffering()`
  - `handleSeekRetryRequestFallback()`
  - `handleSeekRetryGiveUp()`
- Then inspect [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp) `onSessionFallbackRouteRequested()` to confirm fallback-switch failure now logs and returns instead of surfacing an immediate UI error.
- Use the already deployed remote backend baseline on `122.51.73.110` from commit `f0293b3` for runtime verification; if backend behavior looks suspicious, continue backend work only in `shen@192.168.1.208:/home/shen/microservice-deploy`.
