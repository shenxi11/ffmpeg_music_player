#include "main_widget.h"

#include "AudioService.h"
#include <QUrl>
#include <QCursor>
#include <QMessageBox>
#include <QFileInfo>
#include <QInputDialog>
#include <QToolTip>

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

QString normalizeSongPath(const QString& rawPath)
{
    QString path = rawPath.trimmed();
    if (path.startsWith(QStringLiteral("file:///"), Qt::CaseInsensitive)) {
        path = QUrl(path).toLocalFile();
    }
    if (path.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
        path.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        return QUrl(path).toString(QUrl::FullyDecoded).trimmed();
    }
    return QDir::fromNativeSeparators(path);
}

bool isResolvedRemotePlaybackPath(const QString& path)
{
    return path.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
           path.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive);
}

QString normalizeDownloadRelativePath(const QString& rawPath)
{
    QString path = rawPath.trimmed();
    if (path.isEmpty()) {
        return QString();
    }

    if (path.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
        path.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        const QUrl url(path);
        path = QUrl::fromPercentEncoding(url.path().toUtf8()).trimmed();
    }

    while (path.startsWith('/')) {
        path.remove(0, 1);
    }
    if (path.toLower().startsWith(QStringLiteral("uploads/"))) {
        path = path.mid(QStringLiteral("uploads/").size());
    }
    return path;
}

int durationTextToSeconds(const QString& durationText)
{
    const QStringList parts = durationText.trimmed().split(':');
    if (parts.size() == 2) {
        return parts.at(0).toInt() * 60 + parts.at(1).toInt();
    }
    if (parts.size() == 3) {
        return parts.at(0).toInt() * 3600 + parts.at(1).toInt() * 60 + parts.at(2).toInt();
    }
    return 0;
}

QUrl buildActionSongUrl(const QVariantMap& songData)
{
    QString path = songData.value(QStringLiteral("playPath")).toString().trimmed();
    if (path.isEmpty()) {
        path = songData.value(QStringLiteral("path")).toString().trimmed();
    }
    if (path.isEmpty()) {
        return QUrl();
    }

    const bool isLocal = songData.value(QStringLiteral("isLocal")).toBool();
    if (!isLocal && isResolvedRemotePlaybackPath(path)) {
        return QUrl(path);
    }

    if (path.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
        path.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        return QUrl(path);
    }

    return QUrl::fromLocalFile(normalizeSongPath(path));
}

QVariantMap buildPlaylistItemFromSong(const QVariantMap& songData)
{
    QVariantMap item;
    QString musicPath = songData.value(QStringLiteral("playPath")).toString().trimmed();
    if (musicPath.isEmpty()) {
        musicPath = songData.value(QStringLiteral("path")).toString().trimmed();
    }

    const QString title = songData.value(QStringLiteral("title")).toString().trimmed();
    const QString artist = songData.value(QStringLiteral("artist")).toString().trimmed();
    const QString cover = songData.value(QStringLiteral("cover")).toString().trimmed();
    const int durationSeconds = songData.value(QStringLiteral("duration_sec")).toInt() > 0
            ? songData.value(QStringLiteral("duration_sec")).toInt()
            : durationTextToSeconds(songData.value(QStringLiteral("duration")).toString());

    item.insert(QStringLiteral("music_path"), musicPath);
    item.insert(QStringLiteral("music_title"), title.isEmpty() ? QFileInfo(musicPath).completeBaseName() : title);
    item.insert(QStringLiteral("artist"), artist);
    item.insert(QStringLiteral("album"), songData.value(QStringLiteral("album")).toString());
    item.insert(QStringLiteral("duration_sec"), durationSeconds);
    item.insert(QStringLiteral("is_local"), songData.value(QStringLiteral("isLocal")).toBool());

    const QString coverPath = buildPlaylistCoverPathFromSource(cover);
    if (!coverPath.isEmpty()) {
        item.insert(QStringLiteral("cover_art_path"), coverPath);
    } else if (!cover.isEmpty()) {
        item.insert(QStringLiteral("cover_art_url"), cover);
    }
    return item;
}

