# Music Agent Backend

Backend for the FFmpeg Music Player AI assistant.

Current scope:

- multi-turn chat
- streaming assistant output
- LLM-first structured semantic parsing for each `user_message`
- semantic refinement layer for goal understanding, reference resolution, and query normalization
- multi-step LLM action candidates with server-side validation and step-by-step execution
- candidate audit events for step selection and observation replay
- SQLite session and message storage
- SQLite event audit storage for plans and tool calls
- direct Qt tool calls
- script protocol bridge for client-side scripted execution
- automatic playlist lookup, playlist track inspection, and recent-track lookup
- phase-two skeleton for plans and approvals
- multi-step plan for creating a playlist and adding top tracks

## Stack

- `LangGraph`
- `FastAPI`
- `WebSocket`
- `OpenAI-compatible API`

## Setup

```powershell
cd e:\FFmpeg_whisper\ffmpeg_music_player\agent
python -m venv .venv
.venv\Scripts\Activate.ps1
python -m pip install -U pip
python -m pip install -e .[dev]
```

## Environment Variables

```env
OPENAI_API_KEY=sk-xxx
OPENAI_BASE_URL=https://api.openai.com/v1
OPENAI_MODEL=gpt-4o-mini
OPENAI_WIRE_API=chat_completions
OPENAI_TIMEOUT_SECONDS=30
AGENT_HOST=127.0.0.1
AGENT_PORT=8765
AGENT_MAX_HISTORY_MESSAGES=20
AGENT_STORAGE_PATH=data/music_agent.db
AGENT_TOOL_TIMEOUT_SECONDS=15
AGENT_ALLOW_DIRECT_WRITE_ACTIONS=true
```

## Run

```powershell
cd e:\FFmpeg_whisper\ffmpeg_music_player\agent
.venv\Scripts\Activate.ps1
music-agent-server
```

Default address: `http://127.0.0.1:8765`

## CLI Chat

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
- `GET /sessions?query=<optional>&limit=<optional>`
- `POST /sessions`
- `GET /sessions/{session_id}`
- `PATCH /sessions/{session_id}`
- `DELETE /sessions/{session_id}`
- `GET /sessions/{session_id}/messages`
- `GET /sessions/{session_id}/events`
- `GET /plans/{plan_id}/events`

`/healthz` returns:

- `protocolVersion`
- `capabilities`
- `toolModeEnabled`
- `auditEnabled`
- `openaiWireApi`
- `capabilityCatalogVersion`
- `capabilityExecutionModel`

Example:

```json
{
  "status": "ok",
  "modelConfigured": true,
  "missingConfig": [],
  "openaiBaseUrl": "https://open.bigmodel.cn/api/paas/v4",
  "openaiModel": "glm-5",
  "openaiWireApi": "chat_completions",
  "sessionHistoryLimit": 20,
  "storagePath": "data/music_agent.db",
  "protocolVersion": "1.6",
  "capabilities": ["chat", "streaming", "sessions", "storage", "tools", "plans", "approval", "audit", "scripts"],
  "toolModeEnabled": true,
  "auditEnabled": true,
  "capabilityCatalogVersion": "2026-03-30-facade-aware",
  "capabilityExecutionModel": {
    "phase": 6,
    "entryPoint": "AgentCapabilityFacade",
    "backingExecutor": "AgentToolExecutor"
  }
}
```

## WebSocket

Endpoint:

```text
ws://127.0.0.1:8765/ws/chat?session_id=<optional>
```

### Session Ready

```json
{
  "type": "session_ready",
  "sessionId": "generated-or-existing-session-id",
  "title": "新建会话",
  "capabilities": ["chat", "streaming", "sessions", "storage", "tools", "plans", "approval", "audit", "scripts"]
}
```

### Chat Messages

Client sends:

```json
{
  "type": "user_message",
  "requestId": "req-1",
  "content": "你好"
}
```

Server may respond with:

- `assistant_start`
- `assistant_chunk`
- `assistant_final`

### Semantic Parsing

The backend now parses every `user_message` through a dedicated structured semantic layer before deciding whether to:

- reply as plain chat
- emit `tool_call`
- emit `clarification_request`
- enter `plan_preview / approval_request`

The LLM only returns structured JSON for understanding and action suggestions. It does not directly execute Qt tools.
The server then runs a semantic refinement and execution-control pass that combines:

