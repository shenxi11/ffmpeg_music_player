#include "main_widget.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

/*
模块名称: MainWidget 播放列表处理
功能概述: 处理本地/在线/历史列表触发的播放、元数据与缓存同步逻辑。
对外接口: handlePlayWidget* / handleLocal* / handleNet* / handleHistory*
维护说明: 仅处理列表与播放链路，不处理会话状态机切换。
*/

void MainWidget::handleNetLoginRequired() {
    if (m_localOnlyMode) {
        showLocalOnlyUnavailableMessage();
        return;
    }
    qDebug() << "[MainWidget] Download requires login, showing login window";
    showLoginWindow();
}

void MainWidget::handlePlayWidgetLast(const QString& songName, bool netFlag) {
    if (netFlag) {
        emit net_list->signalLast(songName);
    } else {
        emit main_list->signalLast(songName);
    }
}

void MainWidget::handlePlayWidgetNext(const QString& songName, bool netFlag) {
    if (netFlag) {
        emit net_list->signalNext(songName);
    } else {
        emit main_list->signalNext(songName);
    }
}

void MainWidget::handlePlayWidgetNetFlagChanged(bool netFlag) {
    Q_UNUSED(netFlag);
}

void MainWidget::handlePlayWidgetAddSongToCache(const QString& fileName, const QString& path) {
    qDebug() << "[LocalMusicCache] Adding music:" << fileName << path;
    if (m_viewModel) {
        m_viewModel->addLocalMusicCacheEntry(path, fileName);
    }
}

void MainWidget::handlePlayWidgetButtonState(bool playing, const QString& filename) {
    playHistoryWidget->setPlayingState(filename, playing);
    recommendMusicWidget->setPlayingState(filename, playing);
    if (!w->getNetFlag() && !filename.isEmpty()) {
        localAndDownloadWidget->setPlayingState(filename, playing);
    }
}

void MainWidget::handlePlayWidgetMetadataUpdated(const QString& filePath, const QString& coverUrl,
                                                 const QString& duration, const QString& artist) {
    qDebug() << "[LocalMusicCache] Updating metadata:" << filePath << coverUrl << duration
             << artist;
    if (m_viewModel) {
        m_viewModel->updateLocalMusicCacheMetadata(filePath, coverUrl, duration, artist);
    }
}

void MainWidget::applyCommentTrackContext(const QString& musicPath, const QString& title,
                                          const QString& artist, const QString& cover) {
    if (!w) {
        return;
    }

    QVariantMap context;
    context.insert(QStringLiteral("music_path"), musicPath.trimmed());
    context.insert(QStringLiteral("title"), title.trimmed());
    context.insert(QStringLiteral("artist"), artist.trimmed());
    context.insert(QStringLiteral("cover"), cover.trimmed());
    w->setCommentTrackContext(context);
}

void MainWidget::clearCommentTrackContext() {
    if (w) {
        w->clearCommentTrackContext();
    }
}

void MainWidget::openCommentContentPage() {
    if (!w || !commentPageWidget) {
        return;
    }

    if (m_activeContentWidget != commentPageWidget) {
        m_previousContentWidgetBeforeComment = m_activeContentWidget;
    }

    m_commentContentOpening = true;
    showContentPanel(commentPageWidget);
    m_commentContentOpening = false;
    w->setMainContentCommentVisible(true);
    commentPageWidget->raise();
}

void MainWidget::closeCommentContentPage(bool restorePrevious) {
    if (!commentPageWidget || !w) {
        return;
    }

    QWidget* target = restorePrevious ? m_previousContentWidgetBeforeComment : nullptr;
    if (!target || target == commentPageWidget) {
        target = localAndDownloadWidget;
    }

    w->setMainContentCommentVisible(false);
    m_previousContentWidgetBeforeComment = nullptr;
    showContentPanel(target);
}

void MainWidget::handleLocalListPlayClick(const QString& name, bool flag) {
    if (w->getNetFlag()) {
        qDebug() << "[Switch source] Network music -> local music, clear network playing state";
        net_list->setPlayingState("", false);
    }
    localAndDownloadWidget->setPlayingState("", false);
    w->setPlayNet(flag);
    clearCommentTrackContext();

    m_networkMusicArtist.clear();
    m_networkMusicCover.clear();
    w->playClick(name);
}

void MainWidget::handleLocalAndDownloadPlayMusic(const QString& filename) {
    qDebug() << "[LocalAndDownloadWidget] Play music:" << filename;
    if (w->getNetFlag()) {
        net_list->setPlayingState("", false);
    }
    main_list->signalPlayButtonClick(false, "");
    localAndDownloadWidget->setPlayingState(filename, false);
    w->setPlayNet(false);
    clearCommentTrackContext();
    w->playClick(filename);
}

