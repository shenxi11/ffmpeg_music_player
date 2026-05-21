## Goal
Write a repo-tracked client integration document for the new online hot chart so another Codex session can implement the client page without reconstructing the backend contract from the nested service repo or chat history.

## Completed
- Added [说明文档/接口与协议/在线热歌榜接口与客户端实现说明.md](/E:/FFmpeg_whisper/ffmpeg_music_player/说明文档/接口与协议/在线热歌榜接口与客户端实现说明.md) with:
  - the public `GET /music/charts/hot` contract
  - response field definitions and a sample payload
  - Redis-backed hotness semantics and client-side caveats
  - recommended Qt client integration shape across `HttpRequestV2`, `MainShellViewModel`, and a library-side page
  - field mapping guidance into the existing online-song item model
  - loading/empty/error/login states and acceptance criteria
  - a copy-paste prompt block for another AI to implement the client page
- Updated [\.ai/ownership.yaml](/E:/FFmpeg_whisper/ffmpeg_music_player/.ai/ownership.yaml) to claim `docs-and-governance` for this session while the document was being written.
- Added task [feature-client-hot-chart-integration-doc](/E:/FFmpeg_whisper/ffmpeg_music_player/.ai/tasks.yaml) in [\.ai/tasks.yaml](/E:/FFmpeg_whisper/ffmpeg_music_player/.ai/tasks.yaml).

## Changed Scope
- Only `docs-and-governance` artifacts were changed in this session:
  - `.ai/ownership.yaml`
  - `.ai/tasks.yaml`
  - `.ai/handoffs/20260418-1605-codex-client-hot-chart-doc.md`
  - `说明文档/接口与协议/在线热歌榜接口与客户端实现说明.md`
- No client runtime code or service code was changed.

## Open Work
- A future `library-and-search` session still needs to implement the actual client page, request wiring, and UI rendering described by the new document.
- If the eventual client implementation needs a new main-shell navigation entry or any `main_widget*` host change, that future session must claim the correct module and add a shared-surface decision before editing `shell-core`.

## Risks
- The document assumes the first client version will use one public online hot chart page with `7d / 30d / all` switching and a default `30d` window. If product direction changes, the implementation doc will need a small update first.
- The backend is live but the prior deployment used a hot-replaced `profile-service` binary because Docker Hub timed out during a full image rebuild. If the production container is recreated before the image is rebuilt formally, runtime availability could drift from the documented contract.

## Next Entry Point
- Start with [说明文档/接口与协议/在线热歌榜接口与客户端实现说明.md](/E:/FFmpeg_whisper/ffmpeg_music_player/说明文档/接口与协议/在线热歌榜接口与客户端实现说明.md).
- Then inspect:
  - [src/network/httprequest_v2.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/network/httprequest_v2.h)
  - [src/network/httprequest_v2.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/network/httprequest_v2.cpp)
  - [src/viewmodels/MainShellViewModel.connections.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/MainShellViewModel.connections.cpp)
  - [src/ui/widgets/library/recommend_music_widget.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/library/recommend_music_widget.h)
  - [src/ui/widgets/library/music_list_widget_net.connections.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/library/music_list_widget_net.connections.cpp)
