Goal
- Tighten large remote seek latency and landing accuracy for server-local online music, then restore the remote CloudMusic formal environment after an interrupted deployment left the stack down.

Completed
- Kept backend code changes and git commit in `shen@192.168.1.208:/home/shen/microservice-deploy`.
- Added and pushed `bd645af fix(media): densify local seek index points`, changing seek-index reduction from `1500ms` to `250ms` and bumping the version salt from `csv-v2` to `csv-v3`.
- Verified the VM service change with `go test ./internal/media/handler -run SeekIndex -count=1`.
- Tightened desktop client seek behavior in `src/audio/`:
- `AudioCacheManager` chunk size reduced from `512KB` to `128KB`.
- `AudioCacheManager` indexed seek prefetch window reduced to center plus +/-1 chunk.
- `AudioSession` indexed seek tolerance reduced from `6000ms` to `450ms`, with a single corrective reseek budget and shorter indexed hard timeout.
- Verified local client build with `cmake --build E:/FFmpeg_whisper/ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.
- Restored `root@122.51.73.110:/root/CloudMusic` after the interrupted `docker.sh restart` left the CloudMusic stack down.
- Rebuilt a hotfix `cloudmusic:local` image by copying the VM-built `media_server` into the existing image base, then started the stack with `./scripts/docker.sh up --no-build`.
- Verified remote formal environment:
- `GET http://122.51.73.110:8080/health`
- `GET http://122.51.73.110:8080/client/ping`
- `GET /music/local/seek-index?music_path=...` now returns `supported=true` with `csv-v3` and dense points.

Changed Scope
- Local repo:
- `src/audio/AudioCacheManager.h`
- `src/audio/AudioCacheManager.cpp`
- `src/audio/AudioSession.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- Service repo on VM:
- `/home/shen/microservice-deploy/internal/media/handler/seek_index.go`

Open Work
- Do real desktop runtime seek validation against `122.51.73.110:8080` using server-local MP3 tracks.
- Measure whether `seek_to_first_decoded_ms` and `buffer_resume_ms` are now acceptable under cold and warm cache.
- If landing accuracy is still visibly off, the next step is not another big timeout bump. It should be sparse range extent caching or seek-side audio trimming, not relaxing tolerance again.
- The local Qt repo changes are still uncommitted in this Windows checkout.

Risks
- Remote formal recovery used a hotfix image path that reused the existing `cloudmusic:local` base and replaced only `/app/media_server`. This is correct for the current change, but a future broader backend change must go back to a full image rebuild path.
- The seek-index response currently returns mojibake for the sample Chinese `music_path` on the remote curl path shown in logs; functional behavior is correct, but display encoding in shell output is noisy.
- The repo worktree remains globally dirty in many unrelated files; do not treat that as this session's write scope.

Next Entry Point
- Start from `src/audio/AudioCacheManager.cpp` and `src/audio/AudioSession.cpp` if the next task is further seek tuning.
- Start from `root@122.51.73.110:/root/CloudMusic` and validate `docker compose ps`, `/health`, and `/music/local/seek-index` if the next task is deployment confirmation.
- Start from `shen@192.168.1.208:/home/shen/microservice-deploy/internal/media/handler/seek_index.go` if the next task is to further tune seek-index density or caching semantics.
