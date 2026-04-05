# 项目级 RAG 落地说明

## 目标

为当前项目建立一套工程协作优先的私有 RAG，用于：

- 多 Agent 协作
- 代码导航
- 协议联调
- 架构问答
- 问题排障

## 当前实现

当前仓库已经新增 `rag/` 子系统，包含：

- `rag/ingest/`：扫描、切块、清单与索引构建
- `rag/retrieval/`：词法检索、向量检索、混合排序
- `rag/schemas/`：统一 chunk/request/response schema
- `rag/server/`：FastAPI 检索服务
- `rag/cases/`：问题案例库

## 默认知识源

第一版默认收录：

- `说明文档/`
- `src/agent/`
- `src/audio/`
- `src/video/`
- `src/network/`
- `src/viewmodels/`
- `src/app/`
- `qml/`
- `agent/src/music_agent/`
- `agent/README.md`
- `agent/提示文档.md`
- `agent/Qt端对接建议.md`
- `agent/Claude Code方法论对当前音乐Agent的架构借鉴清单.md`
- `README.md`
- `AUDIO_ARCHITECTURE.md`
- `VIDEO_ARCHITECTURE.md`
- `VIDEO_AV_SYNC_STRATEGY.md`
- `说明文档/接口与协议/接口变化.md`
- `说明文档/接口与协议/服务端新增接口.md`
- `说明文档/接口与协议/在线视频接口新增.md`
- `说明文档/测试资料/用户音乐功能测试脚本.md`
- `rag/cases/`

## 接口

- `GET /rag/healthz`
- `POST /rag/index/full`
- `POST /rag/index/incremental`
- `POST /rag/query`
- `POST /rag/query/agent-context`

## 当前边界

- 动态世界状态不进入 RAG
- Capability Catalog 不进入向量库
- 当前不做 Graph-RAG
- 当前不直接索引原始 `打印日志.txt`
- 向量检索优先走 Qdrant，缺失时允许降级

## 建议接入方式

- `Planner Agent`：优先查 `architecture`
- `Execution Agent`：优先查 `protocol_lookup`
- `Debug Agent`：优先查 `incident_lookup`
- `Review Agent`：优先查 `code_lookup`
