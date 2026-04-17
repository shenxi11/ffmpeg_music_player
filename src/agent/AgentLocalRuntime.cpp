#include "AgentLocalRuntime.h"

#include "AgentLocalModelGateway.h"
#include "capability/AgentCapabilityFacade.h"
#include "settings_manager.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUuid>

namespace {

QString nextPlanId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString stringifyReason(const QVariantMap& payload)
{
    return QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(payload)).toJson(QJsonDocument::Compact));
}

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
    text.remove(QRegularExpression(QStringLiteral("^我的")));
    text.remove(QRegularExpression(QStringLiteral("^我(?:的)?")));
    text.remove(QRegularExpression(QStringLiteral("^当前")));
    text.remove(QRegularExpression(QStringLiteral("^这个")));
    text.remove(QRegularExpression(QStringLiteral("^(?:一个|那个|这份)")));
    text.remove(QRegularExpression(QStringLiteral("(?:里|中|中的|里面|内)$")));
    return text.trimmed();
}

QString extractFirstSelectedTrackId(const QVariantMap& hostContext)
{
    const QVariantList selectedTrackIds = hostContext.value(QStringLiteral("selectedTrackIds")).toList();
    if (selectedTrackIds.isEmpty()) {
        return {};
    }
    return selectedTrackIds.first().toString().trimmed();
}

QString effectiveMusicTab(const QVariantMap& arguments, const QVariantMap& hostContext)
{
    const QString requestedTab = arguments.value(QStringLiteral("tab")).toString().trimmed().toLower();
    if (!requestedTab.isEmpty()) {
        return requestedTab;
    }
    return hostContext.value(QStringLiteral("currentMusicTab")).toString().trimmed().toLower();
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

QVariantMap enrichTrackArguments(QVariantMap arguments, const QVariantMap& hostContext)
{
    if (arguments.value(QStringLiteral("useSelectedTrack")).toBool()
        && arguments.value(QStringLiteral("trackId")).toString().trimmed().isEmpty()) {
        const QString selectedTrackId = extractFirstSelectedTrackId(hostContext);
        if (!selectedTrackId.isEmpty()) {
            arguments.insert(QStringLiteral("trackId"), selectedTrackId);
        }
    }

    if (arguments.value(QStringLiteral("playlistId")).toString().trimmed().isEmpty()
        && arguments.value(QStringLiteral("playlistName")).toString().trimmed().isEmpty()) {
        const QVariantMap selectedPlaylist = hostContext.value(QStringLiteral("selectedPlaylist")).toMap();
        const QString playlistId = selectedPlaylist.value(QStringLiteral("playlistId")).toString().trimmed();
        const QString playlistName = selectedPlaylist.value(QStringLiteral("name")).toString().trimmed();
        if (!playlistId.isEmpty()) {
            arguments.insert(QStringLiteral("playlistId"), playlistId);
        } else if (!playlistName.isEmpty()) {
            arguments.insert(QStringLiteral("playlistName"), playlistName);
        }
    }

    return arguments;
}

}

AgentLocalRuntime::AgentLocalRuntime(AgentCapabilityFacade* capabilityFacade, QObject* parent)
    : QObject(parent)
    , m_capabilityFacade(capabilityFacade)
    , m_modelGateway(new AgentLocalModelGateway(this))
{
    connect(m_modelGateway,
            &AgentLocalModelGateway::controlIntentReady,
            this,
            &AgentLocalRuntime::onControlIntentReady);
    connect(m_modelGateway,
            &AgentLocalModelGateway::assistantReplyReady,
            this,
            &AgentLocalRuntime::onAssistantReplyReady);
    connect(m_modelGateway,
            &AgentLocalModelGateway::requestFailed,
            this,
            &AgentLocalRuntime::onModelRequestFailed);
    if (m_capabilityFacade) {
        connect(m_capabilityFacade,
                &AgentCapabilityFacade::capabilityResultReady,
                this,
                &AgentLocalRuntime::onCapabilityResultReady);
    }
}

void AgentLocalRuntime::sendUserMessage(const QString& sessionId,
                                        const QString& requestId,
                                        const QString& userMessage,
                                        const QVariantMap& hostContext,
                                        const QVariantList& capabilityItems)
{
    const QString trimmedSessionId = sessionId.trimmed();
    const QString trimmedRequestId = requestId.trimmed();
    const QString trimmedMessage = userMessage.trimmed();
    if (trimmedSessionId.isEmpty() || trimmedRequestId.isEmpty() || trimmedMessage.isEmpty()) {
        emit requestError(trimmedRequestId,
                          QStringLiteral("invalid_request"),
                          QStringLiteral("会话或消息内容不能为空。"));
        return;
    }

    for (auto it = m_pendingApprovalByPlanId.begin(); it != m_pendingApprovalByPlanId.end();) {
        if (it->sessionId == trimmedSessionId && isReadOnlyIntent(matchFastIntent(trimmedMessage))) {
            it = m_pendingApprovalByPlanId.erase(it);
        } else {
            ++it;
        }
    }

    emit assistantStartReceived(trimmedRequestId);

    if (SettingsManager::instance().agentMode().trimmed().compare(QStringLiteral("assistant"), Qt::CaseInsensitive) == 0) {
        m_modelGateway->generateAssistantReply(trimmedRequestId, trimmedMessage, hostContext);
        return;
    }

    const QVariantMap fastIntent = matchFastIntent(trimmedMessage);
    if (!fastIntent.isEmpty()) {
        if (dispatchIntent(trimmedSessionId, trimmedRequestId, trimmedMessage, hostContext, fastIntent)) {
            return;
        }
    }

    m_pendingCompileContext.insert(trimmedRequestId,
                                   QVariantMap{{QStringLiteral("sessionId"), trimmedSessionId},
                                               {QStringLiteral("requestId"), trimmedRequestId},
                                               {QStringLiteral("userMessage"), trimmedMessage},
                                               {QStringLiteral("hostContext"), hostContext},
                                               {QStringLiteral("capabilityItems"), capabilityItems}});
    m_modelGateway->compileIntent(trimmedRequestId,
                                  trimmedMessage,
                                  hostContext,
                                  capabilityItems,
                                  QVariantMap());
}

