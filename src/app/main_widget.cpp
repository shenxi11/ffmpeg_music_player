#include "main_widget.h"
#include "plugin_manager.h"
#include "searchbox_qml.h"
#include "VideoPlayerWindow.h"
#include "playback_state_manager.h"
#include "plugin_host_window.h"
#include "AgentChatViewModel.h"
#include "AgentChatWindow.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

MainWidget::MainWidget(QWidget *parent) : QWidget(parent)
  ,w(nullptr)
  ,list(nullptr)
  ,videoPlayerWindow(nullptr)
  ,videoListWidget(nullptr)
  ,settingsWidget(nullptr)
  ,recommendMusicWidget(nullptr)
  ,playlistWidget(nullptr)
{
    resize(1180, 760);
    setMinimumSize(1000, 640);
    setWindowFlags(Qt::CustomizeWindowHint);
    setAttribute(Qt::WA_QuitOnClose, true);
    setWindowTitle(QStringLiteral(u"\u4e91\u97f3\u4e50"));
    setWindowIcon(QIcon("qrc:/new/prefix1/icon/netease.ico"));
    
    setObjectName("MainWidget");
    m_viewModel = new MainShellViewModel(this);

    m_playbackStateManager = new PlaybackStateManager(this);
    connect(m_playbackStateManager, &PlaybackStateManager::pauseAudioRequested,
            this, &MainWidget::handlePlaybackPauseAudioRequested);
    connect(m_playbackStateManager, &PlaybackStateManager::pauseVideoRequested,
            this, &MainWidget::handlePlaybackPauseVideoRequested);
    
    m_pluginErrorDialogTimer = new QTimer(this);
    m_pluginErrorDialogTimer->setSingleShot(true);
    m_pluginErrorDialogTimer->setInterval(300);
    connect(m_pluginErrorDialogTimer, &QTimer::timeout,
            this, &MainWidget::handlePluginErrorDialogTimeout);
    connect(m_viewModel, &MainShellViewModel::pluginLoadFailed, this, &MainWidget::enqueuePluginLoadError);

    QString pluginPath = QCoreApplication::applicationDirPath() + "/plugin";
    int loadedCount = m_viewModel ? m_viewModel->loadPlugins(pluginPath) : 0;
    qDebug() << "Loaded" << loadedCount << "plugins from" << pluginPath;
    const QVector<PluginLoadFailure> failures = m_viewModel ? m_viewModel->pluginLoadFailures()
                                                            : QVector<PluginLoadFailure>();
    for (const PluginLoadFailure& failure : failures) {
        enqueuePluginLoadError(failure.filePath, failure.reason);
    }

    topWidget = new QWidget(this);
    topWidget->setObjectName("TopBar");
    
    QPushButton* minimizeButton = new QPushButton(topWidget);
    minimizeButton->setObjectName("WindowToggleButton");
    minimizeButton->setFixedSize(30, 30);

    QPushButton* maximizeButton = new QPushButton(topWidget);
    maximizeButton->setObjectName("WindowMinimizeButton");
    maximizeButton->setFixedSize(30, 30);

    QPushButton* closeButton = new QPushButton(topWidget);
    closeButton->setObjectName("WindowCloseButton");
    closeButton->setFixedSize(30, 30);

    connect(minimizeButton, &QPushButton::clicked, this, &MainWidget::handleWindowToggleRequested);
    connect(maximizeButton, &QPushButton::clicked, this, &MainWidget::showMinimized);
    connect(closeButton, &QPushButton::clicked, this, &MainWidget::close);

    searchBox = new SearchBoxQml(this);
    searchBox->setFixedHeight(46);
    searchBox->setMinimumWidth(240);
    searchBox->setMaximumWidth(420);
    searchBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // userWidget = new UserWidget(this);
    
    userWidgetQml = new UserWidgetQml(this);
    userWidgetQml->setFixedHeight(40);
    userWidgetQml->setMinimumWidth(120);
    userWidgetQml->setMaximumWidth(150);
    
    aiAssistantTopButton = new QPushButton(QStringLiteral(u"AI助手"), this);
    aiAssistantTopButton->setFixedHeight(36);
    aiAssistantTopButton->setMinimumWidth(84);
    aiAssistantTopButton->setObjectName("SideNavButton");
    aiAssistantTopButton->hide();

    menuButton = new QPushButton(QStringLiteral(u"\u83dc\u5355"), this);
    menuButton->setFixedSize(50, 50);
    menuButton->setObjectName("MainMenuButton");
    
    mainMenu = nullptr;

    QHBoxLayout* widget_op_layout = new QHBoxLayout(topWidget);

    widget_op_layout->addWidget(searchBox);
    widget_op_layout->addWidget(aiAssistantTopButton);
    widget_op_layout->addStretch();
    widget_op_layout->addWidget(menuButton);
    widget_op_layout->addWidget(maximizeButton);
    widget_op_layout->addWidget(minimizeButton);
    widget_op_layout->addWidget(closeButton);
    widget_op_layout->setSpacing(10);
    widget_op_layout->setContentsMargins(220, 5, 10, 5);


    topWidget->setLayout(widget_op_layout);
    topWidget->setGeometry(0, 0, this->width(), 60);
    topWidget->raise();
    if (userWidgetQml) {
        userWidgetQml->raise();
    }

    loginWidget = new LoginWidgetQml(this);
    loginWidget->setWindowTitle(QStringLiteral(u"\u767b\u5f55"));
    loginWidget->setSavedAccount(m_viewModel ? m_viewModel->cachedAccount() : QString(),
                                 m_viewModel ? m_viewModel->cachedPassword() : QString(),
                                 m_viewModel ? m_viewModel->cachedUsername() : QString());

    main_list = new MusicListWidgetLocal(this);
    main_list->show();
    main_list->setObjectName("local");

    localAndDownloadWidget = new LocalAndDownloadWidget(this);
    localAndDownloadWidget->hide();
    localAndDownloadWidget->setObjectName("localAndDownload");


    list = new MusicListWidget(this);
    list->clear();
    list->close();

    net_list = new MusicListWidgetNet(this);
    net_list->setMainWidget(this);
    net_list->hide();
    net_list->setObjectName("net");
    
    playHistoryWidget = new PlayHistoryWidget(this);
    playHistoryWidget->hide();
    playHistoryWidget->setObjectName("playHistory");
    
    favoriteMusicWidget = new FavoriteMusicWidget(this);
    favoriteMusicWidget->hide();
    favoriteMusicWidget->setObjectName("favoriteMusic");

    recommendMusicWidget = new RecommendMusicWidget(this);
    recommendMusicWidget->hide();
    recommendMusicWidget->setObjectName("recommendMusic");

    playlistWidget = new PlaylistWidget(this);
    playlistWidget->hide();
    playlistWidget->setObjectName("playlist");

    videoListWidget = nullptr;


    leftWidget = new QWidget(this);
    leftWidget->setObjectName("MainLeftPanel");
    
    recommendButton = new QPushButton(QStringLiteral(u"\u63a8\u8350\u97f3\u4e50"), leftWidget);
    recommendButton->setCheckable(true);
    recommendButton->setObjectName("RecommendMusicBtn");
    recommendButton->setProperty("sideNav", true);

    localButton = new QPushButton(QStringLiteral(u"\u672c\u5730\u4e0e\u4e0b\u8f7d"), leftWidget);
    localButton->setCheckable(true);
    localButton->setObjectName("SideNavButton");
    localButton->setProperty("sideNav", true);

    netButton = new QPushButton(QStringLiteral(u"\u5728\u7ebf\u97f3\u4e50"), leftWidget);
    netButton->setCheckable(true);
    netButton->setObjectName("SideNavButton");
    netButton->setProperty("sideNav", true);

    historyButton = new QPushButton(QStringLiteral(u"\u6700\u8fd1\u64ad\u653e"), leftWidget);
    historyButton->setCheckable(true);
    historyButton->setObjectName("SideNavButton");
    historyButton->setProperty("sideNav", true);
    
    favoriteButton = new QPushButton(QStringLiteral(u"\u6211\u559c\u6b22\u7684\u97f3\u4e50"), leftWidget);
    favoriteButton->setCheckable(true);
    favoriteButton->setObjectName("FavoriteMusicBtn");
    favoriteButton->setVisible(false);
    favoriteButton->setProperty("sideNav", true);

    playlistButton = new QPushButton(QStringLiteral(u"\u6211\u7684\u6b4c\u5355"), leftWidget);
    playlistButton->setCheckable(true);
    playlistButton->setObjectName("SideNavButton");
    playlistButton->setProperty("sideNav", true);
    playlistButton->hide();
    
    videoButton = new QPushButton(QStringLiteral(u"\u89c6\u9891\u64ad\u653e"), leftWidget);
    videoButton->setCheckable(true);
    videoButton->setObjectName("VideoPlayerBtn");
    videoButton->setProperty("sideNav", true);

    QButtonGroup* leftButtons = new QButtonGroup(this);
    leftButtons->addButton(recommendButton);
    leftButtons->addButton(localButton);
    leftButtons->addButton(netButton);
    leftButtons->addButton(historyButton);
    leftButtons->addButton(favoriteButton);
    leftButtons->addButton(playlistButton);
    leftButtons->addButton(videoButton);
    leftButtons->setExclusive(true);

    brandWidget = new QWidget(leftWidget);

    QLabel* icolabel = new QLabel(brandWidget);
    icolabel->setPixmap(QPixmap(":/new/prefix1/icon/netease.ico"));
    icolabel->setScaledContents(true);
    icolabel->setFixedSize(40, 40);

    QLabel *textLabel = new QLabel(QStringLiteral(u"\u4e91\u97f3\u4e50"), brandWidget);
    QFont font;
    font.setFamily("Microsoft YaHei");
    font.setPointSize(16);
    font.setBold(true);
    textLabel->setFont(font);
    textLabel->setObjectName("BrandTitleLabel");
    textLabel->adjustSize();

    QHBoxLayout* layout_text = new QHBoxLayout(brandWidget);
    layout_text->addWidget(icolabel);
    layout_text->addWidget(textLabel);

    brandWidget->setLayout(layout_text);

    sidebarPlaylistSection = new QWidget(leftWidget);
    sidebarPlaylistSection->setObjectName("SidebarPlaylistSection");
    sidebarPlaylistSection->hide();

    auto* playlistSectionLayout = new QVBoxLayout(sidebarPlaylistSection);
    playlistSectionLayout->setContentsMargins(0, 0, 0, 0);
    playlistSectionLayout->setSpacing(8);

    auto* playlistTabsWidget = new QWidget(sidebarPlaylistSection);
    auto* playlistTabsLayout = new QHBoxLayout(playlistTabsWidget);
    playlistTabsLayout->setContentsMargins(0, 0, 0, 0);
    playlistTabsLayout->setSpacing(8);

    sidebarOwnedTabButton = new QPushButton(QStringLiteral(u"\u81ea\u5efa\u6b4c\u5355"), playlistTabsWidget);
    sidebarOwnedTabButton->setObjectName("SidebarPlaylistTabButton");
    sidebarOwnedTabButton->setCheckable(true);

    sidebarSubscribedTabButton = new QPushButton(QStringLiteral(u"\u6536\u85cf\u6b4c\u5355"), playlistTabsWidget);
    sidebarSubscribedTabButton->setObjectName("SidebarPlaylistTabButton");
    sidebarSubscribedTabButton->setCheckable(true);

    sidebarPlaylistAddButton = new QPushButton(QStringLiteral("+"), playlistTabsWidget);
    sidebarPlaylistAddButton->setObjectName("SidebarPlaylistAddButton");
    sidebarPlaylistAddButton->setFixedSize(22, 22);

    playlistTabsLayout->addWidget(sidebarOwnedTabButton);
    playlistTabsLayout->addWidget(sidebarSubscribedTabButton);
    playlistTabsLayout->addStretch();
    playlistTabsLayout->addWidget(sidebarPlaylistAddButton);

    sidebarPlaylistScrollArea = new QScrollArea(sidebarPlaylistSection);
    sidebarPlaylistScrollArea->setWidgetResizable(true);
    sidebarPlaylistScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sidebarPlaylistScrollArea->setFrameShape(QFrame::NoFrame);
    sidebarPlaylistScrollArea->setObjectName("SidebarPlaylistScrollArea");

    sidebarPlaylistListContainer = new QWidget(sidebarPlaylistScrollArea);
    sidebarPlaylistListContainer->setObjectName("SidebarPlaylistListContainer");
    sidebarPlaylistListLayout = new QVBoxLayout(sidebarPlaylistListContainer);
    sidebarPlaylistListLayout->setContentsMargins(0, 0, 0, 0);
    sidebarPlaylistListLayout->setSpacing(6);

    sidebarPlaylistEmptyLabel = new QLabel(QStringLiteral(u"\u6682\u65e0\u6b4c\u5355"), sidebarPlaylistListContainer);
    sidebarPlaylistEmptyLabel->setObjectName("SidebarPlaylistEmptyLabel");
    sidebarPlaylistEmptyLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    sidebarPlaylistListLayout->addWidget(sidebarPlaylistEmptyLabel);
    sidebarPlaylistListLayout->addStretch();

    sidebarPlaylistScrollArea->setWidget(sidebarPlaylistListContainer);

    playlistSectionLayout->addWidget(playlistTabsWidget);
    playlistSectionLayout->addWidget(sidebarPlaylistScrollArea, 1);

    connect(recommendButton, &QPushButton::toggled, this, &MainWidget::handleRecommendTabToggled);
    connect(localButton, &QPushButton::toggled, this, &MainWidget::handleLocalTabToggled);
    connect(netButton, &QPushButton::toggled, this, &MainWidget::handleNetTabToggled);
    connect(historyButton, &QPushButton::toggled, this, &MainWidget::handleHistoryTabToggled);
    connect(favoriteButton, &QPushButton::toggled, this, &MainWidget::handleFavoriteTabToggled);
    connect(playlistButton, &QPushButton::toggled, this, &MainWidget::handlePlaylistTabToggled);
    connect(videoButton, &QPushButton::toggled, this, &MainWidget::handleVideoTabToggled);
    connect(aiAssistantTopButton, &QPushButton::clicked, this, &MainWidget::handleAiAssistantClicked);
    connect(sidebarOwnedTabButton, &QPushButton::clicked, this, &MainWidget::handleSidebarOwnedTabClicked);
    connect(sidebarSubscribedTabButton, &QPushButton::clicked, this, &MainWidget::handleSidebarSubscribedTabClicked);
    connect(sidebarPlaylistAddButton, &QPushButton::clicked, this, &MainWidget::handleSidebarCreatePlaylistClicked);

    localButton->setChecked(true);
    updateSidebarPlaylistTabs();

    qDebug() << "[MainWidget] Creating PlayWidget...";

    w = new PlayWidget(this, this);
    w->setGeometry(rect());
    w->setMask(QRegion(0, height() - w->collapsedPlaybackHeight(),
                       qMax(1, width()), w->collapsedPlaybackHeight()));

    qDebug() << "[MainWidget] PlayWidget created successfully";

    connect(w, &PlayWidget::signalPlayState, this, &MainWidget::handlePlayStateChanged);

    qDebug() << "[MainWidget] Creating VideoListWidget...";

    videoListWidget = new VideoListWidget(w, this);
    videoListWidget->hide();
    videoListWidget->setObjectName("videoList");
    connect(videoListWidget, &VideoListWidget::videoPlayerWindowReady,
            this, &MainWidget::handleVideoPlayerWindowReady);
    connect(videoListWidget, &VideoListWidget::videoPlaybackStateChanged,
            this, &MainWidget::handleVideoPlaybackStateChanged);

    qDebug() << "[MainWidget] VideoListWidget created successfully";

    if (m_viewModel) {
        m_viewModel->registerPluginHostService(QStringLiteral("mainWidget"), this);
        m_viewModel->registerPluginHostService(QStringLiteral("httpRequestV2"),
                                               m_viewModel->requestGateway());
        m_viewModel->registerPluginHostService(QStringLiteral("audioService"),
                                               m_viewModel->audioServiceObject());
        if (m_playbackStateManager) {
            m_viewModel->registerPluginHostService(QStringLiteral("playbackStateManager"), m_playbackStateManager);
        }
    }

    connect(searchBox, &SearchBoxQml::search, this, &MainWidget::handleSearchRequested);

    connect(m_viewModel, &MainShellViewModel::searchResultsReady, net_list, &MusicListWidgetNet::signalAddSonglist);
    connect(m_viewModel, &MainShellViewModel::searchResultsReady, this, &MainWidget::handleSearchResultsReady);
    connect(m_viewModel, &MainShellViewModel::recommendationListReady,
            recommendMusicWidget, &RecommendMusicWidget::loadRecommendations);
    connect(m_viewModel, &MainShellViewModel::similarRecommendationListReady,
            this, &MainWidget::handleSimilarRecommendationListReady);

    connect(recommendMusicWidget, &RecommendMusicWidget::refreshRequested,
            this, &MainWidget::handleRecommendRefreshRequested);
    connect(recommendMusicWidget, &RecommendMusicWidget::loginRequested,
            this, &MainWidget::handleRecommendLoginRequested);
    connect(recommendMusicWidget, &RecommendMusicWidget::playMusicWithMetadata,
            this, &MainWidget::handleRecommendPlayMusicWithMetadata);
    connect(recommendMusicWidget, &RecommendMusicWidget::addToFavorite,
            this, &MainWidget::handleRecommendAddToFavorite);
    connect(recommendMusicWidget, &RecommendMusicWidget::feedbackEvent,
            this, &MainWidget::handleRecommendFeedbackEvent);

    connect(w, &PlayWidget::signalSimilarSongSelected,
            this, &MainWidget::handleSimilarSongSelected);

    setupMenuAndAccountConnections();
    setupPlaybackAndListConnections();
    setupLibraryConnections();

    updateAdaptiveLayout();
}

