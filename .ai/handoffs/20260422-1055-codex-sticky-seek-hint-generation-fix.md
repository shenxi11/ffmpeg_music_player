Goal

Stabilize large-jump seek for server-local online MP3/AAC/M4A by replacing the one-shot proxy seek hint with a seek-generation-scoped sticky hint, preventing the proxy from falling back to older byte ranges during the same seek cycle, and refusing indexed-seek resumes that still land far away from the requested timestamp.

Completed

- Updated [AudioCacheManager.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.h) and [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp) so proxy handles now track one active sticky seek hint per seek generation instead of a one-shot pending offset.
- Added `activateSeekHintForPlaybackUrl()` and `clearSeekHintForPlaybackUrl()`; sticky hints now record generation, target time, byte offset, expiry, and apply to every proxy request in the same seek cycle until they are explicitly cleared.
- Changed proxy request rewriting so requests without a Range or with an older Range start are lifted to the indexed byte offset, while newer Range requests are allowed through without collapsing back to the old lower offset.
- Updated [AudioDecoder.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioDecoder.h) and [AudioDecoder.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioDecoder.cpp) so `seekTo()` returns the assigned seek generation, allowing the session layer to bind proxy hints to the same generation the decoder will accept.
- Updated [AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h) and [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp) so all local-proxy seek dispatches go through `dispatchDecoderSeekWithOptionalHint()`, including initial seeks, corrective reseeks, retry reseeks, and owner-takeover resume seeks.
- Tightened indexed seek acceptance in `handleSeekLatencyOnDecodedTimestamp()`: after the single indexed corrective reseek, drifts larger than 15000 ms now fail explicitly instead of being accepted as a false-success nearest keyframe resume.
- Adjusted [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp) seek dispatch logging to reflect that the seek hint strategy is now `sticky_generation` and is armed by the session/decoder path rather than by a one-shot service-layer prime call.
- Verified build success with `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

Changed Scope

- [AudioCacheManager.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.h)
- [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp)
- [AudioDecoder.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioDecoder.h)
- [AudioDecoder.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioDecoder.cpp)
- [AudioService.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioService.cpp)
- [AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h)
- [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp)
- [ownership.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml)
- [tasks.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml)

Open Work

- Run the desktop client against `122.51.73.110` and capture a fresh large-jump seek log for the same remote MP3 scenario that previously landed around `118xxx ms` after a target near `224000 ms`.
- Confirm that one seek cycle now keeps `proxy_effective_range_start` at or above the indexed byte offset, and no later proxy request in the same cycle falls back to an older lower offset.
- Confirm that successful seeks now log `seek_accept=true` near the requested target, while catastrophic drift logs `seek_accept=false seek_accept_reason=failed_large_drift` and no longer resumes wrong-position audio.
- If remote runtime still shows long stalls, inspect whether the decoder now repeatedly receives pass-through newer ranges or whether FFmpeg is still anchoring to an older internal timeline despite the sticky proxy rewrite.

Risks

- The sticky hint now persists for the whole seek generation, so any bug in seek-generation bookkeeping could over-constrain proxy reads until the hint is cleared; runtime logs should confirm the hint is cleared on success, timeout, stop, and replacement.
- The new large-drift failure path pauses/reset buffers instead of accepting a bad resume. If this path triggers too aggressively, the user will see explicit seek failure instead of wrong-position playback, and thresholds may need one more pass.
- This pass does not change service-side stream delivery or FFmpeg demux behavior; it only removes the current client-side race where the proxy hint was consumed too early and later requests fell back to an old byte range.

Next Entry Point

- Start with [AudioCacheManager.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioCacheManager.cpp) around `activateSeekHintForPlaybackUrl()` and `handleProxyRequest()` while replaying a remote large jump. The key validation point is whether `seek_generation`, `proxy_effective_range_start`, and `seek_hint_override_reason` stay consistent across all requests for one seek.
- Then inspect [AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp) `dispatchDecoderSeekWithOptionalHint()` and `handleSeekLatencyOnDecodedTimestamp()` to verify the decoder generation used for stale-frame filtering matches the generation carried by the sticky proxy hint.
- If the remote log still shows a failed jump after the sticky-hint change, compare `seek_accept_reason`, `first_decoded_ts`, and any repeated decoder retries before changing tolerances again.