void AgentLocalRuntime::sendApprovalResponse(const QString& sessionId,
                                             const QString& planId,
                                             bool approved,
                                             const QString& reason)
{
    Q_UNUSED(sessionId);
    const QString trimmedPlanId = planId.trimmed();
    if (!m_pendingApprovalByPlanId.contains(trimmedPlanId)) {
        emit requestError(QString(),
                          QStringLiteral("approval_missing"),
                          QStringLiteral("未找到待审批的计划。"));
        return;
    }

    const PendingPlan plan = m_pendingApprovalByPlanId.take(trimmedPlanId);
    if (!approved) {
        completePlanFailure(plan,
                            reason.trimmed().isEmpty() ? QStringLiteral("已取消本次操作。") : reason.trimmed());
        return;
    }

    if (plan.intent == QStringLiteral("create_playlist_with_confirmation")) {
        dispatchToolPlan(plan,
                         QStringLiteral("createPlaylist"),
                         QVariantMap{{QStringLiteral("name"), plan.arguments.value(QStringLiteral("playlistName")).toString().trimmed()}},
                         QStringLiteral("正在创建歌单..."));
        return;
    }

    completePlanFailure(plan, QStringLiteral("当前内嵌运行时暂未支持该确认型动作。"));
}

void AgentLocalRuntime::onControlIntentReady(const QString& requestId, const QVariantMap& intentPayload)
{
    const QVariantMap context = m_pendingCompileContext.take(requestId);
    if (context.isEmpty()) {
        return;
    }

    if (!dispatchIntent(context.value(QStringLiteral("sessionId")).toString(),
                        requestId,
                        context.value(QStringLiteral("userMessage")).toString(),
                        context.value(QStringLiteral("hostContext")).toMap(),
                        intentPayload)) {
        emit requestError(requestId,
                          QStringLiteral("unsupported_intent"),
                          QStringLiteral("当前内嵌运行时尚未支持这条控制意图。"));
    }
}

void AgentLocalRuntime::onAssistantReplyReady(const QString& requestId, const QString& text)
{
    emit assistantFinalReceived(requestId,
                                text.trimmed().isEmpty() ? QStringLiteral("助手没有返回有效内容，请换个问法试试。") : text);
}

void AgentLocalRuntime::onModelRequestFailed(const QString& requestId, const QString& code, const QString& message)
{
    m_pendingCompileContext.remove(requestId);
    emit requestError(requestId, code, message);
}

void AgentLocalRuntime::onCapabilityResultReady(const QString& toolCallId,
                                                bool ok,
                                                const QVariantMap& result,
                                                const QVariantMap& error)
{
    const auto it = m_pendingPlanByToolCallId.find(toolCallId);
    if (it == m_pendingPlanByToolCallId.end()) {
        return;
    }

    PendingPlan plan = it.value();
    m_pendingPlanByToolCallId.erase(it);

    if (!ok) {
        completePlanFailure(plan,
                            error.value(QStringLiteral("message")).toString().trimmed().isEmpty()
                                ? QStringLiteral("客户端工具执行失败。")
                                : error.value(QStringLiteral("message")).toString().trimmed());
        return;
    }

    if (plan.intent == QStringLiteral("pause_playback")) {
        completePlanSuccess(plan, QStringLiteral("已暂停播放。"));
        return;
    }
    if (plan.intent == QStringLiteral("resume_playback")) {
        completePlanSuccess(plan, QStringLiteral("已恢复播放。"));
        return;
    }
    if (plan.intent == QStringLiteral("stop_playback")) {
        completePlanSuccess(plan, QStringLiteral("已停止播放。"));
        return;
    }
    if (plan.intent == QStringLiteral("play_next")) {
        completePlanSuccess(plan, QStringLiteral("已切到下一首。"));
        return;
    }
    if (plan.intent == QStringLiteral("play_previous")) {
        completePlanSuccess(plan, QStringLiteral("已切到上一首。"));
        return;
    }
    if (plan.intent == QStringLiteral("set_volume")) {
        completePlanSuccess(plan,
                            QStringLiteral("已将音量调整为 %1。")
                                .arg(result.value(QStringLiteral("volume")).toInt()));
        return;
    }
    if (plan.intent == QStringLiteral("set_play_mode")) {
        completePlanSuccess(plan,
                            QStringLiteral("已切换播放模式为 %1。")
                                .arg(result.value(QStringLiteral("playModeName")).toString()));
        return;
    }
    if (plan.intent == QStringLiteral("query_local_tracks")) {
        completePlanSuccess(plan, summarizeLocalTracks(result));
        return;
    }
    if (plan.intent == QStringLiteral("query_playlist_tracks")) {
        if (plan.currentTool == QStringLiteral("getPlaylists")) {
            const QVariantList playlists = result.value(QStringLiteral("items")).toList();
            QString resolveError;
            const QVariantMap selectedPlaylist =
                resolvePlaylistSelection(playlists, plan.arguments, plan.hostContext, &resolveError);
            if (selectedPlaylist.isEmpty()) {
                completePlanFailure(plan,
                                    resolveError.isEmpty()
                                        ? QStringLiteral("没有找到匹配的歌单。")
                                        : resolveError);
                return;
            }

            const QString playlistId = selectedPlaylist.value(QStringLiteral("playlistId")).toString().trimmed();
            bool playlistIdOk = false;
            const qlonglong playlistIdValue = playlistId.toLongLong(&playlistIdOk);
            if (!playlistIdOk || playlistIdValue <= 0) {
                completePlanFailure(plan, QStringLiteral("目标歌单缺少有效的 playlistId。"));
                return;
            }

            plan.cachedPlaylist = selectedPlaylist;
            dispatchToolPlan(plan,
                             QStringLiteral("getPlaylistTracks"),
                             QVariantMap{{QStringLiteral("playlistId"), playlistIdValue}},
                             QStringLiteral("正在读取歌单内容..."));
            return;
        }

        completePlanSuccess(plan, summarizePlaylistTracks(result));
        return;
    }
    if (plan.intent == QStringLiteral("query_current_playback")) {
        completePlanSuccess(plan, summarizeCurrentPlayback(result));
        return;
    }
    if (plan.intent == QStringLiteral("query_ui_overview")) {
        completePlanSuccess(plan, summarizeUiOverview(result));
        return;
    }
    if (plan.intent == QStringLiteral("query_ui_page_state")) {
        completePlanSuccess(plan, summarizeUiPageState(result));
        return;
    }
    if (plan.intent == QStringLiteral("query_music_tab_items")) {
        completePlanSuccess(plan, summarizeMusicTabItems(plan, result));
        return;
    }
    if (plan.intent == QStringLiteral("query_music_tab_item")) {
        completePlanSuccess(plan, summarizeMusicTabItem(result));
        return;
    }
    if (plan.intent == QStringLiteral("play_music_tab_track")) {
        completePlanSuccess(plan, summarizePlayedMusicTabTrack(result));
        return;
    }
    if (plan.intent == QStringLiteral("act_on_music_tab_track")) {
        completePlanSuccess(plan, summarizeMusicTabAction(result));
        return;
    }
    if (plan.intent == QStringLiteral("create_playlist_with_confirmation")) {
        const QString playlistName = plan.arguments.value(QStringLiteral("playlistName")).toString().trimmed();
        completePlanSuccess(plan,
                            playlistName.isEmpty()
                                ? QStringLiteral("已创建歌单。")
                                : QStringLiteral("已创建歌单“%1”。").arg(playlistName));
        return;
    }
    if (plan.intent == QStringLiteral("play_track_by_query")) {
        if (plan.currentTool == QStringLiteral("searchTracks")) {
            const QVariantList items = result.value(QStringLiteral("items")).toList();
            if (items.isEmpty()) {
                completePlanFailure(plan, QStringLiteral("没有找到符合条件的歌曲。"));
                return;
            }
            const QVariantMap first = items.first().toMap();
            plan.currentTool.clear();
            plan.cachedSearchResult = first;
            QVariantMap playArgs;
            const QString trackId = first.value(QStringLiteral("trackId")).toString().trimmed();
            const QString musicPath = first.value(QStringLiteral("musicPath")).toString().trimmed();
            if (!trackId.isEmpty()) {
                playArgs.insert(QStringLiteral("trackId"), trackId);
            }
            if (!musicPath.isEmpty()) {
                playArgs.insert(QStringLiteral("musicPath"), musicPath);
            }
            dispatchToolPlan(plan,
                             QStringLiteral("playTrack"),
                             playArgs,
                             QStringLiteral("正在播放搜索结果中的歌曲..."));
            return;
        }
        const QString title = plan.cachedSearchResult.value(QStringLiteral("title")).toString();
        const QString artist = plan.cachedSearchResult.value(QStringLiteral("artist")).toString();
        completePlanSuccess(plan,
                            artist.trimmed().isEmpty()
                                ? QStringLiteral("已开始播放《%1》。").arg(title)
                                : QStringLiteral("已开始播放《%1》 - %2。")
                                      .arg(title, artist));
        return;
    }

    completePlanSuccess(plan, QStringLiteral("操作已完成。"));
}

