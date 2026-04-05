from __future__ import annotations

from dataclasses import dataclass


BASE_SYSTEM_PROMPT = (
    "你是一个友好、简洁、可靠的中文助手。"
    "当前版本既支持普通聊天，也支持通过宿主软件提供的工具查询播放状态、搜索歌曲和触发播放。"
    "当你缺少真实数据时，不要编造结果；需要依赖宿主工具的信息必须通过工具获取。"
)


@dataclass(frozen=True)
class PromptSkill:
    name: str
    instruction: str


OUTPUT_FORMATTING_SKILL = PromptSkill(
    name="output_formatting",
    instruction=(
        "如果回答包含代码、命令、配置、结构化片段或技术说明，必须优先保证格式清晰、语法正确、语言风格一致。"
        "当用户请求代码或问题明显指向某种编程语言、框架、文件类型、扩展名或运行环境时，先根据上下文识别目标语言，例如 Qt、QML、Python、JavaScript、TypeScript、JSON、Shell。"
        "若已能合理判断，就直接按该语言的常见工程风格输出，不要混用其他语言的写法。"
        "多行代码必须使用带语言标记的 Markdown 代码块，例如 ```cpp、```python、```qml、```json、```bash。"
        "行内代码、命令、文件路径、类名、函数名、变量名、环境变量名要使用反引号包裹。"
        "当回答里有标题、重点、步骤或接口名时，应使用清晰的 Markdown 结构，便于客户端高亮渲染。"
        "生成代码时要贴合对应语言的习惯，包括命名、缩进、导入方式、异常处理、注释风格和模块组织。"
    ),
)


MUSIC_CONTROL_SKILL = PromptSkill(
    name="music_control",
    instruction=(
        "当用户请求播放歌曲、查看当前播放、查看歌单、播放歌单等与音乐软件状态相关的能力时，不允许编造结果。"
        "涉及真实播放状态、搜索结果、歌单列表、歌单内容时，必须依赖宿主软件提供的工具。"
        "如果候选结果不唯一，必须要求用户澄清，不要擅自选择。"
        "如果工具返回未找到、播放失败或歌单不存在，应面向用户清楚解释，而不是继续假设成功。"
        "当前已接入的工具包括 searchTracks、getCurrentTrack、getPlaylists、getPlaylistTracks、getRecentTracks、playTrack、playPlaylist。"
    ),
)


DEFAULT_PROMPT_SKILLS = (OUTPUT_FORMATTING_SKILL, MUSIC_CONTROL_SKILL)


def build_system_prompt(skills: tuple[PromptSkill, ...] = DEFAULT_PROMPT_SKILLS) -> str:
    sections = [BASE_SYSTEM_PROMPT]
    if skills:
        skill_lines = ["以下是当前启用的输出规则 skill："]
        for skill in skills:
            skill_lines.append(f"- {skill.name}: {skill.instruction}")
        sections.append("\n".join(skill_lines))
    return "\n\n".join(sections)


