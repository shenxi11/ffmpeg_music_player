#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QTimer>
#include <QUuid>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "src/agent/AgentLocalModelGateway.h"
#include "src/common/settings_manager.h"

namespace {

struct SmokeCase
{
    QString message;
    QString expectedIntent;
};

struct SmokeOptions
{
    QVector<SmokeCase> cases;
    int timeoutMs = 300000;
};

QVariantMap buildHostContext()
{
    return {
        {QStringLiteral("offlineMode"), true},
        {QStringLiteral("currentPage"), QStringLiteral("local_download")},
        {QStringLiteral("selectedPlaylist"), QVariantMap()},
        {QStringLiteral("selectedTrackIds"), QVariantList()},
        {QStringLiteral("currentTrack"), QVariantMap()},
        {QStringLiteral("queueSummary"),
         QVariantMap{{QStringLiteral("items"), QVariantList()},
                     {QStringLiteral("count"), 0}}}
    };
}

QVariantList buildCapabilityItems()
{
    return {
        QVariantMap{{QStringLiteral("name"), QStringLiteral("pausePlayback")},
                    {QStringLiteral("domain"), QStringLiteral("playback")},
                    {QStringLiteral("readOnly"), false},
                    {QStringLiteral("confirmPolicy"), QStringLiteral("none")},
                    {QStringLiteral("availabilityPolicy"), QStringLiteral("always")}},
        QVariantMap{{QStringLiteral("name"), QStringLiteral("getUiOverview")},
                    {QStringLiteral("domain"), QStringLiteral("host")},
                    {QStringLiteral("readOnly"), true},
                    {QStringLiteral("confirmPolicy"), QStringLiteral("none")},
                    {QStringLiteral("availabilityPolicy"), QStringLiteral("always")}},
        QVariantMap{{QStringLiteral("name"), QStringLiteral("getUiPageState")},
                    {QStringLiteral("domain"), QStringLiteral("host")},
                    {QStringLiteral("readOnly"), true},
                    {QStringLiteral("confirmPolicy"), QStringLiteral("none")},
                    {QStringLiteral("availabilityPolicy"), QStringLiteral("always")}},
        QVariantMap{{QStringLiteral("name"), QStringLiteral("getMusicTabItems")},
                    {QStringLiteral("domain"), QStringLiteral("host")},
                    {QStringLiteral("readOnly"), true},
                    {QStringLiteral("confirmPolicy"), QStringLiteral("none")},
                    {QStringLiteral("availabilityPolicy"), QStringLiteral("always")}},
        QVariantMap{{QStringLiteral("name"), QStringLiteral("getMusicTabItem")},
                    {QStringLiteral("domain"), QStringLiteral("host")},
                    {QStringLiteral("readOnly"), true},
                    {QStringLiteral("confirmPolicy"), QStringLiteral("none")},
                    {QStringLiteral("availabilityPolicy"), QStringLiteral("always")}},
        QVariantMap{{QStringLiteral("name"), QStringLiteral("playMusicTabTrack")},
                    {QStringLiteral("domain"), QStringLiteral("playback")},
                    {QStringLiteral("readOnly"), false},
                    {QStringLiteral("confirmPolicy"), QStringLiteral("none")},
                    {QStringLiteral("availabilityPolicy"), QStringLiteral("always")}},
        QVariantMap{{QStringLiteral("name"), QStringLiteral("invokeMusicTabAction")},
                    {QStringLiteral("domain"), QStringLiteral("host")},
                    {QStringLiteral("readOnly"), false},
                    {QStringLiteral("confirmPolicy"), QStringLiteral("none")},
                    {QStringLiteral("availabilityPolicy"), QStringLiteral("always")}},
        QVariantMap{{QStringLiteral("name"), QStringLiteral("getLocalTracks")},
                    {QStringLiteral("domain"), QStringLiteral("library")},
                    {QStringLiteral("readOnly"), true},
                    {QStringLiteral("confirmPolicy"), QStringLiteral("none")},
                    {QStringLiteral("availabilityPolicy"), QStringLiteral("always")}},
        QVariantMap{{QStringLiteral("name"), QStringLiteral("getPlaylists")},
                    {QStringLiteral("domain"), QStringLiteral("playlist")},
                    {QStringLiteral("readOnly"), true},
                    {QStringLiteral("confirmPolicy"), QStringLiteral("none")},
                    {QStringLiteral("availabilityPolicy"), QStringLiteral("login_required")}},
        QVariantMap{{QStringLiteral("name"), QStringLiteral("getPlaylistTracks")},
                    {QStringLiteral("domain"), QStringLiteral("playlist")},
                    {QStringLiteral("readOnly"), true},
                    {QStringLiteral("confirmPolicy"), QStringLiteral("none")},
                    {QStringLiteral("availabilityPolicy"), QStringLiteral("login_required")}}
    };
}

SmokeOptions buildOptions(const QStringList& args)
{
    QString message;
    QString expectedIntent;
    SmokeOptions options;

    for (int index = 1; index < args.size(); ++index) {
        const QString arg = args.at(index);
        if (arg == QStringLiteral("--message") && index + 1 < args.size()) {
            message = args.at(++index);
        } else if (arg == QStringLiteral("--expect-intent") && index + 1 < args.size()) {
            expectedIntent = args.at(++index);
        } else if (arg == QStringLiteral("--timeout-ms") && index + 1 < args.size()) {
            options.timeoutMs = qMax(1000, args.at(++index).toInt());
        }
    }

    if (!message.trimmed().isEmpty() && !expectedIntent.trimmed().isEmpty()) {
        options.cases = {{message.trimmed(), expectedIntent.trimmed()}};
        return options;
    }

    options.cases = {
        {QStringLiteral("暂停播放"), QStringLiteral("pause_playback")},
        {QStringLiteral("列出我的本地音乐"), QStringLiteral("query_local_tracks")},
        {QStringLiteral("列出流行歌单的所有歌曲"), QStringLiteral("query_playlist_tracks")},
        {QStringLiteral("看看当前页面状态"), QStringLiteral("query_ui_overview")},
        {QStringLiteral("读一下设置页当前状态"), QStringLiteral("query_ui_page_state")},
        {QStringLiteral("列出在线音乐 tab 的前 10 首歌"), QStringLiteral("query_music_tab_items")},
        {QStringLiteral("把在线音乐里这首歌下载下来"), QStringLiteral("act_on_music_tab_track")}
    };
    return options;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);

