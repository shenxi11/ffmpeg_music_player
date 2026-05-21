## Goal
Finish the public-remote local-audio recovery plan end to end: move service-local public playback off the synchronous playback-info/HLS path, reduce repeated seek recovery loops in the desktop client, land the backend media changes in the VM development repo, and formally roll the backend to the remote server.

## Completed
- Updated [AudioService.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioService.cpp), [AudioService.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioService.h), [AudioSession.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioSession.cpp), [AudioSession.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioSession.h), [AudioDecoder.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioDecoder.cpp), and [AudioDecoder.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioDecoder.h).
- Desktop behavior now prefers direct `/uploads` HTTP for public service-local playback, keeps playback-info as async/cache-only warmup, and limits one seek generation to one route-recovery attempt before terminal failure.
- Local desktop build passed with `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.
- Backend code changes were made only in `shen@192.168.1.208:/home/shen/microservice-deploy` and pushed as commit `c63cb11 fix(media): unblock direct uploads path and async hls readiness`.
- Remote formal deployment on `root@122.51.73.110:/root/CloudMusic` succeeded through the Docker stack using `docker compose --env-file .env.docker up -d --build ...`.
- Remote public validation passed:
  - `http://122.51.73.110:8080/health` returned `200`
  - `GET /music/local/playback-info?music_path=告白气球/告白气球.mp3` returned `200` in about `6ms`
  - `GET /uploads/告白气球/告白气球.mp3` supports `206 Partial Content` with correct `Content-Range`

## Changed Scope
- Remote deployment did not use `/root/CloudMusic/start_split.sh` as the final production path because that host is currently fronted by the Docker gateway on port `8080`.
- During rollout, it was necessary to discover and respect the real production rule: Docker formal deploys on `/root/CloudMusic` must use `--env-file .env.docker`; bare `docker compose up` falls back to `./.data/uploads` and breaks media visibility.
- A temporary host-side split startup attempt was used only for diagnosis and then superseded by the correct Docker deployment path.

## Open Work
- Run fresh desktop playback regression against the remote server and inspect [打印日志.txt](/E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt) to confirm large forward seek and large backward seek timing/position correctness on the new direct-HTTP-first route.
- Check whether the client still receives the desired public `direct_url`/`hls_manifest_url` under the real desktop request host, not only curl validation.
- If remote desktop QA still shows seek-position drift, inspect the new `SEEK_FALLBACK` / `seek_to_first_decoded_ms` / `buffer_resume_ms` logs before changing any more server code.

## Risks
- The desktop client changes are compiled but not yet validated with a fresh real playback log after this exact backend rollout.
- The remote server currently has deployment-only config edits in `/root/CloudMusic/config.yaml` and `/root/CloudMusic/configs/config.yaml` from rollout troubleshooting; do not treat those files as committed source of truth.
- `start_split.sh` on the remote host is still blocked by deployment-environment assumptions unrelated to the Docker formal path, including nginx binary path defaults and migration history drift in that non-Docker route.

## Next Entry Point
- Start with [打印日志.txt](/E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt) after one new remote playback session.
- For client route logic, inspect [AudioService.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioService.cpp) and [AudioSession.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioSession.cpp).
- For backend rollback or redeploy, use `ssh cloudmusic-vm` for source changes in `/home/shen/microservice-deploy`, and use `ssh -i $HOME/.ssh/cloudmusic.pem root@122.51.73.110` with `docker compose --env-file .env.docker ...` in `/root/CloudMusic` for formal remote rollout.
