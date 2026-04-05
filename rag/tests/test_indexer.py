from pathlib import Path

from project_rag.config import RagSettings
from project_rag.ingest.indexer import RagIndexer


def _build_settings(repo_root: Path, data_dir: Path) -> RagSettings:
    return RagSettings(
        RAG_REPO_ROOT=str(repo_root),
        RAG_DATA_DIR=str(data_dir),
        RAG_MANIFEST_DB=str(data_dir / "manifest.db"),
    )


def test_full_and_incremental_index_build(tmp_path: Path) -> None:
    repo_root = tmp_path / "repo"
    repo_root.mkdir(parents=True)
    readme_path = repo_root / "README.md"
    readme_path.write_text("# Intro\nContent\n", encoding="utf-8")

    settings = _build_settings(repo_root, tmp_path / "data")
    indexer = RagIndexer(settings)
    try:
        report = indexer.build_full()
        assert any(item.chunk_count > 0 for item in report.stats)

        readme_path.write_text("# Intro\nUpdated content\n", encoding="utf-8")
        incremental = indexer.build_incremental()
        assert incremental.mode == "incremental"
    finally:
        indexer.close()
