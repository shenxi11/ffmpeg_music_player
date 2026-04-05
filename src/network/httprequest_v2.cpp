#include "httprequest_v2.h"
#include "headers.h"
#include "download_manager.h"
#include "local_music_cache.h"
#include "online_presence_manager.h"
#include "settings_manager.h"
#include "user.h"
#include <QDebug>
#include <QHash>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QSet>
#include <QUrl>
#include <QUrlQuery>

namespace {
QString readArtistFromObject(const QJsonObject& obj);
const QSet<QString> kLegacyHosts = {
    QStringLiteral("slcdut.xyz"),
    QStringLiteral("127.0.0.1"),
    QStringLiteral("localhost")
};

QString normalizePlaylistOwnership(const QJsonObject& obj)
{
    auto normalizeValue = [](QString value) {
        value = value.trimmed().toLower();
        if (value == QStringLiteral("owned") ||
            value == QStringLiteral("owner") ||
            value == QStringLiteral("created") ||
            value == QStringLiteral("creator") ||
            value == QStringLiteral("mine") ||
            value == QStringLiteral("self")) {
            return QStringLiteral("owned");
        }
        if (value == QStringLiteral("subscribed") ||
            value == QStringLiteral("subscription") ||
            value == QStringLiteral("collected") ||
            value == QStringLiteral("favorite") ||
            value == QStringLiteral("favorited") ||
            value == QStringLiteral("collected_playlist")) {
            return QStringLiteral("subscribed");
        }
        return QString();
    };

    static const QStringList stringKeys = {
        QStringLiteral("ownership"),
        QStringLiteral("playlist_type"),
        QStringLiteral("source_type"),
        QStringLiteral("ownership_type")
    };
    for (const QString& key : stringKeys) {
        const QString normalized = normalizeValue(obj.value(key).toString());
        if (!normalized.isEmpty()) {
            return normalized;
        }
    }

    static const QStringList ownedKeys = {
        QStringLiteral("is_owned"),
        QStringLiteral("owned"),
        QStringLiteral("is_owner")
    };
    for (const QString& key : ownedKeys) {
        if (obj.contains(key)) {
            return obj.value(key).toBool() ? QStringLiteral("owned")
                                           : QStringLiteral("subscribed");
        }
    }

    static const QStringList subscribedKeys = {
        QStringLiteral("is_subscribed"),
        QStringLiteral("subscribed"),
        QStringLiteral("is_favorited")
    };
    for (const QString& key : subscribedKeys) {
        if (obj.contains(key)) {
            return obj.value(key).toBool() ? QStringLiteral("subscribed")
                                           : QStringLiteral("owned");
        }
    }

    // 当前服务端若未显式返回归属字段，客户端先全部归入自建歌单。
    return QStringLiteral("owned");
}

bool shouldRewriteAbsoluteServiceUrl(const QUrl& url, const QUrl& base)
{
    const QString host = url.host().toLower();
    if (host.isEmpty()) {
        return false;
    }

    if (kLegacyHosts.contains(host)) {
        return true;
    }

    if (host == base.host().toLower()) {
        return false;
    }

    const QString path = url.path();
    return path.startsWith("/uploads/", Qt::CaseInsensitive) ||
           path.startsWith("/video/", Qt::CaseInsensitive) ||
           path.startsWith("/lrc", Qt::CaseInsensitive) ||
           path.startsWith("/download", Qt::CaseInsensitive) ||
           path.startsWith("/files/", Qt::CaseInsensitive);
}

QString normalizeMusicPathForLookup(QString path)
{
    path = path.trimmed();
    if (path.isEmpty()) {
        return QString();
    }

    if (path.startsWith("file://", Qt::CaseInsensitive)) {
        const QUrl url(path);
        if (url.isLocalFile()) {
            path = url.toLocalFile();
        }
    }

    path.replace('\\', '/');
    if (path.size() >= 2 && path[1] == ':') {
        path = path.toLower();
    }
    return path;
}

QString fileNameFromPath(const QString& path)
{
    QString value = path.trimmed();
    if (value.isEmpty()) {
        return QString();
    }

    const int queryIndex = value.indexOf('?');
    if (queryIndex > 0) {
        value = value.left(queryIndex);
    }

    if (value.startsWith("http://", Qt::CaseInsensitive) ||
        value.startsWith("https://", Qt::CaseInsensitive)) {
        const QUrl url(value);
        value = url.path();
    }

    const QFileInfo info(value);
    const QString baseName = info.completeBaseName().trimmed();
    return baseName;
}

QString formatDurationFromSeconds(int totalSeconds)
{
    if (totalSeconds <= 0) {
        return QString();
    }
    const int minutes = totalSeconds / 60;
    const int seconds = totalSeconds % 60;
    return QString("%1:%2")
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
}

QString durationTextFromValue(const QJsonValue& value)
{
    if (value.isUndefined() || value.isNull()) {
        return QString();
    }

    if (value.isDouble()) {
        return formatDurationFromSeconds(static_cast<int>(value.toDouble()));
    }

    QString text = value.toString().trimmed();
    if (text.isEmpty()) {
        return QString();
    }

    if (text.contains(':')) {
        return text;
    }

    bool ok = false;
    const double asSec = text.toDouble(&ok);
    if (ok) {
        return formatDurationFromSeconds(static_cast<int>(asSec));
    }

    const QStringList parts = text.split(' ', Qt::SkipEmptyParts);
    if (!parts.isEmpty()) {
        const double secFromText = parts.first().toDouble(&ok);
        if (ok) {
            return formatDurationFromSeconds(static_cast<int>(secFromText));
        }
    }

    return QString();
}

QString readDurationFromObject(const QJsonObject& obj)
{
    static const QStringList keys = {
        "duration", "duration_sec", "durationSec", "duration_ms", "durationMs"
    };
    for (const QString& key : keys) {
        if (!obj.contains(key)) {
            continue;
        }
        const QString duration = durationTextFromValue(obj.value(key));
        if (!duration.isEmpty()) {
            return duration;
        }
    }
    return QString();
}

QString normalizeArtist(const QString& artist)
{
    const QString trimmed = artist.trimmed();
    if (trimmed.isEmpty() || trimmed == "null" || trimmed == "NULL") {
        return QString();
    }
    return trimmed;
}

QString readArtistFromArray(const QJsonArray& array)
{
    QStringList names;
    names.reserve(array.size());
    for (const QJsonValue& value : array) {
        if (value.isString()) {
            const QString name = normalizeArtist(value.toString());
            if (!name.isEmpty()) {
                names.append(name);
            }
            continue;
        }

        if (value.isObject()) {
            const QString name = readArtistFromObject(value.toObject());
            if (!name.isEmpty()) {
                names.append(name);
            }
        }
    }
    return names.join(" / ");
}

QString readArtistValue(const QJsonObject& obj, const QString& key)
{
    if (!obj.contains(key)) {
        return QString();
    }

    const QJsonValue value = obj.value(key);
    if (value.isString()) {
        return normalizeArtist(value.toString());
    }
    if (value.isArray()) {
        return readArtistFromArray(value.toArray());
    }
    if (value.isObject()) {
        return readArtistFromObject(value.toObject());
    }
    return QString();
}

QString readArtistFromObject(const QJsonObject& obj)
{
    // Compatible with multiple server payload styles.
    static const QStringList directKeys = {
        "artist", "singer", "author", "artist_name", "name"
    };
    for (const QString& key : directKeys) {
        const QString artist = readArtistValue(obj, key);
        if (!artist.isEmpty()) {
            return artist;
        }
    }

    static const QStringList arrayKeys = {
        "artists", "ar"
    };
    for (const QString& key : arrayKeys) {
        const QString artist = readArtistValue(obj, key);
        if (!artist.isEmpty()) {
            return artist;
        }
    }

    // Some APIs put song metadata under nested objects.
    static const QStringList nestedKeys = {
        "music", "song", "track", "file"
    };
    for (const QString& key : nestedKeys) {
        if (obj.contains(key) && obj.value(key).isObject()) {
            const QString artist = readArtistFromObject(obj.value(key).toObject());
            if (!artist.isEmpty()) {
                return artist;
            }
        }
    }

    return QString();
}

QString extractErrorMessage(const Network::NetworkResponse& response, const QString& fallback)
{
    const QString plain = QString::fromUtf8(response.body).trimmed();
    if (plain.isEmpty()) {
        return fallback;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(response.body, &parseError);
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        const QJsonObject obj = doc.object();
        const QString message = obj.value("message").toString().trimmed();
        if (!message.isEmpty()) {
            return message;
        }
    }

    return plain;
}

QJsonObject unwrapDataObject(const QJsonObject& rootObj)
{
    if (rootObj.contains(QStringLiteral("data")) && rootObj.value(QStringLiteral("data")).isObject()) {
        return rootObj.value(QStringLiteral("data")).toObject();
    }
    return rootObj;
}

bool parseSuccessFlag(const QJsonObject& obj, bool defaultValue)
{
    if (obj.contains(QStringLiteral("success")) && obj.value(QStringLiteral("success")).isBool()) {
        return obj.value(QStringLiteral("success")).toBool();
    }
    if (obj.contains(QStringLiteral("success_bool")) && obj.value(QStringLiteral("success_bool")).isBool()) {
        return obj.value(QStringLiteral("success_bool")).toBool();
    }
    if (obj.contains(QStringLiteral("ok")) && obj.value(QStringLiteral("ok")).isBool()) {
        return obj.value(QStringLiteral("ok")).toBool();
    }
    return defaultValue;
}

QString parseMessageText(const QJsonObject& obj, const QString& fallback = QString())
{
    const QString message = obj.value(QStringLiteral("message")).toString().trimmed();
    if (!message.isEmpty()) {
        return message;
    }
    return fallback;
}

int durationSecondsFromText(const QString& duration)
{
    const QString text = duration.trimmed();
    if (text.isEmpty()) {
        return 0;
    }

    if (text.contains(':')) {
        const QStringList parts = text.split(':', Qt::SkipEmptyParts);
        if (parts.size() == 2) {
            bool okMin = false;
            bool okSec = false;
            const int minutes = parts[0].toInt(&okMin);
            const int seconds = parts[1].toInt(&okSec);
            if (okMin && okSec) {
                return qMax(0, minutes * 60 + seconds);
            }
        } else if (parts.size() == 3) {
            bool okHour = false;
            bool okMin = false;
            bool okSec = false;
            const int hours = parts[0].toInt(&okHour);
            const int minutes = parts[1].toInt(&okMin);
            const int seconds = parts[2].toInt(&okSec);
            if (okHour && okMin && okSec) {
                return qMax(0, hours * 3600 + minutes * 60 + seconds);
            }
        }
    }

    bool ok = false;
    const double asDouble = text.toDouble(&ok);
    if (ok) {
        return qMax(0, static_cast<int>(asDouble));
    }

    const QStringList split = text.split(' ', Qt::SkipEmptyParts);
    if (!split.isEmpty()) {
        const double head = split.first().toDouble(&ok);
        if (ok) {
            return qMax(0, static_cast<int>(head));
        }
    }

    return 0;
}

QString rewriteServiceUrlToBase(const QString& rawUrl, const QString& baseUrl)
{
    const QString trimmed = rawUrl.trimmed();
    QString normalizedPath = trimmed;
    normalizedPath.replace('\\', '/');
    const QString lowerTrimmed = trimmed.toLower();
    const QString lowerNormalized = normalizedPath.toLower();
    if (trimmed.isEmpty()) {
        return QString();
    }
    if (lowerTrimmed == QStringLiteral("null") ||
        lowerTrimmed == QStringLiteral("undefined") ||
        lowerTrimmed == QStringLiteral("none") ||
        lowerTrimmed == QStringLiteral("(null)")) {
        return QString();
    }

    QUrl base(baseUrl);
    if (!base.isValid()) {
        return trimmed;
    }
    if (base.path().isEmpty()) {
        base.setPath("/");
    }

    // Windows 本地路径兜底：转 file:// URL 供 QML Image 加载。
    if (normalizedPath.size() >= 3 &&
        normalizedPath[1] == QLatin1Char(':') &&
        normalizedPath[2] == QLatin1Char('/')) {
        return QUrl::fromLocalFile(normalizedPath).toString();
    }

    if (trimmed.startsWith("http://", Qt::CaseInsensitive) ||
        trimmed.startsWith("https://", Qt::CaseInsensitive)) {
        QUrl url(trimmed);
        if (!url.isValid()) {
            return trimmed;
        }

        QString fixedPath = url.path();
        const QString lowerPath = fixedPath.toLower();
        if (lowerPath.startsWith(QStringLiteral("/uploads/uploads/"))) {
            fixedPath = QStringLiteral("/uploads/") + fixedPath.mid(QStringLiteral("/uploads/uploads/").size());
            url.setPath(fixedPath);
        }

        if (shouldRewriteAbsoluteServiceUrl(url, base)) {
            url.setScheme(base.scheme());
            url.setHost(base.host());
            if (base.port() > 0) {
                url.setPort(base.port());
            } else {
                url.setPort(-1);
            }
            return url.toString();
        }
        return trimmed;
    }

    QString fixedRelative = normalizedPath;
    const QString fixedLower = fixedRelative.toLower();
    if (fixedLower.startsWith(QStringLiteral("uploads/uploads/"))) {
        fixedRelative = QStringLiteral("uploads/") + fixedRelative.mid(QStringLiteral("uploads/uploads/").size());
    } else if (fixedLower.startsWith(QStringLiteral("/uploads/uploads/"))) {
        fixedRelative = QStringLiteral("/uploads/") + fixedRelative.mid(QStringLiteral("/uploads/uploads/").size());
    }

    if (fixedRelative.startsWith('/')) {
        return base.resolved(QUrl(fixedRelative.mid(1))).toString();
    }

    // Service may return relative paths without leading slash, e.g. "uploads/x.jpg".
    // Resolve common service paths to current base URL for private deployment stability.
    const QString fixedRelativeLower = fixedRelative.toLower();
    if (fixedRelativeLower.startsWith("uploads/") ||
        fixedRelativeLower.startsWith("video/") ||
        fixedRelativeLower.startsWith("hls/") ||
        fixedRelativeLower.startsWith("files/") ||
        fixedRelativeLower.startsWith("download") ||
        fixedRelativeLower.startsWith("lrc")) {
        return base.resolved(QUrl(fixedRelative)).toString();
    }

    return fixedRelative;
}

QString normalizeOnlineMediaPath(const QString& rawPath, const QString& baseUrl)
{
    const QString trimmed = rawPath.trimmed();
    const QString lowerTrimmed = trimmed.toLower();
    if (trimmed.isEmpty()) {
        return QString();
    }
    if (lowerTrimmed == QStringLiteral("null") ||
        lowerTrimmed == QStringLiteral("undefined") ||
        lowerTrimmed == QStringLiteral("none") ||
        lowerTrimmed == QStringLiteral("(null)")) {
        return QString();
    }

    if (trimmed.startsWith("http://", Qt::CaseInsensitive) ||
        trimmed.startsWith("https://", Qt::CaseInsensitive)) {
        return rewriteServiceUrlToBase(trimmed, baseUrl);
    }

    QString relative = trimmed;
    while (relative.startsWith('/')) {
        relative.remove(0, 1);
    }

    const QString lower = relative.toLower();
    if (lower.startsWith("uploads/") ||
        lower.startsWith("video/") ||
        lower.startsWith("hls/")) {
        return QUrl(baseUrl + relative).toString();
    }
    return QUrl(baseUrl + "uploads/" + relative).toString();
}

QHash<QString, QString>& coverLookupTable()
{
    static QHash<QString, QString> s_coverByMusicPath;
    return s_coverByMusicPath;
}

QHash<QString, QString>& coverLookupByMetaTable()
{
    static QHash<QString, QString> s_coverByMeta;
    return s_coverByMeta;
}

QString normalizeNullableText(const QString& value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }
    const QString lower = trimmed.toLower();
    if (lower == QStringLiteral("null") ||
        lower == QStringLiteral("undefined") ||
        lower == QStringLiteral("none") ||
        lower == QStringLiteral("(null)")) {
        return QString();
    }
    return trimmed;
}

