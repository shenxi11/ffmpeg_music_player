from music_agent.policy_hooks import (
    compound_script_block_message,
    is_compound_script_intent,
    looks_like_cancellation_request,
    looks_like_non_play_request,
    should_block_compound_script_fallback,
)


def test_non_play_request_hook_blocks_listing_style_text():
    assert looks_like_non_play_request("列出最近播放列表的所有音乐") is True
    assert looks_like_non_play_request("帮我创建一个歌单") is True
    assert looks_like_non_play_request("播放周杰伦的晴天") is False


def test_cancellation_hook_detects_script_cancellation_requests():
    assert looks_like_cancellation_request("取消当前执行") is True
    assert looks_like_cancellation_request("停止任务") is True
    assert looks_like_cancellation_request("播放晴天") is False


def test_compound_script_guard_only_blocks_complete_compound_intent():
    assert is_compound_script_intent("create_playlist_from_playlist_subset") is True
    assert should_block_compound_script_fallback("create_playlist_from_playlist_subset", []) is True
    assert should_block_compound_script_fallback("create_playlist_from_playlist_subset", ["sourcePlaylist"]) is False
    assert should_block_compound_script_fallback("play_track", []) is False
    assert "安全脚本" in compound_script_block_message()
