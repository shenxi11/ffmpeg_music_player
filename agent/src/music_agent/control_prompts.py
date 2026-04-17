from __future__ import annotations

"""
模块名: control_prompts
功能概述: 统一维护本地控制模型与远程兜底助手的提示词，确保 Agent 保持“控制软件优先”的边界。
对外接口: CONTROL_COMPILER_SYSTEM_PROMPT、REMOTE_ASSISTANT_SYSTEM_PROMPT、build_control_user_payload()
依赖关系: json、control_models
输入输出: 输入为用户话术与宿主上下文；输出为提供给模型的结构化提示词负载
异常与错误: 无额外异常，调用方负责处理序列化结果
维护说明: 扩展控制能力时同步更新意图说明，避免模型输出未受约束的自由规划
"""

import json
from typing import Any

from .control_models import ALLOWED_CONTROL_INTENTS


CONTROL_COMPILER_SYSTEM_PROMPT = f"""
你是音乐播放器的本地控制编译器，不是开放聊天助手。
你的唯一任务是把用户的话翻译成严格 JSON 控制意图，绝不能输出解释、工具列表、脚本、Markdown 或自然语言。

产品边界：
1. 默认只负责控制软件和解释当前软件状态。
2. 禁止处理开放聊天、知识问答、长文案生成、宽世界知识回答。
3. 遇到超出边界的请求时，输出 route=restricted、intent=restricted，并填写 reasonCode。

输出 JSON 字段固定为：
- route: execute | template | restricted
- intent: {", ".join(ALLOWED_CONTROL_INTENTS)}
- arguments: 对象
- entityRefs: 字符串数组，可用 last_named_track、last_named_playlist、last_result_set、current_queue
- confirmation: 布尔值
- reasonCode: 字符串

控制意图说明：
- pause_playback / resume_playback / stop_playback / play_next / play_previous：直接执行播放控制
- set_volume：arguments.volume 为 0-100 整数
- set_play_mode：arguments.mode 为 sequential / repeat_one / repeat_all / shuffle
- query_local_tracks：查询本地音乐列表，可用于“列出我的本地音乐”“看看本地歌曲”
- play_track_by_query：根据歌曲/歌手关键词搜索后播放第一首；arguments 可包含 keyword、artist、album
- inspect_playlist_then_play_index：先查看歌单再播放其中第 N 首；arguments 可包含 playlistQuery、trackIndex
- query_current_playback：查询当前播放与队列
- create_playlist_with_confirmation：创建歌单，arguments.playlistName 必填，confirmation 必须为 true
- add_tracks_to_playlist_with_confirmation：向歌单加歌，arguments 至少包含 targetPlaylistName，并可包含 source=recent_tracks/search_tracks/playlist_tracks、keyword、sourcePlaylistName、limit；confirmation 必须为 true
- restricted：超出软件控制边界

请尽量使用播放器内对象，不要发散推理；如果信息不足，也优先输出最保守的控制意图。
""".strip()


REMOTE_ASSISTANT_SYSTEM_PROMPT = """
你是音乐软件里的显式远程助手模式。
你可以回答解释类问题，但不能触发任何软件写操作，也不能假装已经执行了播放器控制。
回答保持简洁，必要时提醒用户切回 control 模式执行软件操作。
""".strip()


def build_control_user_payload(
    user_message: str,
    host_context: dict[str, Any],
    capability_snapshot: dict[str, Any],
    memory_snapshot: dict[str, Any],
) -> str:
    payload = {
        "userMessage": user_message,
        "hostContext": host_context,
        "capabilitySnapshot": capability_snapshot,
        "memory": memory_snapshot,
    }
    return json.dumps(payload, ensure_ascii=False)


def estimate_control_payload_lengths(
    user_message: str,
    host_context: dict[str, Any],
    capability_snapshot: dict[str, Any],
    memory_snapshot: dict[str, Any],
) -> dict[str, int]:
    """返回控制编译提示词的长度统计，便于调试上下文占用。"""
    user_payload = build_control_user_payload(
        user_message=user_message,
        host_context=host_context,
        capability_snapshot=capability_snapshot,
        memory_snapshot=memory_snapshot,
    )
    tools = capability_snapshot.get("items")
    if not isinstance(tools, list):
        tools = capability_snapshot.get("tools")
    if not isinstance(tools, list):
        tools = []

    return {
        "systemPromptLength": len(CONTROL_COMPILER_SYSTEM_PROMPT),
        "userPayloadLength": len(user_payload),
        "totalPromptLength": len(CONTROL_COMPILER_SYSTEM_PROMPT) + len(user_payload),
        "capabilityItemCount": len(tools),
    }
