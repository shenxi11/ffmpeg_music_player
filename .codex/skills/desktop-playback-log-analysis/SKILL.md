---
name: desktop-playback-log-analysis
description: Use when the task is to analyze desktop client playback, seek, buffering, lyrics sync, or audio-position issues from runtime logs. Unless the user explicitly names a different file, always read E:\FFmpeg_whisper\ffmpeg_music_player\打印日志.txt first and treat it as the default log source for diagnosis.
---

# Desktop Playback Log Analysis

Use this skill when the user asks to:

- analyze logs
- combine logs with playback bug diagnosis
- investigate desktop seek delay, buffering, wrong-position resume, lyrics/audio mismatch, or playback state machine problems
- inspect `打印日志.txt` or diagnose desktop playback behavior from runtime output

## Default Log Source

Unless the user explicitly provides another log file path, always read:

- `E:\FFmpeg_whisper\ffmpeg_music_player\打印日志.txt`

Treat this file as the canonical default input for desktop playback log analysis in this repository.

If the user explicitly names another log file, such as `旧打印日志.txt` or any absolute path, use the user-specified file instead of the default.

## Default Analysis Flow

1. Read `E:\FFmpeg_whisper\ffmpeg_music_player\打印日志.txt` first when no other file is specified.
2. Search for the core playback chain markers:
   - `SEEK_EVT`
   - `STATE_EVT`
   - `BUFFER_EVT`
   - `AudioCacheProxy`
   - `AudioDecoder`
   - `AudioSession`
   - `AudioService`
3. Align the failure window before drawing conclusions:
   - find the user-visible symptom time range
   - collect the seek dispatch, proxy request, first decoded packet, buffering resume, and failure logs in the same window
4. Separate the conclusion into:
   - direct log evidence
   - inferred root cause
   - next verification point

## Priority Fields

When the issue is seek or wrong-position playback, prioritize these fields:

- `seek_target_ms`
- `seek_to_first_decoded_ms`
- `buffer_resume_ms`
- `seek_accept_reason`
- `seek_accept_proxy_hint_satisfied`
- `proxy_original_range_start`
- `proxy_effective_range_start`
- `seek_generation`
- `stale_tail_range_blocked`

## Responsibility Split

When summarizing findings, explicitly distinguish which layer the logs implicate most:

- proxy/range rewrite layer
- session seek acceptance/resume gate
- decoder/demux/ffmpeg layer
- service transport or remote network conditions

Do not default to blaming the service. Use the logs to show whether the fault is more likely in the client pipeline, the service transport path, or the interaction between them.
