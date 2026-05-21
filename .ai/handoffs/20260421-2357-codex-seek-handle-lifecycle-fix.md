Goal

Fix the desktop online-local audio regression where repeated large seek operations eventually stop playback entirely because localhost proxy handles expire, seek-index state flips off, and the proxy recursively treats `/audio/<handle>` as an upstream source.

Completed

- Updated [AudioCacheManager.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.h) and [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp) so proxy handles can be bound to active sessions and are only TTL-pruned when they have no owners.
- Added explicit proxy handle lifecycle APIs: bind handle to session, release handles for session, and reuse original-source seek-index checks without depending on the current handle URL.
- Changed proxy-source resolution so missing `/audio/<handle>` requests no longer fall back to `localhost/audio/...`; they now fail explicitly and recursive local proxy sources are blocked.
- Updated [AudioService.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.h) and [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp) to configure per-session source context, bind/release proxy handles on session lifetime, and base reopen-seek eligibility on the original source rather than on handle back-resolution.
- Updated [AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h) and [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp) so seek latency tracking uses a stable session-level `original_source + indexed_seek_capable` context instead of querying the current proxy handle URL every time.
- Verified local build success with `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

Changed Scope

- [AudioCacheManager.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.h)
- [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp)
- [AudioService.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.h)
- [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp)
- [AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h)
- [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp)
- [ownership.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml)
- [tasks.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml)

Open Work

- Run the desktop client and reproduce repeated large MP3 seeks on the same online local track.
- Confirm logs no longer show `src="http://127.0.0.1/audio/...` as the upstream source and no longer emit proxy `Connection refused` after handle expiry.
- Confirm the session log no longer flips from `seek_index_hit=true` at prefetch time to `seek_index_hit=false` inside `AudioSession::beginSeekLatencyTracking()` for the same playable track.
- Re-check whether the remaining “seek lands earlier than target” problem still needs a separate precision pass after this stability fix.

Risks

- This pass fixes the proxy handle lifetime and recursion failure path, but it does not attempt to solve MP3 VBR seek precision drift.
- Missing-handle requests now fail explicitly; if some legacy path still depends on resurrecting an expired handle instead of rebuilding a new playback source, that path will now surface an error instead of silently limping forward.
- Runtime validation is still required because this session only compiled locally and analyzed logs; it did not capture a fresh post-fix playback log yet.

Next Entry Point

- Start with [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp) `applySessionSourceContext()` and `seekTo()` to verify that every active proxy playback session binds its handle and that reopen-on-seek still triggers for indexed local MP3/AAC.
- Then inspect [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp) `resolveOriginalUrlForPlayback()` and `handleProxyRequest()` logs during runtime to confirm missing handles now produce explicit proxy errors instead of recursive localhost upstream requests.
