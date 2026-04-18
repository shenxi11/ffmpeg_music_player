#include "main_widget.h"

#include "VideoPlayerWindow.h"
#include "playback_state_manager.h"
#include "plugin_host_window.h"
#include "search_history_popup.h"
#include "plugin_manager.h"
#include "searchbox_qml.h"
#include "settings_manager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QSaveFile>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QIcon defaultSidebarPlaylistCoverIcon() {
    return QIcon(QStringLiteral(":/qml/assets/ai/icons/default-music-cover.svg"));
}

bool isRemoteCoverSource(const QString& source) {
    return source.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
           source.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive);
}

bool isWindowsDrivePath(const QString& path) {
    return path.size() >= 3 && path.at(1) == QLatin1Char(':') && path.at(0).isLetter() &&
           (path.at(2) == QLatin1Char('/') || path.at(2) == QLatin1Char('\\'));
}

QString collapseDuplicateUploads(QString text) {
    text = QDir::fromNativeSeparators(text.trimmed());
    while (text.contains(QStringLiteral("/uploads/uploads/"), Qt::CaseInsensitive)) {
        text.replace(QStringLiteral("/uploads/uploads/"), QStringLiteral("/uploads/"),
                     Qt::CaseInsensitive);
    }
    while (text.startsWith(QStringLiteral("uploads/uploads/"), Qt::CaseInsensitive)) {
        text = QStringLiteral("uploads/") + text.mid(QStringLiteral("uploads/uploads/").size());
    }
    return text;
}

QString stripUploadsPrefix(QString path) {
    path = collapseDuplicateUploads(path);
    while (path.startsWith(QLatin1Char('/'))) {
        path.remove(0, 1);
    }
    while (path.startsWith(QStringLiteral("uploads/"), Qt::CaseInsensitive)) {
        path = path.mid(QStringLiteral("uploads/").size());
    }
    return path;
}

QString normalizeSidebarPlaylistCoverSource(const QString& rawCover) {
    QString cover = collapseDuplicateUploads(rawCover);
    if (cover.isEmpty()) {
        return QString();
    }

    if (cover.startsWith(QStringLiteral("qrc:/"), Qt::CaseInsensitive)) {
        return QStringLiteral(":") + cover.mid(4);
    }

    if (cover.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        const QUrl localUrl(cover);
        return localUrl.isLocalFile() ? QDir::fromNativeSeparators(localUrl.toLocalFile())
                                      : QString();
    }

    if (isRemoteCoverSource(cover)) {
        const QUrl url(cover);
        if (!url.isValid()) {
            return QString();
        }
        const QString localCandidate =
            stripUploadsPrefix(QUrl::fromPercentEncoding(url.path().toUtf8()));
        if (isWindowsDrivePath(localCandidate) || QFileInfo(localCandidate).isAbsolute()) {
            return localCandidate;
        }
        return cover;
    }

    cover = stripUploadsPrefix(cover);
    return cover;
}

QString playlistCoverCacheDirectory() {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return QDir(base.isEmpty() ? QDir::currentPath() : base)
        .absoluteFilePath(QStringLiteral("playlist_cover_cache"));
}

QString playlistCoverCacheIndexPath() {
    return QDir(playlistCoverCacheDirectory()).absoluteFilePath(QStringLiteral("index.json"));
}