QStringList buildMusicLookupKeys(const QString& rawPath)
{
    QStringList keys;
    const QString cleaned = normalizeNullableText(rawPath);
    if (cleaned.isEmpty()) {
        return keys;
    }

    auto appendKey = [&keys](const QString& candidate) {
        const QString key = normalizeMusicPathForLookup(candidate);
        if (!key.isEmpty() && !keys.contains(key)) {
            keys.append(key);
        }
    };

    auto appendNameKeys = [&appendKey](const QString& candidatePath) {
        QString value = candidatePath;
        value.replace('\\', '/');
        const QFileInfo info(value);
        const QString fileName = info.fileName().trimmed();
        if (!fileName.isEmpty()) {
            appendKey(fileName);
        }
        const QString baseName = info.completeBaseName().trimmed();
        if (!baseName.isEmpty()) {
            appendKey(baseName);
        }
    };

    appendKey(cleaned);
    appendNameKeys(cleaned);

    QUrl url(cleaned);
    if (url.isValid() && (cleaned.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
                          cleaned.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive))) {
        const QString decodedPath = QUrl::fromPercentEncoding(url.path().toUtf8());
        appendKey(decodedPath);
        appendNameKeys(decodedPath);
        if (decodedPath.startsWith('/')) {
            appendKey(decodedPath.mid(1));
            appendNameKeys(decodedPath.mid(1));
        }

        const QString pathLower = decodedPath.toLower();
        const int uploadsPos = pathLower.indexOf(QStringLiteral("/uploads/"));
        if (uploadsPos >= 0) {
            appendKey(decodedPath.mid(uploadsPos + 1));
            appendNameKeys(decodedPath.mid(uploadsPos + 1));
        }
    } else if (cleaned.startsWith('/')) {
        appendKey(cleaned.mid(1));
        appendNameKeys(cleaned.mid(1));
    }

    return keys;
}

void rememberCoverForMusicPath(const QString& rawPath, const QString& rawCover)
{
    const QString cover = normalizeNullableText(rawCover);
    if (cover.isEmpty()) {
        return;
    }
    const QStringList keys = buildMusicLookupKeys(rawPath);
    if (keys.isEmpty()) {
        return;
    }
    QHash<QString, QString>& lookup = coverLookupTable();
    for (const QString& key : keys) {
        lookup.insert(key, cover);
    }
}

QString queryCoverForMusicPath(const QString& rawPath)
{
    const QStringList keys = buildMusicLookupKeys(rawPath);
    if (keys.isEmpty()) {
        return QString();
    }
    const QHash<QString, QString>& lookup = coverLookupTable();
    for (const QString& key : keys) {
        auto it = lookup.constFind(key);
        if (it != lookup.constEnd()) {
            return it.value();
        }
    }
    return QString();
}

QString buildSongMetaKey(const QString& title, const QString& artist)
{
    const QString t = normalizeNullableText(title).toLower();
    const QString a = normalizeNullableText(artist).toLower();
    if (t.isEmpty()) {
        return QString();
    }
    return t + QStringLiteral("|") + a;
}

void rememberCoverForSongMeta(const QString& title, const QString& artist, const QString& rawCover)
{
    const QString cover = normalizeNullableText(rawCover);
    if (cover.isEmpty()) {
        return;
    }
    const QString key = buildSongMetaKey(title, artist);
    if (key.isEmpty()) {
        return;
    }
    coverLookupByMetaTable().insert(key, cover);
}

QString queryCoverForSongMeta(const QString& title, const QString& artist)
{
    const QString key = buildSongMetaKey(title, artist);
    if (key.isEmpty()) {
        return QString();
    }
    return coverLookupByMetaTable().value(key);
}

QString buildCoverPathForPlaylistPayload(const QString& rawCover)
{
    QString cover = normalizeNullableText(rawCover);
    if (cover.isEmpty()) {
        return QString();
    }
    if (cover.startsWith(QStringLiteral("qrc:/"), Qt::CaseInsensitive)) {
        return QString();
    }

    if (cover.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        const QUrl localUrl(cover);
        if (localUrl.isLocalFile()) {
            return localUrl.toLocalFile();
        }
    }

    if (cover.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
        cover.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        const QUrl url(cover);
        if (!url.isValid()) {
            return QString();
        }
        QString path = QUrl::fromPercentEncoding(url.path().toUtf8()).trimmed();
        while (path.startsWith('/')) {
            path.remove(0, 1);
        }
        if (path.toLower().startsWith(QStringLiteral("uploads/"))) {
            path = path.mid(QStringLiteral("uploads/").size());
        }
        return path;
    }

    if (cover.startsWith('/')) {
        cover = cover.mid(1);
    }
    if (cover.toLower().startsWith(QStringLiteral("uploads/"))) {
        cover = cover.mid(QStringLiteral("uploads/").size());
    }
    return cover;
}
} // namespace

HttpRequestV2::HttpRequestV2(QObject* parent)
    : QObject(parent)
    , m_networkService(Network::NetworkService::instance())
{
    m_baseUrl = SettingsManager::instance().serverBaseUrl().trimmed();
    if (m_baseUrl.isEmpty()) {
        m_baseUrl = QStringLiteral("http://127.0.0.1:8080/");
    }
    if (!m_baseUrl.endsWith('/')) {
        m_baseUrl += '/';
    }

    // 设置基础 URL
    m_networkService.setBaseUrl(m_baseUrl);
    
    // DNS棰勭儹
    m_networkService.prewarmDns(QUrl(m_baseUrl).host());
    
    qDebug() << "[HttpRequestV2] Initialized with base URL:" << m_baseUrl;
}

void HttpRequestV2::login(const QString& account, const QString& password)
{
    QJsonObject json;
    json["account"] = account;
    json["password"] = password;
    
    // 登录使用 Critical 优先级，确保快速响应
    auto options = Network::RequestOptions::critical();
    
    qDebug() << "[HttpRequestV2] Login:" << account;
    
    m_networkService.postJson("users/login", json, options,
        [this, account, password](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                
                qDebug() << "[HttpRequestV2] Login response received in" << response.elapsedMs << "ms";
                
                // 检查登录是否成功
                bool loginSuccess = false;
                if (result.contains("success_bool")) {
                    loginSuccess = result.value("success_bool").toBool();
                } else if (result.contains("success")) {
                    QString successStr = result.value("success").toString();
                    loginSuccess = (successStr == "true");
                }
                
                if (loginSuccess) {
                    auto user = User::getInstance();
                    user->setAccount(account);
                    user->setPassword(password);

                    QString username;
                    
                    if (result.contains("username")) {
                        username = result.value("username").toString();
                        user->setUsername(username);
                        emit signalGetusername(username);
                        qDebug() << "[HttpRequestV2] Login successful, username:" << username;
                    }

                    const QString onlineToken = result.value("online_session_token").toString().trimmed();
                    const int heartbeatSec = result.value("online_heartbeat_interval_sec").toInt(30);
                    const int ttlSec = result.value("online_ttl_sec").toInt(600);
                    OnlinePresenceManager::instance().onLoginSucceeded(
                        account, username, onlineToken, heartbeatSec, ttlSec);
                    
                    // 设置用户收藏歌曲列表
                    if (result.contains("song_path_list")) {
                        QJsonArray songPathArray = result.value("song_path_list").toArray();
                        QStringList musics;
                        for (const auto& songPath : songPathArray) {
                            musics << songPath.toString();
                        }
                        user->setMusicPath(musics);
                        qDebug() << "[HttpRequestV2] User's favorite songs:" << musics.size();
                    }
                    
                    emit signalLoginFlag(true);
                } else {
                    qDebug() << "[HttpRequestV2] Login failed: invalid credentials";
                    emit signalLoginFlag(false);
                }
            } else {
                qWarning() << "[HttpRequestV2] Login request error:" << response.errorString;
                emit signalLoginFlag(false);
            }
        }
    );
}

