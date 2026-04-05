from pathlib import Path

from project_rag.ingest.chunkers import chunk_file


def test_markdown_chunker_extracts_headings(tmp_path: Path) -> None:
    file_path = tmp_path / "sample.md"
    file_path.write_text(
        "# Overview\n"
        "Section A\n"
        "## Details\n"
        "Section B\n",
        encoding="utf-8",
    )
    chunks = chunk_file(tmp_path, file_path, project="client", module="docs", source_type="doc", language="md")
    assert len(chunks) == 2
    assert chunks[0].metadata.heading_path == ["Overview"]
    assert chunks[1].metadata.heading_path == ["Overview", "Details"]


def test_python_chunker_extracts_class_and_function(tmp_path: Path) -> None:
    file_path = tmp_path / "demo.py"
    file_path.write_text(
        "class Demo:\n"
        "    def run(self):\n"
        "        return 1\n",
        encoding="utf-8",
    )
    chunks = chunk_file(tmp_path, file_path, project="agent", module="agent", source_type="code", language="py")
    symbols = {chunk.metadata.symbol for chunk in chunks}
    assert "Demo" in symbols
    assert "Demo.run" in symbols
