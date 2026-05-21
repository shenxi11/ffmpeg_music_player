Goal

Stabilize the remote indexed MP3 seek path after repeated large backward jumps by preventing stale tail `Range` requests from surviving into a new seek generation, tightening seek success acceptance, and flushing decoder demux state for large reverse local-proxy seeks.

Completed

- Claimed `playback-and-audio`, implemented the stale-range isolation pass, then returned the module to `handoff` with the task in `review`.
- Updated [AudioCacheManager.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.h) and [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp):
  - proxy handles now track whether the first request of the active seek generation is still pending or already satisfied
  - first-request logic no longer lets a far-late `Range` pass as `pass-through-newer-range`; it rewrites stale tail requests back to the indexed hint window
  - proxy responses now mark the seek hint as satisfied only after real payload bytes are forwarded, including chunk-hit fast paths
  - new logs expose `active_seek_first_request_pending`, `active_seek_first_request_satisfied`, and `stale_tail_range_blocked`
- Updated [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp):
  - indexed seek success now requires proxy-hint satisfaction in addition to timestamp tolerance
  - when a first decoded packet arrives before the proxy hint is satisfied, the session waits briefly and then uses the single indexed corrective reseek budget instead of accepting a false success
  - added `seek_accept_proxy_hint_satisfied` logging on seek acceptance and correction paths
- Updated [AudioDecoder.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioDecoder.h) and [AudioDecoder.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioDecoder.cpp):
  - added large-reverse local-proxy seek detection
  - large reverse proxy seeks now use slightly wider seek/read deadlines
  - successful local-proxy non-HLS seeks now call `avformat_flush()` before `avcodec_flush_buffers()`
- Updated [.ai/ownership.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml) and [.ai/tasks.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml) for the claim and handoff lifecycle.
- Verified build success with:
  - `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`

Changed Scope

- [AudioCacheManager.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.h)
- [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp)
- [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp)
- [AudioDecoder.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioDecoder.h)
- [AudioDecoder.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioDecoder.cpp)
- [.ai/ownership.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml)
- [.ai/tasks.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml)

Open Work

- Run the desktop client against `122.51.73.110` and reproduce the same “连续往后拖，再大幅回拖到前几十秒” sequence.
- Confirm the new proxy logs show:
  - `stale_tail_range_blocked=true` when an old tail range tries to cross into a new backward seek generation
  - the first request of the new generation either rewrites to the seek hint or lands within one chunk of it
  - `seek_accept_proxy_hint_satisfied=true` before `seek_accept=true`
- Verify that the recovered audio content, progress bar, and lyrics stay aligned after the big backward jump.
- If remote runtime still plays the wrong segment after these changes, inspect whether stale decoded frames with the current generation still survive after `avformat_flush()` and whether the active seek generation should explicitly invalidate more decoder-side state.

Risks

- The new first-request lock is stricter than the previous sticky-hint path; if FFmpeg legitimately issues an unusual first `Range` shape for some formats, the proxy may now rewrite it more aggressively.
- `AudioSession` now refuses to accept an indexed seek until the proxy has confirmed hint satisfaction. If the proxy never reaches that state because of a new edge case, the user will see an explicit failed seek instead of wrong-position audio.
- `avformat_flush()` is applied for local-proxy non-HLS seeks after a successful seek. This is intentionally narrower than a full reopen path, but runtime verification is still required on the remote server.

Next Entry Point

- Start with [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp) around `handleProxyRequest()`, `markSatisfiedSeekHintForHandle()`, and the new `Sticky seek hint request` logs while reproducing the remote backward seek case.
- Then compare [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp) `handleSeekLatencyOnDecodedTimestamp()` logs to confirm the seek no longer succeeds before `seek_accept_proxy_hint_satisfied=true`.
- If a further fix is needed, continue in [AudioDecoder.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioDecoder.cpp) around the seek success block and first-packet read timeout tuning, not by restoring the old reopen-on-seek path.