void HttpRequestV2::registerUser(const QString& account, const QString& password, const QString& username)
{
    QJsonObject json;
    json["account"] = account;
    json["password"] = password;
    json["username"] = username;
    
    auto options = Network::RequestOptions::critical();
    // 注册请求不应自动重试，避免重复注册。
    options.maxRetries = 0;
    
    qDebug() << "[HttpRequestV2] Register:" << account;
    
    m_networkService.postJson("users/register", json, options,
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                qDebug() << "[HttpRequestV2] Registration successful";
                emit signalRegisterFlag(true);
                emit signalRegisterResult(true, QString::fromUtf8("娉ㄥ唽鎴愬姛"));
            } else {
                const QString message = extractErrorMessage(response, response.errorString);
                qWarning() << "[HttpRequestV2] Registration error:" << response.statusCode << message;
                emit signalRegisterFlag(false);
                emit signalRegisterResult(false, message);
            }
        }
    );
}

void HttpRequestV2::resetPassword(const QString& account, const QString& newPassword)
{
    QJsonObject json;
    json["account"] = account;
    json["new_password"] = newPassword;

    auto options = Network::RequestOptions::critical();
    // 重置密码属于显式写操作，禁止自动重试。
    options.maxRetries = 0;

    qDebug() << "[HttpRequestV2] Reset password:" << account;

    m_networkService.postJson("users/reset_password", json, options,
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                qDebug() << "[HttpRequestV2] Reset password successful";
                emit signalResetPasswordResult(true, QString::fromUtf8("瀵嗙爜閲嶇疆鎴愬姛"));
            } else {
                const QString message = extractErrorMessage(response, response.errorString);
                qWarning() << "[HttpRequestV2] Reset password error:" << response.statusCode << message;
                emit signalResetPasswordResult(false, message);
            }
        }
    );
}

void HttpRequestV2::getAllFiles(bool useCache)
{
    Network::RequestOptions options;
    if (useCache) {
        options.useCache = true;
        options.cacheTtl = 300;  // 缓存 5 分钟
    }
    options.priority = Network::RequestPriority::High;
    
    qDebug() << "[HttpRequestV2] Getting all files, useCache:" << useCache;
    
    m_networkService.get("files", options,
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonArray filesArray = Network::NetworkService::parseJsonArray(response);
                QList<Music> musicList;
                
                qDebug() << "[HttpRequestV2] Received" << filesArray.size() << "files"
                         << (response.isFromCache ? "(from cache)" : "(from server)")
                         << "in" << response.elapsedMs << "ms";
                
                for (const QJsonValue& value : filesArray) {
                    if (value.isObject()) {
                        QJsonObject fileObj = value.toObject();
                        QString filePath = fileObj["path"].toString();
                        filePath = normalizeOnlineMediaPath(filePath, m_baseUrl);
                        QString durationStr = fileObj["duration"].toString();
                        QString coverUrl = rewriteServiceUrlToBase(fileObj["cover_art_url"].toString(), m_baseUrl);
                        rememberCoverForMusicPath(filePath, coverUrl);
                        QString artist = readArtistFromObject(fileObj);
                        if (artist.trimmed().isEmpty()) {
                            artist = QStringLiteral("Unknown Artist");
                        }
                        
                        // 解析时长
                        double durationValue = 0.0;
                        if (durationStr != "Error") {
                            QStringList parts = durationStr.split(" ");
                            if (!parts.isEmpty()) {
                                durationValue = parts[0].toDouble();
                            }
                        }
                        
                        Music music;
                        music.setSongPath(filePath);
                        music.setSinger(artist);
                        music.setDuration(static_cast<long>(durationValue));
                        music.setPicPath(coverUrl);
                        rememberCoverForSongMeta(fileNameFromPath(filePath), artist, coverUrl);
                        
                        musicList.append(music);
                    }
                }
                
                emit signalAddSongList(musicList);
            } else {
                qWarning() << "[HttpRequestV2] Get all files error:" << response.errorString;
            }
        }
    );
}

void HttpRequestV2::getLyrics(const QString& url)
{
    QString resolvedSongUrl = rewriteServiceUrlToBase(url, m_baseUrl);
    if (resolvedSongUrl.trimmed().isEmpty()) {
        resolvedSongUrl = normalizeOnlineMediaPath(url, m_baseUrl);
    }

    QUrl qurl(resolvedSongUrl);
    if (!qurl.isValid()) {
        qWarning() << "[HttpRequestV2] Invalid song URL for lyrics:" << url;
        return;
    }

    QString mediaPath = qurl.path();
    if (mediaPath.trimmed().isEmpty()) {
        qWarning() << "[HttpRequestV2] Empty media path for lyrics:" << resolvedSongUrl;
        return;
    }

    QString lrcPath;
    if (mediaPath.startsWith("/hls/", Qt::CaseInsensitive)) {
        // HLS: /hls/<folder>/playlist.m3u8 -> /uploads/<folder>/lrc
        QString relative = mediaPath.mid(QStringLiteral("/hls/").size());
        int slashPos = relative.indexOf('/');
        if (slashPos > 0) {
            const QString folder = relative.left(slashPos);
            lrcPath = QStringLiteral("/uploads/") + folder + QStringLiteral("/lrc");
        }
    }

    if (lrcPath.isEmpty()) {
        int lastSlashPos = mediaPath.lastIndexOf('/');
        if (lastSlashPos <= 0) {
            qWarning() << "[HttpRequestV2] Cannot derive lyric path from media path:" << mediaPath;
            return;
        }
        const QString dirPath = mediaPath.left(lastSlashPos);
        lrcPath = dirPath + QStringLiteral("/lrc");
    }

    const QString lrcUrl = rewriteServiceUrlToBase(lrcPath, m_baseUrl);

    qDebug() << "[HttpRequestV2] Getting lyrics from:" << lrcUrl;

    auto options = Network::RequestOptions::withCache(3600);

    m_networkService.get(lrcUrl, options,
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonArray filesArray = Network::NetworkService::parseJsonArray(response);
                QStringList lines;

                for (const auto& item : filesArray) {
                    lines << item.toString();
                }

                qDebug() << "[HttpRequestV2] Lyrics received:" << lines.size() << "lines";
                emit signalLrc(lines);
            } else {
                qWarning() << "[HttpRequestV2] Get lyrics error:" << response.errorString;
            }
        }
    );
}

void HttpRequestV2::downloadFile(const QString& filename, const QString& downloadFolder,
                                bool downloadLyrics, const QString& coverUrl)
{
    // 下载仍使用 DownloadManager（异步下载管理）。
    QString downloadUrl = m_baseUrl + "download";
    
    QJsonObject jsonObject;
    jsonObject["filename"] = filename;
    QJsonDocument jsonDoc(jsonObject);
    QByteArray postData = jsonDoc.toJson();
    
    qDebug() << "[HttpRequestV2] Adding download to queue:" << filename;
    
    DownloadManager::instance().addDownload(downloadUrl, filename, downloadFolder, postData, coverUrl);
    
    if (downloadLyrics) {
        QString lrcFilename = filename;
        int lastDot = lrcFilename.lastIndexOf('.');
        if (lastDot != -1) {
            lrcFilename = lrcFilename.left(lastDot) + ".lrc";
        }
        
        QJsonObject lrcJsonObject;
        lrcJsonObject["filename"] = lrcFilename;
        QJsonDocument lrcJsonDoc(lrcJsonObject);
        QByteArray lrcPostData = lrcJsonDoc.toJson();
        
        DownloadManager::instance().addDownload(downloadUrl, lrcFilename, downloadFolder, lrcPostData);
    }
}

void HttpRequestV2::addFavorite(const QString& userAccount, const QString& path, const QString& title,
                               const QString& artist, const QString& duration, bool isLocal)
{
    if (userAccount.trimmed().isEmpty()) {
        qWarning() << "[HttpRequestV2] addFavorite skipped: userAccount is empty";
        emit signalAddFavoriteResult(false);
        return;
    }

    const int durationSec = durationSecondsFromText(duration);

    // 服务端契约：
    // 接口契约：POST /user/favorites/add?user_account=xxx
    // body: music_path, music_title, artist, duration_sec, is_local
    QString url = "user/favorites/add?user_account=" + QString(QUrl::toPercentEncoding(userAccount));

    QJsonObject json;
    json["music_path"] = path;
    json["music_title"] = title;
    json["artist"] = artist;
    json["duration_sec"] = durationSec;
    json["is_local"] = isLocal;

    // 兼容旧服务端字段
    json["path"] = path;
    json["title"] = title;
    json["duration"] = QString::number(durationSec);

    qDebug() << "[HttpRequestV2] Adding favorite:" << title;

    Network::RequestOptions options;
    options.maxRetries = 0;  // 非幂等写操作，避免自动重试导致重复提交
    m_networkService.postJson(url, json, options,
        [this, title, userAccount](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                bool success = result.value("success").toBool();

                if (success) {
                    m_networkService.invalidateCache(QString("user/favorites?user_account=%1").arg(userAccount));
                }
                qDebug() << "[HttpRequestV2] Add favorite" << (success ? "success" : "failed") << title;
                emit signalAddFavoriteResult(success);
            } else {
                const QString bodyText = QString::fromUtf8(response.body);
                const bool alreadyExists =
                    bodyText.contains(QStringLiteral("宸茬粡鍦ㄥ枩娆㈠垪琛ㄤ腑")) ||
                    bodyText.contains("duplicate", Qt::CaseInsensitive) ||
                    bodyText.contains("already", Qt::CaseInsensitive);

                if (alreadyExists) {
                    qDebug() << "[HttpRequestV2] Favorite already exists, treat as success:" << title;
                    m_networkService.invalidateCache(QString("user/favorites?user_account=%1").arg(userAccount));
                    emit signalAddFavoriteResult(true);
                    return;
                }

                qWarning() << "[HttpRequestV2] Add favorite error:" << response.errorString
                           << "status:" << response.statusCode
                           << "body:" << bodyText;
                emit signalAddFavoriteResult(false);
            }
        }
    );
}

void HttpRequestV2::getFavorites(const QString& userAccount, bool useCache)
{
    QString url = QString("user/favorites?user_account=%1").arg(userAccount);

    Network::RequestOptions options;
    if (useCache) {
        options = Network::RequestOptions::withCache(60);
    } else {
        options = Network::RequestOptions::withPriority(Network::RequestPriority::High);
        options.useCache = false;
    }

    qDebug() << "[HttpRequestV2] Getting favorites for:" << userAccount << "useCache:" << useCache;

    m_networkService.get(url, options,
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                // 服务端直接返回 JSON 数组
                QJsonArray favoritesArray = Network::NetworkService::parseJsonArray(response);

                QHash<QString, QString> localCoverByPath;
                const QList<LocalMusicInfo> localMusicList = LocalMusicCache::instance().getMusicList();
                for (const LocalMusicInfo& info : localMusicList) {
                    if (info.coverUrl.trimmed().isEmpty()) {
                        continue;
                    }
                    const QString key = normalizeMusicPathForLookup(info.filePath);
                    if (!key.isEmpty()) {
                        localCoverByPath.insert(key, info.coverUrl);
                    }
                }

                QVariantList favorites;
                for (const QJsonValue& value : favoritesArray) {
                    QJsonObject favObj = value.toObject();
                    QVariantMap favorite;
                    
                    QString path = favObj["path"].toString();
                    bool isLocal = favObj["is_local"].toBool();
                    
                    // 对在线音乐，若 path 是相对路径，需要补全为完整 URL。
                    if (!isLocal) {
                        const QString normalized = normalizeOnlineMediaPath(path, m_baseUrl);
                        if (normalized != path) {
                            qDebug() << "[HttpRequestV2] Favorite path normalized:" << path << "->" << normalized;
                        }
                        path = normalized;
                    }

                    QString coverArtUrl = rewriteServiceUrlToBase(favObj["cover_art_url"].toString(), m_baseUrl);
                    if (isLocal && coverArtUrl.trimmed().isEmpty()) {
                        const QString key = normalizeMusicPathForLookup(path);
                        if (!key.isEmpty()) {
                            const QString localCover = localCoverByPath.value(key);
                            if (!localCover.trimmed().isEmpty()) {
                                coverArtUrl = localCover;
                                qDebug() << "[HttpRequestV2] Filled local favorite cover from cache:" << path;
                            }
                        }
                    }
                    if (coverArtUrl.trimmed().isEmpty()) {
                        coverArtUrl = queryCoverForMusicPath(path);
                    }
                    rememberCoverForMusicPath(path, coverArtUrl);
                    rememberCoverForSongMeta(favObj["title"].toString(), readArtistFromObject(favObj), coverArtUrl);
                    
                    favorite["path"] = path;
                    favorite["title"] = favObj["title"].toString();
                    favorite["artist"] = favObj["artist"].toString();
                    favorite["duration"] = favObj["duration"].toString();
                    favorite["is_local"] = isLocal;
                    favorite["added_at"] = favObj["added_at"].toString();
                    favorite["cover_art_url"] = coverArtUrl;
                    
                    favorites.append(favorite);
                }
                
                qDebug() << "[HttpRequestV2] Favorites received:" << favorites.size()
                         << (response.isFromCache ? "(from cache)" : "");
                emit signalFavoritesList(favorites);
            } else {
                qWarning() << "[HttpRequestV2] Get favorites error:" << response.errorString;
            }
        }
    );
}

