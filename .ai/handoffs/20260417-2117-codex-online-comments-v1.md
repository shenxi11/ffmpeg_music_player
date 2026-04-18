## Goal
- Implement online music comments v1 in the Qt client from the current playback area only.
- Follow the server protocol in `说明文档/接口与协议/在线音乐评论接口与客户端实现说明.md`.
- Preserve multi-AI workflow state in `.ai/*` before and after editing.

## Completed
- Claimed and later handed off `shell-core`, `library-and-search`, `playback-and-audio`, and `plugins-and-build` for the comments feature.
- Added comments API methods and result signals in [src/network/httprequest_v2.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/network/httprequest_v2.h) and [src/network/httprequest_v2.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/network/httprequest_v2.cpp).
- Added comment request façade and result forwarding in [src/viewmodels/MainShellViewModel.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/MainShellViewModel.h), [src/viewmodels/MainShellViewModel.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/MainShellViewModel.cpp), and [src/viewmodels/MainShellViewModel.connections.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/MainShellViewModel.connections.cpp).
- Propagated original online `music_path` through online list resolution in [src/viewmodels/OnlineMusicListViewModel.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/OnlineMusicListViewModel.h), [src/viewmodels/OnlineMusicListViewModel.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/OnlineMusicListViewModel.cpp), [src/ui/widgets/library/music_list_widget_net.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/library/music_list_widget_net.h), and [src/ui/widgets/library/music_list_widget_net.connections.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/library/music_list_widget_net.connections.cpp).
- Extended recommendation playback to carry raw `music_path` into the playback area in [src/ui/widgets/library/recommend_music_widget.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/library/recommend_music_widget.h), [qml/components/library/RecommendMusicWidget.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/library/RecommendMusicWidget.qml), [src/app/main_widget.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.h), and [src/app/main_widget.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.cpp).
- Added playback comments drawer bridge and UI in [src/ui/qml_bridge/playback/comment_panel_qml.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/qml_bridge/playback/comment_panel_qml.h) and [qml/components/playback/CommentPanel.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/CommentPanel.qml), and registered it in [qml.qrc](E:/FFmpeg_whisper/ffmpeg_music_player/qml.qrc).
- Added comment toggle button and comment/playlist checked-state APIs in [src/ui/qml_bridge/playback/process_slider_qml.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/qml_bridge/playback/process_slider_qml.h) and [qml/components/playback/ProcessSlider.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/ProcessSlider.qml).
- Wired playback comment interactions, login redirect, reply/delete flow, and comment-context resolution in [src/ui/widgets/playback/play_widget.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/playback/play_widget.h), [src/ui/widgets/playback/play_widget.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/playback/play_widget.cpp), [src/ui/widgets/playback/play_widget.controls_connections.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/playback/play_widget.controls_connections.cpp), and [src/ui/widgets/playback/play_widget.viewmodel_connections.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/playback/play_widget.viewmodel_connections.cpp).
- Added comment context application/clear logic and login handoff in [src/app/main_widget.playback_list_handlers.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.playback_list_handlers.cpp), [src/app/main_widget.library_connections.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.library_connections.cpp), and [src/app/main_widget.playback_connections.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.playback_connections.cpp).

## Changed Scope
- Intended scope remained within comments v1 and required path/context propagation.
- One additional shared-surface dependency was introduced: `qml.qrc` registration for the new drawer QML.
- Existing unrelated dirty files were already present in the worktree and were not intentionally modified for comments:
  - `qml/components/settings/Settings.qml`
  - `src/common/settings_manager.*`
  - `src/ui/widgets/settings/*`
  - `src/viewmodels/SettingsViewModel.*`

## Open Work
- Run a full link-complete build after closing any running `ffmpeg_music_player.exe`. Current build reached compile/link but failed at final write with `LNK1168`.
- Runtime-check the new drawer UI:
  - online list song
  - recommendation song
  - local song disabled state
  - unauthenticated send/delete login redirect
  - root comments load
  - replies expand/load
  - send root comment
  - send reply
  - delete own comment
- Review whether history/playlist online playback paths still need extra raw `music_path` propagation beyond the current URL fallback.
- Inspect QML behavior for `CommentPanel.qml` if any runtime warnings appear around `qlonglong` signal parameters or `Repeater/ListView` model data access.

## Risks
- The worktree already contains unrelated unsubmitted settings/theme changes from another session; avoid bundling them into the comments feature commit without explicit separation.
- Comment eligibility currently prefers explicit cached `music_path`, then falls back to extracting a relative path from the current service `uploads/` URL. This is deterministic, but still weaker than explicit origin tracking for every playback source.
- History and playlist online playback may still depend on the fallback extraction path when raw `music_path` is not readily available.
- No full runtime verification was completed in this session because the existing executable stayed locked during the final link step.

## Next Entry Point
- Start from [src/ui/widgets/playback/play_widget.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/playback/play_widget.cpp) and [qml/components/playback/CommentPanel.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/CommentPanel.qml) to validate drawer behavior.
- Re-run:
  - `E:\CMake\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`
- If link still fails, check whether `ffmpeg_music_player.exe` is still running before touching code.
- After validation, update `.ai/tasks.yaml` from `review` to `done` or back to `doing` with concrete notes.
