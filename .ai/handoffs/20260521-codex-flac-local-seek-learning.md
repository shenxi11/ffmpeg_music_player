Goal
Continue the VLC 8/10 parity optimization by reducing repeated remote FLAC seek fallback cost without changing server code.

Completed
Added client-side local seek learning for tracks whose server seek-index endpoint returns unsupported or empty results. Accepted seek landings are persisted in `AudioCacheManager::TrackMeta::localSeekPoints` and reused as a non-exact local seek index on later seeks. The implementation keeps the existing FLAC approximate-anchor backoff path, so learned bytes represent the estimated target byte while `AudioAvioBridge` still applies its range-anchor backoff before opening the HTTP Range.

Changed Scope
Primary changes are in `src/audio/AudioCacheManager*`, `src/audio/AudioSession*`, and `src/audio/stream/AudioPrefetchStream.cpp`. The stream gate changes lower FLAC startup/seek read-gate requirements when measured throughput is healthy, and the session flow-control changes pause/resume FLAC decode by PCM buffered milliseconds instead of only percentage fill.

Validation
`cmake --build --preset vs2022-x64-debug --config Debug` passed. The build still reports the existing `VCINSTALLDIR is not set` warning during `windeployqt`, but the executable and plugins were produced successfully.

Runtime Verification Needed
Run the client and test the same remote FLAC track with multiple seeks. Inspect only `E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt`. Expected signs are `localPoints=` increasing after accepted FLAC seeks, later seeks showing `confidenceReason="local_seek_learning_index"` or seek-observation feedback instead of only `duration_ratio_fallback`, and FLAC `buffer_resume_ms` staying bounded without wrong-position playback.

Risks
The first seek on a FLAC track without a server seek index still starts from duration-ratio estimation. Local learning improves repeated seeks on the same track after valid observations, but it is not a replacement for a real FLAC SEEKTABLE or server-generated precise seek index.

Next Entry Point
If FLAC still seeks too slowly after repeated observations, start at `AudioCacheManager::noteSeekIndexObservation()`, `AudioCacheManager::estimateCenterByteFromSeekIndex()`, and `AudioAvioBridge::prepareSeek()`. Check whether the learned point spacing is too sparse or whether the fixed FLAC backoff size needs to be adaptive.