QVariantMap AgentLocalRuntime::matchFastIntent(const QString& userMessage) const
{
    const QString normalized = userMessage.trimmed();
    if (containsAny(normalized, {QStringLiteral("暂停"), QStringLiteral("暂停播放"), QStringLiteral("停一下")})) {
        return {{QStringLiteral("route"), QStringLiteral("execute")},
                {QStringLiteral("intent"), QStringLiteral("pause_playback")},
                {QStringLiteral("arguments"), QVariantMap()}};
    }
    if (containsAny(normalized, {QStringLiteral("继续播放"), QStringLiteral("恢复播放"), QStringLiteral("继续"), QStringLiteral("接着放")})) {
        return {{QStringLiteral("route"), QStringLiteral("execute")},
                {QStringLiteral("intent"), QStringLiteral("resume_playback")},
                {QStringLiteral("arguments"), QVariantMap()}};
    }
    if (containsAny(normalized, {QStringLiteral("停止播放"), QStringLiteral("停止"), QStringLiteral("停掉"), QStringLiteral("别放了")})) {
        return {{QStringLiteral("route"), QStringLiteral("execute")},
                {QStringLiteral("intent"), QStringLiteral("stop_playback")},
                {QStringLiteral("arguments"), QVariantMap()}};
    }
    if (containsAny(normalized, {QStringLiteral("下一首"), QStringLiteral("下首")})) {
        return {{QStringLiteral("route"), QStringLiteral("execute")},
                {QStringLiteral("intent"), QStringLiteral("play_next")},
                {QStringLiteral("arguments"), QVariantMap()}};
    }
    if (containsAny(normalized, {QStringLiteral("上一首"), QStringLiteral("上首")})) {
        return {{QStringLiteral("route"), QStringLiteral("execute")},
                {QStringLiteral("intent"), QStringLiteral("play_previous")},
                {QStringLiteral("arguments"), QVariantMap()}};
    }

    QRegularExpression volumeRegex(QStringLiteral("(?:音量|声音)\\D{0,4}(\\d{1,3})"));
    const QRegularExpressionMatch volumeMatch = volumeRegex.match(normalized);
    if (volumeMatch.hasMatch()) {
        return {{QStringLiteral("route"), QStringLiteral("execute")},
                {QStringLiteral("intent"), QStringLiteral("set_volume")},
                {QStringLiteral("arguments"), QVariantMap{{QStringLiteral("volume"), qBound(0, volumeMatch.captured(1).toInt(), 100)}}}};
    }

    const struct { const char* mode; const char* words[3]; } playModes[] = {
        {"repeat_one", {"单曲循环", nullptr, nullptr}},
        {"repeat_all", {"列表循环", "顺序循环", nullptr}},
        {"shuffle", {"随机播放", "随机模式", "打乱播放"}},
        {"sequential", {"顺序播放", "顺序模式", nullptr}},
    };
    for (const auto& mode : playModes) {
        for (const char* word : mode.words) {
            if (word && normalized.contains(QString::fromUtf8(word))) {
                return {{QStringLiteral("route"), QStringLiteral("execute")},
                        {QStringLiteral("intent"), QStringLiteral("set_play_mode")},
                        {QStringLiteral("arguments"), QVariantMap{{QStringLiteral("mode"), QString::fromLatin1(mode.mode)}}}};
            }
        }
    }

    const bool mentionsPlaylist =
        containsAny(normalized,
                    {QStringLiteral("歌单"),
                     QStringLiteral("playlist"),
                     QStringLiteral("收藏夹"),
                     QStringLiteral("当前歌单")});
    if (!mentionsPlaylist
        && containsAny(normalized, {QStringLiteral("本地音乐"), QStringLiteral("本地歌曲"), QStringLiteral("本地歌"), QStringLiteral("本地列表")})
        && containsAny(normalized, {QStringLiteral("列出"), QStringLiteral("看看"), QStringLiteral("显示"), QStringLiteral("有哪些"), QStringLiteral("查看"), QStringLiteral("浏览")})) {
        return {{QStringLiteral("route"), QStringLiteral("template")},
                {QStringLiteral("intent"), QStringLiteral("query_local_tracks")},
                {QStringLiteral("arguments"), QVariantMap{{QStringLiteral("limit"), 20}}}};
    }

    if (containsAny(normalized,
                    {QStringLiteral("歌单"), QStringLiteral("playlist"), QStringLiteral("收藏夹")})
        && containsAny(normalized,
                       {QStringLiteral("列出"),
                        QStringLiteral("看看"),
                        QStringLiteral("显示"),
                        QStringLiteral("有哪些"),
                        QStringLiteral("查看"),
                        QStringLiteral("浏览"),
                        QStringLiteral("所有歌曲"),
                        QStringLiteral("内容")})) {
        QVariantMap arguments{{QStringLiteral("limit"), 100}};
        if (containsAny(normalized, {QStringLiteral("当前歌单"), QStringLiteral("这个歌单")})) {
            arguments.insert(QStringLiteral("useCurrentPlaylist"), true);
        } else {
            QString playlistName = normalized;
            playlistName.remove(QRegularExpression(QStringLiteral("^(?:请)?(?:帮我)?(?:列出|看看|显示|查看|浏览)")));
            playlistName.remove(QRegularExpression(QStringLiteral("(?:的)?(?:所有)?(?:歌曲|内容|曲目).*$")));
            playlistName.remove(QRegularExpression(QStringLiteral("(?:里|中|中的|里面)$")));
            playlistName = normalizedPlaylistName(playlistName);
            if (!playlistName.isEmpty()) {
                arguments.insert(QStringLiteral("playlistName"), playlistName);
            }
        }
        return {{QStringLiteral("route"), QStringLiteral("template")},
                {QStringLiteral("intent"), QStringLiteral("query_playlist_tracks")},
                {QStringLiteral("arguments"), arguments}};
    }

    if (containsAny(normalized, {QStringLiteral("当前播放"), QStringLiteral("现在播放"), QStringLiteral("播放队列"), QStringLiteral("队列里")})) {
        return {{QStringLiteral("route"), QStringLiteral("template")},
                {QStringLiteral("intent"), QStringLiteral("query_current_playback")},
                {QStringLiteral("arguments"), QVariantMap()}};
    }

    const QString pageKey = parsePageKey(normalized);
    if (isUiOverviewRequest(normalized) && pageKey.isEmpty()) {
        return {{QStringLiteral("route"), QStringLiteral("template")},
                {QStringLiteral("intent"), QStringLiteral("query_ui_overview")},
                {QStringLiteral("arguments"), QVariantMap()}};
    }

    if (!pageKey.isEmpty()
        && containsAny(normalized, {QStringLiteral("页面"), QStringLiteral("界面"), QStringLiteral("状态")})) {
        return {{QStringLiteral("route"), QStringLiteral("template")},
                {QStringLiteral("intent"), QStringLiteral("query_ui_page_state")},
                {QStringLiteral("arguments"), QVariantMap{{QStringLiteral("page"), pageKey}}}};
    }

    const QString tabKey = parseMusicTabKey(normalized);
    if (!tabKey.isEmpty()
        && tabKey != QStringLiteral("playlist_detail")
        && isMusicTabListRequest(normalized)
        && !containsAny(normalized, {QStringLiteral("歌单"), QStringLiteral("playlist")})) {
        QVariantMap arguments{{QStringLiteral("tab"), tabKey}};
        const int limit = parseRequestedLimit(normalized);
        if (limit > 0) {
            arguments.insert(QStringLiteral("limit"), limit);
        }
        return {{QStringLiteral("route"), QStringLiteral("template")},
                {QStringLiteral("intent"), QStringLiteral("query_music_tab_items")},
                {QStringLiteral("arguments"), arguments}};
    }

    if (!tabKey.isEmpty()
        && containsAny(normalized, {QStringLiteral("信息"), QStringLiteral("详情"), QStringLiteral("资料")})
        && containsAny(normalized, {QStringLiteral("这首歌"), QStringLiteral("当前这首"), QStringLiteral("选中的歌")})) {
        return {{QStringLiteral("route"), QStringLiteral("template")},
                {QStringLiteral("intent"), QStringLiteral("query_music_tab_item")},
                {QStringLiteral("arguments"), QVariantMap{{QStringLiteral("tab"), tabKey},
                                                         {QStringLiteral("useSelectedTrack"), true}}}};
    }

    if (!tabKey.isEmpty()
        && containsAny(normalized, {QStringLiteral("播放"), QStringLiteral("放一下"), QStringLiteral("来一首")})
        && containsAny(normalized, {QStringLiteral("这首歌"), QStringLiteral("当前这首"), QStringLiteral("选中的歌")})) {
        return {{QStringLiteral("route"), QStringLiteral("template")},
                {QStringLiteral("intent"), QStringLiteral("play_music_tab_track")},
                {QStringLiteral("arguments"), QVariantMap{{QStringLiteral("tab"), tabKey},
                                                         {QStringLiteral("useSelectedTrack"), true}}}};
    }

    if (!tabKey.isEmpty()) {
        const QString action = parseMusicTabAction(normalized);
        if (!action.isEmpty()
            && containsAny(normalized, {QStringLiteral("这首歌"), QStringLiteral("当前这首"), QStringLiteral("选中的歌")})) {
            return {{QStringLiteral("route"), QStringLiteral("template")},
                    {QStringLiteral("intent"), QStringLiteral("act_on_music_tab_track")},
                    {QStringLiteral("arguments"), QVariantMap{{QStringLiteral("tab"), tabKey},
                                                             {QStringLiteral("action"), action},
                                                             {QStringLiteral("useSelectedTrack"), true}}}};
        }
    }

    QRegularExpression createPlaylistRegex(QStringLiteral("创建(?:一个)?歌单(?:，|,| )?(?:歌单名(?:为)?|名字(?:为)?)?(.+)"));
    const QRegularExpressionMatch createPlaylistMatch = createPlaylistRegex.match(normalized);
    if (createPlaylistMatch.hasMatch()) {
        const QString playlistName = createPlaylistMatch.captured(1).trimmed();
        if (!playlistName.isEmpty()) {
            return {{QStringLiteral("route"), QStringLiteral("template")},
                    {QStringLiteral("intent"), QStringLiteral("create_playlist_with_confirmation")},
                    {QStringLiteral("arguments"), QVariantMap{{QStringLiteral("playlistName"), playlistName}}},
                    {QStringLiteral("confirmation"), true}};
        }
    }

    if (containsAny(normalized, {QStringLiteral("播放"), QStringLiteral("来一首"), QStringLiteral("放一首")})) {
        QString query = normalized;
        query.remove(QStringLiteral("播放"));
        query.remove(QStringLiteral("来一首"));
        query.remove(QStringLiteral("放一首"));
        query = query.trimmed();
        if (!query.isEmpty()) {
            return {{QStringLiteral("route"), QStringLiteral("template")},
                    {QStringLiteral("intent"), QStringLiteral("play_track_by_query")},
                    {QStringLiteral("arguments"), QVariantMap{{QStringLiteral("keyword"), query}}}};
        }
    }

    return {};
}

