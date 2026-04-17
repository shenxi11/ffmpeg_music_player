# Music Agent Backend

Backend for the FFmpeg Music Player AI assistant.

## Current Positioning

The backend now defaults to a `control-first` architecture instead of a general-purpose chat agent.

Primary goals:

- control the music software
- explain the current software state
- translate natural language into deterministic Qt capability calls
- prefer local `Qwen2.5-3B-Instruct` through an OpenAI-compatible `llama.cpp` server

Out-of-scope for default `control` mode:

- open-domain chat
- world-knowledge Q&A
- long-form copywriting
- automatic free-form planning or script generation

Those requests are restricted by default and can only use remote fallback when the user explicitly enters `/assistant` or `/remote`.

## Runtime Architecture

The Python mainline is now:

- `ControlRuntime`
- `LocalModelGateway`
- deterministic execution templates
- Qt-hosted capability snapshot as the source of truth

Main execution flow:

1. `user_message`
2. fast-path routing for low-latency playback commands
3. local model compiles a strict `ControlIntent` JSON
4. deterministic template issues `tool_call`
5. Qt returns `tool_result`
6. backend emits `final_result`

The old semantic parser, autonomous planner, action-candidate chain, and script-generation mainline are no longer wired into normal control execution. Legacy protocol handlers remain only for compatibility and debug scenarios.

Qt now exposes the full client capability surface to the backend except for login and authentication handshakes. This includes playback, queue, playlists, local library, downloads, recommendations, desktop lyrics, video window controls, settings, host-context reads, user profile reads/writes, logout, and return-to-welcome actions.

## Current Control Coverage

Fast path, no model required:

- pause playback
- resume playback
- stop playback
- next track
- previous track
- volume change
- play mode change
- current playback / queue query

Template-driven control:

- `searchTracks -> playTrack`
- `getPlaylists -> getPlaylistTracks -> playTrack`
- `getCurrentTrack -> getPlaybackQueue`
- `createPlaylist` with chat approval
- `getPlaylists -> getRecentTracks/searchTracks/getPlaylistTracks -> addPlaylistItems` with chat approval

Additional direct tool coverage:

- host-context reads: `getHostContext`, `getVisiblePage`, `getSelectedPlaylist`, `getSelectedTrackIds`
- user profile actions: `getUserProfile`, `refreshUserProfile`, `updateUsername`, `uploadAvatar`, `logoutUser`
- session / environment actions: `returnToWelcome`, `getSettingsSnapshot`, `updateSetting`

Mode constraints:

- `control`: deterministic control execution is allowed
- `assistant`: explanation only; write actions are rejected by the Qt executor

## Stack

- `FastAPI`
- `WebSocket`
- `Pydantic`
- `OpenAI-compatible API`
- `llama.cpp` OpenAI server for local inference

## Setup

```powershell
cd e:\FFmpeg_whisper\ffmpeg_music_player\agent
python -m venv .venv
.venv\Scripts\Activate.ps1
python -m pip install -U pip
python -m pip install -e .[dev]
```

## Environment Variables

Default local-control configuration:

```env
LOCAL_MODEL_BASE_URL=http://127.0.0.1:8081/v1
LOCAL_MODEL_NAME=Qwen2.5-3B-Instruct
LOCAL_MODEL_TIMEOUT_SECONDS=20
REMOTE_MODEL_ENABLED=false
REMOTE_MODEL_BASE_URL=
REMOTE_MODEL_NAME=
OPENAI_API_KEY=
OPENAI_BASE_URL=
OPENAI_MODEL=
OPENAI_WIRE_API=chat_completions
OPENAI_TIMEOUT_SECONDS=30
AGENT_DEFAULT_MODE=control
AGENT_HOST=127.0.0.1
AGENT_PORT=8765
AGENT_MAX_HISTORY_MESSAGES=20
AGENT_STORAGE_PATH=data/music_agent.db
AGENT_TOOL_TIMEOUT_SECONDS=15
AGENT_ALLOW_DIRECT_WRITE_ACTIONS=true
```

Notes:

- `LOCAL_MODEL_BASE_URL` should point to a running `llama.cpp` OpenAI-compatible server.
- `LOCAL_MODEL_NAME` is fixed to `Qwen2.5-3B-Instruct` for this rebuild.
- the default local endpoint is now `127.0.0.1:8081/v1` to avoid conflicts with existing services on `8080`
- `REMOTE_MODEL_ENABLED=false` means remote fallback is off unless you explicitly enable it.
- remote fallback still uses `OPENAI_API_KEY` plus `REMOTE_MODEL_NAME` or `OPENAI_MODEL`.

