from __future__ import annotations

"""
模块名: policy_hooks
功能概述: 提供运行时可复用的策略守卫与行为注入点，先把高频判断从主 runtime 中抽离出来。
对外接口: looks_like_non_play_request()、looks_like_cancellation_request()、is_compound_script_intent()、should_block_compound_script_fallback()、compound_script_block_message()
依赖关系: 无外部依赖；由 music_runtime 等运行时模块直接调用
输入输出: 输入为用户消息、意图或缺失字段；输出为布尔判断或统一文案
异常与错误: 纯函数，不抛出运行时异常
维护说明: 当前仅承载第一阶段高频守卫；后续可继续扩展为更系统的 policy hook 层
"""


NON_PLAY_BLOCKERS = ("列出", "列表", "所有", "全部", "查看内容", "里面有什么", "添加", "加入", "创建", "新建")
CANCELLATION_KEYWORDS = (
    "取消",
    "停止执行",
    "终止执行",
    "别执行了",
    "先停一下",
    "取消脚本",
    "取消任务",
    "停止任务",
)
COMPOUND_SCRIPT_INTENTS = {"create_playlist_from_playlist_subset"}
COMPOUND_SCRIPT_BLOCK_MESSAGE = (
    "当前任务没有生成安全脚本，本轮未执行任何客户端操作。请先确认来源歌单是否唯一，或先查询一次来源歌单后再继续。"
)


def looks_like_non_play_request(user_message: str) -> bool:
    normalized = user_message.strip()
    return any(keyword in normalized for keyword in NON_PLAY_BLOCKERS)


def looks_like_cancellation_request(user_message: str) -> bool:
    normalized = user_message.strip()
    if not normalized:
        return False
    return any(keyword in normalized for keyword in CANCELLATION_KEYWORDS)


def is_compound_script_intent(intent: str) -> bool:
    return intent in COMPOUND_SCRIPT_INTENTS


def should_block_compound_script_fallback(intent: str, missing_fields: list[str] | tuple[str, ...] | set[str]) -> bool:
    return is_compound_script_intent(intent) and not missing_fields


def compound_script_block_message() -> str:
    return COMPOUND_SCRIPT_BLOCK_MESSAGE


def unsupported_capability_block_message(capability_name: str | None) -> str:
    if capability_name:
        return f"当前客户端暂未向服务端开放能力：{capability_name}。本轮未执行任何客户端操作。"
    return "当前客户端暂未向服务端开放完成该任务所需的能力，本轮未执行任何客户端操作。"