bool AgentLocalRuntime::isReadOnlyIntent(const QVariantMap& intentPayload) const
{
    const QString intent = intentPayload.value(QStringLiteral("intent")).toString().trimmed();
    return intent == QStringLiteral("query_local_tracks")
           || intent == QStringLiteral("query_playlist_tracks")
           || intent == QStringLiteral("query_ui_overview")
           || intent == QStringLiteral("query_ui_page_state")
           || intent == QStringLiteral("query_music_tab_items")
           || intent == QStringLiteral("query_music_tab_item")
           || intent == QStringLiteral("query_current_playback")
           || intent == QStringLiteral("restricted");
}

bool AgentLocalRuntime::dispatchIntent(const QString& sessionId,
                                       const QString& requestId,
                                       const QString& userMessage,
                                       const QVariantMap& hostContext,
                                       const QVariantMap& intentPayload)
{
    const QString intent = intentPayload.value(QStringLiteral("intent")).toString().trimmed();
    const QVariantMap arguments = intentPayload.value(QStringLiteral("arguments")).toMap();
    if (intent.isEmpty() || intent == QStringLiteral("restricted")) {
        emit requestError(requestId,
                          QStringLiteral("restricted"),
                          QStringLiteral("这条请求超出了当前软件控制边界，请换成更明确的播放器控制指令。"));
        return true;
    }

    PendingPlan plan;
    plan.sessionId = sessionId;
    plan.requestId = requestId;
    plan.userMessage = userMessage;
    plan.planId = nextPlanId();
    plan.intent = intent;
    plan.arguments = arguments;
    plan.hostContext = hostContext;

    if (intent == QStringLiteral("pause_playback")) {
        dispatchToolPlan(plan, QStringLiteral("pausePlayback"), QVariantMap(), QStringLiteral("正在暂停播放..."));
        return true;
    }
    if (intent == QStringLiteral("resume_playback")) {
        dispatchToolPlan(plan, QStringLiteral("resumePlayback"), QVariantMap(), QStringLiteral("正在恢复播放..."));
        return true;
    }
    if (intent == QStringLiteral("stop_playback")) {
        dispatchToolPlan(plan, QStringLiteral("stopPlayback"), QVariantMap(), QStringLiteral("正在停止播放..."));
        return true;
    }
    if (intent == QStringLiteral("play_next")) {
        dispatchToolPlan(plan, QStringLiteral("playNext"), QVariantMap(), QStringLiteral("正在切到下一首..."));
        return true;
    }
    if (intent == QStringLiteral("play_previous")) {
        dispatchToolPlan(plan, QStringLiteral("playPrevious"), QVariantMap(), QStringLiteral("正在切到上一首..."));
        return true;
    }
    if (intent == QStringLiteral("set_volume")) {
        dispatchToolPlan(plan,
                         QStringLiteral("setVolume"),
                         QVariantMap{{QStringLiteral("volume"), arguments.value(QStringLiteral("volume")).toInt()}},
                         QStringLiteral("正在调整音量..."));
        return true;
    }
    if (intent == QStringLiteral("set_play_mode")) {
        dispatchToolPlan(plan,
                         QStringLiteral("setPlayMode"),
                         QVariantMap{{QStringLiteral("mode"), arguments.value(QStringLiteral("mode")).toString()}},
                         QStringLiteral("正在切换播放模式..."));
        return true;
    }
    if (intent == QStringLiteral("query_local_tracks")) {
        dispatchToolPlan(plan,
                         QStringLiteral("getLocalTracks"),
                         QVariantMap{{QStringLiteral("limit"), arguments.value(QStringLiteral("limit"), 20).toInt()}},
                         QStringLiteral("正在读取本地音乐列表..."));
        return true;
    }
    if (intent == QStringLiteral("query_playlist_tracks")) {
        if (arguments.value(QStringLiteral("useCurrentPlaylist")).toBool()) {
            const QVariantMap selectedPlaylist = hostContext.value(QStringLiteral("selectedPlaylist")).toMap();
            const QString playlistId = selectedPlaylist.value(QStringLiteral("playlistId")).toString().trimmed();
            bool playlistIdOk = false;
            const qlonglong playlistIdValue = playlistId.toLongLong(&playlistIdOk);
            if (playlistIdOk && playlistIdValue > 0) {
                plan.cachedPlaylist = selectedPlaylist;
                dispatchToolPlan(plan,
                                 QStringLiteral("getPlaylistTracks"),
                                 QVariantMap{{QStringLiteral("playlistId"), playlistIdValue}},
                                 QStringLiteral("正在读取当前歌单内容..."));
                return true;
            }
        }
        dispatchToolPlan(plan,
                         QStringLiteral("getPlaylists"),
                         QVariantMap(),
                         QStringLiteral("正在查找目标歌单..."));
        return true;
    }
    if (intent == QStringLiteral("query_current_playback")) {
        dispatchToolPlan(plan,
                         QStringLiteral("getCurrentTrack"),
                         QVariantMap(),
                         QStringLiteral("正在读取当前播放状态..."));
        return true;
    }
    if (intent == QStringLiteral("query_ui_overview")) {
        dispatchToolPlan(plan,
                         QStringLiteral("getUiOverview"),
                         QVariantMap(),
                         QStringLiteral("正在读取当前页面状态..."));
        return true;
    }
    if (intent == QStringLiteral("query_ui_page_state")) {
        const QString page = arguments.value(QStringLiteral("page")).toString().trimmed();
        if (page.isEmpty()) {
            emit requestError(requestId,
                              QStringLiteral("invalid_args"),
                              QStringLiteral("读取页面状态时缺少 page。"));
            return true;
        }
        dispatchToolPlan(plan,
                         QStringLiteral("getUiPageState"),
                         QVariantMap{{QStringLiteral("page"), page}},
                         QStringLiteral("正在读取页面状态..."));
        return true;
    }
    if (intent == QStringLiteral("query_music_tab_items")) {
        const QString tab = effectiveMusicTab(arguments, hostContext);
        if (tab.isEmpty()) {
            emit requestError(requestId,
                              QStringLiteral("invalid_args"),
                              QStringLiteral("读取音乐 tab 列表时缺少 tab。"));
            return true;
        }
        QVariantMap toolArgs{{QStringLiteral("tab"), tab}};
        if (arguments.contains(QStringLiteral("limit"))) {
            toolArgs.insert(QStringLiteral("limit"), arguments.value(QStringLiteral("limit")));
        }
        if (arguments.contains(QStringLiteral("playlistId"))) {
            toolArgs.insert(QStringLiteral("playlistId"), arguments.value(QStringLiteral("playlistId")));
        }
        if (arguments.contains(QStringLiteral("playlistName"))) {
            toolArgs.insert(QStringLiteral("playlistName"), arguments.value(QStringLiteral("playlistName")));
        }
        dispatchToolPlan(plan,
                         QStringLiteral("getMusicTabItems"),
                         toolArgs,
                         QStringLiteral("正在读取当前 tab 列表..."));
        return true;
    }
    if (intent == QStringLiteral("query_music_tab_item")) {
        QVariantMap toolArgs = enrichTrackArguments(arguments, hostContext);
        const QString tab = effectiveMusicTab(toolArgs, hostContext);
        if (tab.isEmpty()) {
            emit requestError(requestId,
                              QStringLiteral("invalid_args"),
                              QStringLiteral("读取 tab 条目时缺少 tab。"));
            return true;
        }
        toolArgs.insert(QStringLiteral("tab"), tab);
        dispatchToolPlan(plan,
                         QStringLiteral("getMusicTabItem"),
                         toolArgs,
                         QStringLiteral("正在读取目标条目信息..."));
        return true;
    }
    if (intent == QStringLiteral("play_music_tab_track")) {
        QVariantMap toolArgs = enrichTrackArguments(arguments, hostContext);
        const QString tab = effectiveMusicTab(toolArgs, hostContext);
        if (tab.isEmpty()) {
            emit requestError(requestId,
                              QStringLiteral("invalid_args"),
                              QStringLiteral("播放 tab 歌曲时缺少 tab。"));
            return true;
        }
        toolArgs.insert(QStringLiteral("tab"), tab);
        dispatchToolPlan(plan,
                         QStringLiteral("playMusicTabTrack"),
                         toolArgs,
                         QStringLiteral("正在播放目标歌曲..."));
        return true;
    }
    if (intent == QStringLiteral("act_on_music_tab_track")) {
        QVariantMap toolArgs = enrichTrackArguments(arguments, hostContext);
        const QString tab = effectiveMusicTab(toolArgs, hostContext);
        const QString action = toolArgs.value(QStringLiteral("action")).toString().trimmed();
        if (tab.isEmpty() || action.isEmpty()) {
            emit requestError(requestId,
                              QStringLiteral("invalid_args"),
                              QStringLiteral("执行 tab 动作时缺少 tab 或 action。"));
            return true;
        }
        toolArgs.insert(QStringLiteral("tab"), tab);
        dispatchToolPlan(plan,
                         QStringLiteral("invokeMusicTabAction"),
                         toolArgs,
                         QStringLiteral("正在执行 tab 动作..."));
        return true;
    }
    if (intent == QStringLiteral("play_track_by_query")) {
        dispatchToolPlan(plan,
                         QStringLiteral("searchTracks"),
                         QVariantMap{{QStringLiteral("keyword"), arguments.value(QStringLiteral("keyword")).toString()},
                                     {QStringLiteral("artist"), arguments.value(QStringLiteral("artist")).toString()},
                                     {QStringLiteral("album"), arguments.value(QStringLiteral("album")).toString()},
                                     {QStringLiteral("limit"), 10}},
                         QStringLiteral("正在搜索匹配的歌曲..."));
        return true;
    }
    if (intent == QStringLiteral("create_playlist_with_confirmation")) {
        const QString playlistName = arguments.value(QStringLiteral("playlistName")).toString().trimmed();
        if (playlistName.isEmpty()) {
            emit requestError(requestId,
                              QStringLiteral("invalid_args"),
                              QStringLiteral("创建歌单时缺少歌单名。"));
            return true;
        }
        plan.awaitingApproval = true;
        m_pendingApprovalByPlanId.insert(plan.planId, plan);
        const QVariantMap preview{{QStringLiteral("summary"), QStringLiteral("创建歌单“%1”").arg(playlistName)},
                                  {QStringLiteral("riskLevel"), QStringLiteral("medium")},
                                  {QStringLiteral("steps"), QVariantList{QVariantMap{{QStringLiteral("title"), QStringLiteral("创建歌单")},
                                                                                      {QStringLiteral("status"), QStringLiteral("pending")}}}}};
        emit planPreviewReceived(plan.planId, preview);
        emit approvalRequestReceived(plan.planId,
                                     QStringLiteral("该操作会创建新歌单，是否继续执行？"),
                                     preview);
        return true;
    }

    return false;
}

