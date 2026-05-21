# Goal

Align the recommend-page hot chart song rows with the existing online music list item style while keeping the current hot-chart data contract and song-action chain unchanged.

# Completed

- Updated `qml/components/library/RecommendMusicWidget.qml` hot-chart helpers to add `formatPlayCount()` and `hotChartSubtitle()`.
- Reworked the hot-chart `ListView` row layout from a text-only table row into a shared list-item structure with:
  - rank column
  - `SongCoverAction` album cover thumbnail
  - title plus heat/source subtitle
  - artist / album / duration columns
  - hover-only `SongActionStrip`
- Wired cover-click behavior to the existing playback chain:
  - current row cover toggles play/pause through `songActionRequested("toggle_current_playback", ...)`
  - non-current row cover plays through `root.playHotChartItem(model)`
  - row double-click still plays the item
- Kept hot-chart payload reuse intact through `buildHotChartPayload(model)` so favorites, downloads, playlists, comments, and source-aware path handling stay on the existing chain.
- Verified with `cmake --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

# Changed Scope

- `qml/components/library/RecommendMusicWidget.qml`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

# Open Work

- Run an in-app visual pass on the recommend tab hot chart and confirm:
  - cover thumbnails load correctly for both native-server and Jamendo items
  - hover action strip does not crowd narrow window widths
  - current-track highlight and cover play/pause overlay match playback state in real use
- If product later wants the heat metric back as a dedicated column, that should be treated as a separate UI pass; this session intentionally folded it into the song-info subtitle to match existing list-item style more closely.

# Risks

- The row now uses a two-line song-info block, so extremely narrow window widths may truncate the title/subtitle area earlier than the previous text-table row.
- Runtime behavior for mixed source items still depends on the correctness of the existing `buildHotChartPayload()` and shell-side remote path resolution; this session did not change those chains.

# Next Entry Point

- Start from `qml/components/library/RecommendMusicWidget.qml` hot-chart `ListView` header/delegate section.
- Compare the runtime look against `qml/components/library/MusicListWidgetNet.qml` if further style convergence is needed.
- The tracking task is `bugfix-recommend-hot-chart-row-style-alignment` in `.ai/tasks.yaml`, currently marked `review`.