- user utterance cues
- working memory and last resolved objects
- query normalization such as `流行歌单 -> 流行`
- automatic intent correction when the model returns an overly generic `chat` result
- action candidate validation against the server-side tool registry

The Qt client is now modeled as a unified capability entry:

- `tool_call` enters through `AgentCapabilityFacade`
- script steps also enter through `AgentCapabilityFacade`
- `AgentToolExecutor` remains the backing executor behind the façade

This means the backend should increasingly reason about "capabilities behind a façade" rather than treating every tool as an isolated transport-level primitive.

Multi-step candidates can now cover chains such as:

- `getPlaylists -> playPlaylist`
- `getPlaylists -> getPlaylistTracks`
- `getPlaylistTracks -> playTrack` with `selectionIndex`
- `searchTracks -> playTrack`
- `createPlaylist`
- `createPlaylist -> getTopPlayedTracks -> addTracksToPlaylist`

Current capability opening strategy follows the latest Qt-side guidance:

- search capabilities are treated as base auto-execution abilities
- single-track low-risk actions are treated as base auto-execution abilities when a structured `Track` object is already available
- playlist structure mutations and collection-level actions remain on the more conservative `confirm / plan` path
- `playPlaylist` is still treated as `partial` and must not be mistaken for a fully stable atomic ability

Candidate selection and post-tool observations are also written into the audit stream as:

- `action_candidate_selected`
- `action_candidate_observed`

`action_candidate_selected` now includes both the original candidate payload and the hydrated `resolvedArgs`, so you can see how the server filled IDs and other runtime parameters before issuing `tool_call`.

### Script Messages

The backend now supports the Qt client script protocol.

### Wire API Compatibility

The backend now supports two upstream response protocols:

- `OPENAI_WIRE_API=chat_completions`
- `OPENAI_WIRE_API=responses`

Use `chat_completions` for standard OpenAI-compatible `/chat/completions` gateways.
Use `responses` for gateways that expose the newer `/responses` wire format.

Current note:

- `responses` mode currently returns assistant text as a single chunk in streaming mode
- this keeps the backend compatible with gateways that do not expose `choices[].delta`

Current version:

- `protocolVersion: 1.6`
- session capabilities include `scripts`

Current first live script scenario:

- `searchTracks -> playTrack` for precise “搜索并播放” requests such as `播放周杰伦的晴天`

Server script requests:

```json
{
  "type": "dry_run_script",
  "requestId": "req-1",
  "script": {
    "version": 1,
    "timeoutMs": 30000,
    "title": "搜索并播放周杰伦 - 晴天",
    "steps": [
      {
        "id": "search",
        "action": "searchTracks",
        "args": {
          "keyword": "晴天",
          "artist": "周杰伦",
          "limit": 5
        },
        "saveAs": "search"
      },
      {
        "id": "play_first",
        "action": "playTrack",
        "args": {
          "trackId": "$steps.search.items.0.trackId"
        }
      }
    ]
  }
}
```

If dry-run passes, the backend continues with:

```json
{
  "type": "validate_script",
  "requestId": "req-1",
  "script": {
    "version": 1,
    "timeoutMs": 30000,
    "title": "搜索并播放周杰伦 - 晴天",
    "steps": [
      {
        "id": "search",
        "action": "searchTracks",
        "args": {
          "keyword": "晴天",
          "artist": "周杰伦",
          "limit": 5
        },
        "saveAs": "search"
      },
      {
        "id": "play_first",
        "action": "playTrack",
        "args": {
          "trackId": "$steps.search.items.0.trackId"
        }
      }
    ]
  }
}
```

Qt returns:

- `script_dry_run_result`
- `script_validation_result`
- `script_execution_started`
- `script_step_event`
- `script_execution_result`
- `script_cancellation_result`

The backend writes all script lifecycle events into the audit stream and folds the final script report back into working memory so later turns can continue to reference the resolved track or playlist objects.

Current script runtime control:

- generated scripts are first sent through `dry_run_script`
- the backend reads `requiresApproval`, `riskLevel`, `domains`, `mutationKinds`, and `targetKinds` from `script_dry_run_result`
- high-risk or approval-required scripts currently stop direct execution and stay available for future approval / replan integration
- generated scripts now carry explicit `timeoutMs`
- the backend waits according to the script timeout instead of a fixed short socket timeout
- when the user asks to cancel a running script, the backend emits `cancel_script`
- timing fields from `script_execution_started.summary` and `script_execution_result.report` such as `startedAt`, `finishedAt`, `durationMs`, and `status` are preserved in audit and observation flow
- cancellation is execution-level only; completed steps are not rolled back