void HttpRequestV2::addPlayHistory(const QString& userAccount, const QString& path, const QString& title,
                                  const QString& artist, const QString& album, const QString& duration, bool isLocal)
{
    if (userAccount.trimmed().isEmpty()) {
        qWarning() << "[HttpRequestV2] addPlayHistory skipped: userAccount is empty";
        emit signalAddHistoryResult(false);
        return;
    }

    // Keep request contract aligned with backend:
    // 接口契约：POST /user/history/add?user_account=xxx
    // body: music_path, music_title, artist, album, duration_sec, is_local
    QString url = "user/history/add?user_account=" + QString(QUrl::toPercentEncoding(userAccount));

    int durationSec = 0;
    const QString trimmedDuration = duration.trimmed();
    if (trimmedDuration.contains(":")) {
        const QStringList parts = trimmedDuration.split(":");
        if (parts.size() >= 2) {
            durationSec = parts[0].toInt() * 60 + parts[1].toInt();
        }
    } else if (trimmedDuration.contains(" ")) {
        durationSec = static_cast<int>(trimmedDuration.split(" ").first().toDouble());
    } else {
        durationSec = trimmedDuration.toInt();
    }

    QJsonObject json;
    json["music_path"] = path;
    json["music_title"] = title;
    json["artist"] = artist;
    json["album"] = album;
    json["duration_sec"] = durationSec;
    json["is_local"] = isLocal;

    // Compatibility fields for older backend implementations.
    json["path"] = path;
    json["title"] = title;
    json["duration"] = QString::number(durationSec);

    auto options = Network::RequestOptions::withPriority(Network::RequestPriority::Low);

    m_networkService.postJson(url, json, options,
        [this, title](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                bool success = result.value("success").toBool();
                emit signalAddHistoryResult(success);
            } else {
                qDebug() << "[HttpRequestV2] Add history failed (not critical):" << response.errorString;
                emit signalAddHistoryResult(false);
            }
        }
    );
}

void HttpRequestV2::getPlayHistory(const QString& userAccount, int limit, bool useCache)
{
    QString url = QString("user/history?user_account=%1&limit=%2").arg(userAccount).arg(limit);
    
    Network::RequestOptions options;
    if (useCache) {
        options = Network::RequestOptions::withCache(30);  // 缓存 30 秒
    } else {
        options = Network::RequestOptions::withPriority(Network::RequestPriority::High);
        options.useCache = false;
    }

    qDebug() << "[HttpRequestV2] Getting play history for:" << userAccount
             << "limit:" << limit << "useCache:" << useCache;
    
    m_networkService.get(url, options,
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                // 服务端直接返回 JSON 数组
                QJsonArray historyArray = Network::NetworkService::parseJsonArray(response);

                QHash<QString, QString> localDurationByPath;
                const QList<LocalMusicInfo> localMusicList = LocalMusicCache::instance().getMusicList();
                for (const LocalMusicInfo& info : localMusicList) {
                    const QString key = normalizeMusicPathForLookup(info.filePath);
                    if (key.isEmpty()) {
                        continue;
                    }
                    const QString normalizedDuration = durationTextFromValue(QJsonValue(info.duration));
                    if (!normalizedDuration.isEmpty()) {
                        localDurationByPath.insert(key, normalizedDuration);
                    }
                }

                QVariantList history;
                for (const QJsonValue& value : historyArray) {
                    QJsonObject histObj = value.toObject();
                    QVariantMap historyItem;
                    
                    QString path = histObj["path"].toString();
                    bool isLocal = histObj["is_local"].toBool();
                    
                    // 对在线音乐，若 path 是相对路径，需要补全为完整 URL。
                    if (!isLocal) {
                        const QString normalized = normalizeOnlineMediaPath(path, m_baseUrl);
                        if (normalized != path) {
                            qDebug() << "[HttpRequestV2] History path normalized:" << path << "->" << normalized;
                        }
                        path = normalized;
                    }

                    QString duration = readDurationFromObject(histObj);
                    if (duration.isEmpty()) {
                        const QString key = normalizeMusicPathForLookup(path);
                        if (!key.isEmpty()) {
                            const QString cachedDuration = localDurationByPath.value(key);
                            if (!cachedDuration.isEmpty()) {
                                duration = cachedDuration;
                                qDebug() << "[HttpRequestV2] Filled history duration from local cache:"
                                         << path << duration;
                            }
                        }
                    }
                    
                    historyItem["path"] = path;
                    historyItem["title"] = histObj["title"].toString();
                    historyItem["artist"] = readArtistFromObject(histObj);
                    historyItem["album"] = histObj["album"].toString();
                    historyItem["duration"] = duration;
                    historyItem["is_local"] = isLocal;
                    historyItem["play_time"] = histObj["play_time"].toString();
                    QString historyCover = rewriteServiceUrlToBase(histObj["cover_art_url"].toString(), m_baseUrl);
                    if (historyCover.trimmed().isEmpty()) {
                        historyCover = queryCoverForMusicPath(path);
                    }
                    historyItem["cover_art_url"] = historyCover;
                    rememberCoverForMusicPath(path, historyCover);
                    rememberCoverForSongMeta(histObj["title"].toString(), readArtistFromObject(histObj), historyCover);
                    
                    history.append(historyItem);
                }
                
                qDebug() << "[HttpRequestV2] History received:" << history.size()
                         << (response.isFromCache ? "(from cache)" : "");
                emit signalHistoryList(history);
            } else {
                qWarning() << "[HttpRequestV2] Get history error:" << response.errorString;
            }
        }
    );
}

void HttpRequestV2::getAudioRecommendations(const QString& userId,
                                            const QString& scene,
                                            int limit,
                                            bool excludePlayed,
                                            const QString& cursor)
{
    const QString trimmedUser = userId.trimmed();
    if (trimmedUser.isEmpty()) {
        qWarning() << "[HttpRequestV2] getAudioRecommendations skipped: userId is empty";
        emit signalRecommendationList(QVariantMap(), QVariantList());
        return;
    }

    const int safeLimit = qBound(1, limit, 100);

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("user_id"), trimmedUser);
    // 兼容字段，服务端可按 user_account / header 回退
    query.addQueryItem(QStringLiteral("user_account"), trimmedUser);
    query.addQueryItem(QStringLiteral("scene"), scene.trimmed().isEmpty() ? QStringLiteral("home") : scene.trimmed());
    query.addQueryItem(QStringLiteral("limit"), QString::number(safeLimit));
    query.addQueryItem(QStringLiteral("exclude_played"), excludePlayed ? QStringLiteral("true") : QStringLiteral("false"));
    if (!cursor.trimmed().isEmpty()) {
        query.addQueryItem(QStringLiteral("cursor"), cursor.trimmed());
    }

    QString url = QStringLiteral("recommendations/audio");
    const QString encodedQuery = query.toString(QUrl::FullyEncoded);
    if (!encodedQuery.isEmpty()) {
        url += QStringLiteral("?") + encodedQuery;
    }

    auto options = Network::RequestOptions::withPriority(Network::RequestPriority::High);
    options.useCache = false;

    qDebug() << "[HttpRequestV2] Getting recommendations for user:" << trimmedUser
             << "scene:" << scene << "limit:" << safeLimit;

    m_networkService.get(url, options,
        [this](const Network::NetworkResponse& response) {
            if (!response.isSuccess()) {
                qWarning() << "[HttpRequestV2] Get recommendations error:" << response.errorString;
                emit signalRecommendationList(QVariantMap(), QVariantList());
                return;
            }

            const QJsonObject rootObj = Network::NetworkService::parseJsonObject(response);
            QJsonObject dataObj = rootObj;
            if (rootObj.contains(QStringLiteral("data")) && rootObj.value(QStringLiteral("data")).isObject()) {
                dataObj = rootObj.value(QStringLiteral("data")).toObject();
            }

            QVariantMap meta;
            meta.insert(QStringLiteral("request_id"), dataObj.value(QStringLiteral("request_id")).toString());
            meta.insert(QStringLiteral("user_id"), dataObj.value(QStringLiteral("user_id")).toString());
            meta.insert(QStringLiteral("scene"), dataObj.value(QStringLiteral("scene")).toString());
            meta.insert(QStringLiteral("model_version"), dataObj.value(QStringLiteral("model_version")).toString());
            meta.insert(QStringLiteral("next_cursor"), dataObj.value(QStringLiteral("next_cursor")).toString());

            QVariantList items;
            const QJsonArray itemArray = dataObj.value(QStringLiteral("items")).toArray();
            items.reserve(itemArray.size());

            for (const QJsonValue& value : itemArray) {
                if (!value.isObject()) {
                    continue;
                }

                const QJsonObject itemObj = value.toObject();
                const QString rawPath = itemObj.value(QStringLiteral("path")).toString();
                const QString rawStreamUrl = itemObj.value(QStringLiteral("stream_url")).toString();

                QString streamUrl = rewriteServiceUrlToBase(rawStreamUrl, m_baseUrl);
                if (streamUrl.trimmed().isEmpty() && !rawPath.trimmed().isEmpty()) {
                    streamUrl = normalizeOnlineMediaPath(rawPath, m_baseUrl);
                }

                const double durationSec = itemObj.value(QStringLiteral("duration_sec")).toDouble(0.0);
                QString durationText = formatDurationFromSeconds(static_cast<int>(durationSec + 0.5));
                if (durationText.isEmpty()) {
                    durationText = readDurationFromObject(itemObj);
                }

                QVariantMap item;
                item.insert(QStringLiteral("song_id"), itemObj.value(QStringLiteral("song_id")).toString());
                item.insert(QStringLiteral("path"), rawPath);
                item.insert(QStringLiteral("play_path"), streamUrl);
                item.insert(QStringLiteral("title"), itemObj.value(QStringLiteral("title")).toString());
                item.insert(QStringLiteral("artist"), readArtistFromObject(itemObj));
                item.insert(QStringLiteral("album"), itemObj.value(QStringLiteral("album")).toString());
                item.insert(QStringLiteral("duration_sec"), durationSec);
                item.insert(QStringLiteral("duration"), durationText);
                QString coverArtUrl = rewriteServiceUrlToBase(
                    itemObj.value(QStringLiteral("cover_art_url")).toString(), m_baseUrl);
                if (coverArtUrl.trimmed().isEmpty()) {
                    coverArtUrl = queryCoverForMusicPath(rawPath);
                }
                item.insert(QStringLiteral("cover_art_url"), coverArtUrl);
                item.insert(QStringLiteral("stream_url"), streamUrl);
                item.insert(QStringLiteral("lrc_url"),
                            rewriteServiceUrlToBase(itemObj.value(QStringLiteral("lrc_url")).toString(), m_baseUrl));
                item.insert(QStringLiteral("score"), itemObj.value(QStringLiteral("score")).toDouble());
                item.insert(QStringLiteral("reason"), itemObj.value(QStringLiteral("reason")).toString());
                item.insert(QStringLiteral("source"), itemObj.value(QStringLiteral("source")).toString());
                item.insert(QStringLiteral("request_id"), meta.value(QStringLiteral("request_id")));
                item.insert(QStringLiteral("model_version"), meta.value(QStringLiteral("model_version")));
                item.insert(QStringLiteral("scene"), meta.value(QStringLiteral("scene")));

                rememberCoverForMusicPath(rawPath, coverArtUrl);
                rememberCoverForMusicPath(streamUrl, coverArtUrl);
                rememberCoverForSongMeta(itemObj.value(QStringLiteral("title")).toString(),
                                         readArtistFromObject(itemObj),
                                         coverArtUrl);
                items.append(item);
            }

            qDebug() << "[HttpRequestV2] Recommendations received:" << items.size();
            emit signalRecommendationList(meta, items);
        }
    );
}

