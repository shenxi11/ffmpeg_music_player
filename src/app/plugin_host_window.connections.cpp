#include "plugin_host_window.h"

#include <QPushButton>

/*
模块名称: PluginHostWindow 连接配置
功能概述: 统一管理插件宿主窗口中的按钮交互连接。
对外接口: PluginHostWindow::setupConnections()
维护说明: 仅维护连接关系，不承载窗口布局构建。
*/

void PluginHostWindow::setupConnections(QPushButton* helpButton)
{
    if (!helpButton) {
        return;
    }
    connect(helpButton, &QPushButton::clicked, this, &PluginHostWindow::showHelpDialog);
}
