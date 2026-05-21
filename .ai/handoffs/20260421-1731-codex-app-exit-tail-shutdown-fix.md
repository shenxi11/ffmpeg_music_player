## Goal
- Fix the bug where clicking the window close button hides the UI path correctly but leaves `ffmpeg_music_player.exe` alive because shutdown work continues after `a.exec()` has already returned.

## Completed
- Kept the fix inside `src/app/main.cpp` to avoid editing the currently active `playback-and-audio` module.
- Added an explicit post-`a.exec()` runtime finalization phase that logs the shutdown stages, clears the online session state, cancels network requests, stops/reset audio through existing public APIs, unloads plugins, and waits for the global thread pool.
- Kept logger cleanup as the final observable step and added a final Windows `ExitProcess(...)` call so late function-static teardown can no longer keep the process alive invisibly after the logger is gone.
- Recorded the cross-module decision that this first fix is shell-level containment rather than a direct edit of the active audio/cache sources.
- Verified the code builds successfully in `E:/FFmpeg_whisper/ffmpeg_music_player_build` (Debug); only the pre-existing `VCINSTALLDIR is not set` deployment warning remains.

## Changed Scope
- `src/app/main.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- `.ai/decisions.md`

## Open Work
- Manually run the client and verify that closing the main window now removes the process from Task Manager in both "immediate close after startup" and "auto-login completed, then close" scenarios.
- If the process still lingers even after the shell-level forced exit path, the next session must claim `playback-and-audio` and add explicit `shutdown()` methods to the hot singletons there instead of relying on late static destructors.

## Risks
- This fix intentionally uses a Windows process-level exit after logger cleanup, so any cleanup still relying on ordinary C++ static destruction will be skipped on Windows.
- The approach is a containment fix, not a root-cause cleanup of the audio/cache singleton lifetimes.

## Next Entry Point
- Start from `src/app/main.cpp`, especially `finalizeRuntimeForProcessExit(...)` and the post-`a.exec()` shutdown block.
- If manual verification still fails, the next module to claim is `playback-and-audio`, with `src/audio/AudioPlayer.cpp`, `src/audio/AudioService.cpp`, and `src/audio/AudioCacheManager.cpp` as the first files to convert from destructor-driven cleanup to explicit shutdown methods.