## Recommended Local Model Launch

Install `llama.cpp` on Windows if `llama-server` is not available yet:

```powershell
winget install --id ggml.llamacpp --accept-package-agreements --accept-source-agreements
```

Example `llama-server` launch:

```powershell
llama-server ^
  -m E:\models\llm\Qwen2.5-3B-Instruct\qwen2.5-3b-instruct-q4_k_m.gguf ^
  --host 127.0.0.1 ^
  --port 8081 ^
  --ctx-size 8192
```

Or use the bundled helper scripts:

```powershell
cd e:\FFmpeg_whisper\ffmpeg_music_player\agent
powershell -ExecutionPolicy Bypass -File .\scripts\start_local_model.ps1
```

Health check:

```powershell
cd e:\FFmpeg_whisper\ffmpeg_music_player\agent
powershell -ExecutionPolicy Bypass -File .\scripts\check_local_model.ps1
```

## Run

```powershell
cd e:\FFmpeg_whisper\ffmpeg_music_player\agent
.venv\Scripts\Activate.ps1
music-agent-server
```

Default address: `http://127.0.0.1:8765`

## CLI Chat

`music-agent-chat` is now treated as a legacy terminal chat entry for remote chat-style debugging. It still requires remote chat model configuration and is not the main control workflow.

```powershell
cd e:\FFmpeg_whisper\ffmpeg_music_player\agent
.venv\Scripts\Activate.ps1
music-agent-chat
```

Optional:

```powershell
music-agent-chat --session-id demo-session
```

## REST APIs

- `GET /healthz`
- `GET /capabilities`
- `GET /sessions?query=<optional>&limit=<optional>`
- `POST /sessions`
- `GET /sessions/{session_id}`
- `PATCH /sessions/{session_id}`
- `DELETE /sessions/{session_id}`
- `GET /sessions/{session_id}/messages`
- `GET /sessions/{session_id}/events`
- `GET /plans/{plan_id}/events`

`/healthz` now reports both local-control and remote-fallback status, including:

- `localModelBaseUrl`
- `localModelName`
- `remoteModelEnabled`
- `remoteModelBaseUrl`
- `remoteModelName`
- `defaultMode`

## WebSocket

Endpoint:

```text
ws://127.0.0.1:8765/ws/chat?session_id=<optional>
```

Key client messages:

- `user_message`
- `host_snapshot`
- `tool_result`
- `approval_response`

Key server messages:

- `session_ready`
- `tool_call`
- `plan_preview`
- `approval_request`
- `clarification_request`
- `final_result`
- `assistant_start`
- `assistant_chunk`
- `assistant_final`

### Host Snapshot

Qt now pushes a capability snapshot and host-context snapshot before control turns:

```json
{
  "type": "host_snapshot",
  "hostContext": {
    "currentPage": "playlists",
    "offlineMode": false,
    "loggedIn": true,
    "currentTrack": {
      "title": "七里香",
      "artist": "周杰伦",
      "isLocal": false
    },
    "selectedPlaylist": {
      "playlistId": 12,
      "name": "默认歌单",
      "trackCount": 24
    },
    "selectedTrackIds": ["101", "102", "103"],
    "queueSummary": {
      "count": 8,
      "currentIndex": 2,
      "playing": true
    }
  },
  "capabilities": [
    {
      "name": "createPlaylist",
      "riskLevel": "medium",
      "confirmPolicy": "chat_approval",
      "availabilityPolicy": "login_required",
      "domain": "playlist",
      "userVisibleName": "创建歌单"
    },
    {
      "name": "getUserProfile",
      "riskLevel": "low",
      "confirmPolicy": "none",
      "availabilityPolicy": "login_required",
      "domain": "account",
      "userVisibleName": "用户资料"
    }
  ],
  "catalogVersion": "qt_tool_registry_v1"
}
```

### Explicit Remote Fallback

Only explicit prefixes can enter remote mode:

```json
{
  "type": "user_message",
  "requestId": "req-1",
  "content": "/assistant 解释一下 AI 发展史"
}
```

In `control` mode, the same request without `/assistant` will be restricted instead of routed to remote chat.

## Test

```powershell
cd e:\FFmpeg_whisper\ffmpeg_music_player\agent
.venv\Scripts\Activate.ps1
pytest
```

If you only want a syntax check:

```powershell
python -m compileall src tests
```
