#include "user_widget.h"

/*
模块名称: UserWidget 连接配置
功能概述: 统一管理用户入口与悬浮弹窗的交互连接。
对外接口: UserPopupWidget::setupConnections / UserWidget::setupConnections
维护说明: 连接逻辑集中维护，降低构造函数复杂度。
*/

void UserPopupWidget::setupConnections()
{
    connect(actionButton, &QPushButton::clicked, this, &UserPopupWidget::onActionButtonClicked);
}

void UserWidget::setupConnections()
{
    connect(hoverTimer, &QTimer::timeout, this, &UserWidget::showPopup);
    connect(hideTimer, &QTimer::timeout, this, &UserWidget::hidePopup);

    connect(popup, &UserPopupWidget::loginRequested, this, &UserWidget::onPopupActionRequested);
    connect(popup, &UserPopupWidget::logoutRequested, this, &UserWidget::onPopupLogoutRequested);
}

void UserWidget::onPopupLogoutRequested()
{
    setLoginState(false);
    emit logoutRequested();
}
