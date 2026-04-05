#include "main_widget.h"

#include "AudioService.h"
#include <QUrl>
#include <QMessageBox>
#include <QFileInfo>

namespace {
QString buildPlaylistCoverPathFromSource(const QString& rawCover)
{
    QString cover = rawCover.trimmed();
    if (cover.isEmpty()) {
        return QString();
    }
    if (cover.startsWith(QStringLiteral("qrc:/"), Qt::CaseInsensitive)) {
        return QString();
    }

    if (cover.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        QUrl localUrl(cover);
        if (localUrl.isLocalFile()) {
            return localUrl.toLocalFile();
        }
    }

    if (cover.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
        cover.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        QUrl url(cover);
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
}

/*
模块名称: MainWidget 收藏与历史连接
功能概述: 统一处理最近播放、我喜欢的音乐、推荐列表的刷新与操作回写连接。
对外接口: MainWidget::setupLibraryConnections()
维护说明: 关注列表数据闭环，避免在界面层散落重复刷新逻辑。
*/

// 建立历史与收藏域的连接关系，并触发必要的数据刷新。
void MainWidget::setupLibraryConnections()
{
    connect(playHistoryWidget, &PlayHistoryWidget::deleteHistory, this, &MainWidget::handleHistoryDeleteRequested);
    connect(m_viewModel, &MainShellViewModel::removeHistoryResultReady, this, &MainWidget::handleRemoveHistoryResult);

    connect(playHistoryWidget, &PlayHistoryWidget::addToFavorite, this,
            &MainWidget::handleHistoryAddToFavorite);

    connect(playHistoryWidget, &PlayHistoryWidget::loginRequested, this, &MainWidget::handleHistoryLoginRequested);
    connect(playHistoryWidget, &PlayHistoryWidget::refreshRequested, this, &MainWidget::handleHistoryRefreshRequested);

    connect(m_viewModel, &MainShellViewModel::historyListReady,
            playHistoryWidget, &PlayHistoryWidget::loadHistory);

    connect(favoriteMusicWidget, &FavoriteMusicWidget::playMusic, this, &MainWidget::handleFavoritePlayMusic);
    connect(favoriteMusicWidget, &FavoriteMusicWidget::removeFavorite, this, &MainWidget::handleFavoriteRemoveRequested);
    connect(favoriteMusicWidget, &FavoriteMusicWidget::refreshRequested, this, &MainWidget::handleFavoriteRefreshRequested);

    connect(m_viewModel, &MainShellViewModel::favoritesListReady,
            favoriteMusicWidget, &FavoriteMusicWidget::loadFavorites);

    connect(m_viewModel, &MainShellViewModel::removeFavoriteResultReady, this, &MainWidget::handleRemoveFavoriteResult);

    connect(localAndDownloadWidget, &LocalAndDownloadWidget::addToFavorite,
            this, &MainWidget::handleLocalAddToFavorite);

    connect(net_list, &MusicListWidgetNet::addToFavorite,
            this, &MainWidget::handleNetAddToFavorite);

    connect(m_viewModel, &MainShellViewModel::addFavoriteResultReady, this, &MainWidget::handleAddFavoriteResult);
    connect(userWidgetQml, &UserWidgetQml::loginStateChanged, this, &MainWidget::handleUserLoginStateChanged);

    connect(playlistWidget, &PlaylistWidget::loginRequested, this, &MainWidget::handlePlaylistLoginRequested);
    connect(playlistWidget, &PlaylistWidget::refreshRequested, this, &MainWidget::handlePlaylistRefreshRequested);
    connect(playlistWidget, &PlaylistWidget::openPlaylistRequested, this, &MainWidget::handlePlaylistOpenRequested);
    connect(playlistWidget, &PlaylistWidget::createPlaylistRequested, this, &MainWidget::handlePlaylistCreateRequested);
    connect(playlistWidget, &PlaylistWidget::updatePlaylistRequested, this, &MainWidget::handlePlaylistUpdateRequested);
    connect(playlistWidget, &PlaylistWidget::deletePlaylistRequested, this, &MainWidget::handlePlaylistDeleteRequested);
    connect(playlistWidget, &PlaylistWidget::removePlaylistItemsRequested, this, &MainWidget::handlePlaylistRemoveSongsRequested);
    connect(playlistWidget, &PlaylistWidget::reorderPlaylistItemsRequested, this, &MainWidget::handlePlaylistReorderSongsRequested);
    connect(playlistWidget, &PlaylistWidget::addCurrentSongRequested, this, &MainWidget::handlePlaylistAddCurrentSongRequested);
    connect(playlistWidget, &PlaylistWidget::playMusicWithMetadata, this, &MainWidget::handlePlaylistPlayMusicWithMetadata);

    connect(m_viewModel, &MainShellViewModel::playlistsListReady, this, &MainWidget::handlePlaylistsListReady);
    connect(m_viewModel, &MainShellViewModel::playlistDetailReady, this, &MainWidget::handlePlaylistDetailReady);
    connect(m_viewModel, &MainShellViewModel::createPlaylistResultReady, this, &MainWidget::handleCreatePlaylistResultReady);
    connect(m_viewModel, &MainShellViewModel::updatePlaylistResultReady, this, &MainWidget::handleUpdatePlaylistResultReady);
    connect(m_viewModel, &MainShellViewModel::deletePlaylistResultReady, this, &MainWidget::handleDeletePlaylistResultReady);
    connect(m_viewModel, &MainShellViewModel::addPlaylistItemsResultReady, this, &MainWidget::handleAddPlaylistItemsResultReady);
    connect(m_viewModel, &MainShellViewModel::removePlaylistItemsResultReady, this, &MainWidget::handleRemovePlaylistItemsResultReady);
    connect(m_viewModel, &MainShellViewModel::reorderPlaylistItemsResultReady, this, &MainWidget::handleReorderPlaylistItemsResultReady);
}

void MainWidget::handleHistoryDeleteRequested(const QStringList& paths)
{
    qDebug() << "[PlayHistoryWidget] Delete history, count:" << paths.size();

    QString userAccount = m_viewModel->currentUserAccount();
    if (!userAccount.isEmpty()) {
        if (m_viewModel) {
            m_viewModel->removeHistory(userAccount, paths);
        }
    } else {
        qWarning() << "[PlayHistoryWidget] Cannot delete history: user not logged in";
    }
}

void MainWidget::handleRemoveHistoryResult(bool success)
{
    if (success) {
        qDebug() << "[PlayHistoryWidget] Delete history success, refreshing list";
        QString userAccount = m_viewModel->currentUserAccount();
        if (!userAccount.isEmpty()) {
            m_viewModel->requestHistory(userAccount, 50, false);
        }
    } else {
        qWarning() << "[PlayHistoryWidget] Delete history failed";
    }
}

void MainWidget::handleHistoryAddToFavorite(const QString& path,
                                            const QString& title,
                                            const QString& artist,
                                            const QString& duration,
                                            bool isLocal)
{
    qDebug() << "[PlayHistoryWidget] Add to favorite:" << title << "path:" << path << "isLocal:" << isLocal;
    if (!isUserLoggedIn()) {
        showLoginWindow();
        return;
    }
    QString userAccount = m_viewModel->currentUserAccount();
    if (m_viewModel) {
        m_viewModel->addFavorite(userAccount, path, title, artist, duration, isLocal);
    }
}

void MainWidget::handleHistoryLoginRequested()
{
    qDebug() << "[PlayHistoryWidget] Login requested";
    showLoginWindow();
}

void MainWidget::handleHistoryRefreshRequested()
{
    qDebug() << "[PlayHistoryWidget] Refresh requested";
    if (isUserLoggedIn()) {
        QString userAccount = m_viewModel->currentUserAccount();
        if (m_viewModel) {
            m_viewModel->requestHistory(userAccount, 50, false);
        }
    }
}

void MainWidget::handleFavoritePlayMusic(const QString& filePath)
{
    qDebug() << "[FavoriteMusicWidget] Play music:" << filePath;
    if (w->getNetFlag()) {
        net_list->signalPlayButtonClick(false, "");
    }
    main_list->signalPlayButtonClick(false, "");
    localAndDownloadWidget->setCurrentPlayingPath("");

    const bool isLocal = !filePath.startsWith("http");
    w->setPlayNet(!isLocal);
    w->playClick(filePath);
}

void MainWidget::handleFavoriteRemoveRequested(const QStringList& paths)
{
    qDebug() << "[FavoriteMusicWidget] Remove favorite, count:" << paths.size();
    QString userAccount = m_viewModel->currentUserAccount();
    if (m_viewModel) {
        m_viewModel->removeFavorite(userAccount, paths);
    }
}

void MainWidget::handleFavoriteRefreshRequested()
{
    qDebug() << "[FavoriteMusicWidget] Refresh requested";
    QString userAccount = m_viewModel->currentUserAccount();
    if (m_viewModel) {
        m_viewModel->requestFavorites(userAccount);
    }
}

void MainWidget::handleRemoveFavoriteResult(bool success)
{
    if (success) {
        qDebug() << "[MainWidget] Remove favorite success, refreshing list";
        QString userAccount = m_viewModel->currentUserAccount();
        m_viewModel->requestFavorites(userAccount, false);
    } else {
        qWarning() << "[MainWidget] Remove favorite failed";
    }
}

void MainWidget::handleLocalAddToFavorite(const QString& path,
                                          const QString& title,
                                          const QString& artist,
                                          const QString& duration)
{
    qDebug() << "[MainWidget] Add to favorite from local/download:" << title;
    if (!isUserLoggedIn()) {
        showLoginWindow();
        return;
    }
    QString userAccount = m_viewModel->currentUserAccount();
    if (m_viewModel) {
        m_viewModel->addFavorite(userAccount, path, title, artist, duration, true);
    }
}

void MainWidget::handleNetAddToFavorite(const QString& path,
                                        const QString& title,
                                        const QString& artist,
                                        const QString& duration)
{
    qDebug() << "[MainWidget] Add to favorite from online:" << title;
    if (!isUserLoggedIn()) {
        showLoginWindow();
        return;
    }
    QString userAccount = m_viewModel->currentUserAccount();
    if (m_viewModel) {
        m_viewModel->addFavorite(userAccount, path, title, artist, duration, false);
    }
}

void MainWidget::handleAddFavoriteResult(bool success)
{
    if (success) {
        qDebug() << "[MainWidget] Add to favorite success, refreshing list";
    } else {
        qWarning() << "[MainWidget] Add to favorite failed, try refreshing list to sync latest state";
    }

    QString userAccount = m_viewModel->currentUserAccount();
    if (!userAccount.isEmpty()) {
        m_viewModel->requestFavorites(userAccount, false);
    }
}

void MainWidget::handleUserLoginStateChanged(bool loggedIn)
{
    qDebug() << "[MainWidget] Login state changed:" << loggedIn;

    QString userAccount = loggedIn ? m_viewModel->currentUserAccount() : "";
    playHistoryWidget->setLoggedIn(loggedIn, userAccount);
    favoriteMusicWidget->setUserAccount(userAccount);
    recommendMusicWidget->setLoggedIn(loggedIn, userAccount);
    playlistWidget->setLoggedIn(loggedIn, userAccount);

    if (favoriteButton) {
        favoriteButton->setVisible(loggedIn);
        qDebug() << "[MainWidget] Favorite music button visibility:" << loggedIn;
    }
    updateSideNavLayout();

    if (!loggedIn) {
        favoriteMusicWidget->clearFavorites();
        playHistoryWidget->clearHistory();
        recommendMusicWidget->clearRecommendations();
        playlistWidget->clearData();
    } else if (recommendButton && recommendButton->isChecked()) {
        if (m_viewModel) {
            m_viewModel->requestRecommendations(userAccount, QStringLiteral("home"), 24, true);
        }
    } else if (playlistButton && playlistButton->isChecked()) {
        if (m_viewModel) {
            m_viewModel->requestPlaylists(userAccount, 1, 20, true);
        }
    }
}

void MainWidget::handlePlaylistLoginRequested()
{
    showLoginWindow();
}

void MainWidget::handlePlaylistRefreshRequested()
{
    if (!isUserLoggedIn() || !m_viewModel) {
        return;
    }
    m_viewModel->requestPlaylists(m_viewModel->currentUserAccount(), 1, 20, false);
}

void MainWidget::handlePlaylistOpenRequested(qint64 playlistId)
{
    if (!isUserLoggedIn() || !m_viewModel || playlistId <= 0) {
        return;
    }
    m_viewModel->requestPlaylistDetail(m_viewModel->currentUserAccount(), playlistId, false);
}

void MainWidget::handlePlaylistCreateRequested(const QString& name, const QString& description)
{
    if (!isUserLoggedIn() || !m_viewModel) {
        showLoginWindow();
        return;
    }
    m_viewModel->createPlaylist(m_viewModel->currentUserAccount(), name, description);
}

void MainWidget::handlePlaylistUpdateRequested(qint64 playlistId, const QString& name, const QString& description)
{
    if (!isUserLoggedIn() || !m_viewModel || playlistId <= 0) {
        return;
    }
    m_viewModel->updatePlaylist(m_viewModel->currentUserAccount(), playlistId, name, description);
}

void MainWidget::handlePlaylistDeleteRequested(qint64 playlistId)
{
    if (!isUserLoggedIn() || !m_viewModel || playlistId <= 0) {
        return;
    }
    m_viewModel->deletePlaylist(m_viewModel->currentUserAccount(), playlistId);
}

void MainWidget::handlePlaylistPlayMusicWithMetadata(const QString& filePath,
                                                     const QString& title,
                                                     const QString& artist,
                                                     const QString& cover)
{
    if (filePath.trimmed().isEmpty()) {
        return;
    }

    if (w->getNetFlag()) {
        net_list->signalPlayButtonClick(false, "");
    }
    main_list->signalPlayButtonClick(false, "");
    localAndDownloadWidget->setCurrentPlayingPath("");

    const bool isLocal = !filePath.startsWith("http", Qt::CaseInsensitive);
    w->setPlayNet(!isLocal);
    const QString normalizedArtist = normalizeArtistForHistory(artist);
    w->setNetworkMetadata(title, normalizedArtist, cover);
    if (!isLocal && !cover.trimmed().isEmpty()) {
        m_networkMusicArtist = normalizedArtist;
        m_networkMusicCover = cover.trimmed();
    }
    w->playClick(filePath);
}

void MainWidget::handlePlaylistRemoveSongsRequested(qint64 playlistId, const QStringList& musicPaths)
{
    if (!isUserLoggedIn() || !m_viewModel || playlistId <= 0 || musicPaths.isEmpty()) {
        return;
    }
    m_viewModel->removePlaylistItems(m_viewModel->currentUserAccount(), playlistId, musicPaths);
}

void MainWidget::handlePlaylistReorderSongsRequested(qint64 playlistId, const QVariantList& orderedItems)
{
    if (!isUserLoggedIn() || !m_viewModel || playlistId <= 0 || orderedItems.isEmpty()) {
        return;
    }
    m_viewModel->reorderPlaylistItems(m_viewModel->currentUserAccount(), playlistId, orderedItems);
}

void MainWidget::handlePlaylistAddCurrentSongRequested(qint64 playlistId)
{
    if (!isUserLoggedIn() || !m_viewModel || playlistId <= 0) {
        return;
    }

    AudioSession* session = AudioService::instance().currentSession();
    if (!session) {
        QMessageBox::information(this,
                                 QStringLiteral("提示"),
                                 QStringLiteral("当前没有可添加的播放歌曲。"));
        return;
    }

    QUrl currentUrl = AudioService::instance().currentUrl();
    if (!currentUrl.isValid() || currentUrl.toString().trimmed().isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("提示"),
                             QStringLiteral("当前歌曲路径为空，无法添加到歌单。"));
        return;
    }

    QString musicPath = currentUrl.isLocalFile() ? currentUrl.toLocalFile() : currentUrl.toString();
    if (musicPath.trimmed().isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("提示"),
                             QStringLiteral("当前歌曲路径为空，无法添加到歌单。"));
        return;
    }