QString playlistCoverCacheKey(const QString& account, qint64 playlistId, const QString& updatedAt) {
    const QString rawKey =
        QStringLiteral("%1|%2|%3").arg(account, QString::number(playlistId), updatedAt);
    const QByteArray digest =
        QCryptographicHash::hash(rawKey.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QString::fromLatin1(digest.constData(), digest.size());
}

QString playlistCoverSourceHash(const QString& source) {
    const QByteArray digest =
        QCryptographicHash::hash(source.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QString::fromLatin1(digest.constData(), digest.size());
}

QString cachedCoverFilePathForSource(const QString& source) {
    if (!isRemoteCoverSource(source)) {
        return QString();
    }
    return QDir(playlistCoverCacheDirectory())
        .absoluteFilePath(playlistCoverSourceHash(source) + QStringLiteral(".img"));
}

QJsonObject loadPlaylistCoverCacheIndex() {
    QFile file(playlistCoverCacheIndexPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    return document.isObject() ? document.object() : QJsonObject{};
}

void savePlaylistCoverCacheIndex(const QJsonObject& index) {
    QDir().mkpath(playlistCoverCacheDirectory());
    QSaveFile file(playlistCoverCacheIndexPath());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[PlaylistCoverCache] Cannot write index:" << playlistCoverCacheIndexPath();
        return;
    }
    file.write(QJsonDocument(index).toJson(QJsonDocument::Compact));
    if (!file.commit()) {
        qWarning() << "[PlaylistCoverCache] Commit index failed:" << playlistCoverCacheIndexPath();
    }
}

} // namespace

MainWidget::MainWidget(bool localOnlyMode, QWidget* parent)
    : QWidget(parent), w(nullptr), list(nullptr), videoPlayerWindow(nullptr),
      videoListWidget(nullptr), settingsWidget(nullptr), recommendMusicWidget(nullptr),
      playlistWidget(nullptr), m_localOnlyMode(localOnlyMode) {
    resize(1180, 760);
    setMinimumSize(1000, 640);
    setWindowFlags(Qt::CustomizeWindowHint);
    setAttribute(Qt::WA_QuitOnClose, true);
    setWindowTitle(QStringLiteral(u"\u4e91\u97f3\u4e50"));
    setWindowIcon(QIcon("qrc:/new/prefix1/icon/netease.ico"));

    setObjectName("MainWidget");
    m_viewModel = new MainShellViewModel(this);

    m_playbackStateManager = new PlaybackStateManager(this);
    connect(m_playbackStateManager, &PlaybackStateManager::pauseAudioRequested, this,
            &MainWidget::handlePlaybackPauseAudioRequested);
    connect(m_playbackStateManager, &PlaybackStateManager::pauseVideoRequested, this,
            &MainWidget::handlePlaybackPauseVideoRequested);

    m_pluginErrorDialogTimer = new QTimer(this);
    m_pluginErrorDialogTimer->setSingleShot(true);
    m_pluginErrorDialogTimer->setInterval(300);
    connect(m_pluginErrorDialogTimer, &QTimer::timeout, this,
            &MainWidget::handlePluginErrorDialogTimeout);
    connect(m_viewModel, &MainShellViewModel::pluginLoadFailed, this,
            &MainWidget::enqueuePluginLoadError);

    QString pluginPath = QCoreApplication::applicationDirPath() + "/plugin";
    int loadedCount = m_viewModel ? m_viewModel->loadPlugins(pluginPath) : 0;
    qDebug() << "Loaded" << loadedCount << "plugins from" << pluginPath;
    const QVector<PluginLoadFailure> failures =
        m_viewModel ? m_viewModel->pluginLoadFailures() : QVector<PluginLoadFailure>();
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

    menuButton = new QPushButton(QStringLiteral(u"\u83dc\u5355"), this);
    menuButton->setFixedHeight(50);
    menuButton->setMinimumWidth(92);
    menuButton->setObjectName("MainMenuButton");
    menuButton->setIcon(QIcon(QStringLiteral(":/qml/assets/ai/icons/main-menu.svg")));
    menuButton->setIconSize(QSize(18, 18));

    mainMenu = nullptr;

    QHBoxLayout* widget_op_layout = new QHBoxLayout(topWidget);

    widget_op_layout->addWidget(searchBox);
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

    userProfileWidget = new UserProfileWidget(this);
    userProfileWidget->hide();
    userProfileWidget->setObjectName("userProfile");

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

    favoriteButton =
        new QPushButton(QStringLiteral(u"\u6211\u559c\u6b22\u7684\u97f3\u4e50"), leftWidget);
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

    QLabel* textLabel = new QLabel(QStringLiteral(u"\u4e91\u97f3\u4e50"), brandWidget);
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

    sidebarOwnedTabButton =
        new QPushButton(QStringLiteral(u"\u81ea\u5efa\u6b4c\u5355"), playlistTabsWidget);
    sidebarOwnedTabButton->setObjectName("SidebarPlaylistTabButton");
    sidebarOwnedTabButton->setCheckable(true);

    sidebarSubscribedTabButton =
        new QPushButton(QStringLiteral(u"\u6536\u85cf\u6b4c\u5355"), playlistTabsWidget);
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

    sidebarPlaylistEmptyLabel =
        new QLabel(QStringLiteral(u"\u6682\u65e0\u6b4c\u5355"), sidebarPlaylistListContainer);
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
    connect(sidebarOwnedTabButton, &QPushButton::clicked, this,
            &MainWidget::handleSidebarOwnedTabClicked);
    connect(sidebarSubscribedTabButton, &QPushButton::clicked, this,
            &MainWidget::handleSidebarSubscribedTabClicked);
    connect(sidebarPlaylistAddButton, &QPushButton::clicked, this,
            &MainWidget::handleSidebarCreatePlaylistClicked);

    localButton->setChecked(true);
    updateSidebarPlaylistTabs();

    qDebug() << "[MainWidget] Creating PlayWidget...";

    w = new PlayWidget(this, this);
    w->setMainShellViewModel(m_viewModel);
    w->setGeometry(rect());
    w->setMask(QRegion(0, height() - w->collapsedPlaybackHeight(), qMax(1, width()),
                       w->collapsedPlaybackHeight()));

    qDebug() << "[MainWidget] PlayWidget created successfully";

    commentPageWidget = new CommentPanelQml(this);
    commentPageWidget->setDisplayMode(QStringLiteral("page"));
    commentPageWidget->hide();
    w->setEmbeddedCommentPanel(commentPageWidget);

    connect(w, &PlayWidget::signalPlayState, this, &MainWidget::handlePlayStateChanged);

    qDebug() << "[MainWidget] Creating VideoListWidget...";

    videoListWidget = new VideoListWidget(w, this);
    videoListWidget->hide();
    videoListWidget->setObjectName("videoList");
    connect(videoListWidget, &VideoListWidget::videoPlayerWindowReady, this,
            &MainWidget::handleVideoPlayerWindowReady);
    connect(videoListWidget, &VideoListWidget::videoPlaybackStateChanged, this,
            &MainWidget::handleVideoPlaybackStateChanged);

    qDebug() << "[MainWidget] VideoListWidget created successfully";

    connect(searchBox, &SearchBoxQml::search, this, &MainWidget::handleSearchRequested);
    connect(searchBox, &SearchBoxQml::inputActivated, this, &MainWidget::handleSearchBoxActivated);
    connect(searchBox, &SearchBoxQml::textEdited, this, &MainWidget::handleSearchBoxTextEdited);

    connect(m_viewModel, &MainShellViewModel::searchResultsReady, net_list,
            &MusicListWidgetNet::signalAddSonglist);
    connect(m_viewModel, &MainShellViewModel::searchResultsReady, this,
            &MainWidget::handleSearchResultsReady);
    connect(m_viewModel, &MainShellViewModel::recommendationListReady, recommendMusicWidget,
            &RecommendMusicWidget::loadRecommendations);
    connect(m_viewModel, &MainShellViewModel::similarRecommendationListReady, this,
            &MainWidget::handleSimilarRecommendationListReady);

    connect(recommendMusicWidget, &RecommendMusicWidget::refreshRequested, this,
            &MainWidget::handleRecommendRefreshRequested);
    connect(recommendMusicWidget, &RecommendMusicWidget::loginRequested, this,
            &MainWidget::handleRecommendLoginRequested);
    connect(recommendMusicWidget, &RecommendMusicWidget::playMusicWithMetadata, this,
            &MainWidget::handleRecommendPlayMusicWithMetadata);
    connect(recommendMusicWidget, &RecommendMusicWidget::addToFavorite, this,
            &MainWidget::handleRecommendAddToFavorite);
    connect(recommendMusicWidget, &RecommendMusicWidget::feedbackEvent, this,
            &MainWidget::handleRecommendFeedbackEvent);

    connect(w, &PlayWidget::signalSimilarSongSelected, this,
            &MainWidget::handleSimilarSongSelected);

    setupMenuAndAccountConnections();
    setupPlaybackAndListConnections();
    setupLibraryConnections();
    if (m_localOnlyMode) {
        applyLocalOnlyModeUi();
    }

    updateAdaptiveLayout();
}

void MainWidget::handlePlaybackPauseAudioRequested() {
    if (m_viewModel) {
        m_viewModel->pauseAudioIfPlaying();
    }
}

void MainWidget::handlePlaybackPauseVideoRequested() {
    if (videoListWidget) {
        videoListWidget->pauseVideoPlayback();
    } else if (videoPlayerWindow) {
        videoPlayerWindow->pausePlayback();
    }
}

void MainWidget::handlePluginErrorDialogTimeout() {
    if (m_pendingPluginErrors.isEmpty()) {
        return;
    }
    QMessageBox::warning(this, QStringLiteral("插件加载告警"),
                         QStringLiteral("以下插件加载失败：\n\n") +
                             m_pendingPluginErrors.join(QStringLiteral("\n\n")));
    m_pendingPluginErrors.clear();
}

void MainWidget::handleWindowToggleRequested() {
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

void MainWidget::showContentPanel(QWidget* activeWidget) {
    if (m_localOnlyMode && activeWidget != localAndDownloadWidget &&
        activeWidget != settingsWidget) {
        activeWidget = localAndDownloadWidget;
    }

    const bool leavingCommentContent =
        commentPageWidget && m_activeContentWidget == commentPageWidget &&
        activeWidget != commentPageWidget;
    if (leavingCommentContent) {
        if (w) {
            w->setMainContentCommentVisible(false);
        }
        if (!m_commentContentOpening) {
            m_previousContentWidgetBeforeComment = nullptr;
        }
    }

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
    if (userProfileWidget) {
        userProfileWidget->setVisible(activeWidget == userProfileWidget);
    }
    if (settingsWidget) {
        settingsWidget->setVisible(activeWidget == settingsWidget);
        if (activeWidget == settingsWidget) {
            settingsWidget->raise();
        }
    }
    if (commentPageWidget) {
        commentPageWidget->setVisible(activeWidget == commentPageWidget);
        if (activeWidget == commentPageWidget) {
            commentPageWidget->raise();
        }
    }
    if (videoListWidget) {
        const bool showVideo = (activeWidget == videoListWidget);
        videoListWidget->setVisible(showVideo);
        if (showVideo) {
            videoListWidget->raise();
        }
    }

    m_activeContentWidget = activeWidget;
}

void MainWidget::showLocalOnlyUnavailableMessage() {
    QMessageBox::information(this, QStringLiteral("离线模式"),
                             QStringLiteral("当前为离线直进模式，仅支持本地功能。"));
}

void MainWidget::updateSearchBoxForMode() {
    if (!searchBox) {
        return;
    }

    if (QQuickItem* rootItem = searchBox->rootObject()) {
        rootItem->setProperty("placeholderText", m_localOnlyMode
                                                     ? QStringLiteral(" 离线模式仅支持本地内容")
                                                     : QStringLiteral(" 搜索想听的歌曲吧..."));
    }
    searchBox->setEnabled(!m_localOnlyMode);
    if (m_localOnlyMode) {
        searchBox->clear();
        hideSearchHistoryPopup();
    }
}

void MainWidget::ensureSearchHistoryPopup() {
    if (m_searchHistoryPopup) {
        return;
    }

    m_searchHistoryPopup = new SearchHistoryPopup(this);
    m_searchHistoryPopup->setAnchorWidget(searchBox);
    connect(m_searchHistoryPopup, &SearchHistoryPopup::historyActivated, this,
            &MainWidget::handleSearchHistoryActivated);
    connect(m_searchHistoryPopup, &SearchHistoryPopup::historyDeleteRequested, this,
            &MainWidget::handleSearchHistoryDeleteRequested);
    connect(m_searchHistoryPopup, &SearchHistoryPopup::clearAllRequested, this,
            &MainWidget::handleClearSearchHistoryRequested);
}

void MainWidget::repositionSearchHistoryPopup() {
    if (!m_searchHistoryPopup || !m_searchHistoryPopup->isVisible() || !searchBox) {
        return;
    }

    const int popupWidth = qMax(300, searchBox->width());
    m_searchHistoryPopup->setFixedWidth(popupWidth);
    m_searchHistoryPopup->adjustSize();

    QPoint anchorPoint = searchBox->mapTo(this, QPoint(0, searchBox->height() + 6));
    int x = anchorPoint.x();
    int y = anchorPoint.y();
    if (x + popupWidth > width() - 12) {
        x = qMax(12, width() - popupWidth - 12);
    }

    const int maxHeight = qMax(120, height() - y - 24);
    const int popupHeight = qMin(m_searchHistoryPopup->sizeHint().height(), maxHeight);
    m_searchHistoryPopup->setGeometry(x, y, popupWidth, popupHeight);
    m_searchHistoryPopup->raise();
}

void MainWidget::showSearchHistoryPopup(const QString& filterText) {
    if (m_localOnlyMode || !searchBox || !searchBox->isEnabled()) {
        return;
    }

    ensureSearchHistoryPopup();
    m_searchHistoryPopup->setHistoryItems(SettingsManager::instance().searchHistoryKeywords());
    m_searchHistoryPopup->setFilterText(filterText);
    repositionSearchHistoryPopup();
    m_searchHistoryPopup->show();
    m_searchHistoryPopup->raise();
}

void MainWidget::hideSearchHistoryPopup() {
    if (m_searchHistoryPopup) {
        m_searchHistoryPopup->hide();
    }
}

void MainWidget::applyLocalOnlyModeUi() {
    updateSearchBoxForMode();

    if (recommendButton) {
        recommendButton->hide();
    }
    if (netButton) {
        netButton->hide();
    }
    if (historyButton) {
        historyButton->hide();
    }
    if (favoriteButton) {
        favoriteButton->hide();
    }
    if (playlistButton) {
        playlistButton->hide();
    }
    if (videoButton) {
        videoButton->hide();
    }
    if (sidebarPlaylistSection) {
        sidebarPlaylistSection->hide();
    }
    if (userWidgetQml) {
        userWidgetQml->setPopupBlocked(true);
        userWidgetQml->setLoginState(false);
        userWidgetQml->setUserInfo(
            QStringLiteral("本地模式"),
            QStringLiteral("qrc:/qml/assets/ai/icons/default-user-avatar.svg"));
    }
    if (localButton) {
        localButton->show();
        localButton->setChecked(true);
    }
    showContentPanel(localAndDownloadWidget);
    updateSideNavLayout();
}

void MainWidget::handleRecommendTabToggled(bool checked) {
    if (!checked) {
        return;
    }
    if (m_localOnlyMode) {
        if (localButton) {
            localButton->setChecked(true);
        }
        showLocalOnlyUnavailableMessage();
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

void MainWidget::handleLocalTabToggled(bool checked) {
    if (!checked) {
        return;
    }
    showContentPanel(localAndDownloadWidget);
}

void MainWidget::handleNetTabToggled(bool checked) {
    if (!checked) {
        return;
    }
    if (m_localOnlyMode) {
        if (localButton) {
            localButton->setChecked(true);
        }
        showLocalOnlyUnavailableMessage();
        return;
    }
    showContentPanel(net_list);
}

void MainWidget::handleHistoryTabToggled(bool checked) {
    if (!checked) {
        return;
    }
    if (m_localOnlyMode) {
        if (localButton) {
            localButton->setChecked(true);
        }
        showLocalOnlyUnavailableMessage();
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

void MainWidget::handleFavoriteTabToggled(bool checked) {
    if (!checked) {
        return;
    }
    if (m_localOnlyMode) {
        if (localButton) {
            localButton->setChecked(true);
        }
        showLocalOnlyUnavailableMessage();
        return;
    }
    showContentPanel(favoriteMusicWidget);

    const QString userAccount = m_viewModel ? m_viewModel->currentUserAccount() : QString();
    if (m_viewModel) {
        m_viewModel->requestFavorites(userAccount);
    }
}

void MainWidget::handlePlaylistTabToggled(bool checked) {
    if (!checked) {
        return;
    }
    if (m_localOnlyMode) {
        if (localButton) {
            localButton->setChecked(true);
        }
        showLocalOnlyUnavailableMessage();
        return;
    }

    showContentPanel(playlistWidget);

    if (!isUserLoggedIn() || !m_viewModel) {
        return;
    }

    const QString userAccount = m_viewModel->currentUserAccount();
    m_viewModel->requestPlaylists(userAccount, 1, 20, true);
}

void MainWidget::handleVideoTabToggled(bool checked) {
    if (!checked) {
        return;
    }
    if (m_localOnlyMode) {
        if (localButton) {
            localButton->setChecked(true);
        }
        showLocalOnlyUnavailableMessage();
        return;
    }
    qDebug() << "[MainWidget] Showing online video list";
    showContentPanel(videoListWidget);
}

void MainWidget::handleAiAssistantClicked() {
    qDebug() << "[MainWidget] Built-in AI assistant entry is disabled; use plugin menu instead.";
}

void MainWidget::handlePlayStateChanged(ProcessSliderQml::State state) {
    if (!m_playbackStateManager) {
        return;
    }

    if (state == ProcessSliderQml::Play) {
        m_playbackStateManager->onAudioPlayIntent();
    } else if (state == ProcessSliderQml::Stop) {
        m_playbackStateManager->onAudioInactive();
    }
}

void MainWidget::handleVideoPlayerWindowReady(VideoPlayerWindow* window) {
    videoPlayerWindow = window;
    qDebug() << "[MainWidget] VideoPlayerWindow linked from VideoListWidget:" << window;
}

void MainWidget::handleVideoPlaybackStateChanged(bool isPlaying) {
    if (!m_playbackStateManager) {
        return;
    }

    if (isPlaying) {
        m_playbackStateManager->onVideoPlayIntent();
    } else {
        m_playbackStateManager->onVideoInactive();
    }
}

void MainWidget::handleSearchBoxActivated() {
    showSearchHistoryPopup(searchBox ? searchBox->text() : QString());
}

void MainWidget::handleSearchBoxTextEdited(const QString& text) {
    if (m_localOnlyMode) {
        return;
    }
    showSearchHistoryPopup(text);
}

void MainWidget::handleSearchHistoryActivated(const QString& keyword) {
    const QString trimmedKeyword = keyword.trimmed();
    if (trimmedKeyword.isEmpty()) {
        return;
    }

    if (searchBox) {
        searchBox->setText(trimmedKeyword);
    }
    hideSearchHistoryPopup();
    handleSearchRequested(trimmedKeyword);
}

void MainWidget::handleSearchHistoryDeleteRequested(const QString& keyword) {
    SettingsManager::instance().removeSearchHistoryKeyword(keyword);
    showSearchHistoryPopup(searchBox ? searchBox->text() : QString());
}

void MainWidget::handleClearSearchHistoryRequested() {
    SettingsManager::instance().clearSearchHistoryKeywords();
    showSearchHistoryPopup(searchBox ? searchBox->text() : QString());
}

void MainWidget::handleSearchRequested(const QString& keyword) {
    hideSearchHistoryPopup();
    if (m_localOnlyMode) {
        showLocalOnlyUnavailableMessage();
        return;
    }

    const QString trimmedKeyword = keyword.trimmed();
    if (trimmedKeyword.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请输入搜索关键词。"));
        return;
    }

    SettingsManager::instance().addSearchHistoryKeyword(trimmedKeyword);
    qDebug() << "[MainWidget] Search keyword:" << trimmedKeyword;
    net_list->clearList();
    if (m_viewModel) {
        m_viewModel->searchMusic(trimmedKeyword);
    }
}

void MainWidget::handleSearchResultsReady() {
    if (m_localOnlyMode) {
        return;
    }
    if (netButton) {
        netButton->setChecked(true);
    }
}

void MainWidget::handleSimilarRecommendationListReady(const QVariantMap& meta,
                                                      const QVariantList& items,
                                                      const QString& anchorSongId) {
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
        m_viewModel->submitRecommendationFeedback(
            userAccount, songId, QStringLiteral("impression"),
            item.value(QStringLiteral("scene")).toString(),
            item.value(QStringLiteral("request_id")).toString(),
            item.value(QStringLiteral("model_version")).toString(), -1, -1);
    }
}

void MainWidget::handleRecommendRefreshRequested() {
    if (!isUserLoggedIn()) {
        if (recommendMusicWidget) {
            recommendMusicWidget->setLoggedIn(false);
        }
        return;
    }

    if (m_viewModel) {
        m_viewModel->requestRecommendations(m_viewModel->currentUserAccount(),
                                            QStringLiteral("home"), 24, true);
    }
}

void MainWidget::handleRecommendLoginRequested() {
    showLoginWindow();
}

void MainWidget::handleRecommendPlayMusicWithMetadata(
    const QString& filePath, const QString& musicPath, const QString& title, const QString& artist,
    const QString& cover, const QString& duration, const QString& songId,
    const QString& requestId, const QString& modelVersion, const QString& scene) {
    Q_UNUSED(duration);
    Q_UNUSED(requestId);
    Q_UNUSED(modelVersion);
    Q_UNUSED(scene);

    qDebug() << "[RecommendMusicWidget] Play music:" << title << "songId:" << songId
             << "path:" << filePath;

    if (!w->getNetFlag()) {
        main_list->signalPlayButtonClick(false, "");
    }
    net_list->setPlayingState("", false);
    localAndDownloadWidget->setPlayingState("", false);

    const QString normalizedArtist = normalizeArtistForHistory(artist);
    rememberPlaybackQueueMetadata(filePath, title, normalizedArtist, cover);
    w->setPlayNet(true);
    w->setNetworkMetadata(title, normalizedArtist, cover);
    applyCommentTrackContext(musicPath, title, normalizedArtist, cover);
    m_networkMusicArtist = normalizedArtist;
    m_networkMusicCover = cover;
    w->playClick(filePath);
}

void MainWidget::handleRecommendAddToFavorite(const QString& path, const QString& title,
                                              const QString& artist, const QString& duration,
                                              bool isLocal) {
    if (!isUserLoggedIn()) {
        showLoginWindow();
        return;
    }
    if (m_viewModel) {
        m_viewModel->addFavorite(m_viewModel->currentUserAccount(), path, title, artist, duration,
                                 isLocal);
    }
}

void MainWidget::handleRecommendFeedbackEvent(const QString& songId, const QString& eventType,
                                              int playMs, int durationMs, const QString& scene,
                                              const QString& requestId,
                                              const QString& modelVersion) {
    if (!isUserLoggedIn() || !m_viewModel) {
        return;
    }

    m_viewModel->submitRecommendationFeedback(m_viewModel->currentUserAccount(), songId, eventType,
                                              scene, requestId, modelVersion, playMs, durationMs);
}

void MainWidget::handleSimilarSongSelected(const QVariantMap& item) {
    qDebug() << "[MainWidget] Similar song click received, scheduling async play pipeline";
    m_pendingSimilarSongItem = item;
    QTimer::singleShot(0, this, &MainWidget::handlePendingSimilarSongPlayback);
}

void MainWidget::handlePendingSimilarSongPlayback() {
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
    const QString artist =
        normalizeArtistForHistory(item.value(QStringLiteral("artist")).toString());
    const QString cover = item.value(QStringLiteral("cover_art_url")).toString();
    const QString originalMusicPath = item.value(QStringLiteral("path")).toString();
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
    net_list->setPlayingState("", false);
    localAndDownloadWidget->setPlayingState("", false);

    w->setPlayNet(true);
    rememberPlaybackQueueMetadata(filePath, title, artist, cover);
    w->setNetworkMetadata(title, artist, cover);
    applyCommentTrackContext(originalMusicPath, title, artist, cover);
    m_networkMusicArtist = artist;
    m_networkMusicCover = cover;
    w->playClick(filePath);

    if (isUserLoggedIn() && !songId.trimmed().isEmpty() && m_viewModel) {
        const QString userAccount = m_viewModel->currentUserAccount();
        m_viewModel->submitRecommendationFeedback(userAccount, songId, QStringLiteral("click"),
                                                  scene, requestId, modelVersion, -1, -1);
        m_viewModel->submitRecommendationFeedback(userAccount, songId, QStringLiteral("play"),
                                                  scene, requestId, modelVersion, 0, -1);
    }
}

void MainWidget::submitPlayHistoryWithRetry(const QString& sessionId, const QString& filePath,
                                            const QString& userAccount, int retryCount) {
    if (!m_viewModel || userAccount.trimmed().isEmpty()) {
        return;
    }

    QString title;
    QString artist;
    QString album;
    qint64 durationMs = 0;
    bool isLocal = true;
    if (!m_viewModel->resolveHistorySnapshot(sessionId, filePath, m_networkMusicArtist, &title,
                                             &artist, &album, &durationMs, &isLocal)) {
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
        const int cachedSeconds =
            m_viewModel ? m_viewModel->localMusicCacheDurationSeconds(filePath) : 0;
        if (cachedSeconds > 0) {
            durationMs = static_cast<qint64>(cachedSeconds) * 1000;
            qDebug() << "[MainWidget] History duration filled from local cache:" << cachedSeconds
                     << "s";
        }
    }

    const qint64 durationSec = qMax<qint64>(0, durationMs / 1000);
    qDebug() << "[MainWidget] Add history:" << title << "durationSec:" << durationSec
             << "isLocal:" << isLocal;

    m_viewModel->addPlayHistory(userAccount, filePath, title, artist, album,
                                QString::number(durationSec), isLocal);
}

void MainWidget::schedulePlayHistoryRetry(const QString& sessionId, const QString& filePath,
                                          const QString& userAccount, int retryCount) {
    m_pendingHistorySessionId = sessionId;
    m_pendingHistoryFilePath = filePath;
    m_pendingHistoryUserAccount = userAccount;
    m_pendingHistoryRetryCount = retryCount;
    QTimer::singleShot(1200, this, &MainWidget::handlePlayHistoryRetryTimeout);
}

void MainWidget::handlePlayHistoryRetryTimeout() {
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

int MainWidget::placeSideNavButton(int row, QPushButton* button, int navStartY, int itemHeight,
                                   int panelWidth) {
    if (!button || !button->isVisible()) {
        if (button) {
            button->setGeometry(0, 0, 0, 0);
        }
        return row;
    }
    button->setGeometry(0, navStartY + row * itemHeight, panelWidth, itemHeight);
    return row + 1;
}

void MainWidget::enqueuePluginLoadError(const QString& pluginFilePath, const QString& reason) {
    const QString oneError = QStringLiteral("文件: %1\n原因: %2").arg(pluginFilePath, reason);
    if (!m_pendingPluginErrors.contains(oneError)) {
        m_pendingPluginErrors.append(oneError);
    }
    if (m_pluginErrorDialogTimer) {
        m_pluginErrorDialogTimer->start();
    }
}

void MainWidget::showPluginDiagnosticsDialog() {
    const QString report = m_viewModel ? m_viewModel->pluginDiagnosticsReport() : QString();

    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("插件诊断"));
    box.setIcon(QMessageBox::Information);
    box.setText(QStringLiteral("插件诊断已生成。可展开“详细信息”查看完整报告。"));
    box.setDetailedText(report);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

QRect MainWidget::computeContentRect() const {
    const int topBarHeight = 60;
    const int leftWidth = leftWidget ? leftWidget->width() : 210;
    const int bottomReserved = (w && !w->isUp) ? (w->collapsedPlaybackHeight() + 8) : 100;

    const int x = leftWidth;
    const int y = topBarHeight;
    const int widthValue = qMax(300, width() - x);
    const int heightValue = qMax(180, height() - y - bottomReserved);
    return QRect(x, y, widthValue, heightValue);
}

void MainWidget::updateSideNavLayout() {
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

void MainWidget::updateSidebarPlaylistTabs() {
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

void MainWidget::clearSidebarPlaylistButtons() {
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

QString MainWidget::lookupPlaylistCoverCache(const QString& account, qint64 playlistId,
                                             const QString& updatedAt, bool* found) const {
    if (found) {
        *found = false;
    }
    const QString normalizedAccount = account.trimmed();
    const QString normalizedUpdatedAt = updatedAt.trimmed();
    if (normalizedAccount.isEmpty() || playlistId <= 0 || normalizedUpdatedAt.isEmpty()) {
        return QString();
    }

    const QJsonObject index = loadPlaylistCoverCacheIndex();
    const QString key = playlistCoverCacheKey(normalizedAccount, playlistId, normalizedUpdatedAt);
    const QJsonObject entry = index.value(key).toObject();
    if (entry.isEmpty()) {
        return QString();
    }
    if (found) {
        *found = true;
    }

    const QString normalizedSource = normalizeSidebarPlaylistCoverSource(
        entry.value(QStringLiteral("normalizedSource")).toString());
    const QString localFilePath =
        QDir::fromNativeSeparators(entry.value(QStringLiteral("localFilePath")).toString());
    if (!localFilePath.isEmpty() && QFileInfo::exists(localFilePath)) {
        return localFilePath;
    }

    const QString remoteCachePath = cachedCoverFilePathForSource(normalizedSource);
    if (!remoteCachePath.isEmpty() && QFileInfo::exists(remoteCachePath)) {
        return remoteCachePath;
    }

    return collapseDuplicateUploads(entry.value(QStringLiteral("coverUrl")).toString());
}

void MainWidget::storePlaylistCoverCache(qint64 playlistId, const QString& updatedAt,
                                         const QString& coverUrl) {
    const QString account = m_viewModel ? m_viewModel->currentUserAccount().trimmed() : QString();
    const QString normalizedUpdatedAt = updatedAt.trimmed();
    if (account.isEmpty() || playlistId <= 0 || normalizedUpdatedAt.isEmpty()) {
        return;
    }

    const QString normalizedCoverUrl = collapseDuplicateUploads(coverUrl);
    const QString normalizedSource = normalizeSidebarPlaylistCoverSource(normalizedCoverUrl);
    QString localFilePath;
    if (!normalizedSource.isEmpty() && !normalizedSource.startsWith(QStringLiteral(":/")) &&
        !isRemoteCoverSource(normalizedSource) && QFileInfo::exists(normalizedSource)) {
        localFilePath = normalizedSource;
    } else {
        const QString remoteCachePath = cachedCoverFilePathForSource(normalizedSource);
        if (!remoteCachePath.isEmpty() && QFileInfo::exists(remoteCachePath)) {
            localFilePath = remoteCachePath;
        }
    }

    QJsonObject index = loadPlaylistCoverCacheIndex();
    QJsonObject entry;
    entry.insert(QStringLiteral("account"), account);
    entry.insert(QStringLiteral("playlistId"), QString::number(playlistId));
    entry.insert(QStringLiteral("updatedAt"), normalizedUpdatedAt);
    entry.insert(QStringLiteral("coverUrl"), normalizedCoverUrl);
    entry.insert(QStringLiteral("normalizedSource"), normalizedSource);
    entry.insert(QStringLiteral("localFilePath"), localFilePath);
    entry.insert(QStringLiteral("cachedAt"),
                 QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    index.insert(playlistCoverCacheKey(account, playlistId, normalizedUpdatedAt), entry);
    savePlaylistCoverCacheIndex(index);
}

void MainWidget::clearPlaylistCoverCacheForPlaylist(qint64 playlistId) {
    const QString account = m_viewModel ? m_viewModel->currentUserAccount().trimmed() : QString();
    if (account.isEmpty() || playlistId <= 0) {
        return;
    }

    QJsonObject index = loadPlaylistCoverCacheIndex();
    bool changed = false;
    const QStringList keys = index.keys();
    for (const QString& key : keys) {
        const QJsonObject entry = index.value(key).toObject();
        if (entry.value(QStringLiteral("account")).toString() != account ||
            entry.value(QStringLiteral("playlistId")).toString().toLongLong() != playlistId) {
            continue;
        }
        index.remove(key);
        changed = true;
    }
    if (changed) {
        savePlaylistCoverCacheIndex(index);
    }
}

QIcon MainWidget::sidebarPlaylistCoverIcon(const QString& coverUrl) {
    const QString source = normalizeSidebarPlaylistCoverSource(coverUrl);
    if (source.isEmpty()) {
        return defaultSidebarPlaylistCoverIcon();
    }

    if (m_sidebarCoverIconCache.contains(source)) {
        return m_sidebarCoverIconCache.value(source);
    }

    if (source.startsWith(QStringLiteral(":/"))) {
        const QIcon icon(source);
        if (!icon.isNull()) {
            m_sidebarCoverIconCache.insert(source, icon);
            return icon;
        }
        return defaultSidebarPlaylistCoverIcon();
    }

    if (isRemoteCoverSource(source)) {
        const QString cachedFilePath = cachedCoverFilePathForSource(source);
        if (!cachedFilePath.isEmpty() && QFileInfo::exists(cachedFilePath)) {
            const QIcon icon(cachedFilePath);
            if (!icon.isNull()) {
                m_sidebarCoverIconCache.insert(source, icon);
                return icon;
            }
        }
        requestSidebarPlaylistCoverIcon(source);
        return defaultSidebarPlaylistCoverIcon();
    }

    if (QFileInfo::exists(source)) {
        const QIcon icon(source);
        if (!icon.isNull()) {
            m_sidebarCoverIconCache.insert(source, icon);
            return icon;
        }
    }

    return defaultSidebarPlaylistCoverIcon();
}

QString MainWidget::normalizePlaylistCoverUrl(const QString& coverUrl) const {
    return collapseDuplicateUploads(coverUrl);
}

void MainWidget::applySidebarPlaylistButtonIcon(QPushButton* button, const QString& coverUrl) {
    if (!button) {
        return;
    }

    const QString source = normalizeSidebarPlaylistCoverSource(coverUrl);
    button->setProperty("coverUrl", coverUrl);
    button->setProperty("sidebarCoverSource", source);
    button->setIcon(sidebarPlaylistCoverIcon(coverUrl));
    button->setIconSize(QSize(18, 18));
}

void MainWidget::requestSidebarPlaylistCoverIcon(const QString& coverUrl) {
    const QString source = normalizeSidebarPlaylistCoverSource(coverUrl);
    if (!isRemoteCoverSource(source) || m_sidebarCoverIconCache.contains(source) ||
        m_sidebarCoverRequestsInFlight.contains(source)) {
        return;
    }

    if (!m_sidebarCoverNetworkManager) {
        m_sidebarCoverNetworkManager = new QNetworkAccessManager(this);
    }

    m_sidebarCoverRequestsInFlight.insert(source);
    QNetworkRequest request{QUrl(source)};
    QNetworkReply* reply = m_sidebarCoverNetworkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, source]() {
        m_sidebarCoverRequestsInFlight.remove(source);

        const QByteArray payload = reply->readAll();
        QPixmap pixmap;
        const bool loaded =
            reply->error() == QNetworkReply::NoError && pixmap.loadFromData(payload);
        if (loaded) {
            m_sidebarCoverIconCache.insert(source, QIcon(pixmap));
            const QString cacheFilePath = cachedCoverFilePathForSource(source);
            if (!cacheFilePath.isEmpty()) {
                QDir().mkpath(QFileInfo(cacheFilePath).absolutePath());
                QSaveFile file(cacheFilePath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(payload);
                    if (!file.commit()) {
                        qWarning() << "[PlaylistCoverCache] Commit image failed:" << cacheFilePath;
                    }
                }
            }
            for (QPushButton* button : m_sidebarPlaylistButtons) {
                if (!button || button->property("sidebarCoverSource").toString() != source) {
                    continue;
                }
                button->setIcon(m_sidebarCoverIconCache.value(source));
                button->update();
            }
        }

        reply->deleteLater();
    });
}

void MainWidget::refreshSidebarPlaylistCoverIcon(qint64 playlistId, const QString& coverUrl) {
    for (QPushButton* button : m_sidebarPlaylistButtons) {
        if (!button || button->property("playlistId").toLongLong() != playlistId) {
            continue;
        }
        applySidebarPlaylistButtonIcon(button, coverUrl);
        button->update();
    }
}

void MainWidget::rebuildSidebarPlaylistButtons() {
    if (!sidebarPlaylistListLayout || !sidebarPlaylistEmptyLabel) {
        return;
    }

    clearSidebarPlaylistButtons();

    const QVariantList& source = m_sidebarShowingSubscribedPlaylists ? m_subscribedSidebarPlaylists
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

        auto* button =
            new QPushButton(playlistName.isEmpty() ? QStringLiteral("未命名歌单") : playlistName,
                            sidebarPlaylistListContainer);
        button->setObjectName("SidebarPlaylistItemButton");
        button->setProperty("sidePlaylistItem", true);
        button->setProperty("selectedPlaylist", playlistId == m_sidebarSelectedPlaylistId);
        button->setProperty("playlistId", playlistId);
        button->setProperty("trackCount", playlist.value(QStringLiteral("track_count")).toInt());
        applySidebarPlaylistButtonIcon(
            button, playlist.value(QStringLiteral("cover_url")).toString().trimmed());
        button->setToolTip(QStringLiteral("%1\n%2 首")
                               .arg(button->text())
                               .arg(playlist.value(QStringLiteral("track_count")).toInt()));
        connect(button, &QPushButton::clicked, this, &MainWidget::handleSidebarPlaylistItemClicked);
        sidebarPlaylistListLayout->insertWidget(insertIndex++, button);
        m_sidebarPlaylistButtons.append(button);
    }
}

void MainWidget::syncSidebarPlaylistSelection(qint64 playlistId) {
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

void MainWidget::updateAdaptiveLayout() {
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
            w->setMask(
                QRegion(0, qMax(0, height() - playbackHeight), qMax(1, width()), playbackHeight));
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
    if (userProfileWidget) {
        userProfileWidget->setGeometry(contentRect);
    }
    if (recommendMusicWidget) {
        recommendMusicWidget->setGeometry(contentRect);
    }
    if (settingsWidget) {
        settingsWidget->setGeometry(contentRect);
    }
    if (commentPageWidget) {
        commentPageWidget->setGeometry(contentRect);
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
    if (w) {
        w->syncCommentOverlayGeometry();
    }
    if (topWidget) {
        topWidget->raise();
    }
    if (userWidgetQml) {
        userWidgetQml->raise();
    }
}

void MainWidget::updatePaint() {
    main_list->update();
}
void MainWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() & Qt::LeftButton) {
        dragging = true;
        this->pos_ = event->globalPos() - this->geometry().topLeft();
        event->accept();
    }
}
void MainWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton && dragging) {
        QPoint p = event->globalPos();
        this->move(p - this->pos_);
        event->accept();
    }
}
void MainWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() & Qt::LeftButton) {
        dragging = false;
        event->accept();
    }
}

void MainWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateAdaptiveLayout();
    repositionSearchHistoryPopup();
}

void MainWidget::closeEvent(QCloseEvent* event) {
    qDebug() << "[MainWidget] closeEvent: start shutdown";
    if (!m_returningToWelcome && m_viewModel) {
        m_viewModel->shutdownUserSessionOnAppExit(true, 1200);
    }

    if (mainMenu) {
        mainMenu->close();
    }
    hideSearchHistoryPopup();
    if (settingsWidget) {
        settingsWidget->close();
        settingsWidget->deleteLater();
        settingsWidget = nullptr;
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

MainWidget::~MainWidget() {
    qDebug() << "MainWidget::~MainWidget() - Starting cleanup...";

    if (videoListWidget) {
        videoListWidget->pauseVideoPlayback();
    }
    if (videoPlayerWindow) {
        videoPlayerWindow->pausePlayback();
    }
    if (m_viewModel) {
        m_viewModel->shutdownAudioPipeline();
    }

    if (w) {
        qDebug() << "MainWidget: Deleting PlayWidget...";
        delete w;
        w = nullptr;
    }

    if (list) {
        delete list;
        list = nullptr;
    }

    if (loginWidget) {
        loginWidget->close();
        delete loginWidget;
        loginWidget = nullptr;
    }

    if (videoPlayerWindow) {
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
