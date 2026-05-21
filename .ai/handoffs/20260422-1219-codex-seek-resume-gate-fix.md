## Goal

Tighten the desktop client's online seek resume state machine so remote local-upload MP3/AAC/M4A playback cannot resume before the current seek generation has produced a valid first decoded packet and enough buffered audio bytes to survive slower public-network delivery.

## Completed

- Claimed `playback-and-audio` and updated the task board for the seek-resume follow-up.
- Kept the existing sticky seek-hint path intact and limited code changes to `AudioSession`.
- Added `AudioSession::canPassActiveSeekResumeGate(...)` and routed active seek resume decisions through it.
- Changed active seek resume behavior so both decoded-write resume and buffering-exit resume now require:
  - current seek generation first packet already accepted
  - non-zero buffered bytes
  - at least `128 KB` buffered bytes for compressed online seek
  - for remote FLAC, the existing stricter bytes+fill gate remains enforced
- Added seek resume visibility fields to logs:
  - `seek_first_packet_received`
  - `seek_resume_gate_passed`
  - `seek_resume_buffered_bytes`
- Verified a Debug build passes:
  - `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`

## Changed Scope

- [src/audio/AudioSession.h](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.h)
- [src/audio/AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp)
- [.ai/ownership.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml)
- [.ai/tasks.yaml](E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml)

## Open Work

- Run the desktop client against the remote server `122.51.73.110` and capture new seek logs.
- Validate that `buffer_resume_ms` is no longer earlier than `seek_to_first_decoded_ms`.
- Confirm that the remote path no longer shows "resume immediately, then pause again into buffering" on large jumps.
- If remote seek still resumes too early or stalls, inspect whether the buffered-byte floor needs another bump or whether old buffered audio is still leaking across seek boundaries elsewhere in the pipeline.

## Risks

- The new `128 KB` compressed-audio resume floor is heuristic; it should stabilize public-network seek recovery, but it may still be too low or slightly conservative depending on bitrate and RTT.
- This pass does not change server behavior, sticky-hint generation logic, or decoder read strategy, so any remaining remote-only issues beyond early resume still need fresh runtime evidence.
- The worktree was already dirty before this pass; do not revert unrelated existing audio changes while validating this follow-up.

## Next Entry Point

- Start from [src/audio/AudioSession.cpp](E:\FFmpeg_whisper\ffmpeg_music_player\src\audio\AudioSession.cpp) around `onDecodedData`, `onBufferStatusChanged`, `shouldResumeFromDecodedWrite`, and `canPassActiveSeekResumeGate`.
- Reproduce one large jump on the same online local MP3 against both VM and remote, then compare:
  - `seek_to_first_decoded_ms`
  - `buffer_resume_ms`
  - `seek_first_packet_received`
  - `seek_resume_buffered_bytes`
  - whether a second immediate `Playback paused` still follows resume on the remote path.
