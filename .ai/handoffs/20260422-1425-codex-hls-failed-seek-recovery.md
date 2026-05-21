## Goal
Implement the agreed online-local playback recovery plan: make service-local online music prefer a formal HLS playback contract, and stop seek timeout/decode-error paths from fake-resuming the player, progress, and lyrics from a stale paused position.

## Completed
- Claimed `playback-and-audio` and `docs-and-governance`, then recorded the shared decision that service-local online playback now prefers HLS and failed seeks must not fake-resume.
- Updated [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp) and [AudioService.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.h):
  - added blocking-but-cached `/music/local/playback-info` discovery for service-local upload URLs
  - prefer server-provided HLS manifests before falling back to the existing proxy/direct path
  - removed the proactive “large seek on HLS -> switch to direct stream” path
  - removed the old global HLS-disable-on-timeout fallback
- Updated [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp) and [AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h):
  - added an explicit failed-seek state
  - seek timeout, large-drift failure, and seek-time decode errors now enter failed-seek state instead of emitting normal `bufferingFinished`
  - pressing play after a failed seek now retries the failed seek target instead of resuming `AudioPlayer` from a fake paused timeline
- Backend changes were made only in `shen@192.168.1.208:/home/shen/microservice-deploy`:
  - added `internal/media/handler/playback_info.go`
  - extended `MediaHandler` with HLS root / ffmpeg support
  - added `/music/local/playback-info` to `cmd/media/main.go`, `cmd/monolith/main.go`, and both nginx configs
  - on-demand HLS generation now writes `/hls/local/<cache_key>/index.m3u8` style outputs under the service HLS root
- Backend development build passed with `./build.sh` in `/home/shen/microservice-deploy`.
- Backend commit was created and pushed from `/home/shen/microservice-deploy`:
  - `c442e92 feat(media): add local playback info hls contract`
- Local Qt Debug build passed with:
  - `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`

## Changed Scope
- Client files:
  - [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp)
  - [AudioService.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.h)
  - [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp)
  - [AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h)
  - [.ai/ownership.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml)
  - [.ai/tasks.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml)
  - [.ai/decisions.md](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\decisions.md)
- Backend files in VM dev repo only:
  - `internal/media/handler/playback_info.go`
  - `internal/media/handler/media_handler.go`
  - `cmd/media/main.go`
  - `cmd/monolith/main.go`
  - `deploy/nginx/nginx.split.conf`
  - `deploy/nginx/nginx.docker.conf`

## Open Work
- The formal remote deployment in `root@122.51.73.110:/root/CloudMusic` is still running in the background because `docker compose build` is rebuilding images and slowly installing runtime `ffmpeg` dependencies.
- New endpoint/runtime validation is therefore still pending:
  - `/music/local/playback-info?music_path=东风破/东风破.mp3`
  - the returned `hls_manifest_url`
  - first large-seek behavior on the remote public server after the new containers come up
- Once the Docker rebuild completes, validate that:
  - `/music/local/playback-info` returns 200 instead of 404
  - `/hls/local/.../index.m3u8` is generated and reachable
  - the client no longer fake-resumes after the first failed large seek

## Risks
- `AudioService` currently performs a synchronous playback-info fetch on first play for a service-local upload track. It is cached per-track for the run, but the first request still adds control-path latency.
- If HLS manifest loading fails once for a track in the current run, the client marks that track as direct-fallback-only for the rest of the process.
- Remote deployment has not yet been runtime-verified because the rebuild is still in progress; the code and build artifacts are ready, but the final container rollout is not confirmed.

## Next Entry Point
- Remote deployment status:
  - `ssh -i "$HOME/.ssh/cloudmusic.pem" root@122.51.73.110`
  - `ps -ef | grep -E "deploy_from_main|docker compose -p cloudmusic" | grep -v grep`
  - `tail -n 120 /tmp/cloudmusic_deploy_20260422_1330.log`
- When the build finishes, verify:
  - `curl -fsS "http://127.0.0.1:8080/music/local/playback-info?music_path=%E4%B8%9C%E9%A3%8E%E7%A0%B4/%E4%B8%9C%E9%A3%8E%E7%A0%B4.mp3"`
  - then play the same song from the desktop client against `122.51.73.110` and capture a fresh `打印日志.txt`
- If a follow-up session needs backend context, start from commit `c442e92` in `/home/shen/microservice-deploy` and confirm the remote `/root/CloudMusic` deploy has finished before doing new runtime diagnosis.
