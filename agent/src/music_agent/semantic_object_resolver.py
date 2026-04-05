from __future__ import annotations

"""
模块名称: semantic_object_resolver
功能概述: 负责从中文用户输入中提取结构化对象，例如歌单名、歌曲名、来源/目标歌单、曲目选择器和上下文引用。
对外接口: normalize_text(), extract_playlist_query(), extract_track_query(), extract_playlist_subset_transfer()
依赖关系: 仅依赖标准库 re；供 semantic_goal_router 和 semantic_refiner 复用。
输入输出: 输入为自然语言文本；输出为对象片段或结构化字典。
异常与错误: 不抛出运行时异常；无法提取时返回 None。
维护说明: 这里专注“对象解析”，不要混入意图判断或执行策略。
"""

import re
from typing import Any


QUERY_HINTS = (
    "查询",
    "查找",
    "找一下",
    "找找",
    "看看",
    "看下",
    "查看",
    "列出",
    "列一下",
    "有没有",
    "帮我找",
    "搜",
)
PLAY_HINTS = ("播放", "播一下", "放一下", "放一首", "来一首", "听一下", "打开")
STOP_HINTS = ("停止播放", "停止", "停一下", "停掉", "停下来", "别放了", "先停一下")
PLAYLIST_CONTENT_HINTS = (
    "里面",
    "里有",
    "内容",
    "歌曲",
    "曲目",
    "所有音乐",
    "全部音乐",
    "所有歌曲",
    "全部歌曲",
    "里有什么歌",
    "里面有什么歌",
)
RECENT_TRACK_HINTS = ("最近播放", "最近听", "最近播放列表", "播放历史", "最近听了什么", "最近播放了什么")
CURRENT_TRACK_HINTS = ("当前播放", "现在播放", "在放什么", "正在播放")
PLAYLIST_REFERENCE_HINTS = ("这个歌单", "那个歌单", "刚才那个歌单", "该歌单", "这个列表", "那个列表")
TRACK_REFERENCE_HINTS = ("这首歌", "那首歌", "刚才那首歌", "这个歌曲", "那个歌曲")
PLAYLIST_FOLLOWUPS = ("那这个呢", "这个呢", "那个呢", "那里面呢", "这里面呢", "那播放一下", "这个播放一下")
LEADING_NOISE = (
    "创建一个",
    "新建一个",
    "建一个",
    "帮我",
    "给我",
    "麻烦",
    "请",
    "请你",
    "我是说",
    "我想",
    "想要",
    "可以帮我",
    "查询",
    "查找",
    "找一下",
    "找找",
    "搜索",
    "看看",
    "看下",
    "查看",
    "列出",
    "列一下",
    "播放",
    "播一下",
    "放一下",
)
LIST_OR_MUTATION_HINTS = ("列出", "列表", "所有", "全部", "查看内容", "里面有什么", "添加", "加入", "创建", "新建")
CREATE_HINTS = ("创建", "新建", "重新创建", "建一个", "建立")
ADD_HINTS = ("放到", "加入", "加到", "添加", "添加到", "放进")
COUNT_MAP = {
    "一": 1,
    "二": 2,
    "两": 2,
    "三": 3,
    "四": 4,
    "五": 5,
    "六": 6,
    "七": 7,
    "八": 8,
    "九": 9,
    "十": 10,
}
TRAILING_PLAYLIST_NOISE = (
    "里面有什么歌",
    "里有什么歌",
    "里面的歌曲",
    "里面的歌",
    "歌曲列表",
    "歌有哪些",
    "都有什么歌",
    "内容",
    "音乐",
    "所有音乐",
    "全部音乐",
    "所有歌曲",
    "全部歌曲",
)


def normalize_text(value: str) -> str:
    text = (value or "").strip()
    text = text.replace("“", '"').replace("”", '"').replace("‘", "'").replace("’", "'")
    text = text.replace("，", ",").replace("。", ".").replace("！", "!").replace("？", "?")
    text = re.sub(r"\s+", "", text)
    return text


def contains_any(text: str, keywords: tuple[str, ...]) -> bool:
    return any(keyword in text for keyword in keywords)


def strip_leading_noise(text: str) -> str:
    cleaned = text.strip()
    changed = True
    while changed and cleaned:
        changed = False
        for token in LEADING_NOISE:
            if cleaned.startswith(token):
                cleaned = cleaned[len(token) :].strip(" ,.!?，。！？")
                changed = True
    return cleaned


def extract_playlist_query(user_message: str) -> str | None:
    text = normalize_text(user_message)
    if not text or contains_any(text, PLAYLIST_REFERENCE_HINTS):
        return None

    content_patterns = (
        r"(?:列出|查看|展示|告诉我)(?P<name>.+?歌单)(?:的)?(?:所有音乐|全部音乐|所有歌曲|全部歌曲|音乐|歌曲|内容)",
        r"(?P<name>.+?歌单)里(?:面)?有什么歌",
        r"(?P<name>.+?歌单)(?:里|里面)?的?(?:所有音乐|全部音乐|所有歌曲|全部歌曲|音乐|歌曲)",
    )
    for pattern in content_patterns:
        match = re.search(pattern, text)
        if match:
            candidate = _cleanup_playlist_name(match.group("name"))
            if candidate:
                return candidate

    quoted_match = re.search(r'["\'](?P<name>[^"\']+?)["\']歌单?', text)
    if quoted_match:
        return _cleanup_playlist_name(quoted_match.group("name"))

    match = re.search(r"(?P<name>[^,.!?，。！？]+?歌单)", text)
    if match:
        candidate = _cleanup_playlist_name(match.group("name"))
        if candidate and candidate != "歌单":
            return candidate
    return None