QVariantMap MainWidget::agentVideoWindowState() const
{
    if (!videoPlayerWindow) {
        return {{QStringLiteral("available"), false}};
    }

    QVariantMap snapshot = videoPlayerWindow->snapshot();
    snapshot.insert(QStringLiteral("available"), true);
    return snapshot;
}

bool MainWidget::agentPlayVideo(const QString& source)
{
    if (!videoPlayerWindow) {
        return false;
    }

    const QString trimmed = source.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    videoPlayerWindow->loadVideo(trimmed);
    videoPlayerWindow->show();
    videoPlayerWindow->raise();
    videoPlayerWindow->activateWindow();
    return true;
}

bool MainWidget::agentPauseVideo()
{
    if (!videoPlayerWindow) {
        return false;
    }
    videoPlayerWindow->pausePlayback();
    return true;
}

bool MainWidget::agentResumeVideo()
{
    return videoPlayerWindow ? videoPlayerWindow->resumePlayback() : false;
}

bool MainWidget::agentSeekVideo(qint64 positionMs)
{
    return videoPlayerWindow ? videoPlayerWindow->seekToPosition(positionMs) : false;
}

bool MainWidget::agentSetVideoFullScreen(bool enabled)
{
    return videoPlayerWindow ? videoPlayerWindow->setFullScreenEnabled(enabled) : false;
}