    const SmokeOptions options = buildOptions(app.arguments());
    const QVector<SmokeCase> cases = options.cases;
    const QVariantMap hostContext = buildHostContext();
    const QVariantList capabilities = buildCapabilityItems();
    const QVariantMap memorySnapshot;

    out << "embedded_agent_smoke\n";
    out << "modelPath=" << SettingsManager::instance().agentLocalModelPath() << "\n";
    out << "contextSize=" << SettingsManager::instance().agentLocalContextSize() << "\n";
    out << "threadCount=" << SettingsManager::instance().agentLocalThreadCount() << "\n";
    out << "timeoutMs=" << options.timeoutMs << "\n";
    out.flush();

    AgentLocalModelGateway gateway;
    bool allPassed = true;

    for (const SmokeCase& smokeCase : cases) {
        const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QVariantMap receivedPayload;
        QString receivedCode;
        QString receivedMessage;
        bool finished = false;
        bool ok = false;

        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.setInterval(options.timeoutMs);

        QObject::connect(&timeout, &QTimer::timeout, &loop, [&]() {
            receivedCode = QStringLiteral("timeout");
            receivedMessage = QStringLiteral("本地控制编译超时。");
            finished = true;
            ok = false;
            loop.quit();
        });

        QObject::connect(&gateway,
                         &AgentLocalModelGateway::controlIntentReady,
                         &loop,
                         [&](const QString& callbackRequestId, const QVariantMap& payload) {
                             if (callbackRequestId != requestId || finished) {
                                 return;
                             }
                             receivedPayload = payload;
                             finished = true;
                             ok = true;
                             loop.quit();
                         });

        QObject::connect(&gateway,
                         &AgentLocalModelGateway::requestFailed,
                         &loop,
                         [&](const QString& callbackRequestId,
                             const QString& code,
                             const QString& message) {
                             if (callbackRequestId != requestId || finished) {
                                 return;
                             }
                             receivedCode = code;
                             receivedMessage = message;
                             finished = true;
                             ok = false;
                             loop.quit();
                         });

        timeout.start();
        gateway.compileIntent(
            requestId, smokeCase.message, hostContext, capabilities, memorySnapshot);
        loop.exec();

        const QString actualIntent = receivedPayload.value(QStringLiteral("intent")).toString();
        const bool matchedIntent = ok && actualIntent == smokeCase.expectedIntent;
        allPassed = allPassed && matchedIntent;

        QVariantMap row{
            {QStringLiteral("message"), smokeCase.message},
            {QStringLiteral("expectedIntent"), smokeCase.expectedIntent},
            {QStringLiteral("ok"), ok},
            {QStringLiteral("actualIntent"), actualIntent},
            {QStringLiteral("matchedIntent"), matchedIntent}
        };
        if (!ok) {
            row.insert(QStringLiteral("code"), receivedCode);
            row.insert(QStringLiteral("messageText"), receivedMessage);
        } else {
            row.insert(QStringLiteral("payload"), receivedPayload);
        }

        out << QString::fromUtf8(
                   QJsonDocument(QJsonObject::fromVariantMap(row)).toJson(QJsonDocument::Compact))
            << "\n";
        out.flush();
    }

    if (!allPassed) {
        err << "embedded_agent_smoke_failed\n";
        return 1;
    }

    out << "embedded_agent_smoke_passed\n";
    out.flush();
    return 0;
}