void HttpRequestV2::getSimilarRecommendations(const QString& songId, int limit)
{
    const QString trimmedSongId = songId.trimmed();
    if (trimmedSongId.isEmpty()) {
        qWarning() << "[HttpRequestV2] getSimilarRecommendations skipped: songId is empty";
        emit signalSimilarRecommendationList(QVariantMap(), QVariantList(), QString());
        return;
    }

    const int safeLimit = qBound(1, limit, 100);
    const QString encodedSongId = QString::fromUtf8(QUrl::toPercentEncoding(trimmedSongId));
    const QString url = QStringLiteral("recommendations/similar/%1?limit=%2")
                            .arg(encodedSongId, QString::number(safeLimit));

    auto options = Network::RequestOptions::withPriority(Network::RequestPriority::High);
    options.useCache = false;

    qDebug() << "[HttpRequestV2] Getting similar recommendations for songId:"
             << trimmedSongId << "limit:" << safeLimit;

    m_networkService.get(url, options,
        [this, trimmedSongId](const Network::NetworkResponse& response) {
            if (!response.isSuccess()) {
                qWarning() << "[HttpRequestV2] Get similar recommendations error:"
                           << response.errorString << "status:" << response.statusCode;
                emit signalSimilarRecommendationList(QVariantMap(), QVariantList(), trimmedSongId);
                return;
            }

            const QJsonObject rootObj = Network::NetworkService::parseJsonObject(response);
            QJsonObject dataObj = rootObj;
            if (rootObj.contains(QStringLiteral("data")) && rootObj.value(QStringLiteral("data")).isObject()) {
                dataObj = rootObj.value(QStringLiteral("data")).toObject();
            }

            QVariantMap meta;
            meta.insert(QStringLiteral("request_id"), dataObj.value(QStringLiteral("request_id")).toString());
            meta.insert(QStringLiteral("user_id"), dataObj.value(QStringLiteral("user_id")).toString());
            meta.insert(QStringLiteral("scene"), dataObj.value(QStringLiteral("scene")).toString());
            meta.insert(QStringLiteral("model_version"), dataObj.value(QStringLiteral("model_version")).toString());
            meta.insert(QStringLiteral("next_cursor"), dataObj.value(QStringLiteral("next_cursor")).toString());

            if (meta.value(QStringLiteral("scene")).toString().trimmed().isEmpty()) {
                meta.insert(QStringLiteral("scene"), QStringLiteral("detail"));
            }

            QVariantList items;
            const QJsonArray itemArray = dataObj.value(QStringLiteral("items")).toArray();
            items.reserve(itemArray.size());

            for (const QJsonValue& value : itemArray) {
                if (!value.isObject()) {
                    continue;
                }

                const QJsonObject itemObj = value.toObject();
                const QString rawPath = itemObj.value(QStringLiteral("path")).toString();
                const QString rawStreamUrl = itemObj.value(QStringLiteral("stream_url")).toString();

                QString streamUrl = rewriteServiceUrlToBase(rawStreamUrl, m_baseUrl);
                if (streamUrl.trimmed().isEmpty() && !rawPath.trimmed().isEmpty()) {
                    streamUrl = normalizeOnlineMediaPath(rawPath, m_baseUrl);
                }

                const double durationSec = itemObj.value(QStringLiteral("duration_sec")).toDouble(0.0);
                QString durationText = formatDurationFromSeconds(static_cast<int>(durationSec + 0.5));
                if (durationText.isEmpty()) {
                    durationText = readDurationFromObject(itemObj);
                }

                QVariantMap item;
                const QString itemSongId = itemObj.value(QStringLiteral("song_id")).toString();
                item.insert(QStringLiteral("song_id"), itemSongId.isEmpty() ? rawPath : itemSongId);
                item.insert(QStringLiteral("path"), rawPath);
                item.insert(QStringLiteral("play_path"), streamUrl);
                item.insert(QStringLiteral("title"), itemObj.value(QStringLiteral("title")).toString());
                item.insert(QStringLiteral("artist"), readArtistFromObject(itemObj));
                item.insert(QStringLiteral("album"), itemObj.value(QStringLiteral("album")).toString());
                item.insert(QStringLiteral("duration_sec"), durationSec);
                item.insert(QStringLiteral("duration"), durationText);
                QString coverArtUrl = rewriteServiceUrlToBase(
                    itemObj.value(QStringLiteral("cover_art_url")).toString(), m_baseUrl);
                if (coverArtUrl.trimmed().isEmpty()) {
                    coverArtUrl = queryCoverForMusicPath(rawPath);
                }
                item.insert(QStringLiteral("cover_art_url"), coverArtUrl);
                item.insert(QStringLiteral("stream_url"), streamUrl);
                item.insert(QStringLiteral("lrc_url"),
                            rewriteServiceUrlToBase(itemObj.value(QStringLiteral("lrc_url")).toString(), m_baseUrl));
                item.insert(QStringLiteral("score"), itemObj.value(QStringLiteral("score")).toDouble());
                item.insert(QStringLiteral("reason"), itemObj.value(QStringLiteral("reason")).toString());
                item.insert(QStringLiteral("source"), itemObj.value(QStringLiteral("source")).toString());
                item.insert(QStringLiteral("request_id"), meta.value(QStringLiteral("request_id")));
                item.insert(QStringLiteral("model_version"), meta.value(QStringLiteral("model_version")));
                item.insert(QStringLiteral("scene"), meta.value(QStringLiteral("scene")));

                rememberCoverForMusicPath(rawPath, coverArtUrl);
                rememberCoverForMusicPath(streamUrl, coverArtUrl);
                rememberCoverForSongMeta(itemObj.value(QStringLiteral("title")).toString(),
                                         readArtistFromObject(itemObj),
                                         coverArtUrl);
                items.append(item);
            }

            qDebug() << "[HttpRequestV2] Similar recommendations received:" << items.size()
                     << "anchorSongId:" << trimmedSongId;
            emit signalSimilarRecommendationList(meta, items, trimmedSongId);
        }
    );
}

void HttpRequestV2::postRecommendationFeedback(const QString& userId,
                                               const QString& songId,
                                               const QString& eventType,
                                               const QString& scene,
                                               const QString& requestId,
                                               const QString& modelVersion,
                                               qint64 playMs,
                                               qint64 durationMs)
{
    const QString trimmedUser = userId.trimmed();
    const QString trimmedSongId = songId.trimmed();
    const QString trimmedEventType = eventType.trimmed();

    if (trimmedUser.isEmpty() || trimmedSongId.isEmpty() || trimmedEventType.isEmpty()) {
        qWarning() << "[HttpRequestV2] postRecommendationFeedback skipped: missing required params"
                   << "userIdEmpty=" << trimmedUser.isEmpty()
                   << "songIdEmpty=" << trimmedSongId.isEmpty()
                   << "eventTypeEmpty=" << trimmedEventType.isEmpty();
        emit signalRecommendationFeedbackResult(false, trimmedEventType, trimmedSongId);
        return;
    }

    QJsonObject json;
    json[QStringLiteral("user_id")] = trimmedUser;
    json[QStringLiteral("song_id")] = trimmedSongId;
    json[QStringLiteral("event_type")] = trimmedEventType;
    json[QStringLiteral("scene")] = scene.trimmed().isEmpty() ? QStringLiteral("home") : scene.trimmed();

    if (!requestId.trimmed().isEmpty()) {
        json[QStringLiteral("request_id")] = requestId.trimmed();
    }
    if (!modelVersion.trimmed().isEmpty()) {
        json[QStringLiteral("model_version")] = modelVersion.trimmed();
    }
    if (playMs >= 0) {
        json[QStringLiteral("play_ms")] = static_cast<double>(playMs);
    }
    if (durationMs >= 0) {
        json[QStringLiteral("duration_ms")] = static_cast<double>(durationMs);
    }

    Network::RequestOptions options = Network::RequestOptions::withPriority(Network::RequestPriority::Low);
    options.maxRetries = 0;
    options.useCache = false;

    m_networkService.postJson(QStringLiteral("recommendations/feedback"), json, options,
        [this, trimmedEventType, trimmedSongId](const Network::NetworkResponse& response) {
            bool success = false;
            if (response.isSuccess()) {
                const QJsonObject rootObj = Network::NetworkService::parseJsonObject(response);
                QJsonObject dataObj = rootObj;
                if (rootObj.contains(QStringLiteral("data")) && rootObj.value(QStringLiteral("data")).isObject()) {
                    dataObj = rootObj.value(QStringLiteral("data")).toObject();
                }
                success = dataObj.value(QStringLiteral("success")).toBool(true);
            } else {
                qWarning() << "[HttpRequestV2] Recommendation feedback error:" << response.errorString
                           << "status:" << response.statusCode;
            }

            emit signalRecommendationFeedbackResult(success, trimmedEventType, trimmedSongId);
        }
    );
}

void HttpRequestV2::getVideoList()
{
    auto options = Network::RequestOptions::withCache(300);  // 缓存 5 分钟
    
    m_networkService.get("videos", options,
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonArray jsonArray = Network::NetworkService::parseJsonArray(response);
                QVariantList videoList;
                
                for (const QJsonValue& value : jsonArray) {
                    QJsonObject obj = value.toObject();
                    QVariantMap videoInfo;
                    videoInfo["name"] = obj["name"].toString();
                    videoInfo["path"] = obj["path"].toString();
                    videoInfo["size"] = obj["size"].toVariant().toLongLong();
                    videoList.append(videoInfo);
                }
                
                qDebug() << "[HttpRequestV2] Video list:" << videoList.size();
                emit signalVideoList(videoList);
            } else {
                qWarning() << "[HttpRequestV2] Get video list error:" << response.errorString;
            }
        }
    );
}

void HttpRequestV2::getVideoStreamUrl(const QString& videoPath)
{
    QJsonObject json;
    json["path"] = videoPath;
    
    m_networkService.postJson("video/stream", json, {},
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                QString videoUrl = rewriteServiceUrlToBase(result["url"].toString(), m_baseUrl);
                
                qDebug() << "[HttpRequestV2] Video stream URL:" << videoUrl;
                emit signalVideoStreamUrl(videoUrl);
            } else {
                qWarning() << "[HttpRequestV2] Get video stream error:" << response.errorString;
            }
        }
    );
}

void HttpRequestV2::searchArtist(const QString& artist)
{
    QJsonObject json;
    json["artist"] = artist;
    
    // 搜索使用高优先级
    auto options = Network::RequestOptions::withPriority(Network::RequestPriority::High);
    
    m_networkService.postJson("artist/search", json, options,
        [this, artist](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                bool exists = result["exists"].toBool();
                
                qDebug() << "[HttpRequestV2] Artist" << artist << "exists:" << exists;
                emit signalArtistExists(exists, artist);
            } else {
                qWarning() << "[HttpRequestV2] Search artist error:" << response.errorString;
                emit signalArtistExists(false, artist);
            }
        }
    );
}

