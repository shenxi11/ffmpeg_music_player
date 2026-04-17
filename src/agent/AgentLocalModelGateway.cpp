#include "AgentLocalModelGateway.h"

#include "AgentEmbeddedLlamaEngine.h"

#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QtConcurrent/QtConcurrent>

namespace {

const char* kControlCompilerPrompt = R"PROMPT(你是音乐播放器的本地控制编译器，不是开放聊天助手。
你的唯一任务是把用户的话翻译成严格 JSON 控制意图，绝不能输出解释、工具列表、脚本、Markdown 或自然语言。

产品边界：
1. 默认只负责控制软件和解释当前软件状态。
2. 禁止处理开放聊天、知识问答、长文案生成、宽世界知识回答。
3. 遇到超出边界的请求时，输出 route=restricted、intent=restricted，并填写 reasonCode。

输出 JSON 字段固定为：
- route: execute | template | restricted
- intent: pause_playback | resume_playback | stop_playback | play_next | play_previous | set_volume | set_play_mode | query_local_tracks | query_playlist_tracks | query_ui_overview | query_ui_page_state | query_music_tab_items | query_music_tab_item | play_music_tab_track | act_on_music_tab_track | play_track_by_query | query_current_playback | create_playlist_with_confirmation | restricted
- arguments: 对象
- entityRefs: 字符串数组
- confirmation: 布尔值
- reasonCode: 字符串

控制意图说明：
- pause_playback / resume_playback / stop_playback / play_next / play_previous：直接执行播放控制
- set_volume：arguments.volume 为 0-100 整数
- set_play_mode：arguments.mode 为 sequential / repeat_one / repeat_all / shuffle
- query_local_tracks：查询本地音乐列表，可用于“列出我的本地音乐”“看看本地歌曲”
- query_playlist_tracks：查询歌单内容；arguments.playlistName 可选，arguments.useCurrentPlaylist 可选
- query_ui_overview：查询当前页面、当前音乐 tab、离线模式等主窗口概览
- query_ui_page_state：查询某个页面的状态摘要，arguments.page 必填
- query_music_tab_items：查询某个音乐 tab 当前可见条目，arguments.tab 必填，可带 limit、playlistId、playlistName
- query_music_tab_item：读取某个音乐 tab 中的单条对象，arguments.tab 必填，条目通过 trackId/musicPath/playlistId/playlistName 定位
- play_music_tab_track：播放某个音乐 tab 中的一首歌，arguments.tab 必填，条目通过 trackId/musicPath/playlistId/playlistName 定位
- act_on_music_tab_track：对某个音乐 tab 中的条目执行动作，arguments.tab 和 arguments.action 必填
- play_track_by_query：根据歌曲/歌手关键词搜索后播放第一首；arguments 可包含 keyword、artist、album
- query_current_playback：查询当前播放与队列
- create_playlist_with_confirmation：创建歌单，arguments.playlistName 必填，confirmation 必须为 true
- restricted：超出软件控制边界

额外规则：
- 只要用户明确提到“歌单/playlist/收藏夹/当前歌单”，优先考虑 query_playlist_tracks，不能误判成 query_local_tracks
- 只有用户明确指向“本地音乐/本地歌曲/本地库”时，才使用 query_local_tracks
- 提到“当前页面/界面状态/现在在哪个页面”时，优先考虑 query_ui_overview 或 query_ui_page_state
- 提到“推荐页/在线音乐/本地音乐/历史/喜欢/歌单详情”的列表内容时，优先考虑 query_music_tab_items
- 提到“播放某个 tab 的某首歌”时，优先考虑 play_music_tab_track
- 提到“下载/加歌单/收藏/取消收藏/删除”且对象来自某个音乐 tab 时，优先考虑 act_on_music_tab_track
- 不要把页面级 UI 查询或 tab 查询误判成 query_local_tracks / query_playlist_tracks，除非用户本意就是本地库或歌单内容
- page 只允许：recommend / local_download / online_music / history / favorites / playlists / user_profile / video / settings
- tab 只允许：recommend / local_music / downloaded_music / downloading_tasks / online_music / history / favorites / playlist_list_owned / playlist_list_subscribed / playlist_detail

请严格返回 JSON 对象，不要输出额外说明。)PROMPT";

