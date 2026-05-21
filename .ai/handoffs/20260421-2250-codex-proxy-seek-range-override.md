## Goal

Reduce large-jump seek delay and wrong-position resume for server-local online MP3/AAC playback by making the client proxy honor seek-index byte positions instead of letting FFmpeg's first Range drift too far backward.

## Completed

- Added one-shot seek override registration in [AudioCacheManager.h](/abs/path/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioCacheManager.h) and [AudioCacheManager.cpp](/abs/path/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioCacheManager.cpp).
- `AudioService::seekTo()` now calls `AudioCacheManager::prepareSeekOverride()` with the loaded playback URL plus the original source URL before dispatching the session seek. See [AudioService.cpp](/abs/path/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioService.cpp).
- The proxy now intercepts the first open-ended seek `Range: bytes=N-` request for the local `127.0.0.1` playback URL and rewrites it to the indexed byte offset when the drift exceeds `32 KB`.
- Added proxy logging for:
  - `seek_target_ms`
  - `seek_index_byte`
  - `proxy_original_range_start`
  - `proxy_overridden_range_start`
  - `proxy_range_override_applied`
- Fixed cached chunk-hit partial responses so they only short-circuit when `TrackMeta.contentLength` is known and then return a real `Content-Range: bytes start-end/total` instead of `*/`.
- Added a narrow duplicate-seek debounce in [AudioSession.cpp](/abs/path/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioSession.cpp): external `seekTo()` calls within `150 ms` and within `300 ms` of the previous target are ignored.
- Debug build passed:
  - `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`

## Changed Scope

- `playback-and-audio`
- `docs-and-governance` only for `.ai/*` write-back and this handoff

## Open Work

- Run the desktop client against the same problematic server-local tracks and verify the previous bad cases:
  - `109000 ms`
  - `220000 ms`
  - `262000 ms`
- Confirm logs now show:
  - `proxy_range_override_applied=true` for the bad large-jump cases
  - `proxy_overridden_range_start` close to `seek_index_byte`
  - `first_decoded_ts` no longer tens of seconds earlier than `seek_target_ms`
- Check whether chunk-hit responses for M4A/AAC still need a more accurate `Content-Type`; this session left the existing `audio/mpeg` default untouched.
- If runtime logs still show large drift after one override, next step is to extend the proxy from one-shot rewrite into a stronger seek-window handoff instead of only replacing the first open-ended Range.

## Risks

- This fix assumes the problematic seek request is the first open-ended Range after the user drag; if FFmpeg changes its request shape for some files, the override may not fire.
- `prepareSeekOverride()` depends on a resolved seek index. Cold-path seeks that occur before the seek-index metadata arrives still fall back to the previous behavior.
- The duplicate-seek debounce is intentionally narrow, but if some caller legitimately emits two distinct user actions within `150 ms` and `300 ms`, the second one will be swallowed.

## Next Entry Point

- Start with [AudioCacheManager.cpp](/abs/path/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioCacheManager.cpp) around `prepareSeekOverride()`, `takeSeekOverrideByteOffset()`, and `handleProxyRequest()`.
- Then inspect [AudioSession.cpp](/abs/path/E:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioSession.cpp) `seekTo()` for duplicate dispatch suppression.
- Use [打印日志.txt](/abs/path/E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt) patterns from the previous bad runs as the comparison baseline when collecting the new runtime log.