void AgentLocalRuntime::dispatchToolPlan(PendingPlan plan,
                                         const QString& tool,
                                         const QVariantMap& args,
                                         const QString& progressMessage)
{
    if (!m_capabilityFacade) {
        emit requestError(plan.requestId,
                          QStringLiteral("runtime_unavailable"),
                          QStringLiteral("Qt 内嵌 Agent 能力执行器尚未初始化。"));
        return;
    }

    plan.toolCallId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    plan.currentTool = tool;
    plan.currentArgs = args;
    m_pendingPlanByToolCallId.insert(plan.toolCallId, plan);
    if (!progressMessage.trimmed().isEmpty()) {
        emit progressReceived(progressMessage,
                              QVariantMap{{QStringLiteral("planId"), plan.planId},
                                          {QStringLiteral("stepId"), tool}});
    }
    if (!m_capabilityFacade->executeCapability(plan.toolCallId, tool, args)) {
        m_pendingPlanByToolCallId.remove(plan.toolCallId);
        emit requestError(plan.requestId,
                          QStringLiteral("dispatch_failed"),
                          QStringLiteral("客户端当前无法调度该控制动作。"));
    }
}

void AgentLocalRuntime::completePlanSuccess(const PendingPlan& plan, const QString& summary)
{
    emit assistantFinalReceived(plan.requestId, summary);
    emit finalResultReceived(QVariantMap{{QStringLiteral("ok"), true},
                                         {QStringLiteral("planId"), plan.planId},
                                         {QStringLiteral("summary"), summary}});
}

