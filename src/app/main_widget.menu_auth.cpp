#include "main_widget.h"

#include "plugin_host_window.h"

#include <QMessageBox>
#include <QPixmap>
#include <QPoint>
#include <QTimer>

/*
模块名称: MainWidget 菜单与账号连接
功能概述: 管理顶部菜单、插件入口、账号信息与登录态相关的信号连接。
对外接口: MainWidget::setupMenuAndAccountConnections()
维护说明: 菜单弹层与账号区域交互仅在本模块集中维护。
*/

// 建立菜单与账号域连接，保持构造函数职责单一。
void MainWidget::setupMenuAndAccountConnections()
{
    connect(menuButton, &QPushButton::clicked, this, &MainWidget::handleMenuButtonClicked);
    connect(userWidgetQml, &UserWidgetQml::loginRequested, this, &MainWidget::handleUserLoginRequested);
    connect(userWidgetQml, &UserWidgetQml::logoutRequested, this, &MainWidget::handleUserLogoutRequested);
    connect(m_viewModel, &MainShellViewModel::sessionExpired, this, &MainWidget::handleSessionExpired);
    connect(loginWidget, &LoginWidgetQml::login_, this, &MainWidget::handleLoginWidgetSuccess);

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
                             QStringLiteral(u"\u9519\u8bef"),
                             QStringLiteral(u"\u63d2\u4ef6\u201c") + pluginId
                                 + QStringLiteral(u"\u201d\u672a\u627e\u5230\u6216\u52a0\u8f7d\u5931\u8d25\u3002"));
    }
}

void MainWidget::handleMainMenuSettingsRequested()
{
    qDebug() << "Settings requested";
    if (!settingsWidget) {
        settingsWidget = new SettingsWidget(nullptr);
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
                       QStringLiteral(u"\u5173\u4e8e"),
                       QStringLiteral(u"FFmpeg \u97f3\u4e50\u64ad\u653e\u5668 v1.0\\n\u5df2\u96c6\u6210\u97f3\u9891\u8f6c\u6362\u4e0e\u8bed\u97f3\u7ffb\u8bd1\u3002"));
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

void MainWidget::handleUserLogoutRequested()
{
    if (m_viewModel) {
        m_viewModel->logoutCurrentUser(false);
    }
    userWidgetQml->setLoginState(false);
}

void MainWidget::handleSessionExpired()
{
    qWarning() << "[MainWidget] online session expired, forcing logout";
    if (m_viewModel) {
        m_viewModel->clearCurrentUserProfile();
    }
    userWidgetQml->setLoginState(false);
    QMessageBox::warning(this,
                         QStringLiteral("登录状态失效"),
                         QStringLiteral("在线会话已失效，请重新登录。"));
}

void MainWidget::handleLoginWidgetSuccess(const QString& username)
{
    QPixmap userAvatar(":/new/prefix1/icon/denglu.png");
    userWidgetQml->setUserInfo(username, userAvatar);
    userWidgetQml->setLoginState(true);
    loginWidget->close();
    if (m_viewModel) {
        m_viewModel->handleLoginSuccess(m_viewModel->currentUserAccount(),
                                        m_viewModel->currentUserPassword(),
                                        username);
        loginWidget->setSavedAccount(m_viewModel->cachedAccount(),
                                     m_viewModel->cachedPassword(),
                                     m_viewModel->cachedUsername());
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