QString playlistNameById(const QVariantList& playlists, qint64 playlistId)
{
    for (const QVariant& item : playlists) {
        const QVariantMap playlist = item.toMap();
        if (playlist.value(QStringLiteral("id")).toLongLong() == playlistId) {
            const QString name = playlist.value(QStringLiteral("name")).toString().trimmed();
            if (!name.isEmpty()) {
                return name;
            }
        }
    }
    return QString();
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
    connect(m_viewModel, &MainShellViewModel::favoritesListReady,
            this, &MainWidget::handleFavoritesListUpdated);

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

    connect(favoriteMusicWidget, &FavoriteMusicWidget::songActionRequested,
            this, &MainWidget::handleSongActionRequested);
    connect(localAndDownloadWidget, &LocalAndDownloadWidget::songActionRequested,
            this, &MainWidget::handleSongActionRequested);
    connect(net_list, &MusicListWidgetNet::songActionRequested,
            this, &MainWidget::handleSongActionRequested);
    connect(playlistWidget, &PlaylistWidget::songActionRequested,
            this, &MainWidget::handleSongActionRequested);
    connect(recommendMusicWidget, &RecommendMusicWidget::songActionRequested,
            this, &MainWidget::handleSongActionRequested);
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

void MainWidget::handleFavoritesListUpdated(const QVariantList& favorites)
{
    QStringList favoritePaths;
    favoritePaths.reserve(favorites.size());
    for (const QVariant& item : favorites) {
        const QString path = item.toMap().value(QStringLiteral("path")).toString().trimmed();
        if (!path.isEmpty()) {
            favoritePaths.append(path);
        }
    }

    favoriteMusicWidget->setFavoritePaths(favoritePaths);
    localAndDownloadWidget->setFavoritePaths(favoritePaths);
    net_list->setFavoritePaths(favoritePaths);
    playlistWidget->setFavoritePaths(favoritePaths);
    recommendMusicWidget->setFavoritePaths(favoritePaths);
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
    if (sidebarPlaylistSection) {
        sidebarPlaylistSection->setVisible(loggedIn);
    }
    updateSideNavLayout();

    if (!loggedIn) {
        favoriteMusicWidget->clearFavorites();
        playHistoryWidget->clearHistory();
        recommendMusicWidget->clearRecommendations();
        playlistWidget->clearData();
        m_ownedSidebarPlaylists.clear();
        m_subscribedSidebarPlaylists.clear();
        m_sidebarSelectedPlaylistId = -1;
        rebuildSidebarPlaylistButtons();
    } else if (recommendButton && recommendButton->isChecked()) {
        if (m_viewModel) {
            m_viewModel->requestRecommendations(userAccount, QStringLiteral("home"), 24, true);
        }
    }

    if (loggedIn && m_viewModel) {
        m_viewModel->requestPlaylists(userAccount, 1, 20, true);
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
    syncSidebarPlaylistSelection(playlistId);
    m_viewModel->requestPlaylistDetail(m_viewModel->currentUserAccount(), playlistId, false);
}

void MainWidget::handleSidebarOwnedTabClicked()
{
    m_sidebarShowingSubscribedPlaylists = false;
    updateSidebarPlaylistTabs();
    rebuildSidebarPlaylistButtons();
}

void MainWidget::handleSidebarSubscribedTabClicked()
{
    m_sidebarShowingSubscribedPlaylists = true;
    updateSidebarPlaylistTabs();
    rebuildSidebarPlaylistButtons();
}

void MainWidget::handleSidebarCreatePlaylistClicked()
{
    if (!isUserLoggedIn()) {
        showLoginWindow();
        return;
    }
    if (playlistButton && !playlistButton->isChecked()) {
        playlistButton->setChecked(true);
    } else if (playlistWidget) {
        showContentPanel(playlistWidget);
    }
    if (playlistWidget) {
        playlistWidget->openCreatePlaylistDialog();
    }
}

void MainWidget::handleSidebarPlaylistItemClicked()
{
    auto* button = qobject_cast<QPushButton*>(sender());
    if (!button) {
        return;
    }
    const qint64 playlistId = button->property("playlistId").toLongLong();
    if (playlistId <= 0 || !isUserLoggedIn() || !m_viewModel) {
        return;
    }

    syncSidebarPlaylistSelection(playlistId);
    if (playlistButton && !playlistButton->isChecked()) {
        playlistButton->setChecked(true);
    } else if (playlistWidget) {
        showContentPanel(playlistWidget);
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

    m_ownedSidebarPlaylists.clear();
    m_subscribedSidebarPlaylists.clear();
    for (const QVariant& item : playlists) {
        const QVariantMap playlist = item.toMap();
        const QString ownership = playlist.value(QStringLiteral("ownership")).toString().trimmed().toLower();
        if (ownership == QStringLiteral("subscribed")) {
            m_subscribedSidebarPlaylists.append(playlist);
        } else {
            m_ownedSidebarPlaylists.append(playlist);
        }
    }
    favoriteMusicWidget->setAvailablePlaylists(m_ownedSidebarPlaylists);
    localAndDownloadWidget->setAvailablePlaylists(m_ownedSidebarPlaylists);
    net_list->setAvailablePlaylists(m_ownedSidebarPlaylists);
    recommendMusicWidget->setAvailablePlaylists(m_ownedSidebarPlaylists);
    rebuildSidebarPlaylistButtons();
}

void MainWidget::handlePlaylistDetailReady(const QVariantMap& detail)
{
    if (playlistWidget) {
        playlistWidget->loadPlaylistDetail(detail);
    }
    syncSidebarPlaylistSelection(detail.value(QStringLiteral("id")).toLongLong());
}

void MainWidget::handleCreatePlaylistResultReady(bool success, qint64 playlistId, const QString& message)
{
    if (!success) {
        m_pendingAddToNewPlaylistSong.clear();
        QMessageBox::warning(this,
                             QStringLiteral("创建歌单失败"),
                             message.trimmed().isEmpty() ? QStringLiteral("请稍后重试。") : message);
        return;
    }

    if (!m_pendingAddToNewPlaylistSong.isEmpty() && m_viewModel && isUserLoggedIn() && playlistId > 0) {
        QVariantList items;
        items.append(buildPlaylistItemFromSong(m_pendingAddToNewPlaylistSong));
        m_viewModel->addPlaylistItems(m_viewModel->currentUserAccount(), playlistId, items);
        m_pendingAddToNewPlaylistSong.clear();
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
    if (!success) {
        QMessageBox::warning(this,
                             QStringLiteral("添加歌曲失败"),
                             message.trimmed().isEmpty() ? QStringLiteral("请稍后重试。") : message);
        return;
    }

    const QString playlistName = playlistNameById(m_ownedSidebarPlaylists, playlistId);
    const QString tipMessage = addedCount > 0
            ? (playlistName.isEmpty()
               ? QStringLiteral("已添加到目标歌单")
               : QStringLiteral("已添加到歌单：%1").arg(playlistName))
            : (skippedCount > 0
               ? QStringLiteral("歌曲已存在于目标歌单中")
               : QStringLiteral("歌单已更新"));
    QToolTip::showText(QCursor::pos(), tipMessage, this, rect(), 1800);

    if (m_viewModel && isUserLoggedIn()) {
        const QString account = m_viewModel->currentUserAccount();
        m_viewModel->requestPlaylists(account, 1, 20, false);
        if (playlistId > 0) {
            m_viewModel->requestPlaylistDetail(account, playlistId, false);
        }
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

void MainWidget::handleSongActionRequested(const QString& action, const QVariantMap& songData)
{
    if (songData.isEmpty()) {
        return;
    }

    const bool isLocal = songData.value(QStringLiteral("isLocal")).toBool();
    QString playPath = songData.value(QStringLiteral("playPath")).toString().trimmed();
    if (playPath.isEmpty()) {
        playPath = songData.value(QStringLiteral("path")).toString().trimmed();
    }
    const bool requiresResolve = !isLocal
            && !playPath.isEmpty()
            && !isResolvedRemotePlaybackPath(playPath)
            && !QFileInfo(playPath).isAbsolute()
            && (action == QStringLiteral("play")
                || action == QStringLiteral("play_next")
                || action == QStringLiteral("queue_append")
                || action == QStringLiteral("add_to_playlist")
                || action == QStringLiteral("create_playlist_and_add"));

    if (requiresResolve) {
        net_list->resolveSongAction(action, songData);
        return;
    }

    if (action == QStringLiteral("play")) {
        playSongByAction(songData);
        return;
    }
    if (action == QStringLiteral("play_next")) {
        queueSongAsNext(songData);
        return;
    }
    if (action == QStringLiteral("queue_append")) {
        appendSongToPlaybackQueue(songData);
        return;
    }
    if (action == QStringLiteral("add_to_playlist")) {
        addSongToPlaylistByAction(songData);
        return;
    }
    if (action == QStringLiteral("create_playlist_and_add")) {
        createPlaylistAndAddSong(songData);
        return;
    }
    if (action == QStringLiteral("add_favorite") || action == QStringLiteral("remove_favorite")) {
        toggleFavoriteByAction(action, songData);
        return;
    }
    if (action == QStringLiteral("download")) {
        const QString relativePath = normalizeDownloadRelativePath(songData.value(QStringLiteral("path")).toString());
        if (!relativePath.isEmpty()) {
            net_list->onDownloadMusic(relativePath);
        }
        return;
    }
    if (action == QStringLiteral("remove_or_delete")) {
        removeOrDeleteSongByAction(songData);
    }
}

void MainWidget::queueSongAsNext(const QVariantMap& songData)
{
    if (!w || !w->playbackViewModel()) {
        return;
    }

    if (AudioService::instance().currentIndex() < 0 || AudioService::instance().playlistSize() <= 0) {
        playSongByAction(songData);
        return;
    }

    const QUrl url = buildActionSongUrl(songData);
    if (!url.isValid() || url.isEmpty()) {
        return;
    }
    w->playbackViewModel()->rememberTrackMetadata(url, songData);
    w->playbackViewModel()->queueTrackAsNext(url);
}

void MainWidget::appendSongToPlaybackQueue(const QVariantMap& songData)
{
    if (!w || !w->playbackViewModel()) {
        return;
    }

    const QUrl url = buildActionSongUrl(songData);
    if (!url.isValid() || url.isEmpty()) {
        return;
    }
    w->playbackViewModel()->rememberTrackMetadata(url, songData);
    w->playbackViewModel()->appendTrackToQueue(url, true);
}

void MainWidget::addSongToPlaylistByAction(const QVariantMap& songData)
{
    if (!isUserLoggedIn() || !m_viewModel) {
        showLoginWindow();
        return;
    }

    const qint64 playlistId = songData.value(QStringLiteral("playlistId")).toLongLong();
    if (playlistId <= 0) {
        QToolTip::showText(QCursor::pos(), QStringLiteral("请选择目标歌单"), this, rect(), 1500);
        return;
    }

    QVariantList items;
    items.append(buildPlaylistItemFromSong(songData));
    m_viewModel->addPlaylistItems(m_viewModel->currentUserAccount(), playlistId, items);
}

void MainWidget::createPlaylistAndAddSong(const QVariantMap& songData)
{
    if (!isUserLoggedIn() || !m_viewModel) {
        showLoginWindow();
        return;
    }

    bool ok = false;
    const QString name = QInputDialog::getText(this,
                                               QStringLiteral("新建歌单"),
                                               QStringLiteral("请输入歌单名称："),
                                               QLineEdit::Normal,
                                               QString(),
                                               &ok).trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }

    m_pendingAddToNewPlaylistSong = songData;
    m_viewModel->createPlaylist(m_viewModel->currentUserAccount(), name, QString());
}

void MainWidget::toggleFavoriteByAction(const QString& action, const QVariantMap& songData)
{
    if (!isUserLoggedIn() || !m_viewModel) {
        showLoginWindow();
        return;
    }

    const QString path = songData.value(QStringLiteral("path")).toString().trimmed();
    if (path.isEmpty()) {
        return;
    }

    if (action == QStringLiteral("remove_favorite")) {
        m_viewModel->removeFavorite(m_viewModel->currentUserAccount(), QStringList{path});
        return;
    }

    m_viewModel->addFavorite(m_viewModel->currentUserAccount(),
                             path,
                             songData.value(QStringLiteral("title")).toString(),
                             songData.value(QStringLiteral("artist")).toString(),
                             songData.value(QStringLiteral("duration")).toString(),
                             songData.value(QStringLiteral("isLocal")).toBool());
}

void MainWidget::removeOrDeleteSongByAction(const QVariantMap& songData)
{
    const QString sourceType = songData.value(QStringLiteral("sourceType")).toString().trimmed().toLower();
    const QString path = songData.value(QStringLiteral("path")).toString().trimmed();

    if (sourceType == QStringLiteral("favorite")) {
        handleFavoriteRemoveRequested(QStringList{path});
        return;
    }
    if (sourceType == QStringLiteral("playlist")) {
        handlePlaylistRemoveSongsRequested(songData.value(QStringLiteral("playlistId")).toLongLong(),
                                           QStringList{path});
        return;
    }

    handleLocalAndDownloadDeleteMusic(path);
}

void MainWidget::playSongByAction(const QVariantMap& songData)
{
    QString path = songData.value(QStringLiteral("playPath")).toString().trimmed();
    if (path.isEmpty()) {
        path = songData.value(QStringLiteral("path")).toString().trimmed();
    }
    if (path.isEmpty()) {
        return;
    }

    if (w->getNetFlag()) {
        net_list->signalPlayButtonClick(false, "");
    }
    main_list->signalPlayButtonClick(false, "");
    localAndDownloadWidget->setCurrentPlayingPath("");

    const bool isLocal = songData.value(QStringLiteral("isLocal")).toBool();
    const QString title = songData.value(QStringLiteral("title")).toString().trimmed();
    const QString artist = normalizeArtistForHistory(songData.value(QStringLiteral("artist")).toString());
    const QString cover = songData.value(QStringLiteral("cover")).toString().trimmed();
    const QUrl url = buildActionSongUrl(songData);

    w->setPlayNet(!isLocal);
    if (!title.isEmpty() || !artist.isEmpty() || !cover.isEmpty()) {
        w->setNetworkMetadata(title, artist, cover);
    }
    if (!isLocal && !cover.isEmpty()) {
        m_networkMusicArtist = artist;
        m_networkMusicCover = cover;
    }
    if (url.isValid() && !url.isEmpty()) {
        w->playbackViewModel()->rememberTrackMetadata(url, songData);
    }
    w->playClick(path);
}
