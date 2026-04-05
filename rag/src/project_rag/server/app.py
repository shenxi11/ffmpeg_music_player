"""
???: server.app
????: ????? RAG ? HTTP API???????????????? Agent ??????
????: create_app?main
????: FastAPI?indexer?retrieval.service?schemas.requests
????: ?? HTTP ???????????????????
?????: ????? HTTP 500 ??????????????????
????: ????????????????????????
"""

from __future__ import annotations

from fastapi import FastAPI, HTTPException

from project_rag.config import get_settings
from project_rag.ingest.indexer import RagIndexer
from project_rag.retrieval.service import RetrievalService
from project_rag.schemas.requests import AgentContextRequest, IndexRequest, QueryRequest


def create_app() -> FastAPI:
    settings = get_settings()
    retrieval = RetrievalService(settings)
    app = FastAPI(title="Project RAG Service", version="0.1.0")

    @app.get("/rag/healthz")
    def healthz() -> dict:
        return {
            "status": "ok",
            "qdrantEnabled": retrieval.vector_store.qdrant_enabled,
            "embeddingProvider": settings.rag_embedding_provider,
            "embeddingModel": settings.rag_embedding_model,
            "rerankerProvider": settings.rag_reranker_provider,
            "rerankerModel": settings.rag_reranker_model,
            "dataDir": str(settings.rag_data_dir),
            "manifestDb": str(settings.rag_manifest_db),
        }

    @app.post("/rag/index/full")
    def build_full_index() -> dict:
        indexer = RagIndexer(settings)
        try:
            report = indexer.build_full()
            return {"ok": True, "report": report.model_dump(mode="json")}
        except Exception as exc:
            raise HTTPException(status_code=500, detail=str(exc)) from exc
        finally:
            indexer.close()

    @app.post("/rag/index/incremental")
    def build_incremental_index(request: IndexRequest) -> dict:
        indexer = RagIndexer(settings)
        try:
            report = indexer.build_incremental(request.changed_paths)
            return {"ok": True, "report": report.model_dump(mode="json")}
        except Exception as exc:
            raise HTTPException(status_code=500, detail=str(exc)) from exc
        finally:
            indexer.close()

    @app.post("/rag/query")
    def query(request: QueryRequest) -> dict:
        response = retrieval.query(query=request.query, mode=request.mode, filters=request.filters, top_k=request.top_k)
        return response.model_dump(mode="json", by_alias=True)

    @app.post("/rag/query/agent-context")
    def query_agent_context(request: AgentContextRequest) -> dict:
        response = retrieval.query(query=request.question, mode=_route_agent_mode(request.target_agent), filters=request.filters, top_k=request.top_k)
        return response.model_dump(mode="json", by_alias=True)

    return app


def _route_agent_mode(target_agent: str) -> str:
    normalized = target_agent.strip().lower()
    if "debug" in normalized:
        return "incident_lookup"
    if "execution" in normalized:
        return "protocol_lookup"
    if "review" in normalized:
        return "code_lookup"
    return "architecture"


def main() -> None:
    import uvicorn

    settings = get_settings()
    uvicorn.run("project_rag.server.app:create_app", host=settings.rag_host, port=settings.rag_port, factory=True, reload=False)