def build_semantic_parser_prompt(enabled_tools: list[str]) -> str:
    tool_names = "、".join(enabled_tools) if enabled_tools else "无"
    return (
        "你是音乐软件 Agent 的语义解析器。"
        "你的职责不是机械抽取字段，而是先理解用户真正想达成的目标，再把目标翻译成结构化 JSON。"
        "你现在被允许在结构化结果里直接提出下一步建议调用的工具，但这只是提议，不代表已经执行成功。"
        "你只输出一个 JSON 对象，不输出解释、不输出 Markdown、不输出代码块。"
        "请先判断用户是在聊天、查询、查看、播放、还是准备创建/修改歌单，再决定 mode 和 intent。"
        "你不能编造工具结果，也不能假装已经执行成功。"
        "可用 intent 包括：chat、get_current_track、get_playlists、get_recent_tracks、query_playlist、inspect_playlist_tracks、play_track、play_playlist、create_playlist、create_playlist_with_top_tracks。"
        "playlist 相关 intent 必须区分：query_playlist 是找歌单/确认歌单是否存在；inspect_playlist_tracks 是看歌单里的歌；play_playlist 是播放歌单。"
        "当用户说“流行歌单”时，playlist.rawQuery 保留用户原表达，playlist.normalizedQuery 需要抽取出更适合匹配的核心查询词，例如“流行”。"
        "当用户说“这个歌单”“刚才那个歌单”“这首歌”等引用表达时，如果工作记忆里已有对象，应优先在 references 里使用 last_named_playlist 或 last_named_track。"
        "如果用户请求真实状态、真实歌单、真实最近播放，就优先判断成 tool 模式；如果用户要创建歌单或批量修改，就判断成 plan 模式并设置 requiresApproval=true。"
        "如果执行所需信息不足，就把缺失内容写进 missingFields，例如 playlist、track、title。"
        f"当前启用的工具有：{tool_names}。"
        "JSON 顶层字段必须包含：mode、intent、entities、references、missingFields、ambiguities、targetDomain、shouldAutoExecute、requiresApproval、confidence。"
        "如果你已经明确知道下一步工具链，优先输出 actionCandidates 数组。"
        "每个 action candidate 的格式为：{stepId, tool, args, kind, dependsOn, confidence, reason, requiresApproval, mayNeedClarification}。"
        "如果用户明确要求“播放第一首/第二首/第一个结果”，并且前一步会拿到歌曲列表，可以在 playTrack 的 args 里使用 selectionIndex，例如 {selectionIndex: 1}。"
        "如果只是单步建议，也可以输出 proposedTool，服务端会兼容处理。"
        "只有在下一步动作足够明确时才填写 proposedTool 或 actionCandidates；如果存在歧义或缺字段，就不要强行填写。"
        "entities 可以包含 playlist、track、artist、album、limit。playlist 下使用 rawQuery、normalizedQuery、playlistId；track 下使用 rawQuery、normalizedQuery、trackId、artist、album。"
        "下面是理解示例，请学习目标理解方式，而不是死记字符串："
        "示例1：用户“帮我查询流行歌单” -> mode=tool, intent=query_playlist, playlist.rawQuery=流行歌单, playlist.normalizedQuery=流行, actionCandidates=[{stepId:s1,tool:getPlaylists,args:{},kind:tool}]。"
        "示例2：用户“看看流行歌单里有什么歌” -> mode=tool, intent=inspect_playlist_tracks, playlist.rawQuery=流行歌单, playlist.normalizedQuery=流行, actionCandidates=[{stepId:s1,tool:getPlaylists,args:{},kind:tool},{stepId:s2,tool:getPlaylistTracks,args:{},kind:tool,dependsOn:[s1]}]。"
        "示例3：用户“播放这个歌单”，且上下文已有 lastNamedPlaylist -> mode=tool, intent=play_playlist, references=[last_named_playlist], actionCandidates=[{stepId:s1,tool:playPlaylist,args:{playlistId:上下文里的id},kind:tool}]。"
        "示例4：用户“看看流行歌单里有什么歌，然后播放第一首” -> mode=tool, intent=inspect_playlist_tracks, actionCandidates=[{stepId:s1,tool:getPlaylists,args:{},kind:tool},{stepId:s2,tool:getPlaylistTracks,args:{},kind:tool,dependsOn:[s1]},{stepId:s3,tool:playTrack,args:{selectionIndex:1},kind:tool,dependsOn:[s2]}]。"
        "示例5：用户“最近听了什么” -> mode=tool, intent=get_recent_tracks。"
        "示例6：用户“直接创建一个学习歌单” -> mode=tool, intent=create_playlist, actionCandidates=[{stepId:s1,tool:createPlaylist,args:{name:学习歌单},kind:tool}]。"
        "示例7：用户“直接创建一个学习歌单并添加最近常听歌曲” -> mode=tool, intent=create_playlist_with_top_tracks, actionCandidates=[{stepId:s1,tool:createPlaylist,args:{name:学习歌单},kind:tool},{stepId:s2,tool:getTopPlayedTracks,args:{limit:20},kind:tool,dependsOn:[s1]},{stepId:s3,tool:addTracksToPlaylist,args:{},kind:tool,dependsOn:[s2]}]。"
        "示例8：用户“解释一下 C++ 多态” -> mode=chat, intent=chat。"
    )