void MainWidget::handleLocalAndDownloadDeleteMusic(const QString& filename) {
    qDebug() << "[LocalAndDownloadWidget] Delete music:" << filename;
    if (m_viewModel) {
        m_viewModel->removeLocalMusicCacheEntry(filename);
    }

    QFile file(filename);
    if (file.exists()) {
        if (file.remove()) {
            qDebug() << "[LocalAndDownloadWidget] File deleted successfully:" << filename;

            QFileInfo fileInfo(filename);
            QString folderPath = fileInfo.dir().absolutePath();
            QDir parentDir = fileInfo.dir();
            parentDir.cdUp();

            QString baseName = fileInfo.completeBaseName();
            QString sameFolderPath = parentDir.absoluteFilePath(baseName);
            QDir sameFolder(sameFolderPath);

            if (sameFolder.exists() && folderPath.contains(baseName)) {
                if (sameFolder.removeRecursively()) {
                    qDebug() << "[LocalAndDownloadWidget] Folder deleted successfully:"
                             << sameFolderPath;
                } else {
                    qWarning() << "[LocalAndDownloadWidget] Failed to delete folder:"
                               << sameFolderPath;
                }
            }
        } else {
            qWarning() << "[LocalAndDownloadWidget] Failed to delete file:" << filename;
        }
    } else {
        qWarning() << "[LocalAndDownloadWidget] File not found:" << filename;
    }
}

void MainWidget::handleNetListPlayClick(const QString& name, const QString& artist,
                                        const QString& cover, bool flag,
                                        const QString& originalMusicPath) {
    qDebug() << "[MainWidget] ========== NET MUSIC PLAY SIGNAL ==========";
    qDebug() << "[MainWidget] name:" << name;
    qDebug() << "[MainWidget] artist:" << artist;
    qDebug() << "[MainWidget] cover:" << cover;
    qDebug() << "[MainWidget] flag:" << flag;
    qDebug() << "[MainWidget] ===============================================";

    if (!w->getNetFlag()) {
        qDebug() << "[Switch source] Local music -> network music, clear local playing state";
        main_list->signalPlayButtonClick(false, "");
    }
    localAndDownloadWidget->setPlayingState("", false);
    w->setPlayNet(flag);

    const QString normalizedArtist = normalizeArtistForHistory(artist);
    rememberPlaybackQueueMetadata(name, QString(), normalizedArtist, cover);
    w->setNetworkMetadata(normalizedArtist, cover);
    applyCommentTrackContext(originalMusicPath, QString(), normalizedArtist, cover);

    m_networkMusicArtist = normalizedArtist;
    m_networkMusicCover = cover;
    w->playClick(name);
}

void MainWidget::handleHistoryPlayMusic(const QString& filePath) {
    qDebug() << "[PlayHistoryWidget] Play music:" << filePath;
    if (w->getNetFlag()) {
        net_list->setPlayingState("", false);
    }
    main_list->signalPlayButtonClick(false, "");
    localAndDownloadWidget->setPlayingState("", false);

    bool isLocal = !filePath.startsWith("http");
    w->setPlayNet(!isLocal);
    if (isLocal) {
        clearCommentTrackContext();
    } else {
        applyCommentTrackContext(filePath, QString(), QString(), QString());
    }
    w->playClick(filePath);
}

void MainWidget::handleHistoryPlayMusicWithMetadata(const QString& filePath, const QString& title,
                                                    const QString& artist, const QString& cover) {
    qDebug() << "[PlayHistoryWidget] Play music with metadata:" << filePath << "title:" << title
             << "artist:" << artist << "cover:" << cover;

    if (w->getNetFlag()) {
        net_list->setPlayingState("", false);
    }
    main_list->signalPlayButtonClick(false, "");
    localAndDownloadWidget->setPlayingState("", false);

    const bool isLocal = !filePath.startsWith("http");
    w->setPlayNet(!isLocal);
    const QString normalizedArtist = normalizeArtistForHistory(artist);
    rememberPlaybackQueueMetadata(filePath, title, normalizedArtist, cover);
    w->setNetworkMetadata(title, normalizedArtist, cover);
    if (isLocal) {
        clearCommentTrackContext();
    } else {
        applyCommentTrackContext(filePath, title, normalizedArtist, cover);
    }
    if (!isLocal) {
        m_networkMusicArtist = normalizedArtist;
        m_networkMusicCover = cover;
    }
    w->playClick(filePath);
}
