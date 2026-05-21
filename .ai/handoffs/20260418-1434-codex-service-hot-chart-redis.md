## Goal
Convert the nested service repo hot chart under `tmp/cloudmusic_server` from SQL-on-read aggregation into a true Redis leaderboard, while keeping the public chart endpoint shape stable and adding an admin rebuild path.

## Completed
- Reworked [tmp/cloudmusic_server/internal/chart/service/chart_service.go](/E:/FFmpeg_whisper/ffmpeg_music_player/tmp/cloudmusic_server/internal/chart/service/chart_service.go) to:
  - read chart rankings from Redis sorted sets
  - write online play counts into all-time plus daily buckets
  - hydrate metadata from Redis, catalog DB, or Jamendo
  - rebuild all-time or rolling day buckets from MySQL
- Updated [tmp/cloudmusic_server/internal/usermusic/service/usermusic_service.go](/E:/FFmpeg_whisper/ffmpeg_music_player/tmp/cloudmusic_server/internal/usermusic/service/usermusic_service.go) so successful online `AddPlayHistory` writes now best-effort push chart increments.
- Updated [tmp/cloudmusic_server/internal/chart/handler/chart_handler.go](/E:/FFmpeg_whisper/ffmpeg_music_player/tmp/cloudmusic_server/internal/chart/handler/chart_handler.go) with `POST /admin/charts/hot/rebuild` and 503 handling when Redis is unavailable.
- Updated startup wiring in [tmp/cloudmusic_server/cmd/profile/main.go](/E:/FFmpeg_whisper/ffmpeg_music_player/tmp/cloudmusic_server/cmd/profile/main.go) and [tmp/cloudmusic_server/cmd/monolith/main.go](/E:/FFmpeg_whisper/ffmpeg_music_player/tmp/cloudmusic_server/cmd/monolith/main.go).
- Updated nginx routes in [tmp/cloudmusic_server/deploy/nginx/nginx.docker.conf](/E:/FFmpeg_whisper/ffmpeg_music_player/tmp/cloudmusic_server/deploy/nginx/nginx.docker.conf) and [tmp/cloudmusic_server/deploy/nginx/nginx.split.conf](/E:/FFmpeg_whisper/ffmpeg_music_player/tmp/cloudmusic_server/deploy/nginx/nginx.split.conf).
- Updated API docs in [tmp/cloudmusic_server/docs/openapi.yaml](/E:/FFmpeg_whisper/ffmpeg_music_player/tmp/cloudmusic_server/docs/openapi.yaml).
- Added/updated tests in:
  - [tmp/cloudmusic_server/internal/chart/service/chart_service_test.go](/E:/FFmpeg_whisper/ffmpeg_music_player/tmp/cloudmusic_server/internal/chart/service/chart_service_test.go)
  - [tmp/cloudmusic_server/internal/chart/handler/chart_handler_test.go](/E:/FFmpeg_whisper/ffmpeg_music_player/tmp/cloudmusic_server/internal/chart/handler/chart_handler_test.go)
  - [tmp/cloudmusic_server/internal/usermusic/service/usermusic_service_test.go](/E:/FFmpeg_whisper/ffmpeg_music_player/tmp/cloudmusic_server/internal/usermusic/service/usermusic_service_test.go)

## Changed Scope
- Only the nested service repo `tmp/cloudmusic_server` plus the required repo-level handoff/governance files were changed in this session.
- No client business code was touched in this round.

## Open Work
- Deploy-side verification is still pending. The new Redis leaderboard path has not been pulled to `/home/shen/CloudMusic` or restarted in the formal deployment flow.
- If the service repo is going to be committed next, review the pre-existing dirty files in `tmp/cloudmusic_server` first, especially the earlier Jamendo-related edits already present before this session.
- Client integration can keep using `GET /music/charts/hot`; admin or ops can call `POST /admin/charts/hot/rebuild?window=all|7d|30d` after migration or Redis data loss.

## Risks
- `profile-service` now degrades when Redis init fails instead of failing fast; the profile APIs keep running, but chart read/rebuild endpoints will return 503 until Redis is healthy.
- The Redis leaderboard depends on play-history writes passing through `AddPlayHistory`; any bypass write path will not increment the leaderboard unless it also calls the new chart writer.
- Jamendo chart metadata still depends on virtual-path parsing plus optional Jamendo detail fetch. If Jamendo upstream is unavailable, chart items still render but may have reduced metadata.

## Next Entry Point
- First inspect [tmp/cloudmusic_server/internal/chart/service/chart_service.go](/E:/FFmpeg_whisper/ffmpeg_music_player/tmp/cloudmusic_server/internal/chart/service/chart_service.go) and [tmp/cloudmusic_server/internal/usermusic/service/usermusic_service.go](/E:/FFmpeg_whisper/ffmpeg_music_player/tmp/cloudmusic_server/internal/usermusic/service/usermusic_service.go).
- Then validate with:
  - `go test ./internal/chart/... ./internal/usermusic/service ./cmd/profile ./cmd/monolith`
- If moving toward deployment, continue in `/home/shen/microservice-deploy` first, verify Redis connectivity plus `POST /admin/charts/hot/rebuild?window=all`, then commit and let `/home/shen/CloudMusic` pull from git for formal deployment.
