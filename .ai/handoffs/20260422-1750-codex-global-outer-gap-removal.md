Goal
- Remove the visible outer gaps between the main shell blocks, page hosts, and docked playback surfaces so the runtime UI attaches directly to its host bounds instead of floating with white margins.

Completed
- Tightened the shell geometry in [src/app/main_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.cpp):
  - removed the window-level outer margins around the sidebar and topbar
  - removed the sidebar-to-content shell gap
  - removed the vertical gaps between topbar, content host, and collapsed playback dock
  - aligned the right-side popup host to the content column edge
- Tightened docked playback host geometry in [src/ui/widgets/playback/play_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/playback/play_widget.cpp) and [qml/components/playback/ProcessSlider.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/ProcessSlider.qml):
  - removed collapsed playback floating margins
  - removed right overlay host outer margin
  - reduced only the playback bar's outer shell padding, while keeping internal control readability
- Removed outer page-host margins from the main runtime QML pages:
  - [qml/components/library/RecommendMusicWidget.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/library/RecommendMusicWidget.qml)
  - [qml/components/library/LocalAndDownloadWidget.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/library/LocalAndDownloadWidget.qml)
  - [qml/components/playback/PlayHistoryList.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/PlayHistoryList.qml)
  - [qml/components/playback/FavoriteMusicList.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/FavoriteMusicList.qml)
  - [qml/components/library/PlaylistWidget.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/library/PlaylistWidget.qml)
  - [qml/components/playback/CommentPanel.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/CommentPanel.qml)
  - [qml/components/settings/Settings.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/settings/Settings.qml)
- Removed the video page host's outer layout margin in [src/ui/widgets/video/video_player_page.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/video/video_player_page.cpp).
- Validation completed:
  - `qmllint qml/components/library/RecommendMusicWidget.qml qml/components/library/LocalAndDownloadWidget.qml qml/components/playback/PlayHistoryList.qml qml/components/playback/FavoriteMusicList.qml qml/components/playback/CommentPanel.qml qml/components/settings/Settings.qml qml/components/playback/ProcessSlider.qml qml/components/library/PlaylistWidget.qml`
  - `E:\cmake\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`

Changed Scope
- Edited only shell/layout host files and page-root containers.
- Did not touch `src/audio/*`.
- Did not flatten internal row spacing inside list items, button groups, or card content; this pass is outer-gap removal only.

Open Work
- Run the client and visually verify:
  - sidebar/topbar/content/dock now attach without white seams
  - settings page and comment page do not feel overly cramped after becoming full-bleed
  - right-side playlist/comment overlays still align correctly in both collapsed and expanded player flows
- If the user now wants the internal spacing also compressed, that needs a separate pass through each page's internal `spacing`, `anchors.margins`, and per-card padding.

Risks
- Some pages, especially Settings and CommentPanel, now use full host width; if they previously depended on centered max-width presentation for readability, a follow-up may be needed to selectively restore internal section padding without reintroducing shell-level gaps.
- Recommend and playlist pages still retain many internal card/header paddings by design; if the user's expectation is literally "everything touches everything," this pass will not be sufficient.

Next Entry Point
- Start with [src/app/main_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.cpp) for shell host geometry.
- Then inspect [qml/components/settings/Settings.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/settings/Settings.qml) and [qml/components/playback/CommentPanel.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/CommentPanel.qml) if further width/readability tuning is needed.
