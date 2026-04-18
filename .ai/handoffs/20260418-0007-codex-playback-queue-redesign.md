## Goal
- Rewrite the expanded-playback right-side playlist drawer based on the approved draft in `E:\FFmpeg_whisper\提示词\播放历史列表设计稿.txt`.
- Keep the work inside `playback-and-audio`, preserve existing queue actions, and avoid inventing new backend behavior for draft-only actions.
- Keep playlist/comments on the shared right overlay host, but allow the playlist drawer to use a narrower width than comments.

## Completed
- Rebuilt `qml/components/playback/PlaylistHistory.qml` into a draft-aligned white queue drawer with a new header, tighter typography, hover states, current-track badge, empty state, and a popover-style more menu.
- Preserved existing queue actions:
- play another item
- pause/resume the current item
- remove one item
- clear the queue
- Rendered unsupported draft actions as disabled menu shells instead of fake implementations.
- Updated `PlayWidget` overlay geometry so `RightOverlayPage::Playlist` uses a narrower drawer width while `RightOverlayPage::Comments` keeps the existing wider layout.
- Updated `.ai/ownership.yaml` and `.ai/tasks.yaml` to record the playback queue redesign session.
- Verified:
- `E:\CMake\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`
- short launch smoke test of `E:\FFmpeg_whisper\ffmpeg_music_player_build\Debug\ffmpeg_music_player.exe` for 6 seconds without immediate exit/crash

## Changed Scope
- `E:\FFmpeg_whisper\ffmpeg_music_player\qml\components\playback\PlaylistHistory.qml`
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\ui\widgets\playback\play_widget.cpp`
- `E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml`
- `E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml`

## Open Work
- Manual visual verification is still needed for:
- expanded playback + open playlist drawer
- hover states and more-menu placement near the lower part of the list
- switching playlist <-> comments repeatedly and confirming the width swap feels correct
- checking whether the disabled shell actions need different wording or should be hidden instead
- If the queue drawer should match the draft even more closely, continue inside `PlaylistHistory.qml`; the data bridge and queue behavior do not need further expansion for this version.

## Risks
- The new queue drawer uses a custom top-level menu overlay inside the QML root. It should avoid `ListView` clipping, but its final on-screen alignment still needs visual confirmation in the running app.
- The draft includes business concepts like paid upsell, similar-song play, and add-to-playlist flows; these are intentionally not wired, so product review may ask for either better placeholder wording or follow-up implementation.

## Next Entry Point
- Queue UI:
- `E:\FFmpeg_whisper\ffmpeg_music_player\qml\components\playback\PlaylistHistory.qml`
- Shared overlay width policy:
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\ui\widgets\playback\play_widget.cpp`
