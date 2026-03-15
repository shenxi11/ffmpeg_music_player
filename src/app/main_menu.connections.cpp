#include "main_menu.h"

/*
模块名称: MainMenu 连接配置
功能概述: 统一管理工具菜单的静态连接与动态按钮连接。
对外接口: setupBaseConnections / connectPluginButton / connectActionButtons
维护说明: 所有菜单交互连接集中在此文件，避免 UI 构建代码掺杂连接细节。
*/

void MainMenu::setupBaseConnections()
{
    connect(hideTimer, &QTimer::timeout, this, &MainMenu::hideMenu);
}

void MainMenu::connectPluginButton(QPushButton* button)
{
    if (!button) {
        return;
    }
    connect(button, &QPushButton::clicked, this, &MainMenu::onPluginButtonClicked);
}

void MainMenu::connectActionButtons()
{
    if (diagnosticsBtn) {
        connect(diagnosticsBtn, &QPushButton::clicked, this, &MainMenu::onPluginDiagnosticsClicked);
    }
    if (settingsBtn) {
        connect(settingsBtn, &QPushButton::clicked, this, &MainMenu::onSettingsClicked);
    }
    if (aboutBtn) {
        connect(aboutBtn, &QPushButton::clicked, this, &MainMenu::onAboutClicked);
    }
}
