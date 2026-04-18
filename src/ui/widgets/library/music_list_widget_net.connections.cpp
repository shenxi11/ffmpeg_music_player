#include "music_list_widget_net.h"

#include <QCryptographicHash>
#include <QDebug>

namespace {

QString fallbackTrackId(const QString& path, const QString& title, const QString& artist)
{
    const QString base = QStringLiteral("%1|%2|%3").arg(path, title, artist);
    const QByteArray digest =
        QCryptographicHash::hash(base.toUtf8(), QCryptographicHash::Md5).toHex();
    return QString::fromLatin1(digest.constData(), digest.size());
}

}

/*
模块名称: MusicListWidgetNet 连接配置
功能概述: 统一管理在线音乐列表与下载流程、播放状态和流地址解析的连接。
对外接口: MusicListWidgetNet::setupConnections()
维护说明: 列表展示与 ViewModel 通道在此集中维护，便于后续扩展推荐/下载策略。
*/

void MusicListWidgetNet::setupConnections() {
    connect(listWidget, &MusicListWidgetNetQml::signalPlayClick, this,
            &MusicListWidgetNet::onPlayClick);
    connect(listWidget, &MusicListWidgetNetQml::signalRemoveClick, this,
            &MusicListWidgetNet::onRemoveClick);
    connect(listWidget, &MusicListWidgetNetQml::signalDownloadClick, this,
            &MusicListWidgetNet::onDownloadMusic);
    connect(listWidget, &MusicListWidgetNetQml::addToFavorite, this,
            &MusicListWidgetNet::addToFavorite);
    connect(this, &MusicListWidgetNet::signalNext, listWidget, &MusicListWidgetNetQml::signalNext);
    connect(this, &MusicListWidgetNet::signalLast, listWidget, &MusicListWidgetNetQml::signalLast);

    connect(m_viewModel, &OnlineMusicListViewModel::downloadStarted, this,
            [](const QString& taskId, const QString& filename) {
                qDebug() << "[MusicListWidgetNet] Download started:" << filename
                         << "TaskID:" << taskId;
            });
    connect(m_viewModel, &OnlineMusicListViewModel::downloadProgress, this,
            [](const QString&, const QString& filename, qint64 bytesReceived, qint64 bytesTotal) {
                if (bytesTotal > 0) {
                    const int progress = static_cast<int>((bytesReceived * 100) / bytesTotal);
                    if (progress % 10 == 0) {
                        qDebug() << "[MusicListWidgetNet] Download progress:" << filename
                                 << progress << "%";
                    }
                }
            });
    connect(m_viewModel, &OnlineMusicListViewModel::downloadFinished, this,
            [](const QString&, const QString& filename, const QString& savePath) {
                qDebug() << "[MusicListWidgetNet] Download completed:" << filename;
                qDebug() << "  Saved to:" << savePath;
            });
    connect(m_viewModel, &OnlineMusicListViewModel::downloadFailed, this,
            [](const QString&, const QString& filename, const QString& error) {
                qWarning() << "[MusicListWidgetNet] Download failed:" << filename;
                qWarning() << "  Error:" << error;
            });

    connect(this, &MusicListWidgetNet::signalAddSonglist, this,
            [this](const QList<Music>& musicList) {
                qDebug() << "MusicListWidgetNet: Received" << musicList.size() << "songs";

                QStringList songNames;
                QStringList relativePaths;
                QList<double> durations;
                QStringList coverUrls;
                QStringList artists;
                QVariantList items;

                for (const Music& music : musicList) {
                    const QString fullPath = music.getSongPath();
                    QString relativePath;

                    if (fullPath.startsWith("http://") || fullPath.startsWith("https://")) {
                        const int uploadsPos = fullPath.indexOf("/uploads/");
                        if (uploadsPos != -1) {
                            relativePath = fullPath.mid(uploadsPos + 9);
                        } else {
                            relativePath = fullPath;
                        }
                    } else {
                        relativePath = fullPath;
                    }

                    songNames.append(music.getSongName());
                    relativePaths.append(relativePath);
                    durations.append(static_cast<double>(music.getDuration()));
                    coverUrls.append(music.getPicPath());
                    artists.append(music.getSinger());
                    const QString title = music.getSongName().trimmed().isEmpty()
                        ? relativePath
                        : music.getSongName().trimmed();
                    const QString artist = music.getSinger().trimmed().isEmpty()
                        ? QStringLiteral("未知艺术家")
                        : music.getSinger().trimmed();
                    items.push_back(QVariantMap{
                        {QStringLiteral("trackId"), fallbackTrackId(relativePath, title, artist)},
                        {QStringLiteral("musicPath"), relativePath},
                        {QStringLiteral("path"), relativePath},
                        {QStringLiteral("playPath"), relativePath},
                        {QStringLiteral("title"), title},
                        {QStringLiteral("artist"), artist},
                        {QStringLiteral("coverUrl"), music.getPicPath()},
                        {QStringLiteral("cover"), music.getPicPath()},
                        {QStringLiteral("durationMs"),
                         static_cast<qint64>(music.getDuration() > 0 ? music.getDuration() * 1000
                                                                     : 0)},
                        {QStringLiteral("isLocal"), false},
                        {QStringLiteral("sourceType"), QStringLiteral("online")}
                    });

                    song_duration[relativePath] = static_cast<double>(music.getDuration());
                    song_cover[relativePath] = music.getPicPath();
                }

                m_currentItems = items;
                listWidget->addSongList(songNames, relativePaths, durations, coverUrls, artists);
            });

    connect(m_viewModel, &OnlineMusicListViewModel::streamReady, this,
            [this](const QString& file, const QString& artist, const QString& cover,
                   const QString& originalMusicPath) {
                m_currentResolvedOriginalPath = originalMusicPath.trimmed();
                if (hasPendingResolvedAction()) {
                    QVariantMap payload = m_pendingResolvedSongData;
                    payload.insert(QStringLiteral("playPath"), file);
                    payload.insert(QStringLiteral("comment_music_path"), m_currentResolvedOriginalPath);
                    payload.insert(QStringLiteral("path"), file);
                    payload.insert(QStringLiteral("artist"), artist);
                    payload.insert(QStringLiteral("cover"), cover);
                    const QString action = m_pendingResolvedAction;
                    clearPendingResolvedAction();
                    emit songActionRequested(action, payload);
                    return;
                }
                emit signalPlayClick(file, artist, cover, true, m_currentResolvedOriginalPath);
            });
    connect(m_viewModel, &OnlineMusicListViewModel::streamResolveFailed, this, [this]() {
        clearPendingResolvedAction();
    });

    if (QQuickItem* root = listWidget->rootObject()) {
        connect(root, SIGNAL(songActionRequested(QString, QVariant)), this,
                SLOT(onSongActionRequested(QString, QVariant)));
    }
}

void MusicListWidgetNet::onSongActionRequested(const QString& action,
                                               const QVariant& payloadValue) {
    const QVariantMap payload = payloadValue.toMap();
    if (action == QStringLiteral("add_favorite")) {
        emit addToFavorite(payload.value(QStringLiteral("path")).toString(),
                           payload.value(QStringLiteral("title")).toString(),
                           payload.value(QStringLiteral("artist")).toString(),
                           payload.value(QStringLiteral("duration")).toString());
        return;
    }

    if (action == QStringLiteral("download")) {
        onDownloadMusic(payload.value(QStringLiteral("path")).toString());
        return;
    }

    resolveSongAction(action, payload);
}