bool MainWidget::agentSetVideoPlaybackRate(double rate)
{
    return videoPlayerWindow ? videoPlayerWindow->setPlaybackRateValue(rate) : false;
}

bool MainWidget::agentSetVideoQualityPreset(const QString& preset)
{
    return videoPlayerWindow ? videoPlayerWindow->setQualityPresetValue(preset) : false;
}

bool MainWidget::agentCloseVideoWindow()
{
    if (!videoPlayerWindow) {
        return false;
    }
    videoPlayerWindow->close();
    return true;
}

QVariantMap MainWidget::agentDesktopLyricsState() const
{
    return w ? w->desktopLyricSnapshot() : QVariantMap{{QStringLiteral("available"), false}};
}

bool MainWidget::agentSetDesktopLyricsVisible(bool visible)
{
    return w ? w->setDesktopLyricVisible(visible) : false;
}

bool MainWidget::agentSetDesktopLyricsStyle(const QVariantMap& style)
{
    return w ? w->setDesktopLyricStyleFromMap(style) : false;
}

void MainWidget::handlePlaybackPauseAudioRequested()
{
    if (m_viewModel) {
        m_viewModel->pauseAudioIfPlaying();
    }
}

void MainWidget::handlePlaybackPauseVideoRequested()
{
    if (videoListWidget) {
        videoListWidget->pauseVideoPlayback();
    } else if (videoPlayerWindow) {
        videoPlayerWindow->pausePlayback();
    }
}

