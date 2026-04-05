from music_agent.response_style import (
    build_playlist_clarification,
    build_track_clarification,
    summarize_create_playlist,
    summarize_playlist_tracks,
    summarize_recent_tracks,
)


def test_track_clarification_formats_candidates():
    question, options = build_track_clarification(
        [
            {"title": "晴天", "artist": "周杰伦"},
            {"title": "七里香", "artist": "周杰伦"},
        ]
    )

    assert "多个候选歌曲" in question
    assert options == ["1. 周杰伦 - 晴天", "2. 周杰伦 - 七里香"]


def test_playlist_clarification_formats_candidates():
    question, options = build_playlist_clarification(
        [
            {"name": "流行"},
            {"name": "轻音乐"},
        ]
    )

    assert "多个候选歌单" in question
    assert options == ["1. 流行", "2. 轻音乐"]


def test_playlist_track_summary_lists_songs():
    text = summarize_playlist_tracks(
        {"name": "流行"},
        [
            {"title": "第一首", "artist": "歌手A"},
            {"title": "第二首", "artist": "歌手B"},
        ],
    )

    assert "歌单 **流行** 里有这些歌曲：" in text
    assert "1. **第一首** - **歌手A**" in text
    assert "2. **第二首** - **歌手B**" in text


def test_recent_tracks_summary_handles_empty_and_non_empty():
    assert summarize_recent_tracks([]) == "最近播放列表为空。"
    text = summarize_recent_tracks([{"title": "晴天", "artist": "周杰伦"}])
    assert "最近播放的歌曲如下：" in text
    assert "**晴天** - **周杰伦**" in text


def test_create_playlist_summary_formats_name():
    assert summarize_create_playlist({"name": "周杰伦"}) == "已创建歌单 **周杰伦**。"