void HttpRequestV2::getMusicByArtist(const QString& artist)
{
    QJsonObject json;
    json["artist"] = artist;
    
    // 缓存艺术家音乐列表。
    auto options = Network::RequestOptions::withCache(600);  // 缓存 10 分钟
    
    m_networkService.postJson("music/artist", json, options,
        [this, artist](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonArray musicArray;
                QJsonParseError parseError;
                const QJsonDocument doc = QJsonDocument::fromJson(response.body, &parseError);
                if (parseError.error == QJsonParseError::NoError) {
                    if (doc.isArray()) {
                        musicArray = doc.array();
                    } else if (doc.isObject()) {
                        const QJsonObject obj = doc.object();
                        if (obj.value("music_list").isArray()) {
                            musicArray = obj.value("music_list").toArray();
                        } else if (obj.value("musics").isArray()) {
                            musicArray = obj.value("musics").toArray();
                        }
                    }
                }
                
                QList<Music> musicList;
                for (const auto& item : musicArray) {
                    QJsonObject obj = item.toObject();
                    
                    QString songPath = obj["path"].toString();
                    songPath = normalizeOnlineMediaPath(songPath, m_baseUrl);
                    QString artistName = readArtistFromObject(obj);
                    if (artistName.trimmed().isEmpty()) {
                        artistName = QStringLiteral("Unknown Artist");
                    }
                    const QString durationText = readDurationFromObject(obj);
                    int durationSec = 0;
                    if (!durationText.isEmpty()) {
                        const QStringList parts = durationText.split(':');
                        if (parts.size() == 2) {
                            durationSec = parts[0].toInt() * 60 + parts[1].toInt();
                        }
                    }
                    if (durationSec <= 0 && obj.value("duration").isDouble()) {
                        durationSec = static_cast<int>(obj.value("duration").toDouble());
                    }

                    QString coverRaw = obj.value("cover_art_url").toString();
                    if (coverRaw.trimmed().isEmpty()) {
                        coverRaw = obj.value("cover_url").toString();
                    }
                    if (coverRaw.trimmed().isEmpty()) {
                        coverRaw = obj.value("album_cover_url").toString();
                    }

                    Music music;
                    music.setSongPath(songPath);
                    music.setSinger(artistName);
                    music.setDuration(durationSec);
                    const QString coverUrl = rewriteServiceUrlToBase(coverRaw, m_baseUrl);
                    music.setPicPath(coverUrl);
                    rememberCoverForMusicPath(songPath, coverUrl);
                    rememberCoverForSongMeta(fileNameFromPath(songPath), artistName, coverUrl);
                    
                    musicList.append(music);
                }
                
                qDebug() << "[HttpRequestV2] Music by artist" << artist << ":" << musicList.size();
                emit signalArtistMusicList(musicList, artist);
            } else {
                qWarning() << "[HttpRequestV2] Get music by artist error:" << response.errorString;
            }
        }
    );
}

void HttpRequestV2::getMusicData(const QString& fileName)
{
    const QString trimmedName = fileName.trimmed();
    if (trimmedName.isEmpty()) {
        qWarning() << "[HttpRequestV2] getMusicData called with empty filename";
        emit signalStreamurl(false, "");
        return;
    }

    // Fast path: if the input is already an absolute media URL, play it directly.
    if (trimmedName.startsWith("http://", Qt::CaseInsensitive) ||
        trimmedName.startsWith("https://", Qt::CaseInsensitive)) {
        const QString resolved = rewriteServiceUrlToBase(trimmedName, m_baseUrl);
        emit signalStreamurl(true, resolved);
        return;
    }

    QString relativePath = trimmedName;
    relativePath.replace('\\', '/');
    while (relativePath.startsWith('/')) {
        relativePath.remove(0, 1);
    }
    if (relativePath.startsWith("uploads/", Qt::CaseInsensitive)) {
        relativePath = relativePath.mid(QStringLiteral("uploads/").size());
    }

    if (relativePath.contains('/')) {
        const QString directUrl = QUrl(m_baseUrl + "uploads/" + relativePath).toString();
        qDebug() << "[HttpRequestV2] Fast stream URL (skip /stream):" << directUrl;
        emit signalStreamurl(true, directUrl);
        return;
    }

    QString url = m_baseUrl + "stream";
    
    QJsonObject jsonObj;
    jsonObj["filename"] = trimmedName;
    QJsonDocument jsonDoc(jsonObj);
    QByteArray postData = jsonDoc.toJson();
    
    using namespace Network;
    RequestOptions options = RequestOptions::withPriority(RequestPriority::High);
    
    m_networkService.post(url, postData, options, 
        [this](const NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonDocument jsonDoc = QJsonDocument::fromJson(response.body);
                if (jsonDoc.isObject()) {
                    QJsonObject obj = jsonDoc.object();
                    if (obj.contains("stream_url")) {
                        QString streamUrl = rewriteServiceUrlToBase(obj["stream_url"].toString(), m_baseUrl);
                        qDebug() << "[HttpRequestV2] Music stream URL:" << streamUrl;
                        emit signalStreamurl(true, streamUrl);
                    } else {
                        qWarning() << "[HttpRequestV2] stream_url not found in response";
                        emit signalStreamurl(false, "");
                    }
                } else {
                    qWarning() << "[HttpRequestV2] Invalid JSON in get music data response";
                    emit signalStreamurl(false, "");
                }
            } else {
                qWarning() << "[HttpRequestV2] Get music data error:" << response.errorString;
                emit signalStreamurl(false, "");
            }
        });
}

void HttpRequestV2::addMusic(const QString& musicPath)
{
    QString url = m_baseUrl + "users/add_music";
    
    auto user = User::getInstance();
    QJsonObject jsonObj;
    jsonObj["username"] = user->getUsername();
    jsonObj["music_path"] = musicPath;
    QJsonDocument jsonDoc(jsonObj);
    QByteArray postData = jsonDoc.toJson();
    
    qDebug() << "[HttpRequestV2] Adding music:" << musicPath;
    
    m_networkService.post(url, postData, Network::RequestOptions(), 
        [musicPath](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                qDebug() << "[HttpRequestV2] Add music success:" << musicPath;
            } else {
                qWarning() << "[HttpRequestV2] Add music error:" << response.errorString;
            }
        });
}

void HttpRequestV2::getMusic(const QString& keyword)
{
    QJsonObject json;
    json["keyword"] = keyword;
    
    qDebug() << "[HttpRequestV2] Searching music with keyword:" << keyword;
    
    m_networkService.postJson("music/search", json, Network::RequestOptions::withCache(),
        [this](const Network::NetworkResponse& response) {
            if (response.isSuccess()) {
                QJsonArray filesArray = Network::NetworkService::parseJsonArray(response);
                
                qDebug() << "[HttpRequestV2] Search found" << filesArray.size() << "results";
                QList<Music> musicList;
                
                for (const QJsonValue& value : filesArray) {
                    if (value.isObject()) {
                        QJsonObject fileObj = value.toObject();
                        QString filePath = fileObj["path"].toString();
                        filePath = normalizeOnlineMediaPath(filePath, m_baseUrl);
                        QString durationStr = fileObj["duration"].toString();
                        QString coverUrl = rewriteServiceUrlToBase(fileObj["cover_art_url"].toString(), m_baseUrl);
                        rememberCoverForMusicPath(filePath, coverUrl);
                        QString artist = readArtistFromObject(fileObj);
                        if (artist.trimmed().isEmpty()) {
                            artist = QStringLiteral("Unknown Artist");
                        }

                        // 解析时长字符串："239.49 seconds" -> 秒数
                        double durationValue = 0.0;
                        if (durationStr != "Error") {
                            QStringList parts = durationStr.split(" ");
                            if (!parts.isEmpty()) {
                                durationValue = parts[0].toDouble();
                            }
                        }
                        
                        int totalSeconds = static_cast<int>(durationValue);
                        int minutes = totalSeconds / 60;
                        int seconds = totalSeconds % 60;
                        
                        // 使用 Music 的 setter 方法设置属性
                        Music music;
                        music.setSongPath(filePath);
                        music.setDuration(totalSeconds * 1000);  // 转换为毫秒
                        music.setPicPath(coverUrl);
                        music.setSinger(artist);

                        musicList.append(music);
                    }
                }
                
                emit signalAddSongList(musicList);
            } else {
                qWarning() << "[HttpRequestV2] Search music error:" << response.errorString;
            }
        });
}

void HttpRequestV2::removeFavorite(const QString& userAccount, const QStringList& paths)
{
    if (paths.isEmpty()) {
        qWarning() << "[HttpRequestV2] removeFavorite called with empty paths";
        emit signalRemoveFavoriteResult(false);
        return;
    }
    
    QJsonObject json;
    json["user_account"] = userAccount;
    json["music_path"] = paths[0];  // 服务端接口仅支持单条删除
    
    qDebug() << "[HttpRequestV2] Removing favorite for user:" << userAccount << "path:" << paths[0];
    
    // 使用 POST 请求删除收藏（服务端通过路由区分操作）
    m_networkService.postJson("user/favorites/remove", json, Network::RequestOptions(),
        [this, userAccount](const Network::NetworkResponse& response) {
            bool success = false;
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                success = result["success"].toBool();
                if (success) {
                    m_networkService.invalidateCache(QString("user/favorites?user_account=%1").arg(userAccount));
                }
                qDebug() << "[HttpRequestV2] Remove favorite success:" << success;
            } else {
                qWarning() << "[HttpRequestV2] Remove favorite error:" << response.errorString;
            }
            emit signalRemoveFavoriteResult(success);
        });
}

void HttpRequestV2::removePlayHistory(const QString& userAccount, const QStringList& paths)
{
    if (paths.isEmpty()) {
        qWarning() << "[HttpRequestV2] removePlayHistory called with empty paths";
        emit signalRemoveHistoryResult(false);
        return;
    }
    
    QJsonObject json;
    QJsonArray pathArray;
    for (const QString& path : paths) {
        pathArray.append(path);
    }
    json["music_paths"] = pathArray;
    
    QString url = "user/history/delete?user_account=" + QString(QUrl::toPercentEncoding(userAccount));
    
    qDebug() << "[HttpRequestV2] Deleting play history for user:" << userAccount << "paths count:" << paths.size();
    
    m_networkService.postJson(url, json, Network::RequestOptions(),
        [this, userAccount](const Network::NetworkResponse& response) {
            bool success = false;
            int deletedCount = 0;
            
            if (response.isSuccess()) {
                QJsonObject result = Network::NetworkService::parseJsonObject(response);
                success = result["success"].toBool();
                deletedCount = result["deleted_count"].toInt();
                qDebug() << "[HttpRequestV2] Delete history success:" << success << "deleted count:" << deletedCount;
                if (success) {
                    m_networkService.invalidateCache(QString("user/history?user_account=%1&limit=50").arg(userAccount));
                }
            } else {
                qWarning() << "[HttpRequestV2] Delete history error:" << response.errorString;
            }
            
            emit signalRemoveHistoryResult(success);
        });
}

void HttpRequestV2::getPlaylists(const QString& userAccount, int page, int pageSize, bool useCache)
{
    const QString trimmedUser = userAccount.trimmed();
    const int safePage = qMax(1, page);
    const int safePageSize = qBound(1, pageSize, 100);
    if (trimmedUser.isEmpty()) {
        qWarning() << "[HttpRequestV2] getPlaylists skipped: userAccount is empty";
        emit signalPlaylistsList(QVariantList(), safePage, safePageSize, 0);
        return;
    }

    const QString url = QString("user/playlists?user_account=%1&page=%2&page_size=%3")
                            .arg(QString(QUrl::toPercentEncoding(trimmedUser)))
                            .arg(safePage)
                            .arg(safePageSize);

    Network::RequestOptions options;
    if (useCache) {
        options = Network::RequestOptions::withCache(30);
    } else {
        options = Network::RequestOptions::withPriority(Network::RequestPriority::High);
        options.useCache = false;
    }

    m_networkService.get(url, options,
        [this, safePage, safePageSize](const Network::NetworkResponse& response) {
            if (!response.isSuccess()) {
                qWarning() << "[HttpRequestV2] Get playlists error:" << response.errorString;
                emit signalPlaylistsList(QVariantList(), safePage, safePageSize, 0);
                return;
            }

            QJsonObject root = Network::NetworkService::parseJsonObject(response);
            QJsonObject data = unwrapDataObject(root);
            QJsonArray itemsArray;
            if (data.value(QStringLiteral("items")).isArray()) {
                itemsArray = data.value(QStringLiteral("items")).toArray();
            } else if (root.value(QStringLiteral("items")).isArray()) {
                itemsArray = root.value(QStringLiteral("items")).toArray();
            } else {
                itemsArray = Network::NetworkService::parseJsonArray(response);
            }

            const int pageValue = data.value(QStringLiteral("page")).toInt(
                root.value(QStringLiteral("page")).toInt(safePage));
            const int pageSizeValue = data.value(QStringLiteral("page_size")).toInt(
                root.value(QStringLiteral("page_size")).toInt(safePageSize));
            const int totalValue = data.value(QStringLiteral("total")).toInt(
                root.value(QStringLiteral("total")).toInt(itemsArray.size()));

            QVariantList playlists;
            playlists.reserve(itemsArray.size());
            for (const QJsonValue& value : itemsArray) {
                if (!value.isObject()) {
                    continue;
                }
                const QJsonObject obj = value.toObject();
                QVariantMap playlist;
                const int totalDurationSec = obj.value(QStringLiteral("total_duration_sec")).toInt();

                playlist.insert(QStringLiteral("id"), obj.value(QStringLiteral("id")).toVariant().toLongLong());
                playlist.insert(QStringLiteral("name"), obj.value(QStringLiteral("name")).toString());
                playlist.insert(QStringLiteral("description"), obj.value(QStringLiteral("description")).toString());
                playlist.insert(QStringLiteral("cover_url"),
                                rewriteServiceUrlToBase(obj.value(QStringLiteral("cover_url")).toString(), m_baseUrl));
                playlist.insert(QStringLiteral("track_count"), obj.value(QStringLiteral("track_count")).toInt());
                playlist.insert(QStringLiteral("total_duration_sec"), totalDurationSec);
                playlist.insert(QStringLiteral("total_duration"), formatDurationFromSeconds(totalDurationSec));
                playlist.insert(QStringLiteral("created_at"), obj.value(QStringLiteral("created_at")).toString());
                playlist.insert(QStringLiteral("updated_at"), obj.value(QStringLiteral("updated_at")).toString());
                playlist.insert(QStringLiteral("ownership"), normalizePlaylistOwnership(obj));
                playlists.append(playlist);
            }

            emit signalPlaylistsList(playlists, pageValue, pageSizeValue, totalValue);
        });
}