void MainWidget::handlePluginErrorDialogTimeout()
{
    if (m_pendingPluginErrors.isEmpty()) {
        return;
    }
    QMessageBox::warning(this,
                         QStringLiteral("插件加载告警"),
                         QStringLiteral("以下插件加载失败：\n\n")
                             + m_pendingPluginErrors.join(QStringLiteral("\n\n")));
    m_pendingPluginErrors.clear();
}

void MainWidget::handleWindowToggleRequested()
{
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

void MainWidget::showContentPanel(QWidget* activeWidget)
{
    if (recommendMusicWidget) {
        recommendMusicWidget->setVisible(activeWidget == recommendMusicWidget);
    }
    if (main_list) {
        main_list->setVisible(activeWidget == main_list);
    }
    if (localAndDownloadWidget) {
        localAndDownloadWidget->setVisible(activeWidget == localAndDownloadWidget);
    }
    if (net_list) {
        net_list->setVisible(activeWidget == net_list);
    }
    if (playHistoryWidget) {
        playHistoryWidget->setVisible(activeWidget == playHistoryWidget);
    }
    if (favoriteMusicWidget) {
        favoriteMusicWidget->setVisible(activeWidget == favoriteMusicWidget);
    }
    if (playlistWidget) {
        playlistWidget->setVisible(activeWidget == playlistWidget);
    }
    if (videoListWidget) {
        const bool showVideo = (activeWidget == videoListWidget);
        videoListWidget->setVisible(showVideo);
        if (showVideo) {
            videoListWidget->raise();
        }
    }
}

void MainWidget::handleRecommendTabToggled(bool checked)
{
    if (!checked) {
        return;
    }

    showContentPanel(recommendMusicWidget);

    if (isUserLoggedIn()) {
        const QString userAccount = m_viewModel ? m_viewModel->currentUserAccount() : QString();
        if (m_viewModel) {
            m_viewModel->requestRecommendations(userAccount, QStringLiteral("home"), 24, true);
        }
    } else if (recommendMusicWidget) {
        recommendMusicWidget->setLoggedIn(false);
    }
}

void MainWidget::handleLocalTabToggled(bool checked)
{
    if (!checked) {
        return;
    }
    showContentPanel(localAndDownloadWidget);
}

void MainWidget::handleNetTabToggled(bool checked)
{
    if (!checked) {
        return;
    }
    showContentPanel(net_list);
}

void MainWidget::handleHistoryTabToggled(bool checked)
{
    if (!checked) {
        return;
    }
    showContentPanel(playHistoryWidget);

    if (isUserLoggedIn()) {
        const QString userAccount = m_viewModel ? m_viewModel->currentUserAccount() : QString();
        if (m_viewModel) {
            m_viewModel->requestHistory(userAccount, 50);
        }
    }
}

void MainWidget::handleFavoriteTabToggled(bool checked)
{
    if (!checked) {
        return;
    }
    showContentPanel(favoriteMusicWidget);

    const QString userAccount = m_viewModel ? m_viewModel->currentUserAccount() : QString();
    if (m_viewModel) {
        m_viewModel->requestFavorites(userAccount);
    }
}

void MainWidget::handlePlaylistTabToggled(bool checked)
{
    if (!checked) {
        return;
    }

    showContentPanel(playlistWidget);

    if (!isUserLoggedIn() || !m_viewModel) {
        return;
    }

    const QString userAccount = m_viewModel->currentUserAccount();
    m_viewModel->requestPlaylists(userAccount, 1, 20, true);
}

void MainWidget::handleVideoTabToggled(bool checked)
{
    if (!checked) {
        return;
    }
    qDebug() << "[MainWidget] Showing online video list";
    showContentPanel(videoListWidget);
}

void MainWidget::handleAiAssistantClicked()
{
    qDebug() << "[MainWidget] AI assistant button clicked";
    ensureAgentChatWindow();
    if (!m_agentChatWindow || !m_agentChatViewModel) {
        QMessageBox::warning(this,
                             QStringLiteral("AI 助手"),
                             QStringLiteral("聊天窗口创建失败，请检查日志。"));
        return;
    }

    m_agentChatViewModel->initialize();
    m_agentChatWindow->showNormal();
    const QRect hostRect = geometry();
    const QSize chatSize = m_agentChatWindow->size();
    const QPoint centerPos = mapToGlobal(QPoint(hostRect.width() / 2 - chatSize.width() / 2,
                                                 hostRect.height() / 2 - chatSize.height() / 2));
    m_agentChatWindow->move(centerPos);
    m_agentChatWindow->show();
    m_agentChatWindow->raise();
    m_agentChatWindow->activateWindow();
    qDebug() << "[MainWidget] AI assistant window visible =" << m_agentChatWindow->isVisible()
             << "pos =" << m_agentChatWindow->pos()
             << "size =" << m_agentChatWindow->size();
}

void MainWidget::handlePlayStateChanged(ProcessSliderQml::State state)
{
    if (!m_playbackStateManager) {
        return;
    }

    if (state == ProcessSliderQml::Play) {
        m_playbackStateManager->onAudioPlayIntent();
    } else if (state == ProcessSliderQml::Stop) {
        m_playbackStateManager->onAudioInactive();
    }
}

void MainWidget::handleVideoPlayerWindowReady(VideoPlayerWindow* window)
{
    videoPlayerWindow = window;
    qDebug() << "[MainWidget] VideoPlayerWindow linked from VideoListWidget:" << window;
}

void MainWidget::handleVideoPlaybackStateChanged(bool isPlaying)
{
    if (!m_playbackStateManager) {
        return;
    }

    if (isPlaying) {
        m_playbackStateManager->onVideoPlayIntent();
    } else {
        m_playbackStateManager->onVideoInactive();
    }
}

void MainWidget::handleSearchRequested(const QString& keyword)
{
    const QString trimmedKeyword = keyword.trimmed();
    if (trimmedKeyword.isEmpty()) {
        QMessageBox::information(this,
                                 QStringLiteral("提示"),
                                 QStringLiteral("请输入搜索关键词。"));
        return;
    }

    qDebug() << "[MainWidget] Search keyword:" << trimmedKeyword;
    net_list->clearList();
    if (m_viewModel) {
        m_viewModel->searchMusic(trimmedKeyword);
    }
}

void MainWidget::handleSearchResultsReady()
{
    if (netButton) {
        netButton->setChecked(true);
    }
}

void MainWidget::handleSimilarRecommendationListReady(const QVariantMap& meta,
                                                      const QVariantList& items,
                                                      const QString& anchorSongId)
{
    Q_UNUSED(meta);
    Q_UNUSED(anchorSongId);

    if (w) {
        w->setSimilarRecommendations(items);
    }

    if (!isUserLoggedIn() || !m_viewModel) {
        return;
    }

    const QString userAccount = m_viewModel->currentUserAccount();
    for (const QVariant& value : items) {
        const QVariantMap item = value.toMap();
        const QString songId = item.value(QStringLiteral("song_id")).toString();
        if (songId.trimmed().isEmpty()) {
            continue;
        }
        m_viewModel->submitRecommendationFeedback(userAccount,
                                                  songId,
                                                  QStringLiteral("impression"),
                                                  item.value(QStringLiteral("scene")).toString(),
                                                  item.value(QStringLiteral("request_id")).toString(),
                                                  item.value(QStringLiteral("model_version")).toString(),
                                                  -1,
                                                  -1);
    }
}

void MainWidget::handleRecommendRefreshRequested()
{
    if (!isUserLoggedIn()) {
        if (recommendMusicWidget) {
            recommendMusicWidget->setLoggedIn(false);
        }
        return;
    }

    if (m_viewModel) {
        m_viewModel->requestRecommendations(m_viewModel->currentUserAccount(),
                                            QStringLiteral("home"),
                                            24,
                                            true);
    }
}

void MainWidget::handleRecommendLoginRequested()
{
    showLoginWindow();
}

void MainWidget::handleRecommendPlayMusicWithMetadata(const QString& filePath,
                                                      const QString& title,
                                                      const QString& artist,
                                                      const QString& cover,
                                                      const QString& duration,
                                                      const QString& songId,
                                                      const QString& requestId,
                                                      const QString& modelVersion,
                                                      const QString& scene)
{
    Q_UNUSED(duration);
    Q_UNUSED(requestId);
    Q_UNUSED(modelVersion);
    Q_UNUSED(scene);

    qDebug() << "[RecommendMusicWidget] Play music:" << title
             << "songId:" << songId << "path:" << filePath;

    if (!w->getNetFlag()) {
        main_list->signalPlayButtonClick(false, "");
    }
    net_list->signalPlayButtonClick(false, "");
    localAndDownloadWidget->setPlayingState("", false);

    const QString normalizedArtist = normalizeArtistForHistory(artist);
    w->setPlayNet(true);
    w->setNetworkMetadata(title, normalizedArtist, cover);
    m_networkMusicArtist = normalizedArtist;
    m_networkMusicCover = cover;
    w->playClick(filePath);
}

void MainWidget::handleRecommendAddToFavorite(const QString& path,
                                              const QString& title,
                                              const QString& artist,
                                              const QString& duration,
                                              bool isLocal)
{
    if (!isUserLoggedIn()) {
        showLoginWindow();
        return;
    }
    if (m_viewModel) {
        m_viewModel->addFavorite(m_viewModel->currentUserAccount(),
                                 path, title, artist, duration, isLocal);
    }
}

void MainWidget::handleRecommendFeedbackEvent(const QString& songId,
                                              const QString& eventType,
                                              int playMs,
                                              int durationMs,
                                              const QString& scene,
                                              const QString& requestId,
                                              const QString& modelVersion)
{
    if (!isUserLoggedIn() || !m_viewModel) {
        return;
    }

    m_viewModel->submitRecommendationFeedback(m_viewModel->currentUserAccount(),
                                              songId,
                                              eventType,
                                              scene,
                                              requestId,
                                              modelVersion,
                                              playMs,
                                              durationMs);
}

void MainWidget::handleSimilarSongSelected(const QVariantMap& item)
{
    qDebug() << "[MainWidget] Similar song click received, scheduling async play pipeline";
    m_pendingSimilarSongItem = item;
    QTimer::singleShot(0, this, &MainWidget::handlePendingSimilarSongPlayback);
}

void MainWidget::handlePendingSimilarSongPlayback()
{
    if (!w) {
        qWarning() << "[MainWidget] Similar song selected but PlayWidget is null";
        return;
    }

    const QVariantMap item = m_pendingSimilarSongItem;
    m_pendingSimilarSongItem.clear();

    const QString filePath = item.value(QStringLiteral("play_path")).toString().trimmed().isEmpty()
            ? item.value(QStringLiteral("stream_url")).toString()
            : item.value(QStringLiteral("play_path")).toString();
    if (filePath.trimmed().isEmpty()) {
        qWarning() << "[MainWidget] Similar song selected but play path is empty";
        return;
    }

    const QString title = item.value(QStringLiteral("title")).toString();
    const QString artist = normalizeArtistForHistory(item.value(QStringLiteral("artist")).toString());
    const QString cover = item.value(QStringLiteral("cover_art_url")).toString();
    const QString songId = item.value(QStringLiteral("song_id")).toString();
    const QString requestId = item.value(QStringLiteral("request_id")).toString();
    const QString modelVersion = item.value(QStringLiteral("model_version")).toString();
    const QString scene = item.value(QStringLiteral("scene")).toString().trimmed().isEmpty()
            ? QStringLiteral("detail")
            : item.value(QStringLiteral("scene")).toString();

    qDebug() << "[MainWidget] Play similar song:" << title << "songId:" << songId;

    if (!w->getNetFlag()) {
        main_list->signalPlayButtonClick(false, "");
    }
    net_list->signalPlayButtonClick(false, "");
    localAndDownloadWidget->setPlayingState("", false);

    w->setPlayNet(true);
    w->setNetworkMetadata(title, artist, cover);
    m_networkMusicArtist = artist;
    m_networkMusicCover = cover;
    w->playClick(filePath);

    if (isUserLoggedIn() && !songId.trimmed().isEmpty() && m_viewModel) {
        const QString userAccount = m_viewModel->currentUserAccount();
        m_viewModel->submitRecommendationFeedback(userAccount,
                                                  songId,
                                                  QStringLiteral("click"),
                                                  scene,
                                                  requestId,
                                                  modelVersion,
                                                  -1,
                                                  -1);
        m_viewModel->submitRecommendationFeedback(userAccount,
                                                  songId,
                                                  QStringLiteral("play"),
                                                  scene,
                                                  requestId,
                                                  modelVersion,
                                                  0,
                                                  -1);
    }
}

void MainWidget::submitPlayHistoryWithRetry(const QString& sessionId,
                                            const QString& filePath,
                                            const QString& userAccount,
                                            int retryCount)
{
    if (!m_viewModel || userAccount.trimmed().isEmpty()) {
        return;
    }

    QString title;
    QString artist;
    QString album;
    qint64 durationMs = 0;
    bool isLocal = true;
    if (!m_viewModel->resolveHistorySnapshot(sessionId,
                                             filePath,
                                             m_networkMusicArtist,
                                             &title,
                                             &artist,
                                             &album,
                                             &durationMs,
                                             &isLocal)) {
        qDebug() << "[MainWidget] Skip history add: session not available for" << filePath;
        return;
    }

    if (durationMs <= 0 && retryCount > 0) {
        qDebug() << "[MainWidget] History add delayed: duration not ready for" << filePath
                 << "retry left:" << retryCount;
        schedulePlayHistoryRetry(sessionId, filePath, userAccount, retryCount - 1);
        return;
    }

    if (durationMs <= 0 && isLocal) {
        const int cachedSeconds = m_viewModel ? m_viewModel->localMusicCacheDurationSeconds(filePath) : 0;
        if (cachedSeconds > 0) {
            durationMs = static_cast<qint64>(cachedSeconds) * 1000;
            qDebug() << "[MainWidget] History duration filled from local cache:" << cachedSeconds << "s";
        }
    }

    const qint64 durationSec = qMax<qint64>(0, durationMs / 1000);
    qDebug() << "[MainWidget] Add history:" << title
             << "durationSec:" << durationSec
             << "isLocal:" << isLocal;

    m_viewModel->addPlayHistory(userAccount, filePath, title, artist, album,
                                QString::number(durationSec), isLocal);
}

void MainWidget::schedulePlayHistoryRetry(const QString& sessionId,
                                          const QString& filePath,
                                          const QString& userAccount,
                                          int retryCount)
{
    m_pendingHistorySessionId = sessionId;
    m_pendingHistoryFilePath = filePath;
    m_pendingHistoryUserAccount = userAccount;
    m_pendingHistoryRetryCount = retryCount;
    QTimer::singleShot(1200, this, &MainWidget::handlePlayHistoryRetryTimeout);
}

void MainWidget::handlePlayHistoryRetryTimeout()
{
    if (m_pendingHistoryRetryCount < 0) {
        return;
    }

    const QString sessionId = m_pendingHistorySessionId;
    const QString filePath = m_pendingHistoryFilePath;
    const QString userAccount = m_pendingHistoryUserAccount;
    const int retryCount = m_pendingHistoryRetryCount;

    m_pendingHistorySessionId.clear();
    m_pendingHistoryFilePath.clear();
    m_pendingHistoryUserAccount.clear();
    m_pendingHistoryRetryCount = -1;

    submitPlayHistoryWithRetry(sessionId, filePath, userAccount, retryCount);
}

int MainWidget::placeSideNavButton(int row,
                                   QPushButton* button,
                                   int navStartY,
                                   int itemHeight,
                                   int panelWidth)
{
    if (!button) {
        return row;
    }
    button->setGeometry(0, navStartY + row * itemHeight, panelWidth, itemHeight);
    return row + 1;
}

void MainWidget::enqueuePluginLoadError(const QString& pluginFilePath, const QString& reason)
{
    const QString oneError = QStringLiteral("文件: %1\n原因: %2")
                                 .arg(pluginFilePath, reason);
    if (!m_pendingPluginErrors.contains(oneError)) {
        m_pendingPluginErrors.append(oneError);
    }
    if (m_pluginErrorDialogTimer) {
        m_pluginErrorDialogTimer->start();
    }
}

void MainWidget::showPluginDiagnosticsDialog()
{
    const QString report = m_viewModel ? m_viewModel->pluginDiagnosticsReport() : QString();

    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("插件诊断"));
    box.setIcon(QMessageBox::Information);
    box.setText(QStringLiteral("插件诊断已生成。可展开“详细信息”查看完整报告。"));
    box.setDetailedText(report);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

QRect MainWidget::computeContentRect() const
{
    const int topBarHeight = 60;
    const int leftWidth = leftWidget ? leftWidget->width() : 210;
    const int bottomReserved = (w && !w->isUp) ? (w->collapsedPlaybackHeight() + 8) : 100;

    const int x = leftWidth;
    const int y = topBarHeight;
    const int widthValue = qMax(300, width() - x);
    const int heightValue = qMax(180, height() - y - bottomReserved);
    return QRect(x, y, widthValue, heightValue);
}

void MainWidget::updateSideNavLayout()
{
    if (!leftWidget || !brandWidget) {
        return;
    }

    const int panelWidth = leftWidget->width();
    const int itemHeight = 50;
    const int brandTop = 10;
    const int brandHeight = 50;
    brandWidget->setGeometry(8, brandTop, panelWidth - 16, brandHeight);

    const int navStartY = brandTop + brandHeight + 14;
    int row = 0;
    row = placeSideNavButton(row, recommendButton, navStartY, itemHeight, panelWidth);
    row = placeSideNavButton(row, localButton, navStartY, itemHeight, panelWidth);
    row = placeSideNavButton(row, netButton, navStartY, itemHeight, panelWidth);
    row = placeSideNavButton(row, historyButton, navStartY, itemHeight, panelWidth);
    if (favoriteButton && favoriteButton->isVisible()) {
        row = placeSideNavButton(row, favoriteButton, navStartY, itemHeight, panelWidth);
    }
    row = placeSideNavButton(row, videoButton, navStartY, itemHeight, panelWidth);

    if (playlistButton) {
        playlistButton->setGeometry(0, 0, 0, 0);
    }

    if (sidebarPlaylistSection) {
        const int desiredSectionTop = navStartY + row * itemHeight + 18;
        const int maxSectionTop = qMax(0, leftWidget->height() - 18);
        const int sectionTop = qMin(desiredSectionTop, maxSectionTop);
        const int sectionHeight = qMax(0, leftWidget->height() - sectionTop - 18);
        sidebarPlaylistSection->setGeometry(14, sectionTop, panelWidth - 28, sectionHeight);
    }
}

void MainWidget::updateSidebarPlaylistTabs()
{
    if (!sidebarOwnedTabButton || !sidebarSubscribedTabButton) {
        return;
    }

    sidebarOwnedTabButton->setChecked(!m_sidebarShowingSubscribedPlaylists);
    sidebarSubscribedTabButton->setChecked(m_sidebarShowingSubscribedPlaylists);
    sidebarOwnedTabButton->style()->unpolish(sidebarOwnedTabButton);
    sidebarOwnedTabButton->style()->polish(sidebarOwnedTabButton);
    sidebarSubscribedTabButton->style()->unpolish(sidebarSubscribedTabButton);
    sidebarSubscribedTabButton->style()->polish(sidebarSubscribedTabButton);
}

void MainWidget::clearSidebarPlaylistButtons()
{
    for (QPushButton* button : m_sidebarPlaylistButtons) {
        if (sidebarPlaylistListLayout) {
            sidebarPlaylistListLayout->removeWidget(button);
        }
        if (button) {
            button->deleteLater();
        }
    }
    m_sidebarPlaylistButtons.clear();
}

void MainWidget::rebuildSidebarPlaylistButtons()
{
    if (!sidebarPlaylistListLayout || !sidebarPlaylistEmptyLabel) {
        return;
    }

    clearSidebarPlaylistButtons();

    const QVariantList& source = m_sidebarShowingSubscribedPlaylists
                                 ? m_subscribedSidebarPlaylists
                                 : m_ownedSidebarPlaylists;

    if (source.isEmpty()) {
        sidebarPlaylistEmptyLabel->show();
        sidebarPlaylistEmptyLabel->setText(m_sidebarShowingSubscribedPlaylists
                                           ? QStringLiteral("暂无收藏歌单")
                                           : QStringLiteral("暂无自建歌单"));
        return;
    }

    sidebarPlaylistEmptyLabel->hide();
    int insertIndex = 0;
    for (const QVariant& item : source) {
        const QVariantMap playlist = item.toMap();
        const qint64 playlistId = playlist.value(QStringLiteral("id")).toLongLong();
        const QString playlistName = playlist.value(QStringLiteral("name")).toString().trimmed();

        auto* button = new QPushButton(playlistName.isEmpty() ? QStringLiteral("未命名歌单") : playlistName,
                                       sidebarPlaylistListContainer);
        button->setObjectName("SidebarPlaylistItemButton");
        button->setProperty("sidePlaylistItem", true);
        button->setProperty("selectedPlaylist", playlistId == m_sidebarSelectedPlaylistId);
        button->setProperty("playlistId", playlistId);
        button->setProperty("trackCount", playlist.value(QStringLiteral("track_count")).toInt());
        button->setIcon(QIcon(":/new/prefix1/icon/Music.png"));
        button->setIconSize(QSize(18, 18));
        button->setToolTip(QStringLiteral("%1\n%2 首")
                               .arg(button->text())
                               .arg(playlist.value(QStringLiteral("track_count")).toInt()));
        connect(button, &QPushButton::clicked, this, &MainWidget::handleSidebarPlaylistItemClicked);
        sidebarPlaylistListLayout->insertWidget(insertIndex++, button);
        m_sidebarPlaylistButtons.append(button);
    }
}

void MainWidget::syncSidebarPlaylistSelection(qint64 playlistId)
{
    m_sidebarSelectedPlaylistId = playlistId;
    for (QPushButton* button : m_sidebarPlaylistButtons) {
        if (!button) {
            continue;
        }
        const bool selected = button->property("playlistId").toLongLong() == playlistId;
        button->setProperty("selectedPlaylist", selected);
        button->style()->unpolish(button);
        button->style()->polish(button);
        button->update();
    }
}

void MainWidget::ensureAgentChatWindow()
{
    if (!m_agentChatViewModel) {
        m_agentChatViewModel = new AgentChatViewModel(this);
        m_agentChatViewModel->setMainShellViewModel(m_viewModel);
    }

    if (!m_agentChatWindow) {
        m_agentChatWindow = new AgentChatWindow(m_agentChatViewModel, nullptr);
    }
}

void MainWidget::updateAdaptiveLayout()
{
    const int topBarHeight = 60;
    const int leftWidth = qBound(190, width() / 5, 240);

    if (topWidget) {
        topWidget->setGeometry(0, 0, width(), topBarHeight);
        topWidget->raise();
        if (auto* layout = qobject_cast<QHBoxLayout*>(topWidget->layout())) {
            layout->setContentsMargins(leftWidth + 10, 5, 10, 5);
        }
    }

    if (userWidgetQml) {
        const int avatarWidth = qMin(150, qMax(120, leftWidth - 20));
        userWidgetQml->setGeometry(10, 10, avatarWidth, 40);
        userWidgetQml->raise();
    }

    if (searchBox) {
        const int searchWidth = qBound(240, width() / 4, 460);
        searchBox->setMinimumWidth(searchWidth);
        searchBox->setMaximumWidth(searchWidth);
    }

    if (leftWidget) {
        leftWidget->setGeometry(0, topBarHeight, leftWidth, qMax(200, height() - topBarHeight));
    }

    if (w) {
        w->setGeometry(rect());
        if (!w->isUp) {
            const int playbackHeight = w->collapsedPlaybackHeight();
            w->setMask(QRegion(0, qMax(0, height() - playbackHeight),
                               qMax(1, width()), playbackHeight));
            // 收起时仅显示底部区域，但仍需要保证底部控件可交互。
            w->raise();
        } else {
            w->clearMask();
            w->raise();
        }
    }

    const QRect contentRect = computeContentRect();
    if (main_list) {
        main_list->setGeometry(contentRect);
    }
    if (localAndDownloadWidget) {
        localAndDownloadWidget->setGeometry(contentRect);
    }
    if (net_list) {
        net_list->setGeometry(contentRect);
    }
    if (playHistoryWidget) {
        playHistoryWidget->setGeometry(contentRect);
    }
    if (favoriteMusicWidget) {
        favoriteMusicWidget->setGeometry(contentRect);
    }
    if (playlistWidget) {
        playlistWidget->setGeometry(contentRect);
    }
    if (recommendMusicWidget) {
        recommendMusicWidget->setGeometry(contentRect);
    }
    if (videoListWidget) {
        videoListWidget->setGeometry(contentRect);
    }
    if (list) {
        const int popupWidth = qBound(240, width() / 4, 360);
        const int bottomReserved = (w && !w->isUp) ? (w->collapsedPlaybackHeight() + 8) : 20;
        const int popupHeight = qMax(220, height() - topBarHeight - bottomReserved);
        list->setGeometry(width() - popupWidth, topBarHeight, popupWidth, popupHeight);
    }

    updateSideNavLayout();
}

void MainWidget::updatePaint()
{
    main_list->update();
}
void MainWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() & Qt::LeftButton)
    {
        dragging = true;
        this->pos_ = event->globalPos() - this->geometry().topLeft();
        event->accept();
    }
}
void MainWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons() & Qt::LeftButton && dragging)
    {
        QPoint p = event->globalPos();
        this->move(p - this->pos_);
        event->accept();
    }
}
void MainWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() & Qt::LeftButton)
    {
        dragging = false;
        event->accept();
    }
}

void MainWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateAdaptiveLayout();
}

void MainWidget::closeEvent(QCloseEvent *event)
{
    qDebug() << "[MainWidget] closeEvent: start shutdown";
    if (!m_returningToWelcome && m_viewModel) {
        m_viewModel->shutdownUserSessionOnAppExit(true, 1200);
    }

    if (mainMenu) {
        mainMenu->close();
    }
    if (settingsWidget) {
        settingsWidget->close();
        settingsWidget->deleteLater();
        settingsWidget = nullptr;
    }
    if (m_agentChatWindow) {
        m_agentChatWindow->close();
    }
    if (m_agentChatViewModel) {
        m_agentChatViewModel->shutdownForAppExit();
    }
    if (loginWidget) {
        loginWidget->close();
    }
    if (videoPlayerWindow) {
        videoPlayerWindow->close();
    }
    if (w) {
        w->close();
    }

    const auto topLevelList = QApplication::topLevelWidgets();
    for (QWidget* top : topLevelList) {
        if (!top || top == this) {
            continue;
        }
        top->close();
    }

    QWidget::closeEvent(event);
}

MainWidget::~MainWidget()
{
    qDebug() << "MainWidget::~MainWidget() - Starting cleanup...";

    if (m_agentChatViewModel) {
        m_agentChatViewModel->shutdownForAppExit();
    }

    if (videoListWidget) {
        videoListWidget->pauseVideoPlayback();
    }
    if (videoPlayerWindow) {
        videoPlayerWindow->pausePlayback();
    }
    if (m_viewModel) {
        m_viewModel->shutdownAudioPipeline();
    }
    
    if(w) {
        qDebug() << "MainWidget: Deleting PlayWidget...";
        delete w;
        w = nullptr;
    }
    
    if(list) {
        delete list;
        list = nullptr;
    }
    
    if(loginWidget) {
        loginWidget->close();
        delete loginWidget;
        loginWidget = nullptr;
    }
    
    if(videoPlayerWindow) {
        qDebug() << "MainWidget: Deleting VideoPlayerWindow...";
        videoPlayerWindow->close();
        delete videoPlayerWindow;
        videoPlayerWindow = nullptr;
    }

    if (videoListWidget) {
        delete videoListWidget;
        videoListWidget = nullptr;
    }
    
    qDebug() << "MainWidget: Waiting for thread pool...";
    QThreadPool::globalInstance()->waitForDone(3000);
    
    qDebug() << "MainWidget: Unloading plugins...";
    if (m_viewModel) {
        m_viewModel->unloadAllPlugins();
    }
    
    qDebug() << "MainWidget::~MainWidget() - Cleanup complete";
}


