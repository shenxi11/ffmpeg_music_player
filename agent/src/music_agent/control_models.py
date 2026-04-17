from __future__ import annotations

"""
模块名: control_models
功能概述: 定义本地控制型 Agent 的核心结构化模型，包括意图、宿主快照、能力快照与执行策略结果。
对外接口: ControlIntent、HostContextSnapshot、CapabilitySnapshot、RiskPolicyDecision、ExecutionTemplate
依赖关系: pydantic、dataclasses
输入输出: 输入为模型输出与宿主快照原始数据；输出为经过约束的结构化对象
异常与错误: 校验失败时抛出 pydantic ValidationError，由上层运行时统一兜底
维护说明: 新增控制意图时需同步更新 ALLOWED_CONTROL_INTENTS 与本地模型 prompt
"""

from dataclasses import dataclass, field
from typing import Any, Literal

from pydantic import BaseModel, ConfigDict, Field


ALLOWED_CONTROL_INTENTS = (
    "pause_playback",
    "resume_playback",
    "stop_playback",
    "play_next",
    "play_previous",
    "set_volume",
    "set_play_mode",
    "query_local_tracks",
    "play_track_by_query",
    "inspect_playlist_then_play_index",
    "query_current_playback",
    "create_playlist_with_confirmation",
    "add_tracks_to_playlist_with_confirmation",
    "restricted",
)


class ControlIntent(BaseModel):
    model_config = ConfigDict(populate_by_name=True, extra="ignore")

    route: Literal["execute", "template", "restricted"]
    intent: Literal[
        "pause_playback",
        "resume_playback",
        "stop_playback",
        "play_next",
        "play_previous",
        "set_volume",
        "set_play_mode",
        "query_local_tracks",
        "play_track_by_query",
        "inspect_playlist_then_play_index",
        "query_current_playback",
        "create_playlist_with_confirmation",
        "add_tracks_to_playlist_with_confirmation",
        "restricted",
    ]
    arguments: dict[str, Any] = Field(default_factory=dict)
    entity_refs: list[str] = Field(default_factory=list, alias="entityRefs")
    confirmation: bool = False
    reason_code: str = Field(default="", alias="reasonCode")


class HostContextSnapshot(BaseModel):
    model_config = ConfigDict(populate_by_name=True, extra="ignore")

    current_page: str = Field(default="", alias="currentPage")
    offline_mode: bool = Field(default=False, alias="offlineMode")
    logged_in: bool = Field(default=False, alias="loggedIn")
    current_track: dict[str, Any] = Field(default_factory=dict, alias="currentTrack")
    selected_playlist: dict[str, Any] = Field(default_factory=dict, alias="selectedPlaylist")
    selected_track_ids: list[str] = Field(default_factory=list, alias="selectedTrackIds")
    queue_summary: dict[str, Any] = Field(default_factory=dict, alias="queueSummary")


class CapabilitySnapshot(BaseModel):
    model_config = ConfigDict(populate_by_name=True, extra="ignore")

    catalog_version: str = Field(default="", alias="catalogVersion")
    tools: list[dict[str, Any]] = Field(default_factory=list)


class RiskPolicyDecision(BaseModel):
    require_approval: bool = Field(default=False, alias="requireApproval")
    allowed: bool = True
    risk_level: Literal["low", "medium", "high"] = Field(default="low", alias="riskLevel")
    reason: str = ""


@dataclass
class ExecutionTemplate:
    name: str
    summary: str
    risk_level: Literal["low", "medium", "high"] = "low"
    requires_approval: bool = False
    tool_chain: list[str] = field(default_factory=list)