void AgentLocalRuntime::completePlanFailure(const PendingPlan& plan, const QString& summary)
{
    emit requestError(plan.requestId,
                      QStringLiteral("plan_failed"),
                      summary);
    emit finalResultReceived(QVariantMap{{QStringLiteral("ok"), false},
                                         {QStringLiteral("planId"), plan.planId},
                                         {QStringLiteral("summary"), summary}});
}

QString AgentLocalRuntime::summarizeCurrentPlayback(const QVariantMap& result) const
{
    const QString title = result.value(QStringLiteral("title")).toString().trimmed();
    if (title.isEmpty()) {
        return QStringLiteral("当前没有正在播放的歌曲。");
    }
    const QString artist = result.value(QStringLiteral("artist")).toString().trimmed();
    const bool playing = result.value(QStringLiteral("playing"), false).toBool();
    return artist.isEmpty()
        ? QStringLiteral("当前%1《%2》。").arg(playing ? QStringLiteral("正在播放") : QStringLiteral("已停在"), title)
        : QStringLiteral("当前%1《%2》 - %3。")
              .arg(playing ? QStringLiteral("正在播放") : QStringLiteral("已停在"), title, artist);
}

QString AgentLocalRuntime::summarizeUiOverview(const QVariantMap& result) const
{
    const QString currentPage = result.value(QStringLiteral("currentPage")).toString().trimmed();
    const QString currentMusicTab = result.value(QStringLiteral("currentMusicTab")).toString().trimmed();
    const bool offlineMode = result.value(QStringLiteral("offlineMode")).toBool();
    const QVariantMap selectedPlaylist = result.value(QStringLiteral("selectedPlaylist")).toMap();

    QStringList fragments;
    if (!currentPage.isEmpty()) {
        fragments << QStringLiteral("当前页面是 %1").arg(currentPage);
    }
    if (!currentMusicTab.isEmpty()) {
        fragments << QStringLiteral("当前音乐 tab 是 %1").arg(currentMusicTab);
    }
    fragments << (offlineMode ? QStringLiteral("当前处于离线模式") : QStringLiteral("当前处于在线模式"));
    const QString playlistName = selectedPlaylist.value(QStringLiteral("name")).toString().trimmed();
    if (!playlistName.isEmpty()) {
        fragments << QStringLiteral("当前歌单是“%1”").arg(playlistName);
    }

    return fragments.isEmpty()
        ? QStringLiteral("当前没有可用的界面状态摘要。")
        : fragments.join(QStringLiteral("，")) + QStringLiteral("。");
}

