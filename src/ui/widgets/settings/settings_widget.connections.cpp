#include "settings_widget.h"

/*
模块名称: SettingsWidget 连接配置
功能概述: 统一管理 SettingsWidget 的 QML 根对象连接、ViewModel 回写连接和刷新定时器。
对外接口: setupRootConnections / setupViewModelConnections / setupRefreshTimer
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

    connect(root, SIGNAL(downloadPathChanged()), this, SLOT(onDownloadPathChanged()));
    connect(root, SIGNAL(downloadLyricsChanged()), this, SLOT(onDownloadLyricsChanged()));
    connect(root, SIGNAL(downloadCoverChanged()), this, SLOT(onDownloadCoverChanged()));
    connect(root, SIGNAL(audioCachePathChanged()), this, SLOT(onAudioCachePathChanged()));
    connect(root, SIGNAL(logPathChanged()), this, SLOT(onLogPathChanged()));
    connect(root, SIGNAL(playerPageStyleChanged()), this, SLOT(onPlayerPageStyleChanged()));
    connect(root, SIGNAL(agentModeChanged()), this, SLOT(onAgentModeChanged()));
    connect(root, SIGNAL(agentLocalModelPathChanged()),
            this,
            SLOT(onAgentLocalModelPathChanged()));
    connect(root, SIGNAL(agentLocalModelBaseUrlChanged()),
            this,
            SLOT(onAgentLocalModelBaseUrlChanged()));
    connect(root, SIGNAL(agentLocalModelNameChanged()), this, SLOT(onAgentLocalModelNameChanged()));
    connect(root, SIGNAL(agentLocalContextSizeChanged()),
            this,
            SLOT(onAgentLocalContextSizeChanged()));
    connect(root, SIGNAL(agentLocalThreadCountChanged()),
            this,
            SLOT(onAgentLocalThreadCountChanged()));
    connect(root, SIGNAL(agentRemoteFallbackEnabledChanged()),
            this,
            SLOT(onAgentRemoteFallbackEnabledChanged()));
    connect(root, SIGNAL(agentRemoteBaseUrlChanged()), this, SLOT(onAgentRemoteBaseUrlChanged()));
    connect(root, SIGNAL(agentRemoteModelNameChanged()),
            this,
            SLOT(onAgentRemoteModelNameChanged()));
    connect(root, SIGNAL(refreshPresenceRequested()), this, SLOT(onRefreshPresenceRequested()));
}

void SettingsWidget::setupViewModelConnections()
{
    connect(m_viewModel, &SettingsViewModel::downloadPathChanged,
            this, &SettingsWidget::syncViewModelToRoot);
    connect(m_viewModel, &SettingsViewModel::audioCachePathChanged,
            this, &SettingsWidget::syncViewModelToRoot);
    connect(m_viewModel, &SettingsViewModel::logPathChanged,
            this, &SettingsWidget::syncViewModelToRoot);
    connect(m_viewModel, &SettingsViewModel::downloadLyricsChanged,
            this, &SettingsWidget::syncViewModelToRoot);
    connect(m_viewModel, &SettingsViewModel::downloadCoverChanged,
            this, &SettingsWidget::syncViewModelToRoot);
    connect(m_viewModel, &SettingsViewModel::playerPageStyleChanged,
            this, &SettingsWidget::syncViewModelToRoot);
    connect(m_viewModel, &SettingsViewModel::agentSettingsChanged,
            this, &SettingsWidget::syncViewModelToRoot);
    connect(m_viewModel, &SettingsViewModel::presenceChanged,
            this, &SettingsWidget::syncPresenceToRoot);
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