    QVariantMap item;
    item.insert(QStringLiteral("music_path"), musicPath);
    item.insert(QStringLiteral("music_title"), session->title().trimmed().isEmpty()
                                              ? QFileInfo(musicPath).completeBaseName()
                                              : session->title());
    item.insert(QStringLiteral("artist"), normalizeArtistForHistory(session->artist()));
    item.insert(QStringLiteral("album"), QString());
    item.insert(QStringLiteral("duration_sec"), static_cast<int>(qMax<qint64>(0, session->duration() / 1000)));
    const bool isLocal = !musicPath.startsWith("http", Qt::CaseInsensitive);
    item.insert(QStringLiteral("is_local"), isLocal);

    QString coverSource = session->albumArt().trimmed();
    if (!isLocal && !m_networkMusicCover.trimmed().isEmpty()) {
        coverSource = m_networkMusicCover.trimmed();
    }
    const QString coverPath = buildPlaylistCoverPathFromSource(coverSource);
    if (!coverPath.isEmpty()) {
        item.insert(QStringLiteral("cover_art_path"), coverPath);
    } else if (!coverSource.isEmpty()) {
        item.insert(QStringLiteral("cover_art_url"), coverSource);
    }

    QVariantList items;
    items.append(item);
    m_viewModel->addPlaylistItems(m_viewModel->currentUserAccount(), playlistId, items);
}

