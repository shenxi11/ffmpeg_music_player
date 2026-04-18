#include "settings_widget.h"

/*
模块名称: SettingsWidget 连接配置
功能概述: 统一管理 SettingsWidget 的 QML 根对象连接与在线状态刷新定时器。
对外接口: setupRootConnections / setupRefreshTimer
维护说明: 仅放置信号连接与刷新调度，避免构造函数过长。
*/

void SettingsWidget::setupRootConnections(QQuickItem* root)
{
    if (!root) {
        return;
    }

    connect(root, SIGNAL(chooseDownloadPath()), this, SLOT(onChooseDownloadPath()));
    connect(root, SIGNAL(chooseAudioCachePath()), this, SLOT(onChooseAudioCachePath()));
    connect(root, SIGNAL(chooseLogPath()), this, SLOT(onChooseLogPath()));
    connect(root, SIGNAL(clearLocalCacheRequested()), this, SLOT(onClearLocalCacheRequested()));
    connect(root, SIGNAL(settingsClosed()), this, SLOT(close()));
    connect(root, SIGNAL(returnToWelcomeRequested()), this, SLOT(onReturnToWelcomeRequested()));
    connect(root, SIGNAL(refreshPresenceRequested()), this, SLOT(onRefreshPresenceRequested()));
}

void SettingsWidget::setupRefreshTimer()
{
    m_presenceRefreshTimer.setInterval(5000);
    m_presenceRefreshTimer.setSingleShot(false);
    connect(&m_presenceRefreshTimer, &QTimer::timeout,
            this, &SettingsWidget::onRefreshPresenceRequested);
    m_presenceRefreshTimer.start();

    QTimer::singleShot(0, this, &SettingsWidget::onRefreshPresenceRequested);
}
