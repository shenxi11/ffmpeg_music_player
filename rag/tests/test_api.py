from pathlib import Path

from fastapi.testclient import TestClient

from project_rag.server.app import create_app


def test_healthz_endpoint(monkeypatch, tmp_path: Path) -> None:
    repo_root = tmp_path / "repo"
    repo_root.mkdir()
    data_dir = tmp_path / "data"
    monkeypatch.setenv("RAG_REPO_ROOT", str(repo_root))
    monkeypatch.setenv("RAG_DATA_DIR", str(data_dir))
    monkeypatch.setenv("RAG_MANIFEST_DB", str(data_dir / "manifest.db"))
    app = create_app()
    client = TestClient(app)
    response = client.get("/rag/healthz")
    assert response.status_code == 200
    payload = response.json()
    assert payload["status"] == "ok"
