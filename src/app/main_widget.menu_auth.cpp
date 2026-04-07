#include "main_widget.h"

#include "plugin_host_window.h"

#include <QDateTime>
#include <QMessageBox>
#include <QPoint>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {

QString defaultAvatarSource()
{
    return QStringLiteral("qrc:/qml/assets/ai/icons/default-user-avatar.svg");
}

QString buildAvatarDisplaySource(const QString& avatarUrl, bool forceRefresh)
{
    const QString trimmed = avatarUrl.trimmed();
    if (trimmed.isEmpty()) {
        return defaultAvatarSource();
    }

    if (!forceRefresh ||
        (!trimmed.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) &&
         !trimmed.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive))) {
        return trimmed;
    }

    QUrl url(trimmed);
    if (!url.isValid()) {
        return trimmed;
    }

    QUrlQuery query(url);
    query.removeAllQueryItems(QStringLiteral("t"));
    query.addQueryItem(QStringLiteral("t"),
                       QString::number(QDateTime::currentMSecsSinceEpoch()));
    url.setQuery(query);
    return url.toString();
}

}

/*
模块名称: MainWidget 菜单与账号连接
功能概述: 管理顶部菜单、插件入口、账号信息与登录态相关的信号连接。
对外接口: MainWidget::setupMenuAndAccountConnections()
维护说明: 菜单弹层与账号区域交互仅在本模块集中维护。
*/

void MainWidget::setupMenuAndAccountConnections()
{
    connect(menuButton, &QPushButton::clicked, this, &MainWidget::handleMenuButtonClicked);
    connect(userWidgetQml, &UserWidgetQml::loginRequested, this, &MainWidget::handleUserLoginRequested);
    connect(userWidgetQml, &UserWidgetQml::profileRequested, this, &MainWidget::handleUserProfileRequested);
    connect(userWidgetQml, &UserWidgetQml::logoutRequested, this, &MainWidget::handleUserLogoutRequested);
    connect(m_viewModel, &MainShellViewModel::sessionExpired, this, &MainWidget::handleSessionExpired);
    connect(loginWidget, &LoginWidgetQml::login_, this, &MainWidget::handleLoginWidgetSuccess);
    connect(m_viewModel, &MainShellViewModel::userProfileReady,
            this, &MainWidget::handleUserProfileReady);
    connect(m_viewModel, &MainShellViewModel::userProfileRequestFailed,
            this, &MainWidget::handleUserProfileRequestFailed);
    connect(m_viewModel, &MainShellViewModel::updateUsernameResultReady,
            this, &MainWidget::handleUpdateUsernameResultReady);
    connect(m_viewModel, &MainShellViewModel::uploadAvatarResultReady,
            this, &MainWidget::handleUploadAvatarResultReady);

    if (userProfileWidget) {
        connect(userProfileWidget, &UserProfileWidget::refreshRequested,
                this, &MainWidget::handleUserProfileRefreshRequested);
        connect(userProfileWidget, &UserProfileWidget::usernameSaveRequested,
                this, &MainWidget::handleUserProfileUsernameSaveRequested);
        connect(userProfileWidget, &UserProfileWidget::avatarFileSelected,
                this, &MainWidget::handleUserProfileAvatarFileSelected);
        connect(userProfileWidget, &UserProfileWidget::favoritesShortcutRequested,
                this, &MainWidget::handleUserProfileFavoritesShortcutRequested);
        connect(userProfileWidget, &UserProfileWidget::historyShortcutRequested,
                this, &MainWidget::handleUserProfileHistoryShortcutRequested);
        connect(userProfileWidget, &UserProfileWidget::playlistsShortcutRequested,
                this, &MainWidget::handleUserProfilePlaylistsShortcutRequested);
        connect(userProfileWidget, &UserProfileWidget::reloginRequested,
                this, &MainWidget::handleUserProfileReloginRequested);
    }

    applyUserIdentityToUi(m_viewModel ? m_viewModel->cachedUsername() : QString(),
                          m_viewModel ? m_viewModel->cachedAvatarUrl() : QString(),
                          false);

    triggerAutoLoginIfNeeded();
}