void MainWidget::handlePlaylistsListReady(const QVariantList& playlists, int page, int pageSize, int total)
{
    if (playlistWidget) {
        playlistWidget->loadPlaylists(playlists, page, pageSize, total);
    }
}

void MainWidget::handlePlaylistDetailReady(const QVariantMap& detail)
{
    if (playlistWidget) {
        playlistWidget->loadPlaylistDetail(detail);
    }
}

void MainWidget::handleCreatePlaylistResultReady(bool success, qint64 playlistId, const QString& message)
{
    Q_UNUSED(playlistId);
    if (!success) {
        QMessageBox::warning(this,
                             QStringLiteral("创建歌单失败"),
                             message.trimmed().isEmpty() ? QStringLiteral("请稍后重试。") : message);
        return;
    }

    if (m_viewModel && isUserLoggedIn()) {
        m_viewModel->requestPlaylists(m_viewModel->currentUserAccount(), 1, 20, false);
    }
}

void MainWidget::handleUpdatePlaylistResultReady(bool success, qint64 playlistId, const QString& message)
{
    if (!success) {
        QMessageBox::warning(this,
                             QStringLiteral("更新歌单失败"),
                             message.trimmed().isEmpty() ? QStringLiteral("请稍后重试。") : message);
        return;
    }

    if (m_viewModel && isUserLoggedIn()) {
        const QString account = m_viewModel->currentUserAccount();
        m_viewModel->requestPlaylists(account, 1, 20, false);
        if (playlistId > 0) {
            m_viewModel->requestPlaylistDetail(account, playlistId, false);
        }
    }
}

