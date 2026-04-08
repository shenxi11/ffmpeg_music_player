#include "main_widget.h"

/*
模块名称: MainWidget 播放连接装配
功能概述: 集中维护播放域、列表域与状态域的信号连接映射。
对外接口: MainWidget::setupPlaybackAndListConnections()
维护说明: 仅保留连接表，具体处理逻辑下沉到各命名处理函数。
*/

void MainWidget::setupPlaybackAndListConnections() {
    connect(net_list, &MusicListWidgetNet::loginRequired, this,
            &MainWidget::handleNetLoginRequired);
    connect(main_list, &MusicListWidgetLocal::signalAddButtonClicked, w, &PlayWidget::openfile);

    connect(w, &PlayWidget::signalLast, this, &MainWidget::handlePlayWidgetLast);
    connect(w, &PlayWidget::signalNext, this, &MainWidget::handlePlayWidgetNext);
    connect(w, &PlayWidget::signalNetFlagChanged, this,
            &MainWidget::handlePlayWidgetNetFlagChanged);
    connect(w, &PlayWidget::signalAddSong, main_list, &MusicListWidgetLocal::onAddSong);
    connect(w, &PlayWidget::signalAddSong, this, &MainWidget::handlePlayWidgetAddSongToCache);
    connect(w, &PlayWidget::signalPlayButtonClick, main_list,
            &MusicListWidgetLocal::onPlayButtonClick);
    connect(w, &PlayWidget::signalPlayButtonClick, this, &MainWidget::handlePlayWidgetButtonState);

    connect(w, &PlayWidget::signalMetadataUpdated, main_list,
            &MusicListWidgetLocal::onUpdateMetadata);
    connect(w, &PlayWidget::signalMetadataUpdated, localAndDownloadWidget,
            [this](const QString& filePath, const QString& coverUrl, const QString& duration,
                   const QString& artist) {
                localAndDownloadWidget->updateDownloadedSongMetadata(filePath, coverUrl, duration,
                                                                     artist);
            });
    connect(w, &PlayWidget::signalMetadataUpdated, this,
            &MainWidget::handlePlayWidgetMetadataUpdated);
    connect(main_list, &MusicListWidgetLocal::signalPlayClick, this,
            &MainWidget::handleLocalListPlayClick);
    connect(main_list, &MusicListWidgetLocal::signalRemoveClick, w, &PlayWidget::removeClick);

    connect(localAndDownloadWidget, &LocalAndDownloadWidget::playMusic, this,
            &MainWidget::handleLocalAndDownloadPlayMusic);
    connect(localAndDownloadWidget, &LocalAndDownloadWidget::addLocalMusicRequested, w,
            &PlayWidget::openfile);
    connect(localAndDownloadWidget, &LocalAndDownloadWidget::deleteMusic, this,
            &MainWidget::handleLocalAndDownloadDeleteMusic);

    connect(net_list, &MusicListWidgetNet::signalPlayClick, this,
            &MainWidget::handleNetListPlayClick);
    connect(playHistoryWidget, &PlayHistoryWidget::playMusic, this,
            &MainWidget::handleHistoryPlayMusic);
    connect(playHistoryWidget, &PlayHistoryWidget::playMusicWithMetadata, this,
            &MainWidget::handleHistoryPlayMusicWithMetadata);

    connect(m_viewModel, &MainShellViewModel::audioPlaybackStarted, this,
            &MainWidget::handleAudioPlaybackStarted);
    connect(m_viewModel, &MainShellViewModel::audioPlaybackPaused, this,
            &MainWidget::handleAudioPlaybackPaused);
    connect(m_viewModel, &MainShellViewModel::audioPlaybackResumed, this,
            &MainWidget::handleAudioPlaybackResumed);
    connect(m_viewModel, &MainShellViewModel::audioPlaybackStopped, this,
            &MainWidget::handleAudioPlaybackStopped);

    connect(w, &PlayWidget::signalBigClicked, this, &MainWidget::handlePlayWidgetBigClicked);
}