void MainWidget::handleMenuButtonClicked()
{
    ensureMainMenuCreated();
    if (!mainMenu) {
        return;
    }

    if (m_viewModel) {
        mainMenu->setPluginInfos(m_viewModel->pluginInfos());
    }

    const QPoint globalPos = menuButton->mapToGlobal(QPoint(0, menuButton->height() + 5));
    qDebug() << "Menu button position:" << globalPos;
    qDebug() << "Showing main menu...";
    mainMenu->showMenu(globalPos);
}

void MainWidget::ensureMainMenuCreated()
{
    if (mainMenu) {
        return;
    }

    mainMenu = new MainMenu(this);
    connect(mainMenu, &MainMenu::pluginRequested, this, &MainWidget::handleMainMenuPluginRequested);
    connect(mainMenu, &MainMenu::settingsRequested, this, &MainWidget::handleMainMenuSettingsRequested);
    connect(mainMenu, &MainMenu::pluginDiagnosticsRequested, this, &MainWidget::handleMainMenuPluginDiagnosticsRequested);
    connect(mainMenu, &MainMenu::aboutRequested, this, &MainWidget::handleMainMenuAboutRequested);
}

void MainWidget::handleMainMenuPluginRequested(const QString& pluginId)
{
    qDebug() << "Plugin requested, id:" << pluginId;
    PluginInterface* plugin = m_viewModel ? m_viewModel->pluginById(pluginId) : nullptr;

    if (plugin) {
        PluginHostWindow::Meta meta;
        meta.pluginId = pluginId.trimmed();
        if (meta.pluginId.isEmpty()) {
            meta.pluginId = plugin->pluginId().trimmed();
        }
        meta.name = plugin->pluginName();
        meta.description = plugin->pluginDescription();
        meta.version = plugin->pluginVersion();
        meta.icon = plugin->pluginIcon().isNull() ? windowIcon() : plugin->pluginIcon();

        PluginHostWindow* pluginWindow = new PluginHostWindow(meta, this);
        QWidget* pluginWidget = plugin->createWidget(pluginWindow);
        if (pluginWidget) {
            pluginWindow->setPluginContent(pluginWidget);
            pluginWindow->show();
            pluginWindow->raise();
            pluginWindow->activateWindow();
        } else {
            pluginWindow->deleteLater();
        }
    } else {
        qWarning() << "Plugin not found:" << pluginId;
        QMessageBox::warning(this,
                             QStringLiteral(u"错误"),
                             QStringLiteral(u"插件“") + pluginId
                                 + QStringLiteral(u"”未找到或加载失败。"));
    }
}

void MainWidget::handleMainMenuSettingsRequested()
{
    qDebug() << "Settings requested";
    if (!settingsWidget) {
        settingsWidget = new SettingsWidget(nullptr);
        connect(settingsWidget, &QObject::destroyed, this, [this]() {
            settingsWidget = nullptr;
        });
        connect(settingsWidget, &SettingsWidget::returnToWelcomeRequested,
                this, &MainWidget::handleSettingsReturnToWelcomeRequested);
    }
    settingsWidget->show();
    settingsWidget->raise();
    settingsWidget->activateWindow();
}

void MainWidget::handleMainMenuPluginDiagnosticsRequested()
{
    showPluginDiagnosticsDialog();
}

void MainWidget::handleMainMenuAboutRequested()
{
    QMessageBox::about(this,
                       QStringLiteral(u"关于"),
                       QStringLiteral(u"FFmpeg 音乐播放器 v1.0\n已集成音频转换与语音翻译。"));
}

void MainWidget::handleUserLoginRequested()
{
    loginWidget->isVisible = !loginWidget->isVisible;
    if (loginWidget->isVisible) {
        loginWidget->show();
    } else {
        loginWidget->close();
    }
}

