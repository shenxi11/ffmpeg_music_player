from music_agent.prompts import OUTPUT_FORMATTING_SKILL, build_system_prompt


def test_system_prompt_includes_output_formatting_skill():
    prompt = build_system_prompt()

    assert "output_formatting" in prompt
    assert "Markdown" in prompt
    assert "```cpp" in prompt
    assert "```python" in prompt
    assert "行内代码" in prompt


def test_output_formatting_skill_mentions_language_specific_style():
    instruction = OUTPUT_FORMATTING_SKILL.instruction

    assert "Qt" in instruction
    assert "QML" in instruction
    assert "Markdown" in instruction
    assert "代码块" in instruction
    assert "语言" in instruction