void MainWidget::handleDeletePlaylistResultReady(bool success, qint64 playlistId, const QString& message)
{
    Q_UNUSED(playlistId);
    if (!success) {
        QMessageBox::warning(this,
                             QStringLiteral("删除歌单失败"),
                             message.trimmed().isEmpty() ? QStringLiteral("请稍后重试。") : message);
        return;
    }

    if (m_viewModel && isUserLoggedIn()) {
        m_viewModel->requestPlaylists(m_viewModel->currentUserAccount(), 1, 20, false);
    }
}

void MainWidget::handleAddPlaylistItemsResultReady(bool success,
                                                   qint64 playlistId,
                                                   int addedCount,
                                                   int skippedCount,
                                                   const QString& message)
{
    Q_UNUSED(addedCount);
    Q_UNUSED(skippedCount);
    if (!success) {
        QMessageBox::warning(this,
                             QStringLiteral("添加歌曲失败"),
                             message.trimmed().isEmpty() ? QStringLiteral("请稍后重试。") : message);
        return;
    }

    if (m_viewModel && isUserLoggedIn() && playlistId > 0) {
        m_viewModel->requestPlaylistDetail(m_viewModel->currentUserAccount(), playlistId, false);
    }
}

void MainWidget::handleRemovePlaylistItemsResultReady(bool success,
                                                      qint64 playlistId,
                                                      int deletedCount,
                                                      const QString& message)
{
    Q_UNUSED(deletedCount);
    if (!success) {
        QMessageBox::warning(this,
                             QStringLiteral("移除歌曲失败"),
                             message.trimmed().isEmpty() ? QStringLiteral("请稍后重试。") : message);
        return;
    }

    if (m_viewModel && isUserLoggedIn() && playlistId > 0) {
        m_viewModel->requestPlaylistDetail(m_viewModel->currentUserAccount(), playlistId, false);
    }
}

void MainWidget::handleReorderPlaylistItemsResultReady(bool success, qint64 playlistId, const QString& message)
{
    if (!success) {
        QMessageBox::warning(this,
                             QStringLiteral("排序失败"),
                             message.trimmed().isEmpty() ? QStringLiteral("请稍后重试。") : message);
    }

    if (m_viewModel && isUserLoggedIn() && playlistId > 0) {
        m_viewModel->requestPlaylistDetail(m_viewModel->currentUserAccount(), playlistId, false);
    }
}
