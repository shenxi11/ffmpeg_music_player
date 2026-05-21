## Goal
- Implement the desktop local-music seek latency optimization on the client side inside `playback-and-audio`, while following the fixed server workflow that only allows backend work in `shen@192.168.1.208:/home/shen/microservice_deploy`.

## Completed
- Updated `.ai/ownership.yaml` and `.ai/tasks.yaml` so this session took over `playback-and-audio`, then handed it off with the truthful final state.
- Completed the missing seek-index implementation in `src/audio/AudioCacheManager.h` / `src/audio/AudioCacheManager.cpp`:
  - reverse-derived `music_path` from `/uploads/<relative-path>` URLs
  - added seek-index request/caching/refresh logic
  - persisted seek-index metadata and points into `meta.json`
  - recognized indexed localhost proxy playback URLs
  - switched `prefetchForSeek()` to prefer seek-index byte offsets before falling back to linear duration ratio
- Tuned seek correction behavior in `src/audio/AudioSession.h` / `src/audio/AudioSession.cpp`:
  - indexed localhost proxy seeks now allow a larger first-packet drift window
  - corrective reseek budget is capped at one round for indexed proxy seeks
  - added seek logs for `seek_target_ms`, `seek_index_hit`, `seek_to_first_decoded_ms`, `corrective_reseek_count`, and `buffer_resume_ms`
- Relaxed local-proxy FFmpeg deadlines in `src/audio/AudioDecoder.cpp` while waiting for the first packet after seek.
- Verified the client changes with:
  - `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`

## Changed Scope
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp`
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioDecoder.cpp`
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp`
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h`
- `E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml`
- `E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml`

## Open Work
- Implement the server endpoint `GET /music/local/seek-index?music_path=...` in `shen@192.168.1.208:/home/shen/microservice_deploy` only. No backend code was edited locally because ssh to the VM still times out.
- After the backend exists, run end-to-end validation for:
  - supported local MP3/AAC/M4A seek-index response shape
  - cache hit / refresh behavior after source file changes
  - desktop runtime seek behavior and latency targets with real server-local uploads
- If runtime validation shows logs but poor seek quality, inspect the new `AudioCache` seek-index interpolation and `AudioSession` indexed tolerance values first.

## Risks
- Client seek-index fetching is now wired, but without the server endpoint the flow will stay on fallback path in runtime.
- `music_path` derivation is intentionally strict: anything outside `http/https + /uploads/ + non-traversal relative path` is rejected and will fall back silently.
- `AudioCacheManager` now persists richer JSON metadata; if old cache entries contain malformed `meta.json`, they will simply reload as best-effort defaults and refetch later.

## Next Entry Point
- Start from `E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp` to review the client contract and fallback behavior.
- Then move to `E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp` for indexed proxy retry/tolerance rules.
- For the blocked backend half, follow `.codex/skills/cloudmusic-server-workflow/SKILL.md`, reconnect to `shen@192.168.1.208`, enter `/home/shen/microservice_deploy`, implement the endpoint there, and only deploy from `/home/shen/CloudMusic` or `122.51.73.110:/root/CloudMusic`.