void HttpRequestV2::createPlaylist(const QString& userAccount,
                                   const QString& name,
                                   const QString& description,
                                   const QString& coverPath)
{
    const QString trimmedUser = userAccount.trimmed();
    const QString trimmedName = name.trimmed();
    if (trimmedUser.isEmpty() || trimmedName.isEmpty()) {
        emit signalCreatePlaylistResult(false, 0, QStringLiteral("歌单名称不能为空"));
        return;
    }

    const QString url = QString("user/playlists?user_account=%1")
                            .arg(QString(QUrl::toPercentEncoding(trimmedUser)));

    QJsonObject json;
    json[QStringLiteral("name")] = trimmedName;
    if (!description.trimmed().isEmpty()) {
        json[QStringLiteral("description")] = description.trimmed();
    }
    if (!coverPath.trimmed().isEmpty()) {
        json[QStringLiteral("cover_path")] = coverPath.trimmed();
    }

    Network::RequestOptions options = Network::RequestOptions::withPriority(Network::RequestPriority::High);
    options.maxRetries = 0;
    options.useCache = false;

    m_networkService.postJson(url, json, options,
        [this, trimmedUser](const Network::NetworkResponse& response) {
            bool success = false;
            qint64 playlistId = 0;
            QString message = QStringLiteral("创建失败");

            if (response.isSuccess()) {
                const QJsonObject root = Network::NetworkService::parseJsonObject(response);
                const QJsonObject data = unwrapDataObject(root);
                success = parseSuccessFlag(data, parseSuccessFlag(root, true));
                playlistId = data.value(QStringLiteral("playlist_id")).toVariant().toLongLong();
                if (playlistId <= 0) {
                    playlistId = root.value(QStringLiteral("playlist_id")).toVariant().toLongLong();
                }
                message = parseMessageText(data, parseMessageText(root, success ? QStringLiteral("创建成功")
                                                                                 : QStringLiteral("创建失败")));
                if (success) {
                    m_networkService.invalidateCache(
                        QString("user/playlists?user_account=%1&page=1&page_size=20").arg(trimmedUser));
                    m_networkService.invalidateCache(
                        QString("user/playlists?user_account=%1").arg(trimmedUser));
                }
            } else {
                message = extractErrorMessage(response, QStringLiteral("创建歌单失败"));
            }

            emit signalCreatePlaylistResult(success, playlistId, message);
        });
}

void HttpRequestV2::getPlaylistDetail(const QString& userAccount, qint64 playlistId, bool useCache)
{
    const QString trimmedUser = userAccount.trimmed();
    if (trimmedUser.isEmpty() || playlistId <= 0) {
        emit signalPlaylistDetail(QVariantMap());
        return;
    }

    const QString url = QString("user/playlists/%1?user_account=%2")
                            .arg(playlistId)
                            .arg(QString(QUrl::toPercentEncoding(trimmedUser)));

    Network::RequestOptions options;
    if (useCache) {
        options = Network::RequestOptions::withCache(15);
    } else {
        options = Network::RequestOptions::withPriority(Network::RequestPriority::High);
        options.useCache = false;
    }

    m_networkService.get(url, options,
        [this](const Network::NetworkResponse& response) {
            if (!response.isSuccess()) {
                qWarning() << "[HttpRequestV2] Get playlist detail error:" << response.errorString;
                emit signalPlaylistDetail(QVariantMap());
                return;
            }

            const QJsonObject root = Network::NetworkService::parseJsonObject(response);
            const QJsonObject detailObj = unwrapDataObject(root);

            QVariantMap detail;
            detail.insert(QStringLiteral("id"), detailObj.value(QStringLiteral("id")).toVariant().toLongLong());
            detail.insert(QStringLiteral("name"), detailObj.value(QStringLiteral("name")).toString());
            detail.insert(QStringLiteral("description"), detailObj.value(QStringLiteral("description")).toString());
            detail.insert(QStringLiteral("cover_url"),
                          rewriteServiceUrlToBase(detailObj.value(QStringLiteral("cover_url")).toString(), m_baseUrl));
            detail.insert(QStringLiteral("created_at"), detailObj.value(QStringLiteral("created_at")).toString());
            detail.insert(QStringLiteral("updated_at"), detailObj.value(QStringLiteral("updated_at")).toString());

            QHash<QString, QString> localCoverByPath;
            const QList<LocalMusicInfo> localMusicList = LocalMusicCache::instance().getMusicList();
            for (const LocalMusicInfo& info : localMusicList) {
                if (info.coverUrl.trimmed().isEmpty()) {
                    continue;
                }
                const QString key = normalizeMusicPathForLookup(info.filePath);
                if (!key.isEmpty()) {
                    localCoverByPath.insert(key, info.coverUrl);
                }
            }

            QVariantList items;
            int totalDurationSec = 0;
            const QJsonArray itemsArray = detailObj.value(QStringLiteral("items")).toArray();
            items.reserve(itemsArray.size());
            for (const QJsonValue& value : itemsArray) {
                if (!value.isObject()) {
                    continue;
                }

                const QJsonObject obj = value.toObject();
                QVariantMap item;

                QString musicPath = obj.value(QStringLiteral("music_path")).toString();
                if (musicPath.trimmed().isEmpty()) {
                    musicPath = obj.value(QStringLiteral("path")).toString();
                }
                bool isLocal = obj.value(QStringLiteral("is_local")).toBool();
                if (!isLocal) {
                    musicPath = normalizeOnlineMediaPath(musicPath, m_baseUrl);
                }

                QString durationText = readDurationFromObject(obj);
                if (durationText.isEmpty()) {
                    durationText = formatDurationFromSeconds(obj.value(QStringLiteral("duration_sec")).toInt());
                }
                totalDurationSec += obj.value(QStringLiteral("duration_sec")).toInt();

                QString coverArtUrl = rewriteServiceUrlToBase(
                    obj.value(QStringLiteral("cover_art_url")).toString(), m_baseUrl);
                if (coverArtUrl.trimmed().isEmpty()) {
                    coverArtUrl = rewriteServiceUrlToBase(
                        obj.value(QStringLiteral("cover_url")).toString(), m_baseUrl);
                }
                if (coverArtUrl.trimmed().isEmpty()) {
                    coverArtUrl = rewriteServiceUrlToBase(
                        obj.value(QStringLiteral("cover_art_path")).toString(), m_baseUrl);
                }
                if (isLocal && coverArtUrl.trimmed().isEmpty()) {
                    const QString localKey = normalizeMusicPathForLookup(musicPath);
                    if (!localKey.isEmpty()) {
                        coverArtUrl = localCoverByPath.value(localKey);
                    }
                }
                if (coverArtUrl.trimmed().isEmpty()) {
                    coverArtUrl = queryCoverForMusicPath(musicPath);
                }
                if (coverArtUrl.trimmed().isEmpty()) {
                    const QString titleText = obj.value(QStringLiteral("music_title")).toString();
                    const QString artistText = readArtistFromObject(obj);
                    coverArtUrl = queryCoverForSongMeta(titleText, artistText);
                }
                rememberCoverForMusicPath(musicPath, coverArtUrl);
                rememberCoverForSongMeta(obj.value(QStringLiteral("music_title")).toString(),
                                         readArtistFromObject(obj),
                                         coverArtUrl);

                item.insert(QStringLiteral("id"), obj.value(QStringLiteral("id")).toVariant().toLongLong());
                item.insert(QStringLiteral("position"), obj.value(QStringLiteral("position")).toInt());
                item.insert(QStringLiteral("path"), musicPath);
                item.insert(QStringLiteral("music_path"), musicPath);
                item.insert(QStringLiteral("title"),
                            obj.value(QStringLiteral("music_title")).toString().trimmed().isEmpty()
                                ? fileNameFromPath(musicPath)
                                : obj.value(QStringLiteral("music_title")).toString());
                item.insert(QStringLiteral("artist"), readArtistFromObject(obj));
                item.insert(QStringLiteral("album"), obj.value(QStringLiteral("album")).toString());
                item.insert(QStringLiteral("duration"), durationText);
                item.insert(QStringLiteral("duration_sec"), obj.value(QStringLiteral("duration_sec")).toInt());
                item.insert(QStringLiteral("is_local"), isLocal);
                item.insert(QStringLiteral("added_at"), obj.value(QStringLiteral("added_at")).toString());
                item.insert(QStringLiteral("cover_art_url"), coverArtUrl);

                items.append(item);
            }

            const int trackCount = detailObj.value(QStringLiteral("track_count")).toInt(items.size());
            const int serverDurationSec = detailObj.value(QStringLiteral("total_duration_sec")).toInt(totalDurationSec);
            detail.insert(QStringLiteral("track_count"), trackCount);
            detail.insert(QStringLiteral("total_duration_sec"), serverDurationSec);
            detail.insert(QStringLiteral("total_duration"), formatDurationFromSeconds(serverDurationSec));
            detail.insert(QStringLiteral("items"), items);

            emit signalPlaylistDetail(detail);
        });
}

void HttpRequestV2::deletePlaylist(const QString& userAccount, qint64 playlistId)
{
    const QString trimmedUser = userAccount.trimmed();
    if (trimmedUser.isEmpty() || playlistId <= 0) {
        emit signalDeletePlaylistResult(false, playlistId, QStringLiteral("参数错误"));
        return;
    }

    const QString url = QString("user/playlists/%1/delete?user_account=%2")
                            .arg(playlistId)
                            .arg(QString(QUrl::toPercentEncoding(trimmedUser)));

    Network::RequestOptions options = Network::RequestOptions::withPriority(Network::RequestPriority::High);
    options.maxRetries = 0;
    options.useCache = false;

    m_networkService.postJson(url, QJsonObject(), options,
        [this, playlistId, trimmedUser](const Network::NetworkResponse& response) {
            bool success = false;
            QString message = QStringLiteral("删除失败");
            if (response.isSuccess()) {
                const QJsonObject root = Network::NetworkService::parseJsonObject(response);
                const QJsonObject data = unwrapDataObject(root);
                success = parseSuccessFlag(data, parseSuccessFlag(root, true));
                message = parseMessageText(data, parseMessageText(root, success ? QStringLiteral("删除成功")
                                                                                 : QStringLiteral("删除失败")));
                if (success) {
                    m_networkService.invalidateCache(
                        QString("user/playlists?user_account=%1&page=1&page_size=20").arg(trimmedUser));
                    m_networkService.invalidateCache(
                        QString("user/playlists/%1?user_account=%2").arg(playlistId).arg(trimmedUser));
                }
            } else {
                message = extractErrorMessage(response, QStringLiteral("删除歌单失败"));
            }
            emit signalDeletePlaylistResult(success, playlistId, message);
        });
}

