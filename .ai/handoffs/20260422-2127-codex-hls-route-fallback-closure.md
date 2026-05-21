## Goal

Close the remote service-local audio HLS rollout by turning the desktop client into an HLS-primary plus direct/proxy-fallback player, while hardening the backend `playback-info` contract so the remote public server only advertises HLS when manifest artifacts are actually ready.

## Completed

- Claimed `playback-and-audio` and `docs-and-governance`, added the cross-module decision that remote service-local mp3/aac/m4a playback uses HLS as the primary route with prepared direct/proxy fallback.
- Updated [AudioService.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.h), [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp), and [AudioService.connections.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.connections.cpp):
  - added explicit per-session route state for `Hls` vs `DirectProxy`
  - exposed route-state getters for current/target session lookup
  - changed `playback-info` handling to distinguish `Supported`, `Unsupported`, and `TemporaryFailure`
  - kept explicit unsupported tracks cached, but stopped turning temporary failures into permanent HLS disable
  - prepared direct/proxy fallback immediately after successful HLS start via cache warmup, proxy resolve, and seek-index prefetch
  - added proactive fallback switching for large HLS seek jumps and runtime fallback reopening for failed HLS seek sessions
- Updated [AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h) and [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp):
  - added `fallbackRouteRequested(targetMs, reason)` for failed HLS seek sessions
  - HLS seek failures now ask `AudioService` to switch route instead of immediately surfacing a generic session error
- Local client validation passed:
  - `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`
- Paired server worker completed all backend work in `shen@192.168.1.208:/home/shen/microservice-deploy` and deployed to `root@122.51.73.110:/root/CloudMusic`:
  - remote commits:
    - `0a26a87 fix(media): gate local playback hls readiness`
    - `f0293b3 fix(gateway): preserve playback-info host port`
  - remote files changed:
    - `internal/media/handler/playback_info.go`
    - `internal/media/handler/playback_info_test.go`
    - `deploy/nginx/nginx.docker.conf`
    - `deploy/nginx/nginx.split.conf`
  - remote curl validation confirmed:
    - `http://127.0.0.1:8080/health` returns ok
    - `暗号/暗号.mp3` returns `hls_supported=true` with a reachable manifest URL on `122.51.73.110:8080`
    - `林俊杰-我很想爱他/林俊杰-我很想爱他.flac` returns `hls_supported=false`

## Changed Scope

- [AudioService.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.h)
- [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp)
- [AudioService.connections.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.connections.cpp)
- [AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h)
- [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp)
- [.ai/ownership.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml)
- [.ai/tasks.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml)
- [.ai/decisions.md](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\decisions.md)

## Open Work

- Run the desktop client against `122.51.73.110:8080` and capture a fresh [打印日志.txt](E:\FFmpeg_whisper\ffmpeg_music_player\打印日志.txt) for the exact cold-start seek cases:
  - first seek on a never-cached remote mp3
  - repeated large forward seeks
  - large backward seek after several forward jumps
- Confirm logs now show:
  - `active_route=hls` on initial HLS path
  - `fallback_prepared=true` before large HLS seek dispatch
  - `switching current session from HLS to direct fallback` when route switch occurs
  - no repeated `decoded=0` HLS failure loop without route switch
- If runtime still shows wrong-position audio after fallback reopen, inspect whether `newSession->play(); newSession->seekTo(targetMs);` needs a tighter paused-state preservation or delayed seek ordering.

## Risks

- The new fallback reopen currently preserves paused state with a `QTimer::singleShot(0, newSession, &AudioSession::pause)` after `play()+seekTo()`. This is pragmatic, but runtime verification is still needed for “paused drag then stay paused” behavior.
- `prepareDirectFallback()` currently uses `prefetchForSeek(originalSource, 0, 0)` as a lightweight seek-index/bootstrap trigger. It builds and should be harmless, but it is still an indirect path rather than a dedicated explicit “prepare seek-index only” API.
- The track-level `m_hlsRuntimeFallbackTracks` set intentionally keeps a track on direct/proxy for the rest of the process after a runtime HLS fallback. That matches the rollout goal, but it also means HLS is not retried for the same track until app restart.
- Remote runtime curl validation covered `mp3` and `flac`; `aac/m4a` readiness is covered by backend tests but was not curl-validated on the formal server because no sample files were found there.

## Next Entry Point

- Start from [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp) around:
  - `loadPreferredSessionSource()`
  - `prepareDirectFallback()`
  - `onSeekDispatchTimeout()`
  - `trySwitchCurrentSessionToFallbackRoute()`
- Then inspect [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp) `enterFailedSeekState()` to confirm the HLS-only fallback signal path is the one taken in logs.
- For backend confirmation, use the already deployed remote state at `root@122.51.73.110:/root/CloudMusic` on `main` `HEAD=f0293b3`; if more backend work is needed, go back to `shen@192.168.1.208:/home/shen/microservice-deploy` instead of editing `CloudMusic`.
