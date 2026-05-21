Goal
Reduce the first few remote FLAC seek delays by applying VLC-style controlled byte anchoring and input-layer coalescing without changing service behavior.

Completed
Added controlled FLAC soft range-anchor trust in `src/audio/stream/AudioAvioBridge.cpp`: exact hints still pass, `duration_ratio_fallback` stays rejected, local FLAC learning feedback/approx can force a backed-off Range anchor only above configured confidence thresholds.
Updated `src/audio/AudioCacheManager.cpp` so FLAC seek observations require current hint byte/time/expiry/generation and never learn from `duration_ratio_fallback`; accurate local FLAC observations can now accumulate good samples.
Added `seekHintGeneration` in `src/audio/AudioSession.h` and `src/audio/AudioSession.cpp` so observation recording matches AudioCacheManager's seek hint generation instead of decoder generation.
Adjusted `src/audio/stream/AudioPrefetchStream.cpp` to coalesce FLAC in-flight near/backward probes in the same generation, reducing extra hard seek churn while preserving stale response rejection.
Ran `cmake --build --preset vs2022-x64-debug --config Debug --target ffmpeg_music_player`; build passed after closing the running client, with the existing `VCINSTALLDIR is not set` deploy warning.

Changed Scope
`src/audio/stream/AudioAvioBridge.cpp`
`src/audio/AudioCacheManager.cpp`
`src/audio/AudioSession.cpp`
`src/audio/AudioSession.h`
`src/audio/stream/AudioPrefetchStream.cpp`
`.ai/ownership.yaml`
`.ai/tasks.yaml`

Open Work
Run the client and validate only `E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt` with the same remote FLAC.
Expected logs: `local_flac_learning_feedback` or `local_flac_learning_approx` may show `trustedFlacAnchor=true`; `duration_ratio_fallback` must keep `allowRangeAnchor=false`.
Expected metrics: `seek_to_first_decoded_ms` and `buffer_resume_ms` should drop versus the previous 800-1600ms / 900-1800ms range, while `[FLAC_SEEK_VERIFY]` drift remains within tolerance.

Risks
If a local FLAC learning point is wrong, soft anchoring can still cause a rejected first packet; AudioSession should reject by drift and recovery should pause rather than play a wrong position.
The in-flight FLAC probe coalescing intentionally changes `replyStart` within the same generation; stale chunks are expected to be ignored by the existing generation/replyStart guard.
MP3 should remain on its existing ordinary remote fast seek path; fresh runtime logs should confirm MP3 does not enter FLAC anchor logic.

Next Entry Point
Start `E:/FFmpeg_whisper/ffmpeg_music_player_build/Debug/ffmpeg_music_player.exe`, reproduce FLAC seek tests, then inspect `打印日志.txt` for `direct_seek_hint`, `flac_anchor_byte`, `seek_coalesced_inflight`, `[FLAC_SEEK_VERIFY]`, `seek_to_first_decoded_ms`, and `buffer_resume_ms`.
