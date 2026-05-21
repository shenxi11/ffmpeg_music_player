Goal

Implement the next seek-latency reduction pass for desktop online local music by replacing query-string proxy seek overrides with opaque localhost handles, routing indexed MP3/AAC large seeks through a reopen-on-seek path, and tightening FFmpeg local-proxy HTTP behavior before runtime log validation.

Completed

- Updated [AudioCacheManager.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.h) and [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp) to replace `audio?src=...` playback URLs with opaque `/audio/<handle>.<ext>` localhost proxy URLs.
- Added per-handle pending seek metadata so the first proxy request after reopen can override the initial upstream `Range` to the indexed byte offset plus a bounded 256 KB window.
- Kept legacy proxy-source resolution as fallback so old query-string style URLs can still be decoded if they appear.
- Updated [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp) so indexed local MP3/AAC/M4A seeks can reopen on a fresh `AudioSession` instead of relying on in-place FFmpeg reseek.
- Added `prepareReopenSeek()` to [AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h) and [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp) to prime seek-latency tracking, clock base, and grace-window state for reopened sessions before `play()`.
- Updated [AudioDecoder.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioDecoder.cpp) so localhost proxy HTTP opens enable `multiple_requests=1`, `request_size=262144`, and `short_seek_size=262144`.
- Verified the desktop target builds successfully with `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

Changed Scope

- [AudioCacheManager.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.h)
- [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp)
- [AudioDecoder.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioDecoder.cpp)
- [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp)
- [AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h)
- [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp)
- [ownership.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml)
- [tasks.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml)

Open Work

- Run the desktop client against the current service and capture fresh seek logs for large MP3 jumps to confirm the first packet now follows the reopen handle request instead of stale buffered data.
- Check whether runtime logs now show `proxy_range_override_applied=true` together with a matching `Reopen seek handle request` for every indexed reopen-seek path.
- Measure `seek_to_first_decoded_ms`, `buffer_resume_ms`, and `corrective_reseek_count` on cold and warm cache paths to decide whether more decoder tolerance or proxy-window tuning is still needed.
- If drift remains, inspect whether FFmpeg still consumes stale packets before reconnect and decide whether the reopen path should force a stronger decoder/session reset.

Risks

- The new reopen path only activates for indexed localhost MP3/AAC/M4A sources; unsupported formats still follow the previous in-session seek logic.
- FFmpeg HTTP dictionary options are runtime-tolerant, but actual effect still needs confirmation from logs because option support depends on the bundled FFmpeg build.
- Handle TTL is currently 30 seconds and pending seek override is one-shot; very delayed first requests or unusual reconnect timing could miss the override and fall back to ordinary proxy behavior.
- This session only completed local compilation, not remote playback verification.

Next Entry Point

- Start from [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp) `seekTo()` reopen branch and [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp) `handleProxyRequest()` logging around `Reopen seek handle request`.
- Reproduce large-jump seeks with the updated desktop build, compare against [打印日志.txt](E:\FFmpeg_whisper\ffmpeg_music_player\打印日志.txt), and only then decide whether further work belongs in `AudioDecoder::decode()` timeout policy or in the proxy request/window strategy.