const char* kAssistantPrompt = R"PROMPT(你是音乐播放器里的助手模式。
你可以解释当前软件状态或回答与当前播放器操作直接相关的问题，但不能假装已经执行了任何写操作。
如果用户想真正控制软件，请提醒对方切回 control 模式。)PROMPT";

struct GatewayTaskResult
{
    QString requestId;
    bool assistant = false;
    bool ok = false;
    QString code;
    QString message;
    QString text;
    QVariantMap payload;
};

bool containsAny(const QString& text, const QStringList& words)
{
    for (const QString& word : words) {
        if (text.contains(word, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

QString normalizedPlaylistName(QString text)
{
    text = text.trimmed();
    text.remove(QRegularExpression(QStringLiteral("^(?:请)?(?:帮我)?(?:列出|看看|显示|查看|浏览)")));
    text.remove(QRegularExpression(QStringLiteral("(?:的)?(?:所有)?(?:歌曲|内容|曲目).*$")));
    text.remove(QRegularExpression(QStringLiteral("(?:里|中|中的|里面)$")));
    text.remove(QRegularExpression(QStringLiteral("^我的")));
    text.remove(QRegularExpression(QStringLiteral("^我(?:的)?")));
    text.remove(QRegularExpression(QStringLiteral("^当前")));
    text.remove(QRegularExpression(QStringLiteral("^这个")));
    return text.trimmed();
}

bool isPlaylistReadRequest(const QString& userMessage)
{
    return containsAny(userMessage,
                       {QStringLiteral("歌单"), QStringLiteral("playlist"), QStringLiteral("收藏夹")})
           && containsAny(userMessage,
                          {QStringLiteral("列出"),
                           QStringLiteral("看看"),
                           QStringLiteral("显示"),
                           QStringLiteral("查看"),
                           QStringLiteral("浏览"),
                           QStringLiteral("所有歌曲"),
                           QStringLiteral("内容")});
}

QString parsePageKey(const QString& userMessage)
{
    const QString text = userMessage.trimmed();
    if (containsAny(text, {QStringLiteral("设置页"), QStringLiteral("设置界面"), QStringLiteral("设置页面")})) return QStringLiteral("settings");
    if (containsAny(text, {QStringLiteral("个人主页"), QStringLiteral("用户主页"), QStringLiteral("资料页")})) return QStringLiteral("user_profile");
    if (containsAny(text, {QStringLiteral("视频页"), QStringLiteral("视频界面")})) return QStringLiteral("video");
    if (containsAny(text, {QStringLiteral("歌单页"), QStringLiteral("我的歌单"), QStringLiteral("歌单页面")})) return QStringLiteral("playlists");
    if (containsAny(text, {QStringLiteral("喜欢页"), QStringLiteral("我喜欢"), QStringLiteral("喜欢音乐")})) return QStringLiteral("favorites");
    if (containsAny(text, {QStringLiteral("历史页"), QStringLiteral("最近播放"), QStringLiteral("历史页面")})) return QStringLiteral("history");
    if (containsAny(text, {QStringLiteral("在线音乐"), QStringLiteral("在线页"), QStringLiteral("在线列表")})) return QStringLiteral("online_music");
    if (containsAny(text, {QStringLiteral("本地与下载"), QStringLiteral("本地下载"), QStringLiteral("下载页")})) return QStringLiteral("local_download");
    if (containsAny(text, {QStringLiteral("推荐页"), QStringLiteral("推荐界面"), QStringLiteral("推荐页面")})) return QStringLiteral("recommend");
    return {};
}

QString parseMusicTabKey(const QString& userMessage)
{
    const QString text = userMessage.trimmed();
    if (containsAny(text, {QStringLiteral("当前歌单"), QStringLiteral("歌单详情"), QStringLiteral("这个歌单")})) return QStringLiteral("playlist_detail");
    if (containsAny(text, {QStringLiteral("自建歌单"), QStringLiteral("我的歌单列表")})) return QStringLiteral("playlist_list_owned");
    if (containsAny(text, {QStringLiteral("收藏歌单"), QStringLiteral("订阅歌单")})) return QStringLiteral("playlist_list_subscribed");
    if (containsAny(text, {QStringLiteral("正在下载"), QStringLiteral("下载任务")})) return QStringLiteral("downloading_tasks");
    if (containsAny(text, {QStringLiteral("已下载"), QStringLiteral("下载音乐")})) return QStringLiteral("downloaded_music");
    if (containsAny(text, {QStringLiteral("本地音乐"), QStringLiteral("本地歌曲"), QStringLiteral("本地歌"), QStringLiteral("本地库"), QStringLiteral("本地列表")})) return QStringLiteral("local_music");
    if (containsAny(text, {QStringLiteral("在线音乐"), QStringLiteral("在线歌曲"), QStringLiteral("在线列表")})) return QStringLiteral("online_music");
    if (containsAny(text, {QStringLiteral("最近播放"), QStringLiteral("历史页"), QStringLiteral("历史列表")})) return QStringLiteral("history");
    if (containsAny(text, {QStringLiteral("喜欢音乐"), QStringLiteral("我喜欢"), QStringLiteral("收藏歌曲")})) return QStringLiteral("favorites");
    if (containsAny(text, {QStringLiteral("推荐页"), QStringLiteral("推荐列表"), QStringLiteral("推荐音乐")})) return QStringLiteral("recommend");
    return {};
}

int parseRequestedLimit(const QString& userMessage)
{
    const QRegularExpression regex(QStringLiteral("前\\s*(\\d{1,3})\\s*(?:首|条|个)?"));
    const QRegularExpressionMatch match = regex.match(userMessage);
    return match.hasMatch() ? qBound(1, match.captured(1).toInt(), 100) : 0;
}

QString parseMusicTabAction(const QString& userMessage)
{
    const QString text = userMessage.trimmed();
    if (containsAny(text, {QStringLiteral("下载"), QStringLiteral("下下来")})) return QStringLiteral("download");
    if (containsAny(text, {QStringLiteral("加入歌单"), QStringLiteral("加到歌单"), QStringLiteral("添加到歌单")})) return QStringLiteral("add_to_playlist");
    if (containsAny(text, {QStringLiteral("取消收藏"), QStringLiteral("移出喜欢"), QStringLiteral("不喜欢了")})) return QStringLiteral("remove_favorite");
    if (containsAny(text, {QStringLiteral("收藏"), QStringLiteral("加入喜欢"), QStringLiteral("标记喜欢")})) return QStringLiteral("add_favorite");
    if (containsAny(text, {QStringLiteral("删除"), QStringLiteral("移除"), QStringLiteral("删掉")})) return QStringLiteral("remove_or_delete");
    if (containsAny(text, {QStringLiteral("下一首播放"), QStringLiteral("下首播放"), QStringLiteral("加入下一首")})) return QStringLiteral("play_next");
    if (containsAny(text, {QStringLiteral("切换播放"), QStringLiteral("播放或暂停"), QStringLiteral("暂停或继续")})) return QStringLiteral("toggle_current_playback");
    return {};
}

bool isUiOverviewRequest(const QString& userMessage)
{
    return containsAny(userMessage,
                       {QStringLiteral("当前页面"),
                        QStringLiteral("当前界面"),
                        QStringLiteral("界面状态"),
                        QStringLiteral("页面状态"),
                        QStringLiteral("在哪个页面"),
                        QStringLiteral("在哪个界面")});
}

bool isMusicTabListRequest(const QString& userMessage)
{
    return containsAny(userMessage,
                       {QStringLiteral("列出"),
                        QStringLiteral("看看"),
                        QStringLiteral("显示"),
                        QStringLiteral("查看"),
                        QStringLiteral("浏览"),
                        QStringLiteral("读取"),
                        QStringLiteral("读一下")})
           && containsAny(userMessage,
                          {QStringLiteral("歌曲"),
                           QStringLiteral("歌"),
                           QStringLiteral("音乐"),
                           QStringLiteral("列表"),
                           QStringLiteral("条目"),
                           QStringLiteral("内容")});
}

QVariantMap normalizeCompiledIntent(const QString& userMessage, QVariantMap payload)
{
    const QString intent = payload.value(QStringLiteral("intent")).toString().trimmed();
    const QString parsedTab = parseMusicTabKey(userMessage);
    if (!parsedTab.isEmpty()) {
        static const QSet<QString> kLegacyTabActions{
            QStringLiteral("download"),
            QStringLiteral("add_to_playlist"),
            QStringLiteral("add_favorite"),
            QStringLiteral("remove_favorite"),
            QStringLiteral("remove_or_delete"),
            QStringLiteral("toggle_current_playback"),
            QStringLiteral("play_next"),
        };
        if (kLegacyTabActions.contains(intent)) {
            QVariantMap arguments = payload.value(QStringLiteral("arguments")).toMap();
            arguments.insert(QStringLiteral("tab"), parsedTab);
            arguments.insert(QStringLiteral("action"), intent);
            if (arguments.value(QStringLiteral("trackId")).toString().trimmed().isEmpty()
                && arguments.value(QStringLiteral("musicPath")).toString().trimmed().isEmpty()) {
                arguments.insert(QStringLiteral("useSelectedTrack"), true);
            }
            payload.insert(QStringLiteral("route"), QStringLiteral("template"));
            payload.insert(QStringLiteral("intent"), QStringLiteral("act_on_music_tab_track"));
            payload.insert(QStringLiteral("arguments"), arguments);
            payload.insert(QStringLiteral("reasonCode"), QStringLiteral("legacy_music_tab_action_normalized"));
            return payload;
        }
    }

    if (intent == QStringLiteral("query_local_tracks")) {
        if (isPlaylistReadRequest(userMessage)) {
            QVariantMap arguments = payload.value(QStringLiteral("arguments")).toMap();
            if (containsAny(userMessage, {QStringLiteral("当前歌单"), QStringLiteral("这个歌单")})) {
                arguments.insert(QStringLiteral("useCurrentPlaylist"), true);
                arguments.remove(QStringLiteral("playlistName"));
            } else {
                const QString playlistName = normalizedPlaylistName(userMessage);
                if (!playlistName.isEmpty()) {
                    arguments.insert(QStringLiteral("playlistName"), playlistName);
                }
            }
            payload.insert(QStringLiteral("route"), QStringLiteral("template"));
            payload.insert(QStringLiteral("intent"), QStringLiteral("query_playlist_tracks"));
            payload.insert(QStringLiteral("arguments"), arguments);
            payload.insert(QStringLiteral("reasonCode"), QStringLiteral("playlist_query_normalized"));
            return payload;
        }
        if (!parsedTab.isEmpty() && parsedTab != QStringLiteral("local_music") && isMusicTabListRequest(userMessage)) {
            QVariantMap arguments{{QStringLiteral("tab"), parsedTab}};
            const int limit = parseRequestedLimit(userMessage);
            if (limit > 0) {
                arguments.insert(QStringLiteral("limit"), limit);
            }
            payload.insert(QStringLiteral("route"), QStringLiteral("template"));
            payload.insert(QStringLiteral("intent"), QStringLiteral("query_music_tab_items"));
            payload.insert(QStringLiteral("arguments"), arguments);
            payload.insert(QStringLiteral("reasonCode"), QStringLiteral("music_tab_query_normalized"));
            return payload;
        }
    }

    if (intent == QStringLiteral("query_current_playback") && isUiOverviewRequest(userMessage)) {
        payload.insert(QStringLiteral("route"), QStringLiteral("template"));
        payload.insert(QStringLiteral("intent"), QStringLiteral("query_ui_overview"));
        payload.insert(QStringLiteral("arguments"), QVariantMap());
        payload.insert(QStringLiteral("reasonCode"), QStringLiteral("ui_overview_normalized"));
        return payload;
    }

    if (intent == QStringLiteral("restricted") && !parsedTab.isEmpty()) {
        if (isMusicTabListRequest(userMessage) && !containsAny(userMessage, {QStringLiteral("歌单"), QStringLiteral("playlist")})) {
            QVariantMap arguments{{QStringLiteral("tab"), parsedTab}};
            const int limit = parseRequestedLimit(userMessage);
            if (limit > 0) {
                arguments.insert(QStringLiteral("limit"), limit);
            }
            payload.insert(QStringLiteral("route"), QStringLiteral("template"));
            payload.insert(QStringLiteral("intent"), QStringLiteral("query_music_tab_items"));
            payload.insert(QStringLiteral("arguments"), arguments);
            payload.insert(QStringLiteral("reasonCode"), QStringLiteral("music_tab_query_from_restricted"));
            return payload;
        }

        const QString action = parseMusicTabAction(userMessage);
        if (!action.isEmpty()) {
            payload.insert(QStringLiteral("route"), QStringLiteral("template"));
            payload.insert(QStringLiteral("intent"), QStringLiteral("act_on_music_tab_track"));
            payload.insert(QStringLiteral("arguments"),
                           QVariantMap{{QStringLiteral("tab"), parsedTab},
                                       {QStringLiteral("action"), action},
                                       {QStringLiteral("useSelectedTrack"), true}});
            payload.insert(QStringLiteral("reasonCode"), QStringLiteral("music_tab_action_from_restricted"));
            return payload;
        }

        if (containsAny(userMessage, {QStringLiteral("播放"), QStringLiteral("放一下"), QStringLiteral("来一首")})
            && containsAny(userMessage, {QStringLiteral("这首歌"), QStringLiteral("当前这首"), QStringLiteral("选中的歌")})) {
            payload.insert(QStringLiteral("route"), QStringLiteral("template"));
            payload.insert(QStringLiteral("intent"), QStringLiteral("play_music_tab_track"));
            payload.insert(QStringLiteral("arguments"),
                           QVariantMap{{QStringLiteral("tab"), parsedTab},
                                       {QStringLiteral("useSelectedTrack"), true}});
            payload.insert(QStringLiteral("reasonCode"), QStringLiteral("play_music_tab_from_restricted"));
            return payload;
        }

        if (containsAny(userMessage, {QStringLiteral("信息"), QStringLiteral("详情"), QStringLiteral("资料")})
            && containsAny(userMessage, {QStringLiteral("这首歌"), QStringLiteral("当前这首"), QStringLiteral("选中的歌")})) {
            payload.insert(QStringLiteral("route"), QStringLiteral("template"));
            payload.insert(QStringLiteral("intent"), QStringLiteral("query_music_tab_item"));
            payload.insert(QStringLiteral("arguments"),
                           QVariantMap{{QStringLiteral("tab"), parsedTab},
                                       {QStringLiteral("useSelectedTrack"), true}});
            payload.insert(QStringLiteral("reasonCode"), QStringLiteral("music_tab_item_from_restricted"));
            return payload;
        }
    }

    if (intent == QStringLiteral("query_ui_overview")) {
        const QString page = parsePageKey(userMessage);
        if (!page.isEmpty() && containsAny(userMessage, {QStringLiteral("页面"), QStringLiteral("界面"), QStringLiteral("状态")})) {
            payload.insert(QStringLiteral("intent"), QStringLiteral("query_ui_page_state"));
            payload.insert(QStringLiteral("arguments"), QVariantMap{{QStringLiteral("page"), page}});
            payload.insert(QStringLiteral("reasonCode"), QStringLiteral("ui_page_state_normalized"));
        }
    }

    if (intent == QStringLiteral("query_ui_page_state")) {
        const QString page = parsePageKey(userMessage);
        if (page.isEmpty() && isUiOverviewRequest(userMessage)) {
            payload.insert(QStringLiteral("intent"), QStringLiteral("query_ui_overview"));
            payload.insert(QStringLiteral("arguments"), QVariantMap());
            payload.insert(QStringLiteral("reasonCode"), QStringLiteral("ui_overview_without_explicit_page"));
        }
    }
    return payload;
}

} // namespace

AgentLocalModelGateway::AgentLocalModelGateway(QObject* parent)
    : QObject(parent)
{
}

void AgentLocalModelGateway::compileIntent(const QString& requestId,
                                           const QString& userMessage,
                                           const QVariantMap& hostContext,
                                           const QVariantList& capabilityItems,
                                           const QVariantMap& memorySnapshot)
{
    const QVariantMap projection = buildControlCapabilityProjection(capabilityItems);
    const QVariantMap payload{{QStringLiteral("userMessage"), userMessage},
                              {QStringLiteral("hostContext"), hostContext},
                              {QStringLiteral("capabilitySnapshot"), projection},
                              {QStringLiteral("memory"), memorySnapshot}};
    const QString userPayload = QString::fromUtf8(
        QJsonDocument(QJsonObject::fromVariantMap(payload)).toJson(QJsonDocument::Compact));

    auto* watcher = new QFutureWatcher<GatewayTaskResult>(this);
    connect(watcher, &QFutureWatcher<GatewayTaskResult>::finished, this, [this, watcher]() {
        const GatewayTaskResult result = watcher->result();
        watcher->deleteLater();
        if (!result.ok) {
            emit requestFailed(result.requestId, result.code, result.message);
            return;
        }
        emit controlIntentReady(result.requestId, result.payload);
    });

    watcher->setFuture(QtConcurrent::run([this, requestId, userPayload, userMessage]() -> GatewayTaskResult {
        GatewayTaskResult task;
        task.requestId = requestId;

        AgentEmbeddedLlamaEngine::Request request;
        request.systemPrompt = QString::fromUtf8(kControlCompilerPrompt);
        request.userPrompt = userPayload;
        request.maxTokens = 256;
        request.temperature = 0.1f;
        request.deterministic = true;

        const AgentEmbeddedLlamaEngine::Result modelResult =
            AgentEmbeddedLlamaEngine::instance().generate(request);
        if (!modelResult.ok) {
            task.code = modelResult.code;
            task.message = modelResult.message;
            return task;
        }

        QString errorMessage;
        task.payload = extractJsonObject(modelResult.text, &errorMessage);
        if (task.payload.isEmpty()) {
            task.code = QStringLiteral("format_error");
            task.message = errorMessage.isEmpty()
                ? QStringLiteral("本地模型未返回合法 JSON 控制意图。")
                : errorMessage;
            return task;
        }

        task.payload = normalizeCompiledIntent(userMessage, task.payload);

        task.ok = true;
        return task;
    }));
}

void AgentLocalModelGateway::generateAssistantReply(const QString& requestId,
                                                    const QString& userMessage,
                                                    const QVariantMap& hostContext)
{
    const QVariantMap payload{{QStringLiteral("userMessage"), userMessage},
                              {QStringLiteral("hostContext"), hostContext}};
    const QString userPayload = QString::fromUtf8(
        QJsonDocument(QJsonObject::fromVariantMap(payload)).toJson(QJsonDocument::Compact));

    auto* watcher = new QFutureWatcher<GatewayTaskResult>(this);
    connect(watcher, &QFutureWatcher<GatewayTaskResult>::finished, this, [this, watcher]() {
        const GatewayTaskResult result = watcher->result();
        watcher->deleteLater();
        if (!result.ok) {
            emit requestFailed(result.requestId, result.code, result.message);
            return;
        }
        emit assistantReplyReady(result.requestId, result.text);
    });

    watcher->setFuture(QtConcurrent::run([requestId, userPayload]() -> GatewayTaskResult {
        GatewayTaskResult task;
        task.requestId = requestId;
        task.assistant = true;

        AgentEmbeddedLlamaEngine::Request request;
        request.systemPrompt = QString::fromUtf8(kAssistantPrompt);
        request.userPrompt = userPayload;
        request.maxTokens = 320;
        request.temperature = 0.3f;
        request.deterministic = false;

        const AgentEmbeddedLlamaEngine::Result modelResult =
            AgentEmbeddedLlamaEngine::instance().generate(request);
        if (!modelResult.ok) {
            task.code = modelResult.code;
            task.message = modelResult.message;
            return task;
        }

        task.ok = true;
        task.text = modelResult.text.trimmed().isEmpty()
            ? QStringLiteral("助手没有返回有效内容，请换个问法试试。")
            : modelResult.text.trimmed();
        return task;
    }));
}

QVariantMap AgentLocalModelGateway::buildControlCapabilityProjection(const QVariantList& capabilityItems) const
{
    QSet<QString> toolNames;
    QVariantList tools;
    for (const QVariant& value : capabilityItems) {
        const QVariantMap item = value.toMap();
        const QString name = item.value(QStringLiteral("name")).toString().trimmed();
        if (name.isEmpty()) {
            continue;
        }
        toolNames.insert(name);
        tools.push_back(item);
    }

    auto hasTool = [&](const QStringList& names) {
        for (const QString& name : names) {
            if (toolNames.contains(name)) {
                return true;
            }
        }
        return false;
    };

    QStringList intents;
    if (hasTool({QStringLiteral("pausePlayback")})) intents << QStringLiteral("pause_playback");
    if (hasTool({QStringLiteral("resumePlayback")})) intents << QStringLiteral("resume_playback");
    if (hasTool({QStringLiteral("stopPlayback")})) intents << QStringLiteral("stop_playback");
    if (hasTool({QStringLiteral("playNext")})) intents << QStringLiteral("play_next");
    if (hasTool({QStringLiteral("playPrevious")})) intents << QStringLiteral("play_previous");
    if (hasTool({QStringLiteral("setVolume")})) intents << QStringLiteral("set_volume");
    if (hasTool({QStringLiteral("setPlayMode")})) intents << QStringLiteral("set_play_mode");
    if (hasTool({QStringLiteral("getLocalTracks")})) intents << QStringLiteral("query_local_tracks");
    if (hasTool({QStringLiteral("getPlaylists"), QStringLiteral("getPlaylistTracks")})) intents << QStringLiteral("query_playlist_tracks");
    if (hasTool({QStringLiteral("getUiOverview")})) intents << QStringLiteral("query_ui_overview");
    if (hasTool({QStringLiteral("getUiPageState")})) intents << QStringLiteral("query_ui_page_state");
    if (hasTool({QStringLiteral("getMusicTabItems")})) intents << QStringLiteral("query_music_tab_items");
    if (hasTool({QStringLiteral("getMusicTabItem")})) intents << QStringLiteral("query_music_tab_item");
    if (hasTool({QStringLiteral("playMusicTabTrack")})) intents << QStringLiteral("play_music_tab_track");
    if (hasTool({QStringLiteral("invokeMusicTabAction")})) intents << QStringLiteral("act_on_music_tab_track");
    if (hasTool({QStringLiteral("searchTracks"), QStringLiteral("playTrack")})) intents << QStringLiteral("play_track_by_query");
    if (hasTool({QStringLiteral("getCurrentTrack")})) intents << QStringLiteral("query_current_playback");
    if (hasTool({QStringLiteral("createPlaylist")})) intents << QStringLiteral("create_playlist_with_confirmation");

    QSet<QString> domains;
    QStringList writesRequireConfirmation;
    QSet<QString> loginDomains;
    for (const QVariant& value : tools) {
        const QVariantMap item = value.toMap();
        const QString name = item.value(QStringLiteral("name")).toString().trimmed();
        const QString domain = item.value(QStringLiteral("domain")).toString().trimmed();
        if (!domain.isEmpty()) {
            domains.insert(domain);
        }
        const bool readOnly = item.value(QStringLiteral("readOnly"), true).toBool();
        const QString confirmPolicy =
            item.value(QStringLiteral("confirmPolicy")).toString().trimmed().toLower();
        if (!readOnly && confirmPolicy != QStringLiteral("none") && !name.isEmpty()) {
            writesRequireConfirmation << name;
        }
        const QString availability =
            item.value(QStringLiteral("availabilityPolicy")).toString().trimmed().toLower();
        if ((availability == QStringLiteral("login_required") ||
             item.value(QStringLiteral("requiresLogin")).toBool()) &&
            !domain.isEmpty()) {
            loginDomains.insert(domain);
        }
    }

    return {{QStringLiteral("catalogVersion"), QStringLiteral("qt_tool_registry_v1")},
            {QStringLiteral("availableIntents"), intents},
            {QStringLiteral("availableDomains"), QStringList(domains.values())},
            {QStringLiteral("writesRequireConfirmation"), writesRequireConfirmation},
            {QStringLiteral("requiresLoginDomains"), QStringList(loginDomains.values())},
            {QStringLiteral("assistantWriteBlocked"), true},
            {QStringLiteral("uiPages"),
             QStringList{QStringLiteral("recommend"),
                         QStringLiteral("local_download"),
                         QStringLiteral("online_music"),
                         QStringLiteral("history"),
                         QStringLiteral("favorites"),
                         QStringLiteral("playlists"),
                         QStringLiteral("user_profile"),
                         QStringLiteral("video"),
                         QStringLiteral("settings")}},
            {QStringLiteral("musicTabs"),
             QStringList{QStringLiteral("recommend"),
                         QStringLiteral("local_music"),
                         QStringLiteral("downloaded_music"),
                         QStringLiteral("downloading_tasks"),
                         QStringLiteral("online_music"),
                         QStringLiteral("history"),
                         QStringLiteral("favorites"),
                         QStringLiteral("playlist_list_owned"),
                         QStringLiteral("playlist_list_subscribed"),
                         QStringLiteral("playlist_detail")}},
            {QStringLiteral("musicTabActions"),
             QStringList{QStringLiteral("toggle_current_playback"),
                         QStringLiteral("play_next"),
                         QStringLiteral("add_to_playlist"),
                         QStringLiteral("download"),
                         QStringLiteral("add_favorite"),
                         QStringLiteral("remove_favorite"),
                         QStringLiteral("remove_or_delete")}}};
}

QVariantMap AgentLocalModelGateway::extractJsonObject(const QString& text, QString* errorMessage) const
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("本地模型返回为空。");
        }
        return {};
    }

    auto parseObject = [&](const QString& candidate) -> QVariantMap {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(candidate.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            return {};
        }
        return doc.object().toVariantMap();
    };

    QVariantMap payload = parseObject(trimmed);
    if (!payload.isEmpty()) {
        return payload;
    }

    const int start = trimmed.indexOf(QLatin1Char('{'));
    const int end = trimmed.lastIndexOf(QLatin1Char('}'));
    if (start < 0 || end <= start) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("本地模型响应中未找到 JSON 对象。");
        }
        return {};
    }

    payload = parseObject(trimmed.mid(start, end - start + 1));
    if (payload.isEmpty() && errorMessage) {
        *errorMessage = QStringLiteral("本地模型响应中的 JSON 无法解析。");
    }
    return payload;
}