void HttpRequestV2::updatePlaylist(const QString& userAccount,
                                   qint64 playlistId,
                                   const QString& name,
                                   const QString& description,
                                   const QString& coverPath)
{
    const QString trimmedUser = userAccount.trimmed();
    const QString trimmedName = name.trimmed();
    if (trimmedUser.isEmpty() || playlistId <= 0 || trimmedName.isEmpty()) {
        emit signalUpdatePlaylistResult(false, playlistId, QStringLiteral("参数错误"));
        return;
    }

    const QString url = QString("user/playlists/%1/update?user_account=%2")
                            .arg(playlistId)
                            .arg(QString(QUrl::toPercentEncoding(trimmedUser)));

    QJsonObject payload;
    payload[QStringLiteral("name")] = trimmedName;
    payload[QStringLiteral("description")] = description.trimmed();
    if (!coverPath.trimmed().isEmpty()) {
        payload[QStringLiteral("cover_path")] = coverPath.trimmed();
    }

    Network::RequestOptions options = Network::RequestOptions::withPriority(Network::RequestPriority::High);
    options.maxRetries = 0;
    options.useCache = false;

    m_networkService.postJson(url, payload, options,
        [this, playlistId, trimmedUser](const Network::NetworkResponse& response) {
            bool success = false;
            QString message = QStringLiteral("更新失败");

            if (response.isSuccess()) {
                const QJsonObject root = Network::NetworkService::parseJsonObject(response);
                const QJsonObject data = unwrapDataObject(root);
                success = parseSuccessFlag(data, parseSuccessFlag(root, true));
                message = parseMessageText(data, parseMessageText(root, success ? QStringLiteral("更新成功")
                                                                                 : QStringLiteral("更新失败")));
                if (success) {
                    m_networkService.invalidateCache(
                        QString("user/playlists?user_account=%1&page=1&page_size=20").arg(trimmedUser));
                    m_networkService.invalidateCache(
                        QString("user/playlists/%1?user_account=%2").arg(playlistId).arg(trimmedUser));
                }
            } else {
                message = extractErrorMessage(response, QStringLiteral("更新歌单失败"));
            }

            emit signalUpdatePlaylistResult(success, playlistId, message);
        });
}

void HttpRequestV2::addPlaylistItems(const QString& userAccount, qint64 playlistId, const QVariantList& items)
{
    const QString trimmedUser = userAccount.trimmed();
    if (trimmedUser.isEmpty() || playlistId <= 0 || items.isEmpty()) {
        emit signalAddPlaylistItemsResult(false, playlistId, 0, 0, QStringLiteral("参数错误"));
        return;
    }

    QJsonArray itemArray;
    for (const QVariant& value : items) {
        const QVariantMap map = value.toMap();
        const QString musicPath = map.value(QStringLiteral("music_path")).toString().trimmed().isEmpty()
                                      ? map.value(QStringLiteral("path")).toString().trimmed()
                                      : map.value(QStringLiteral("music_path")).toString().trimmed();
        if (musicPath.isEmpty()) {
            continue;
        }

        QJsonObject itemObj;
        itemObj[QStringLiteral("music_path")] = musicPath;
        itemObj[QStringLiteral("music_title")] =
            map.value(QStringLiteral("music_title")).toString().trimmed().isEmpty()
                ? map.value(QStringLiteral("title")).toString().trimmed()
                : map.value(QStringLiteral("music_title")).toString().trimmed();
        itemObj[QStringLiteral("artist")] = map.value(QStringLiteral("artist")).toString();
        itemObj[QStringLiteral("album")] = map.value(QStringLiteral("album")).toString();

        int durationSec = map.value(QStringLiteral("duration_sec")).toInt();
        if (durationSec <= 0) {
            durationSec = durationSecondsFromText(map.value(QStringLiteral("duration")).toString());
        }
        itemObj[QStringLiteral("duration_sec")] = durationSec;
        const bool isLocal = map.contains(QStringLiteral("is_local"))
                                 ? map.value(QStringLiteral("is_local")).toBool()
                                 : !(musicPath.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
                                     musicPath.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive));
        itemObj[QStringLiteral("is_local")] = isLocal;

        QString coverArtPath = buildCoverPathForPlaylistPayload(
            map.value(QStringLiteral("cover_art_path")).toString());
        if (coverArtPath.isEmpty()) {
            coverArtPath = buildCoverPathForPlaylistPayload(
                map.value(QStringLiteral("cover_art_url")).toString());
        }
        if (coverArtPath.isEmpty()) {
            coverArtPath = buildCoverPathForPlaylistPayload(queryCoverForMusicPath(musicPath));
        }
        if (coverArtPath.isEmpty()) {
            const QString titleForLookup =
                map.value(QStringLiteral("music_title")).toString().trimmed().isEmpty()
                    ? map.value(QStringLiteral("title")).toString()
                    : map.value(QStringLiteral("music_title")).toString();
            coverArtPath = buildCoverPathForPlaylistPayload(
                queryCoverForSongMeta(titleForLookup, map.value(QStringLiteral("artist")).toString()));
        }
        if (!coverArtPath.isEmpty()) {
            itemObj[QStringLiteral("cover_art_path")] = coverArtPath;
        }

        itemArray.append(itemObj);
    }

    if (itemArray.isEmpty()) {
        emit signalAddPlaylistItemsResult(false, playlistId, 0, 0, QStringLiteral("加歌列表为空"));
        return;
    }

    const QString url = QString("user/playlists/%1/items/add?user_account=%2")
                            .arg(playlistId)
                            .arg(QString(QUrl::toPercentEncoding(trimmedUser)));

    QJsonObject payload;
    payload[QStringLiteral("items")] = itemArray;

    Network::RequestOptions options = Network::RequestOptions::withPriority(Network::RequestPriority::High);
    options.maxRetries = 0;
    options.useCache = false;

    m_networkService.postJson(url, payload, options,
        [this, playlistId, trimmedUser](const Network::NetworkResponse& response) {
            bool success = false;
            int addedCount = 0;
            int skippedCount = 0;
            QString message = QStringLiteral("添加失败");

            if (response.isSuccess()) {
                const QJsonObject root = Network::NetworkService::parseJsonObject(response);
                const QJsonObject data = unwrapDataObject(root);
                success = parseSuccessFlag(data, parseSuccessFlag(root, true));
                addedCount = data.value(QStringLiteral("added_count")).toInt(
                    root.value(QStringLiteral("added_count")).toInt());
                skippedCount = data.value(QStringLiteral("skipped_count")).toInt(
                    root.value(QStringLiteral("skipped_count")).toInt());
                message = parseMessageText(data, parseMessageText(root, success ? QStringLiteral("添加成功")
                                                                                 : QStringLiteral("添加失败")));
                if (success) {
                    m_networkService.invalidateCache(
                        QString("user/playlists?user_account=%1&page=1&page_size=20").arg(trimmedUser));
                    m_networkService.invalidateCache(
                        QString("user/playlists/%1?user_account=%2").arg(playlistId).arg(trimmedUser));
                }
            } else {
                message = extractErrorMessage(response, QStringLiteral("添加歌曲失败"));
            }

            emit signalAddPlaylistItemsResult(success, playlistId, addedCount, skippedCount, message);
        });
}

void HttpRequestV2::removePlaylistItems(const QString& userAccount, qint64 playlistId, const QStringList& musicPaths)
{
    const QString trimmedUser = userAccount.trimmed();
    if (trimmedUser.isEmpty() || playlistId <= 0 || musicPaths.isEmpty()) {
        emit signalRemovePlaylistItemsResult(false, playlistId, 0, QStringLiteral("参数错误"));
        return;
    }

    const QString url = QString("user/playlists/%1/items/remove?user_account=%2")
                            .arg(playlistId)
                            .arg(QString(QUrl::toPercentEncoding(trimmedUser)));

    QJsonArray pathArray;
    for (const QString& path : musicPaths) {
        if (!path.trimmed().isEmpty()) {
            pathArray.append(path.trimmed());
        }
    }

    if (pathArray.isEmpty()) {
        emit signalRemovePlaylistItemsResult(false, playlistId, 0, QStringLiteral("删歌列表为空"));
        return;
    }

    QJsonObject payload;
    payload[QStringLiteral("music_paths")] = pathArray;

    Network::RequestOptions options = Network::RequestOptions::withPriority(Network::RequestPriority::High);
    options.maxRetries = 0;
    options.useCache = false;

    m_networkService.postJson(url, payload, options,
        [this, playlistId, trimmedUser](const Network::NetworkResponse& response) {
            bool success = false;
            int deletedCount = 0;
            QString message = QStringLiteral("删除失败");

            if (response.isSuccess()) {
                const QJsonObject root = Network::NetworkService::parseJsonObject(response);
                const QJsonObject data = unwrapDataObject(root);
                success = parseSuccessFlag(data, parseSuccessFlag(root, true));
                deletedCount = data.value(QStringLiteral("deleted_count")).toInt(
                    root.value(QStringLiteral("deleted_count")).toInt());
                message = parseMessageText(data, parseMessageText(root, success ? QStringLiteral("删除成功")
                                                                                 : QStringLiteral("删除失败")));
                if (success) {
                    m_networkService.invalidateCache(
                        QString("user/playlists?user_account=%1&page=1&page_size=20").arg(trimmedUser));
                    m_networkService.invalidateCache(
                        QString("user/playlists/%1?user_account=%2").arg(playlistId).arg(trimmedUser));
                }
            } else {
                message = extractErrorMessage(response, QStringLiteral("移除歌曲失败"));
            }

            emit signalRemovePlaylistItemsResult(success, playlistId, deletedCount, message);
        });
}

void HttpRequestV2::reorderPlaylistItems(const QString& userAccount, qint64 playlistId, const QVariantList& orderedItems)
{
    const QString trimmedUser = userAccount.trimmed();
    if (trimmedUser.isEmpty() || playlistId <= 0 || orderedItems.isEmpty()) {
        emit signalReorderPlaylistItemsResult(false, playlistId, QStringLiteral("参数错误"));
        return;
    }

    QJsonArray itemArray;
    for (const QVariant& value : orderedItems) {
        const QVariantMap map = value.toMap();
        const QString musicPath = map.value(QStringLiteral("music_path")).toString().trimmed().isEmpty()
                                      ? map.value(QStringLiteral("path")).toString().trimmed()
                                      : map.value(QStringLiteral("music_path")).toString().trimmed();
        const int position = map.value(QStringLiteral("position")).toInt();
        if (musicPath.isEmpty() || position <= 0) {
            continue;
        }

        QJsonObject itemObj;
        itemObj[QStringLiteral("music_path")] = musicPath;
        itemObj[QStringLiteral("position")] = position;
        itemArray.append(itemObj);
    }

    if (itemArray.isEmpty()) {
        emit signalReorderPlaylistItemsResult(false, playlistId, QStringLiteral("排序数据为空"));
        return;
    }

    const QString url = QString("user/playlists/%1/items/reorder?user_account=%2")
                            .arg(playlistId)
                            .arg(QString(QUrl::toPercentEncoding(trimmedUser)));

    QJsonObject payload;
    payload[QStringLiteral("items")] = itemArray;

    Network::RequestOptions options = Network::RequestOptions::withPriority(Network::RequestPriority::High);
    options.maxRetries = 0;
    options.useCache = false;

    m_networkService.postJson(url, payload, options,
        [this, playlistId, trimmedUser](const Network::NetworkResponse& response) {
            bool success = false;
            QString message = QStringLiteral("排序失败");
            if (response.isSuccess()) {
                const QJsonObject root = Network::NetworkService::parseJsonObject(response);
                const QJsonObject data = unwrapDataObject(root);
                success = parseSuccessFlag(data, parseSuccessFlag(root, true));
                message = parseMessageText(data, parseMessageText(root, success ? QStringLiteral("排序成功")
                                                                                 : QStringLiteral("排序失败")));
                if (success) {
                    m_networkService.invalidateCache(
                        QString("user/playlists/%1?user_account=%2").arg(playlistId).arg(trimmedUser));
                }
            } else {
                message = extractErrorMessage(response, QStringLiteral("排序失败"));
            }

            emit signalReorderPlaylistItemsResult(success, playlistId, message);
        });
}