void MainWidget::handleUserProfileRequested()
{
    if (!isUserLoggedIn()) {
        showLoginWindow();
        return;
    }
    if (!userProfileWidget || !m_viewModel) {
        return;
    }

    userProfileWidget->setProfileData(cachedUserProfileSnapshot());
    syncUserProfileStatsToPage();
    syncUserProfilePreviewToPage();
    userProfileWidget->setBusy(true);
    userProfileWidget->setSessionExpired(false);
    userProfileWidget->setStatusMessage(QStringLiteral("info"),
                                        QStringLiteral("正在同步服务端最新资料..."));
    showContentPanel(userProfileWidget);

    m_viewModel->requestUserProfile();
    refreshUserProfileStats();
}

void MainWidget::handleUserLogoutRequested()
{
    if (m_viewModel) {
        m_viewModel->logoutCurrentUser(false);
    }
    userWidgetQml->setLoginState(false);
}

void MainWidget::handleSettingsReturnToWelcomeRequested()
{
    qDebug() << "[MainWidget] return to welcome requested";
    m_returningToWelcome = true;

    if (m_viewModel) {
        m_viewModel->returnToWelcomeAndKeepAccountCache(true, 1200);
    }

    if (settingsWidget) {
        settingsWidget->close();
    }

    emit requestReturnToWelcome();
    close();
}

void MainWidget::handleSessionExpired()
{
    qWarning() << "[MainWidget] online session expired, forcing logout";
    if (userProfileWidget) {
        userProfileWidget->setBusy(false);
        userProfileWidget->setUsernameSaving(false);
        userProfileWidget->setAvatarUploading(false);
        userProfileWidget->setSessionExpired(true);
        userProfileWidget->setStatusMessage(QStringLiteral("error"),
                                            QStringLiteral("登录已过期，请重新登录后再试。"));
    }
    if (m_viewModel) {
        m_viewModel->clearCurrentUserProfile();
    }
    userWidgetQml->setLoginState(false);
    QMessageBox::warning(this,
                         QStringLiteral("登录状态失效"),
                         QStringLiteral("在线会话已失效，请重新登录。"));
}

void MainWidget::handleLoginWidgetSuccess(const QString& username,
                                          const QString& avatarUrl,
                                          const QString& onlineSessionToken)
{
    if (m_viewModel) {
        m_viewModel->handleLoginSuccess(m_viewModel->currentUserAccount(),
                                        m_viewModel->currentUserPassword(),
                                        username,
                                        avatarUrl,
                                        onlineSessionToken);
        loginWidget->setSavedAccount(m_viewModel->cachedAccount(),
                                     m_viewModel->cachedPassword(),
                                     m_viewModel->cachedUsername());
    }
    applyUserIdentityToUi(username, avatarUrl, true);
    loginWidget->close();
}

void MainWidget::handleUserProfileRefreshRequested()
{
    if (!isUserLoggedIn() || !m_viewModel || !userProfileWidget) {
        return;
    }
    userProfileWidget->setBusy(true);
    userProfileWidget->setStatusMessage(QStringLiteral("info"),
                                        QStringLiteral("正在刷新个人资料..."));
    m_viewModel->requestUserProfile();
    refreshUserProfileStats();
}

void MainWidget::handleUserProfileUsernameSaveRequested(const QString& username)
{
    if (!m_viewModel || !userProfileWidget) {
        return;
    }

    const QString trimmed = username.trimmed();
    if (trimmed.isEmpty()) {
        userProfileWidget->setStatusMessage(QStringLiteral("error"),
                                            QStringLiteral("用户名不能为空。"));
        return;
    }

    userProfileWidget->setUsernameSaving(true);
    userProfileWidget->setStatusMessage(QStringLiteral("info"),
                                        QStringLiteral("正在更新用户名..."));
    m_viewModel->updateCurrentUsername(trimmed);
}

void MainWidget::handleUserProfileAvatarFileSelected(const QString& filePath)
{
    if (!m_viewModel || !userProfileWidget || filePath.trimmed().isEmpty()) {
        return;
    }
    userProfileWidget->setAvatarUploading(true);
    userProfileWidget->setStatusMessage(QStringLiteral("info"),
                                        QStringLiteral("正在上传头像..."));
    m_viewModel->uploadCurrentAvatar(filePath);
}

