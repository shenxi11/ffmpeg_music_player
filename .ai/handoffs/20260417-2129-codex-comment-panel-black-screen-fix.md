## Goal
- Fix the online comments drawer showing a pure black page instead of the QML UI.
- Keep the fix limited to the playback comments drawer load path and QML bridge compatibility.

## Completed
- Confirmed the root cause from [打印日志.txt](E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt):
  - `Invalid signal parameter type: qlonglong`
  - `CommentPanelQml: root object is null`
- Replaced invalid QML signal parameter types in [CommentPanel.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/CommentPanel.qml) from `qlonglong` to `var`.
- Simplified comment/reply delegate declarations in [CommentPanel.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/CommentPanel.qml) to avoid relying on stricter `required property` parsing for this panel.
- Updated [comment_panel_qml.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/qml_bridge/playback/comment_panel_qml.h) to:
  - print QML load errors explicitly
  - bridge `QVariant` signal payloads back to `qint64` C++ signals through slots
- Rebuilt successfully:
  - `E:\CMake\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`

## Changed Scope
- Only touched playback comments drawer files plus `.ai/*` governance state.
- No service protocol changes.
- No comment business-logic changes.

## Open Work
- Runtime-verify the actual drawer UI now that it loads:
  - open comments on an online song
  - verify header, empty state, and list rendering
  - expand replies
  - check unauthenticated login redirect
- If a new runtime QML warning appears, inspect the fresh log rather than assuming rendering logic is broken.

## Risks
- The black-screen cause is fixed at the loader/signature level, but there may still be ordinary runtime layout/data issues inside the panel once it renders.
- The repository still contains unrelated unsubmitted settings-side changes from another session; avoid bundling them with comments work.

## Next Entry Point
- Start from [CommentPanel.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/CommentPanel.qml) and [comment_panel_qml.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/qml_bridge/playback/comment_panel_qml.h) if further comment-drawer issues appear.
- Reproduce by opening comments on an online track and checking the latest [打印日志.txt](E:/FFmpeg_whisper/ffmpeg_music_player/打印日志.txt) for any new QML warnings.
