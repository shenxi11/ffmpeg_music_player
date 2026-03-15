#include "main_widget.h"

#include <QUrl>

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

    if (favoriteButton) {
        favoriteButton->setVisible(loggedIn);
        qDebug() << "[MainWidget] Favorite music button visibility:" << loggedIn;
    }
    updateSideNavLayout();

    if (!loggedIn) {
        favoriteMusicWidget->clearFavorites();
        playHistoryWidget->clearHistory();
        recommendMusicWidget->clearRecommendations();
    } else if (recommendButton && recommendButton->isChecked()) {
        if (m_viewModel) {
            m_viewModel->requestRecommendations(userAccount, QStringLiteral("home"), 24, true);
        }
    }
}