void MainWidget::handleUserProfileFavoritesShortcutRequested()
{
    if (favoriteButton) {
        favoriteButton->setChecked(true);
    } else if (favoriteMusicWidget) {
        showContentPanel(favoriteMusicWidget);
    }
}

void MainWidget::handleUserProfileHistoryShortcutRequested()
{
    if (historyButton) {
        historyButton->setChecked(true);
    } else if (playHistoryWidget) {
        showContentPanel(playHistoryWidget);
    }
}

void MainWidget::handleUserProfilePlaylistsShortcutRequested()
{
    if (!isUserLoggedIn()) {
        showLoginWindow();
        return;
    }
    showContentPanel(playlistWidget);
    if (m_viewModel) {
        m_viewModel->requestPlaylists(m_viewModel->currentUserAccount(), 1, 20, true);
    }
}

void MainWidget::handleUserProfileReloginRequested()
{
    showLoginWindow();
}

void MainWidget::handleUserProfileReady(const QVariantMap& profile)
{
    if (!userProfileWidget) {
        return;
    }
    userProfileWidget->setBusy(false);
    userProfileWidget->setUsernameSaving(false);
    userProfileWidget->setAvatarUploading(false);
    userProfileWidget->setSessionExpired(false);
    userProfileWidget->setProfileData(profile);
    userProfileWidget->setStatusMessage(QStringLiteral("success"),
                                        QStringLiteral("个人资料已同步到最新状态。"));
    if (loginWidget && m_viewModel) {
        loginWidget->setSavedAccount(m_viewModel->cachedAccount(),
                                     m_viewModel->cachedPassword(),
                                     m_viewModel->cachedUsername());
    }
    applyUserIdentityToUi(profile.value(QStringLiteral("username")).toString(),
                          profile.value(QStringLiteral("avatar_url")).toString(),
                          true);
}

void MainWidget::handleUserProfileRequestFailed(const QString& message, int statusCode)
{
    Q_UNUSED(statusCode);
    if (!userProfileWidget) {
        return;
    }
    userProfileWidget->setBusy(false);
    userProfileWidget->setStatusMessage(QStringLiteral("error"),
                                        message.trimmed().isEmpty()
                                            ? QStringLiteral("个人资料加载失败，请稍后重试。")
                                            : message);
}

void MainWidget::handleUpdateUsernameResultReady(bool success,
                                                 const QString& username,
                                                 const QString& message,
                                                 int statusCode)
{
    Q_UNUSED(statusCode);
    if (!userProfileWidget) {
        return;
    }

    userProfileWidget->setUsernameSaving(false);
    if (!success) {
        userProfileWidget->setStatusMessage(QStringLiteral("error"),
                                            message.trimmed().isEmpty()
                                                ? QStringLiteral("用户名修改失败，请稍后重试。")
                                                : message);
        return;
    }

    userProfileWidget->setProfileData(cachedUserProfileSnapshot());
    userProfileWidget->setStatusMessage(QStringLiteral("success"),
                                        QStringLiteral("用户名已更新。"));
    if (loginWidget && m_viewModel) {
        loginWidget->setSavedAccount(m_viewModel->cachedAccount(),
                                     m_viewModel->cachedPassword(),
                                     m_viewModel->cachedUsername());
    }
    applyUserIdentityToUi(username,
                          m_viewModel ? m_viewModel->cachedAvatarUrl() : QString(),
                          true);
    if (m_viewModel) {
        m_viewModel->requestUserProfile();
    }
}

void MainWidget::handleUploadAvatarResultReady(bool success,
                                               const QString& avatarUrl,
                                               const QString& message,
                                               int statusCode)
{
    Q_UNUSED(statusCode);
    if (!userProfileWidget) {
        return;
    }

    userProfileWidget->setAvatarUploading(false);
    if (!success) {
        userProfileWidget->clearPendingAvatarPreview();
        userProfileWidget->setProfileData(cachedUserProfileSnapshot());
        userProfileWidget->setStatusMessage(QStringLiteral("error"),
                                            message.trimmed().isEmpty()
                                                ? QStringLiteral("头像上传失败，请稍后重试。")
                                                : message);
        return;
    }

    applyUserIdentityToUi(m_viewModel ? m_viewModel->cachedUsername() : QString(),
                          avatarUrl,
                          true,
                          true);
    userProfileWidget->clearPendingAvatarPreview();
    userProfileWidget->setProfileData(cachedUserProfileSnapshot());
    userProfileWidget->setStatusMessage(QStringLiteral("success"),
                                        QStringLiteral("头像已更新。"));
    if (m_viewModel) {
        m_viewModel->requestUserProfile();
    }
}

