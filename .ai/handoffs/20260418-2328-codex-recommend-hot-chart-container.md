## Goal
Implement the client-side hot chart inside the existing recommend tab, using the approved “recommend tab hosts a discovery container” approach instead of adding a new first-level sidebar entry.

## Completed
- Added hot-chart request wiring in [src/network/httprequest_v2.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/network/httprequest_v2.h) and [src/network/httprequest_v2.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/network/httprequest_v2.cpp):
  - `getHotMusicChart(window, limit)`
  - `signalHotChartResult(...)`
  - parsing for `GET /music/charts/hot?window=7d|30d|all&limit=20`
- Extended [src/viewmodels/MainShellViewModel.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/MainShellViewModel.h), [src/viewmodels/MainShellViewModel.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/MainShellViewModel.cpp), and [src/viewmodels/MainShellViewModel.connections.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/MainShellViewModel.connections.cpp) with:
  - `requestHotChart(...)`
  - `hotChartReady(...)`
  - `hotChartRequestFailed(...)`
- Rebuilt [src/ui/widgets/library/recommend_music_widget.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/library/recommend_music_widget.h), [src/ui/widgets/library/recommend_music_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/library/recommend_music_widget.cpp), and [qml/components/library/RecommendMusicWidget.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/library/RecommendMusicWidget.qml) so the recommend page is now a two-section discovery container:
  - `个性推荐`
  - `热歌榜`
  - hot chart hero header
  - 7d/30d/all switching
  - public hot-chart browsing without login
  - reuse of existing song action menus for play/favorite/download/playlist actions
- Updated [src/app/main_widget.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.h), [src/app/main_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.cpp), and [src/app/main_widget.library_connections.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.library_connections.cpp) so:
  - the existing recommend sidebar tab activates the discovery container instead of directly hard-requesting personalized recommendations
  - hot chart requests are forwarded to `MainShellViewModel`
  - login-state changes re-activate the current recommend section
  - generic remote-song play actions now restore comment-track context from `musicPath/music_path/path`
- Recorded the shared-surface decision in [\.ai/decisions.md](/E:/FFmpeg_whisper/ffmpeg_music_player/.ai/decisions.md).
- Validation: `cmake --build E:\\FFmpeg_whisper\\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player` passed. The build emitted the existing `VCINSTALLDIR is not set` `windeployqt` warning, but the target executable was produced successfully.

## Changed Scope
- `shell-core`
- `library-and-search`
- shared governance files under `.ai/`
- No build-graph edits were needed because the implementation stayed inside existing C++ and QML files.

## Open Work
- Manual runtime verification is still needed for:
  - recommend tab first-entry behavior when logged out
  - switching from hot chart to personalized recommendations immediately after login
  - hot-chart row hover/action-strip layout at multiple window widths
  - remote play/comment context behavior for Jamendo chart items
- The hot chart hero action is currently `播放榜首`, not true “全部播放”. If product insists on full queue play for the entire chart, a follow-up session should implement multi-item remote resolve/queue preparation instead of faking the behavior.

## Risks
- `RecommendMusicWidget.qml` is now substantially larger and more stateful. If later sessions keep adding sections to the same file, it should be split into child QML components before the page becomes hard to maintain.
- The hot chart page intentionally omits design-draft elements that the service does not provide, including trend arrows and VIP/quality tags. If product later wants those, the backend contract must be extended first.
- Build validation covered C++ compilation only. QML runtime issues that require actual interaction may still surface on first manual launch.

## Next Entry Point
- Start with [qml/components/library/RecommendMusicWidget.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/library/RecommendMusicWidget.qml).
- Then inspect:
  - [src/ui/widgets/library/recommend_music_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/library/recommend_music_widget.cpp)
  - [src/network/httprequest_v2.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/network/httprequest_v2.cpp)
  - [src/app/main_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.cpp)
  - [src/app/main_widget.library_connections.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.library_connections.cpp)
- For a manual check, launch `E:\\FFmpeg_whisper\\ffmpeg_music_player_build\\Debug\\ffmpeg_music_player.exe` and verify the recommend tab in both logged-in and logged-out states.
