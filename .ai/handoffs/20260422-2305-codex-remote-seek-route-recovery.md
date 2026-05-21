## Goal
Stabilize remote service-local mp3/aac seek on the desktop client by reducing direct-proxy stale-range pollution, retrying HLS takeover before large direct seeks, and adding route recovery when the first decoded packet lands on an impossible timeline.

## Completed
- Updated [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp) and [AudioService.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.h) so large direct-proxy seeks can retry `/music/local/playback-info` and switch back to HLS before dispatching the seek.
- Added broader route recovery in [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp): fallback requests can now recover either by switching HLS -> direct proxy, direct proxy -> HLS, or reopening a fresh direct/proxy session when the current proxy session is poisoned.
- Tightened the pre-accept sticky-hint fence in [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp) so post-satisfied stale tail ranges are held at the seek fence until the first valid packet is accepted instead of passing through to near-EOF ranges.
- Updated [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp) so indexed direct-proxy seeks request route recovery when proxy hint satisfaction never converges or when the first decoded timestamp is still `0ms` / grossly wrong after hint satisfaction.
- Local Debug build passed with `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

## Changed Scope
- Client-only changes this round:
  - [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp)
  - [AudioService.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.h)
  - [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp)
  - [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp)
  - [.ai/ownership.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml)
  - [.ai/tasks.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml)
- No backend code changed.
- No VM `/home/shen/microservice-deploy` commit was needed.
- No VM `/home/shen/CloudMusic` or remote `/root/CloudMusic` deployment was performed in this pass.

## Open Work
- Run the desktop client against `122.51.73.110` and capture a fresh [打印日志.txt](E:\FFmpeg_whisper\ffmpeg_music_player\打印日志.txt) after:
  - first cold large forward seek
  - multiple consecutive forward seeks
  - a later large backward seek toward the first 30-60 seconds
- Verify whether the new HLS retry path actually takes over on the first large direct-proxy seek instead of timing out again.
- Verify whether any remaining failures now converge into `reopen direct/proxy session` rather than staying paused on a poisoned timeline.

## Risks
- `requestServiceLocalHlsManifest()` now retries synchronously with a longer timeout budget; cold-start control latency may increase when the server is genuinely slow or unavailable.
- Direct/proxy route reopen is still a recovery path, not a root-cause transport replacement. If the remote public chain or decoder state still misbehaves, repeated recoveries may still be visible to the user.
- This pass was only compile-verified locally; no new runtime proof from the remote server logs has been captured yet.

## Next Entry Point
- Start with [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp) around `shouldRetryHlsBeforeLargeDirectSeek`, `trySwitchCurrentSessionToHlsRoute`, and `tryRecoverCurrentSessionRoute`.
- Then inspect [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp) around the `hold-preaccept-*` seek-hint override reasons.
- Finally validate [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp) logs for `[SEEK_RECOVER]` and whether they lead to a successful new session.
