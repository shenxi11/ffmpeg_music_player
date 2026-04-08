#include "cover_lookup.h"

#include <QFileInfo>
#include <QHash>
#include <QStringList>
#include <QUrl>

namespace {

QHash<QString, QString>& coverLookupTable() {
    static QHash<QString, QString> s_coverByMusicPath;
    return s_coverByMusicPath;
}

QHash<QString, QString>& coverLookupByMetaTable() {
    static QHash<QString, QString> s_coverByMeta;
    return s_coverByMeta;
}

QString normalizeNullableText(const QString& value) {
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    const QString lower = trimmed.toLower();
    if (lower == QStringLiteral("null") || lower == QStringLiteral("undefined") ||
        lower == QStringLiteral("none") || lower == QStringLiteral("(null)")) {
        return QString();
    }

    return trimmed;
}

QStringList buildMusicLookupKeys(const QString& rawPath) {
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

    const QUrl url(cleaned);
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

QString buildSongMetaKey(const QString& title, const QString& artist) {
    const QString t = normalizeNullableText(title).toLower();
    const QString a = normalizeNullableText(artist).toLower();
    if (t.isEmpty()) {
        return QString();
    }
    return t + QStringLiteral("|") + a;
}

} // namespace

QString normalizeMusicPathForLookup(QString path) {
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

void rememberCoverForMusicPath(const QString& rawPath, const QString& rawCover) {
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

QString queryCoverForMusicPath(const QString& rawPath) {
    const QStringList keys = buildMusicLookupKeys(rawPath);
    if (keys.isEmpty()) {
        return QString();
    }

    const QHash<QString, QString>& lookup = coverLookupTable();
    for (const QString& key : keys) {
        const auto it = lookup.constFind(key);
        if (it != lookup.constEnd()) {
            return it.value();
        }
    }

    return QString();
}

void rememberCoverForSongMeta(const QString& title, const QString& artist,
                              const QString& rawCover) {
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

QString queryCoverForSongMeta(const QString& title, const QString& artist) {
    const QString key = buildSongMetaKey(title, artist);
    if (key.isEmpty()) {
        return QString();
    }

    return coverLookupByMetaTable().value(key);
}

QString queryBestCoverForTrack(const QString& rawPath, const QString& title,
                               const QString& artist) {
    const QString byPath = queryCoverForMusicPath(rawPath);
    if (!byPath.trimmed().isEmpty()) {
        return byPath.trimmed();
    }

    return queryCoverForSongMeta(title, artist).trimmed();
}