Current limitation:

- more complex playlist-content and filtered recent-track flows still fall back to the older tool-call path because the current Qt DSL only supports sequential steps and result references; it does not yet support filtering, branching, loops, or rollback

If you need to disable direct write execution and force write actions back onto the existing `plan/approval` path, set:

```env
AGENT_ALLOW_DIRECT_WRITE_ACTIONS=false
```

### Tool Messages

Server tool request:

```json
{
  "type": "tool_call",
  "toolCallId": "tool-123",
  "sessionId": "session-1",
  "tool": "searchTracks",
  "args": {
    "keyword": "晴天",
    "artist": "周杰伦",
    "limit": 5
  }
}
```

Qt client returns:

```json
{
  "type": "tool_result",
  "toolCallId": "tool-123",
  "ok": true,
  "result": {
    "items": []
  }
}
```

### Clarification

When multiple candidates exist, server sends:

```json
{
  "type": "clarification_request",
  "sessionId": "session-1",
  "requestId": "req-1",
  "question": "我找到了多个候选歌曲，请告诉我你想播放哪一个。",
  "options": ["1. 周杰伦 - 晴天", "2. 五月天 - 晴天"]
}
```

Qt can continue with a normal `user_message`, for example `第一个`。

### Plan And Approval Skeleton

Current phase-two skeleton supports `createPlaylist`.

Server plan preview:

```json
{
  "type": "plan_preview",
  "planId": "plan-1",
  "sessionId": "session-1",
  "summary": "创建歌单“学习歌单”",
  "riskLevel": "medium",
  "steps": [
    {
      "stepId": "step-1",
      "title": "创建歌单 学习歌单",
      "status": "pending"
    }
  ]
}
```

Server approval request:

```json
{
  "type": "approval_request",
  "planId": "plan-1",
  "sessionId": "session-1",
  "message": "即将创建歌单“学习歌单”，是否继续？",
  "riskLevel": "medium"
}
```

Qt approval response:

```json
{
  "type": "approval_response",
  "planId": "plan-1",
  "approved": true
}
```

Then the server may send:

- `progress`
- `tool_call(createPlaylist)`
- `tool_call(getTopPlayedTracks)`
- `tool_call(addTracksToPlaylist)`
- `final_result`
- `assistant_start/chunk/final`

### Audit Events

The backend now writes plan and tool execution events into SQLite and exposes them through:

- `GET /sessions/{session_id}/events`
- `GET /plans/{plan_id}/events`

Sample event item:

```json
{
  "eventId": "evt-1",
  "sessionId": "session-1",
  "planId": "plan-1",
  "eventType": "tool_call",
  "payload": {
    "type": "tool_call",
    "toolCallId": "tool-1",
    "sessionId": "session-1",
    "tool": "createPlaylist",
    "args": {
      "name": "学习歌单"
    }
  },
  "createdAt": "2026-03-28T10:00:00+00:00"
}
```

### Error

```json
{
  "type": "error",
  "sessionId": "session-1",
  "requestId": "req-1",
  "code": "invalid_message",
  "message": "content must be a non-empty string"
}
```

## Current Tool Scope

Implemented direct tool mode:

- `searchTracks`
- `getCurrentTrack`
- `getPlaylists`
- `getPlaylistTracks`
- `getRecentTracks`
- `playTrack`
- `playPlaylist`
- `createPlaylist` through phase-two approval skeleton
- `getTopPlayedTracks` through phase-two plan execution
- `addTracksToPlaylist` through phase-two plan execution

Implemented intent coverage:

- play a track
- inspect current playing track
- list playlists
- query a named playlist
- inspect playlist tracks
- inspect recent tracks
- play a playlist
- resolve playlist names with both raw and normalized queries, such as `流行歌单 -> 流行`
- reuse `last_named_playlist` / `last_named_track` for phrases like `这个歌单`
- clarify ambiguous candidates
- create a playlist with approval
- create a playlist and add recent top tracks with approval

## Tests

```powershell
cd e:\FFmpeg_whisper\ffmpeg_music_player\agent
.venv\Scripts\Activate.ps1
pytest
```