def extract_track_query(user_message: str) -> tuple[str | None, str | None]:
    text = strip_leading_noise(normalize_text(user_message)).strip('"\' ,.!?，。！？')
    if not text or contains_any(text, TRACK_REFERENCE_HINTS):
        return (None, None)

    for token in PLAY_HINTS:
        if text.startswith(token):
            text = text[len(token) :].strip('"\' ,.!?，。！？')
            break

    artist_match = re.search(r"(?P<artist>.+?)的(?P<title>.+)", text)
    if artist_match:
        artist = artist_match.group("artist").strip('"\' ,.!?，。！？')
        title = artist_match.group("title").strip('"\' ,.!?，。！？')
        return (title or None, artist or None)

    return (text or None, None)


def extract_playlist_subset_transfer(user_message: str) -> dict[str, Any] | None:
    text = normalize_text(user_message)
    if "歌单" not in text:
        return None
    if not any(keyword in text for keyword in CREATE_HINTS):
        return None
    if not any(keyword in text for keyword in ADD_HINTS):
        return None

    target_raw = _extract_target_playlist_name(text)
    source_raw = _extract_source_playlist_name(text)
    selection_mode, selection_count = _extract_track_selection(text)
    if not target_raw or not source_raw or selection_mode is None or selection_count is None:
        return None

    return {
        "target_raw": target_raw,
        "source_raw": source_raw,
        "selection_mode": selection_mode,
        "selection_count": selection_count,
    }


def looks_like_playlist_content_followup(user_message: str) -> bool:
    text = normalize_text(user_message)
    return bool(text) and (
        contains_any(text, PLAYLIST_FOLLOWUPS)
        or ("里面" in text and ("歌" in text or "音乐" in text or "内容" in text))
    )


def looks_like_playlist_play_followup(user_message: str, current_goal: str) -> bool:
    text = normalize_text(user_message)
    if not text:
        return False
    if contains_any(text, PLAYLIST_REFERENCE_HINTS) and contains_any(text, PLAY_HINTS):
        return True
    if current_goal in {"query_playlist", "inspect_playlist_tracks"} and text in {"播放这个歌单", "就放刚才那个", "那播放一下"}:
        return True
    return False


def looks_like_track_play_followup(user_message: str) -> bool:
    text = normalize_text(user_message)
    return contains_any(text, TRACK_REFERENCE_HINTS) and contains_any(text, PLAY_HINTS)


def _cleanup_playlist_name(value: str | None) -> str | None:
    if not value:
        return None
    cleaned = strip_leading_noise(value)
    for suffix in TRAILING_PLAYLIST_NOISE:
        if cleaned.endswith(suffix):
            cleaned = cleaned[: -len(suffix)]
            break
    cleaned = cleaned.strip('"\' ,.!?，。！？')
    cleaned = re.sub(r"^(?:我的|那个|这个|刚才那个)", "", cleaned)
    return cleaned or None


def _extract_target_playlist_name(text: str) -> str | None:
    patterns = (
        r"(?:歌单名为|歌单叫做|叫做)(?P<name>[^,.!?，。！？]+)",
        r"(?:创建|新建|重新创建)(?:一个)?歌单[,，]?(?:歌单名为|叫做)?(?P<name>[^,.!?，。！？]+)",
        r"(?P<name>[^,.!?，。！？]+)歌单里面添加",
    )
    for pattern in patterns:
        match = re.search(pattern, text)
        if match:
            candidate = match.group("name").strip('"\' ,.!?，。！？')
            candidate = re.sub(r"^名为", "", candidate)
            if candidate and "流行歌单" not in candidate:
                return candidate
    return None


def _extract_source_playlist_name(text: str) -> str | None:
    patterns = (
        r"[把将](?P<name>[^,.!?，。！？]+?歌单)(?:的|里|里面)?",
        r"从(?P<name>[^,.!?，。！？]+?歌单)(?:里|里面)?",
        r"(?:添加|加入)(?P<name>[^,.!?，。！？]+?歌单)(?:的|里|里面)?",
    )
    for pattern in patterns:
        match = re.search(pattern, text)
        if match:
            candidate = match.group("name").strip('"\' ,.!?，。！？')
            if candidate and candidate not in {"这个歌单", "那个歌单", "该歌单"}:
                return candidate
    return None


def _extract_track_selection(text: str) -> tuple[str | None, int | None]:
    match = re.search(r"前(?P<count>\d+)首", text)
    if match:
        return ("first_n", int(match.group("count")))
    match = re.search(r"前(?P<count>[一二两三四五六七八九十])首", text)
    if match:
        return ("first_n", COUNT_MAP.get(match.group("count")))

    match = re.search(r"后(?P<count>\d+)首", text)
    if match:
        return ("last_n", int(match.group("count")))
    match = re.search(r"后(?P<count>[一二两三四五六七八九十])首", text)
    if match:
        return ("last_n", COUNT_MAP.get(match.group("count")))

    if "最后一首" in text or "倒数第一首" in text:
        return ("last_n", 1)
    return (None, None)
