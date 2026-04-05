from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class ToolDefinition:
    name: str
    description: str
    required_args: tuple[str, ...]
    optional_args: tuple[str, ...]
    read_only: bool
    direct_execute: bool


class ToolRegistry:
    def __init__(self, definitions: list[ToolDefinition]) -> None:
        self._definitions = {definition.name: definition for definition in definitions}

    def get(self, name: str) -> ToolDefinition | None:
        return self._definitions.get(name)

    def names(self) -> list[str]:
        return list(self._definitions.keys())


DEFAULT_TOOL_REGISTRY = ToolRegistry(
    [
        ToolDefinition(
            name="searchTracks",
            description="根据关键词和可选歌手信息搜索歌曲。",
            required_args=tuple(),
            optional_args=("keyword", "artist", "album", "limit"),
            read_only=True,
            direct_execute=True,
        ),
        ToolDefinition(
            name="getCurrentTrack",
            description="获取当前播放歌曲。",
            required_args=tuple(),
            optional_args=tuple(),
            read_only=True,
            direct_execute=True,
        ),
        ToolDefinition(
            name="getPlaylists",
            description="获取歌单列表。",
            required_args=tuple(),
            optional_args=tuple(),
            read_only=True,
            direct_execute=True,
        ),
        ToolDefinition(
            name="getPlaylistTracks",
            description="获取指定歌单中的歌曲列表。",
            required_args=("playlistId",),
            optional_args=tuple(),
            read_only=True,
            direct_execute=True,
        ),
        ToolDefinition(
            name="getRecentTracks",
            description="获取最近播放或最近访问的歌曲列表。",
            required_args=tuple(),
            optional_args=("limit",),
            read_only=True,
            direct_execute=True,
        ),
        ToolDefinition(
            name="playTrack",
            description="播放指定歌曲。",
            required_args=("trackId",),
            optional_args=tuple(),
            read_only=False,
            direct_execute=True,
        ),
        ToolDefinition(
            name="playPlaylist",
            description="播放指定歌单。",
            required_args=("playlistId",),
            optional_args=tuple(),
            read_only=False,
            direct_execute=True,
        ),
        ToolDefinition(
            name="stopPlayback",
            description="停止当前播放。",
            required_args=tuple(),
            optional_args=tuple(),
            read_only=False,
            direct_execute=True,
        ),
        ToolDefinition(
            name="createPlaylist",
            description="创建一个新的歌单。",
            required_args=("name",),
            optional_args=tuple(),
            read_only=False,
            direct_execute=False,
        ),
        ToolDefinition(
            name="getTopPlayedTracks",
            description="获取最近常听或高频播放的歌曲列表。",
            required_args=tuple(),
            optional_args=("limit",),
            read_only=True,
            direct_execute=True,
        ),
        ToolDefinition(
            name="addTracksToPlaylist",
            description="将多首歌曲加入指定歌单。",
            required_args=("playlistId", "trackIds"),
            optional_args=tuple(),
            read_only=False,
            direct_execute=False,
        ),
    ]
)
