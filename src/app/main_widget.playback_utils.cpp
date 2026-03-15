#include "main_widget.h"

#include <QUrl>
#include <QUrlQuery>

/*
模块名称: MainWidget 播放工具函数
功能概述: 提供最近播放与推荐流程的公共字符串解析工具。
对外接口: normalizeArtistForHistory / extractSongIdFromMediaPath
维护说明: 仅做纯函数处理，不依赖界面状态。
*/

QString MainWidget::normalizeArtistForHistory(const QString& artist)
{
    const QString trimmed = artist.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    const QString lower = trimmed.toLower();
    if (trimmed == QStringLiteral("未知艺术家") ||
        trimmed == QStringLiteral("未知歌手") ||
        lower == QStringLiteral("unknown artist") ||
        lower == QStringLiteral("unknown") ||
        lower == QStringLiteral("<unknown>")) {
        return QString();
    }
    return trimmed;
}

QString MainWidget::extractSongIdFromMediaPath(const QString& rawPath)
{
    QString text = rawPath.trimmed();
    if (text.isEmpty()) {
        return QString();
    }

    auto extractFromHttpUrl = [](const QUrl& url) -> QString {
        if (!url.isValid()) {
            return QString();
        }

        if (url.path().contains(QStringLiteral("/proxy"), Qt::CaseInsensitive)) {
            QUrlQuery query(url);
            const QString src = query.queryItemValue(QStringLiteral("src"), QUrl::FullyDecoded);
            if (!src.trimmed().isEmpty()) {
                return src;
            }
        }

        QString decodedPath = QUrl::fromPercentEncoding(url.path().toUtf8());
        if (decodedPath.startsWith(QStringLiteral("/uploads/"), Qt::CaseInsensitive)) {
            return decodedPath.mid(QStringLiteral("/uploads/").size());
        }

        while (decodedPath.startsWith('/')) {
            decodedPath.remove(0, 1);
        }
        return decodedPath;
    };

    if (text.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
        text.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        const QString extracted = extractFromHttpUrl(QUrl(text));
        if (extracted.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
            extracted.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
            return extractSongIdFromMediaPath(extracted);
        }

        if (extracted.contains('/')) {
            return extracted;
        }
        return QString();
    }

    if (text.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        return QString();
    }

    QString normalized = text;
    normalized.replace('\\', '/');
    if (normalized.size() >= 2 && normalized[1] == ':') {
        return QString();
    }
    if (normalized.startsWith(QStringLiteral("uploads/"), Qt::CaseInsensitive)) {
        normalized = normalized.mid(QStringLiteral("uploads/").size());
    }
    while (normalized.startsWith('/')) {
        normalized.remove(0, 1);
    }

    return normalized.contains('/') ? normalized : QString();
}