QString AgentLocalRuntime::summarizeUiPageState(const QVariantMap& result) const
{
    const QString page = result.value(QStringLiteral("page")).toString().trimmed();
    const int count = result.value(QStringLiteral("count")).toInt();
    if (page.isEmpty()) {
        return QStringLiteral("当前页面状态为空。");
    }
    if (count > 0) {
        return QStringLiteral("%1 页面当前有 %2 条可见数据。").arg(page).arg(count);
    }
    return QStringLiteral("%1 页面状态已读取。").arg(page);
}

QString AgentLocalRuntime::summarizeMusicTabItems(const PendingPlan& plan, const QVariantMap& result) const
{
    const QString tab = result.value(QStringLiteral("tab"),
                                     plan.arguments.value(QStringLiteral("tab"))).toString().trimmed();
    const QVariantList items = result.value(QStringLiteral("items")).toList();
    if (items.isEmpty()) {
        return QStringLiteral("%1 当前没有可见条目。").arg(tab.isEmpty() ? QStringLiteral("该 tab") : tab);
    }

    QStringList titles;
    const int limit = qMin(3, items.size());
    for (int i = 0; i < limit; ++i) {
        const QVariantMap item = items.at(i).toMap();
        const QString title = item.value(QStringLiteral("title")).toString().trimmed();
        if (!title.isEmpty()) {
            titles << QStringLiteral("《%1》").arg(title);
        }
    }

    if (titles.isEmpty()) {
        return QStringLiteral("%1 当前共有 %2 条可见数据。")
            .arg(tab.isEmpty() ? QStringLiteral("该 tab") : tab)
            .arg(items.size());
    }
    return QStringLiteral("%1 当前共有 %2 条可见数据，例如：%3。")
        .arg(tab.isEmpty() ? QStringLiteral("该 tab") : tab)
        .arg(result.value(QStringLiteral("count"), items.size()).toInt())
        .arg(titles.join(QStringLiteral("、")));
}

QString AgentLocalRuntime::summarizeMusicTabItem(const QVariantMap& result) const
{
    const QString title = result.value(QStringLiteral("title")).toString().trimmed();
    const QString artist = result.value(QStringLiteral("artist")).toString().trimmed();
    const QString tab = result.value(QStringLiteral("tab")).toString().trimmed();
    if (title.isEmpty()) {
        return tab.isEmpty()
            ? QStringLiteral("已读取该条目的信息。")
            : QStringLiteral("已读取 %1 中的目标条目信息。").arg(tab);
    }
    return artist.isEmpty()
        ? QStringLiteral("已读取到歌曲《%1》。").arg(title)
        : QStringLiteral("已读取到歌曲《%1》 - %2。").arg(title, artist);
}

QString AgentLocalRuntime::summarizePlayedMusicTabTrack(const QVariantMap& result) const
{
    const QString tab = result.value(QStringLiteral("tab")).toString().trimmed();
    const QVariantMap item = result.value(QStringLiteral("item")).toMap();
    const QString title = item.value(QStringLiteral("title")).toString().trimmed();
    const QString artist = item.value(QStringLiteral("artist")).toString().trimmed();
    if (title.isEmpty()) {
        return tab.isEmpty()
            ? QStringLiteral("已开始播放目标歌曲。")
            : QStringLiteral("已从 %1 播放目标歌曲。").arg(tab);
    }
    return artist.isEmpty()
        ? QStringLiteral("已从 %1 播放《%2》。").arg(tab, title)
        : QStringLiteral("已从 %1 播放《%2》 - %3。").arg(tab, title, artist);
}