void MainWidget::triggerAutoLoginIfNeeded()
{
    const QString cachedAccount = m_viewModel ? m_viewModel->cachedAccount() : QString();
    const QString cachedPassword = m_viewModel ? m_viewModel->cachedPassword() : QString();
    if (!m_viewModel || !loginWidget) {
        qDebug() << "[MainWidget] Auto login skipped: viewModel or loginWidget is null";
        return;
    }

    const bool autoAllowed = m_viewModel->shouldAutoLogin();
    if (!autoAllowed || cachedAccount.isEmpty() || cachedPassword.isEmpty()) {
        qDebug() << "[MainWidget] Auto login skipped:"
                 << "shouldAutoLogin=" << autoAllowed
                 << "cachedAccountEmpty=" << cachedAccount.isEmpty()
                 << "cachedPasswordEmpty=" << cachedPassword.isEmpty();
        return;
    }

    qDebug() << "[MainWidget] Auto login with cached account:" << cachedAccount;
    QTimer::singleShot(0, this, [this, cachedAccount, cachedPassword]() {
        if (!loginWidget) {
            return;
        }
        loginWidget->requestLogin(cachedAccount, cachedPassword, true);
    });
}

QVariantMap MainWidget::cachedUserProfileSnapshot() const
{
    QVariantMap profile;
    if (!m_viewModel) {
        return profile;
    }

    const QString username = m_viewModel->cachedUsername().trimmed();
    profile.insert(QStringLiteral("username"),
                   username.isEmpty() ? QStringLiteral("未命名用户") : username);
    profile.insert(QStringLiteral("account"), m_viewModel->cachedAccount());
    profile.insert(QStringLiteral("avatar_url"), m_viewModel->cachedAvatarUrl());
    profile.insert(QStringLiteral("created_at"), m_viewModel->cachedProfileCreatedAt());
    profile.insert(QStringLiteral("updated_at"), m_viewModel->cachedProfileUpdatedAt());
    return profile;
}

void MainWidget::refreshUserProfileStats()
{
    if (!m_viewModel || !isUserLoggedIn()) {
        return;
    }

    const QString account = m_viewModel->currentUserAccount();
    m_viewModel->requestFavorites(account, true);
    m_viewModel->requestHistory(account, 50, true);
    m_viewModel->requestPlaylists(account, 1, 20, true);
}

void MainWidget::syncUserProfileStatsToPage()
{
    if (!userProfileWidget) {
        return;
    }
    userProfileWidget->setStatsData(m_profileFavoritesCount,
                                    m_profileHistoryCount,
                                    m_profileOwnedPlaylistsCount);
}

void MainWidget::syncUserProfilePreviewToPage()
{
    if (!userProfileWidget) {
        return;
    }
    userProfileWidget->setPreviewData(m_profileFavoritesPreview,
                                      m_profileHistoryPreview,
                                      m_profileOwnedPlaylistsPreview);
}

void MainWidget::applyUserIdentityToUi(const QString& username,
                                       const QString& avatarUrl,
                                       bool loggedIn,
                                       bool forceAvatarRefresh)
{
    if (!userWidgetQml) {
        return;
    }

    const QString displayName = loggedIn
            ? (username.trimmed().isEmpty() ? QStringLiteral("未命名用户") : username.trimmed())
            : QStringLiteral("未登录");
    const QString displayAvatar = loggedIn
            ? buildAvatarDisplaySource(avatarUrl, forceAvatarRefresh)
            : defaultAvatarSource();
    userWidgetQml->setUserInfo(displayName, displayAvatar);
    userWidgetQml->setLoginState(loggedIn);
}
