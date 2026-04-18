## Goal
- Fix the expanded-playback playlist crash that occurred when clicking another queued song while the playlist contained multiple songs.
- Remove the related QML warning about `requestPaint` not being a function.
- Keep the fix inside the playlist QML and playlist bridge without changing the playback core.

## Completed
- Updated `qml/components/playback/PlaylistHistory.qml` so queue actions are deferred to the next event-loop turn through a zero-interval `Timer` instead of being emitted synchronously from the active QML click handler.
- Covered all queue actions that can mutate the list from this surface:
- play another item
- pause/resume current item
- remove item from the popover menu
- clear all
- Replaced the unstable `parent.requestPaint()` call in the cover-state `Canvas` connections with a stable `id`-based repaint call.
- Updated `src/ui/qml_bridge/playback/playlist_history_qml.h` so `loadPlaylist()` and `clearAll()` invoke the QML side with `Qt::QueuedConnection`, preventing synchronous model teardown during the originating click handler.
- Verified:
- `E:\CMake\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`
- short launch smoke test of `E:\FFmpeg_whisper\ffmpeg_music_player_build\Debug\ffmpeg_music_player.exe` for 6 seconds without immediate exit/crash

## Changed Scope
- `E:\FFmpeg_whisper\ffmpeg_music_player\qml\components\playback\PlaylistHistory.qml`
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\ui\qml_bridge\playback\playlist_history_qml.h`
- `E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml`
- `E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml`

## Open Work
- Manual regression is still needed for the exact user path:
- play two different songs so the queue has two entries
- open the expanded-playback playlist
- click the other song repeatedly in both directions
- confirm there is no Runtime Library `abort()` popup and no new `FATAL` log
- Also recheck:
- click current playing item to pause/resume
- remove item from the queue menu
- clear the queue
- If a residual crash remains, continue from `PlaylistHistory.qml` and inspect any remaining synchronous property/model mutations during item handlers.

## Risks
- The fix intentionally keeps the current whole-snapshot refresh model. If later work adds more synchronous list mutations inside row handlers, the same class of issue could reappear unless those actions are also deferred.
- The smoke test only covers startup; it does not replace manual interaction verification for the exact crash path.

## Next Entry Point
- Queue interaction timing:
- `E:\FFmpeg_whisper\ffmpeg_music_player\qml\components\playback\PlaylistHistory.qml`
- Bridge-side refresh timing:
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\ui\qml_bridge\playback\playlist_history_qml.h`