QString AgentLocalRuntime::summarizeMusicTabAction(const QVariantMap& result) const
{
    const QString tab = result.value(QStringLiteral("tab")).toString().trimmed();
    const QString action = result.value(QStringLiteral("action")).toString().trimmed();
    const QVariantMap item = result.value(QStringLiteral("item")).toMap();
    const QString title = item.value(QStringLiteral("title")).toString().trimmed();
    const QString displayTitle = title.isEmpty() ? QStringLiteral("目标歌曲") : QStringLiteral("《%1》").arg(title);

    if (action == QStringLiteral("download")) {
        return QStringLiteral("已在 %1 对 %2 发起下载。").arg(tab, displayTitle);
    }
    if (action == QStringLiteral("add_to_playlist")) {
        const QString playlistName = item.value(QStringLiteral("playlistName")).toString().trimmed();
        return playlistName.isEmpty()
            ? QStringLiteral("已在 %1 对 %2 执行加歌单操作。").arg(tab, displayTitle)
            : QStringLiteral("已在 %1 将 %2 加入歌单“%3”。").arg(tab, displayTitle, playlistName);
    }
    if (action == QStringLiteral("add_favorite")) {
        return QStringLiteral("已在 %1 收藏 %2。").arg(tab, displayTitle);
    }
    if (action == QStringLiteral("remove_favorite")) {
        return QStringLiteral("已在 %1 取消收藏 %2。").arg(tab, displayTitle);
    }
    if (action == QStringLiteral("remove_or_delete")) {
        return QStringLiteral("已在 %1 移除 %2。").arg(tab, displayTitle);
    }
    if (action == QStringLiteral("play_next")) {
        return QStringLiteral("已将 %2 设为 %1 的下一首播放项。").arg(tab, displayTitle);
    }
    if (action == QStringLiteral("toggle_current_playback")) {
        return QStringLiteral("已在 %1 对 %2 执行播放切换。").arg(tab, displayTitle);
    }
    return QStringLiteral("已在 %1 对 %2 执行 %3。").arg(tab, displayTitle, action);
}

QString AgentLocalRuntime::summarizeLocalTracks(const QVariantMap& result) const
{
    const QVariantList items = result.value(QStringLiteral("items")).toList();
    if (items.isEmpty()) {
        return QStringLiteral("当前没有扫描到本地音乐。");
    }
    QStringList titles;
    const int limit = qMin(3, items.size());
    for (int i = 0; i < limit; ++i) {
        const QVariantMap item = items.at(i).toMap();
        const QString title = item.value(QStringLiteral("title")).toString().trimmed();
        if (!title.isEmpty()) {
            titles << QStringLiteral("《%1》").arg(title);
        }
    }
    return QStringLiteral("当前本地音乐共有 %1 首，例如：%2。")
        .arg(result.value(QStringLiteral("count"), items.size()).toInt())
        .arg(titles.join(QStringLiteral("、")));
}

QString AgentLocalRuntime::summarizePlaylistTracks(const QVariantMap& result) const
{
    const QVariantMap playlist = result.value(QStringLiteral("playlist")).toMap();
    const QVariantList items = result.value(QStringLiteral("items")).toList();
    const QString playlistName = playlist.value(QStringLiteral("name")).toString().trimmed();
    if (items.isEmpty()) {
        return playlistName.isEmpty()
            ? QStringLiteral("这个歌单当前没有歌曲。")
            : QStringLiteral("歌单“%1”当前没有歌曲。").arg(playlistName);
    }

    QStringList titles;
    const int limit = qMin(5, items.size());
    for (int index = 0; index < limit; ++index) {
        const QVariantMap item = items.at(index).toMap();
        const QString title = item.value(QStringLiteral("title")).toString().trimmed();
        const QString artist = item.value(QStringLiteral("artist")).toString().trimmed();
        if (title.isEmpty()) {
            continue;
        }
        titles << (artist.isEmpty() || artist == QStringLiteral("未知艺术家")
                       ? QStringLiteral("《%1》").arg(title)
                       : QStringLiteral("《%1》 - %2").arg(title, artist));
    }

    const int count = playlist.value(QStringLiteral("trackCount"), items.size()).toInt();
    return playlistName.isEmpty()
        ? QStringLiteral("当前歌单共有 %1 首歌曲，例如：%2。").arg(count).arg(titles.join(QStringLiteral("、")))
        : QStringLiteral("歌单“%1”共有 %2 首歌曲，例如：%3。")
              .arg(playlistName)
              .arg(count)
              .arg(titles.join(QStringLiteral("、")));
}

QVariantMap AgentLocalRuntime::resolvePlaylistSelection(const QVariantList& playlists,
                                                        const QVariantMap& arguments,
                                                        const QVariantMap& hostContext,
                                                        QString* errorMessage) const
{
    if (arguments.value(QStringLiteral("useCurrentPlaylist")).toBool()) {
        const QVariantMap selectedPlaylist = hostContext.value(QStringLiteral("selectedPlaylist")).toMap();
        const QString playlistId = selectedPlaylist.value(QStringLiteral("playlistId")).toString().trimmed();
        if (!playlistId.isEmpty()) {
            return selectedPlaylist;
        }
        if (errorMessage) {
            *errorMessage = QStringLiteral("当前没有选中的歌单，请先打开歌单或直接指定歌单名称。");
        }
        return {};
    }

    const QString requestedName =
        normalizedPlaylistName(arguments.value(QStringLiteral("playlistName")).toString());
    if (requestedName.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("请明确告诉我要读取哪个歌单。");
        }
        return {};
    }

    QVariantList exactMatches;
    QVariantList partialMatches;
    for (const QVariant& value : playlists) {
        const QVariantMap playlist = value.toMap();
        const QString playlistName = playlist.value(QStringLiteral("name")).toString().trimmed();
        if (playlistName.isEmpty()) {
            continue;
        }
        if (playlistName.compare(requestedName, Qt::CaseInsensitive) == 0) {
            exactMatches.push_back(playlist);
            continue;
        }
        if (playlistName.contains(requestedName, Qt::CaseInsensitive)
            || requestedName.contains(playlistName, Qt::CaseInsensitive)) {
            partialMatches.push_back(playlist);
        }
    }

    if (exactMatches.size() == 1) {
        return exactMatches.first().toMap();
    }
    if (exactMatches.size() > 1) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("找到了多个同名歌单“%1”，请换一个更具体的名称。").arg(requestedName);
        }
        return {};
    }
    if (partialMatches.size() == 1) {
        return partialMatches.first().toMap();
    }
    if (partialMatches.size() > 1) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("有多个歌单和“%1”相关，请说得更具体一些。").arg(requestedName);
        }
        return {};
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral("没有找到名为“%1”的歌单。").arg(requestedName);
    }
    return {};
}
