#include "main_widget.h"

#include "playback_state_manager.h"

#include <QDebug>
#include <QUrl>

/*
模块名称: MainWidget 播放状态处理
功能概述: 处理音频会话状态变更与播放页展开/收起交互，驱动跨列表 UI 同步。
对外接口: handleAudioPlayback* / handlePlayWidgetBigClicked
维护说明: 仅维护状态切换与界面同步，不承载列表播放入口逻辑。
*/

void MainWidget::handleAudioPlaybackStarted(const QString& sessionId, const QUrl& url) {
    if (m_playbackStateManager) {
        m_playbackStateManager->onAudioPlayIntent();
    }

    qDebug() << "[MainWidget] playbackStarted signal received! sessionId:" << sessionId
             << "url:" << url;

    QString filePath = url.toLocalFile();
    if (filePath.isEmpty()) {
        filePath = url.toString();
    }

    qDebug() << "[MainWidget] Extracted filePath:" << filePath;
    qDebug() << "[MainWidget] About to call setCurrentPlayingPath on both widgets...";
    playHistoryWidget->setCurrentPlayingPath(filePath);
    favoriteMusicWidget->setCurrentPlayingPath(filePath);
    favoriteMusicWidget->setPlayingState(filePath, true);
    localAndDownloadWidget->setPlayingState(filePath, true);
    net_list->setPlayingState(filePath, true);
    recommendMusicWidget->setCurrentPlayingPath(filePath);
    recommendMusicWidget->setPlayingState(filePath, true);
    playlistWidget->setCurrentPlayingPath(filePath);
    playlistWidget->setPlayingState(filePath, true);
    qDebug() << "[MainWidget] setCurrentPlayingPath calls completed";

    const QString songId = extractSongIdFromMediaPath(filePath);
    if (!m_localOnlyMode && !songId.isEmpty() && m_viewModel) {
        m_viewModel->requestSimilarRecommendations(songId, 12);
    } else {
        w->clearSimilarRecommendations();
    }

    const QString userAccount = m_viewModel ? m_viewModel->currentUserAccount() : QString();
    if (userAccount.isEmpty()) {
        qDebug() << "[MainWidget] User not logged in, skipping history add";
        return;
    }

    submitPlayHistoryWithRetry(sessionId, filePath, userAccount, 1);
}

void MainWidget::handleAudioPlaybackPaused() {
    if (m_playbackStateManager) {
        m_playbackStateManager->onAudioInactive();
    }

    QUrl url = AudioService::instance().currentUrl();
    QString filePath = url.toLocalFile();
    if (filePath.isEmpty()) {
        filePath = url.toString();
    }
    localAndDownloadWidget->setPlayingState(filePath, false);
    net_list->setPlayingState(filePath, false);
    playHistoryWidget->setPlayingState(filePath, false);
    favoriteMusicWidget->setPlayingState(filePath, false);
    recommendMusicWidget->setPlayingState(filePath, false);
    playlistWidget->setPlayingState(filePath, false);
}

void MainWidget::handleAudioPlaybackResumed() {
    if (m_playbackStateManager) {
        m_playbackStateManager->onAudioPlayIntent();
    }

    QUrl url = AudioService::instance().currentUrl();
    QString filePath = url.toLocalFile();
    if (filePath.isEmpty()) {
        filePath = url.toString();
    }
    localAndDownloadWidget->setPlayingState(filePath, true);
    net_list->setPlayingState(filePath, true);
    favoriteMusicWidget->setPlayingState(filePath, true);
    playHistoryWidget->setPlayingState(filePath, true);
    recommendMusicWidget->setPlayingState(filePath, true);
    playlistWidget->setPlayingState(filePath, true);
}

void MainWidget::handleAudioPlaybackStopped() {
    if (m_playbackStateManager) {
        m_playbackStateManager->onAudioInactive();
    }

    qDebug() << "[MainWidget] playbackStopped signal received, clearing currentPlayingPath";
    playHistoryWidget->setCurrentPlayingPath("");
    favoriteMusicWidget->setCurrentPlayingPath("");
    favoriteMusicWidget->setPlayingState("", false);
    recommendMusicWidget->setCurrentPlayingPath("");
    recommendMusicWidget->setPlayingState("", false);
    playlistWidget->setCurrentPlayingPath("");
    playlistWidget->setPlayingState("", false);
    net_list->setPlayingState("", false);
    localAndDownloadWidget->setPlayingState("", false);
    w->clearSimilarRecommendations();
}

void MainWidget::handlePlayWidgetBigClicked(bool checked) {
    qDebug() << "[MainWidget] signalBigClicked checked =" << checked;
    if (checked) {
        if (w && w->isMainContentCommentVisible()) {
            closeCommentContentPage(true);
        }
        if (topWidget) {
            topWidget->hide();
        }
        if (searchBox) {
            searchBox->hide();
        }
        if (userWidgetQml) {
            userWidgetQml->hide();
        }

        w->raise();
        w->clearMask();
        w->setIsUp(true);
        update();
        return;
    }

    if (topWidget) {
        topWidget->show();
        topWidget->raise();
    }
    if (searchBox) {
        searchBox->show();
    }
    if (userWidgetQml) {
        userWidgetQml->show();
    }

    // 收起态也保持播放层在最上方，避免底部点击事件被其他面板抢占。
    w->raise();
    w->setIsUp(false);
    w->closeCommentDrawerPanel();
    update();
    w->setPianWidgetEnable(false);
    updateAdaptiveLayout();
}

void MainWidget::handleMainCommentPageRequested(bool show) {
    if (!w) {
        return;
    }

    if (show) {
        w->closePlaylistHistoryPanel();
        openCommentContentPage();
        return;
    }

    closeCommentContentPage(true);
}
