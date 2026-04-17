#include "main_widget.h"

#include "VideoPlayerWindow.h"
#include "client_automation_host_service.h"
#include "playback_state_manager.h"
#include "plugin_host_window.h"
#include "plugin_manager.h"
#include "search_history_popup.h"
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

// 顶部 AI 入口临时关闭，底层 Agent 能力保留用于内部联调和后续恢复。
constexpr bool kAiAssistantTopEntryEnabled = false;

bool agentPathIsLocal(const QString& rawPath)
{
    const QString trimmed = rawPath.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    const QUrl url = QUrl::fromUserInput(trimmed);
    if (url.isLocalFile()) {
        return true;
    }

    const QFileInfo fileInfo(trimmed);
    return fileInfo.isAbsolute();
}

qint64 agentDurationMsFromText(const QString& durationText)
{
    const QString trimmed = durationText.trimmed();
    if (trimmed.isEmpty()) {
        return 0;
    }

    if (trimmed.contains(':')) {
        const QStringList parts = trimmed.split(':');
        if (parts.size() >= 2) {
            bool okMin = false;
            bool okSec = false;
            const qint64 minutes = parts.at(0).toLongLong(&okMin);
            const qint64 seconds = parts.at(1).toLongLong(&okSec);
            if (okMin && okSec) {
                return qMax<qint64>(0, minutes * 60 + seconds) * 1000;
            }
        }
    }

    bool ok = false;
    const qint64 seconds = trimmed.toLongLong(&ok);
    return ok ? qMax<qint64>(0, seconds) * 1000 : 0;
}

qint64 parsePlaylistTrackId(const QVariantMap& rawItem)
{
    qint64 trackId = rawItem.value(QStringLiteral("track_id")).toLongLong();
    if (trackId > 0) {
        return trackId;
    }

    trackId = rawItem.value(QStringLiteral("music_id")).toLongLong();
    if (trackId > 0) {
        return trackId;
    }

    trackId = rawItem.value(QStringLiteral("id")).toLongLong();
    if (trackId > 0) {
        return trackId;
    }

    return rawItem.value(QStringLiteral("trackId")).toLongLong();
}

qint64 parsePlaylistCollectionId(const QVariantMap& raw)
{
    qint64 playlistId = raw.value(QStringLiteral("playlist_id")).toLongLong();
    if (playlistId > 0) {
        return playlistId;
    }

    playlistId = raw.value(QStringLiteral("playlistId")).toLongLong();
    if (playlistId > 0) {
        return playlistId;
    }

    return raw.value(QStringLiteral("id")).toLongLong();
}

QString fallbackTrackId(const QString& path, const QString& title, const QString& artist)
{
    const QString base = QStringLiteral("%1|%2|%3").arg(path, title, artist);
    const QByteArray digest =
        QCryptographicHash::hash(base.toUtf8(), QCryptographicHash::Md5).toHex();
    return QString::fromLatin1(digest.constData(), digest.size());
}

QIcon defaultSidebarPlaylistCoverIcon()
{
    return QIcon(QStringLiteral(":/qml/assets/ai/icons/default-music-cover.svg"));
}

bool isRemoteCoverSource(const QString& source)
{
    return source.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
           source.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive);
}

bool isWindowsDrivePath(const QString& path)
{
    return path.size() >= 3 && path.at(1) == QLatin1Char(':') &&
           path.at(0).isLetter() &&
           (path.at(2) == QLatin1Char('/') || path.at(2) == QLatin1Char('\\'));
}

QString collapseDuplicateUploads(QString text)
{
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

QString stripUploadsPrefix(QString path)
{
    path = collapseDuplicateUploads(path);
    while (path.startsWith(QLatin1Char('/'))) {
        path.remove(0, 1);
    }
    while (path.startsWith(QStringLiteral("uploads/"), Qt::CaseInsensitive)) {
        path = path.mid(QStringLiteral("uploads/").size());
    }
    return path;
}

QString normalizeSidebarPlaylistCoverSource(const QString& rawCover)
{
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

QString playlistCoverCacheDirectory()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return QDir(base.isEmpty() ? QDir::currentPath() : base)
        .absoluteFilePath(QStringLiteral("playlist_cover_cache"));
}

QString playlistCoverCacheIndexPath()
{
    return QDir(playlistCoverCacheDirectory()).absoluteFilePath(QStringLiteral("index.json"));
}

QString playlistCoverCacheKey(const QString& account, qint64 playlistId, const QString& updatedAt)
{
    const QString rawKey = QStringLiteral("%1|%2|%3").arg(account, QString::number(playlistId), updatedAt);
    const QByteArray digest =
        QCryptographicHash::hash(rawKey.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QString::fromLatin1(digest.constData(), digest.size());
}

QString playlistCoverSourceHash(const QString& source)
{
    const QByteArray digest =
        QCryptographicHash::hash(source.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QString::fromLatin1(digest.constData(), digest.size());
}

QString cachedCoverFilePathForSource(const QString& source)
{
    if (!isRemoteCoverSource(source)) {
        return QString();
    }
    return QDir(playlistCoverCacheDirectory())
        .absoluteFilePath(playlistCoverSourceHash(source) + QStringLiteral(".img"));
}

QJsonObject loadPlaylistCoverCacheIndex()
{
    QFile file(playlistCoverCacheIndexPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    return document.isObject() ? document.object() : QJsonObject{};
}

void savePlaylistCoverCacheIndex(const QJsonObject& index)
{
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

QVariantMap normalizePlaylistEntry(const QVariantMap& raw, const QString& ownership)
{
    const qint64 playlistId = parsePlaylistCollectionId(raw);
    return {
        {QStringLiteral("playlistId"), playlistId},
        {QStringLiteral("name"), raw.value(QStringLiteral("name")).toString().trimmed()},
        {QStringLiteral("description"), raw.value(QStringLiteral("description")).toString().trimmed()},
        {QStringLiteral("coverUrl"), raw.value(QStringLiteral("cover_url")).toString().trimmed()},
        {QStringLiteral("trackCount"), raw.value(QStringLiteral("track_count")).toInt()},
        {QStringLiteral("ownership"), ownership}
    };
}

QVariantMap normalizeTrackItem(QVariantMap item, const QString& sourceType, qint64 playlistId = -1)
{
    QString musicPath = item.value(QStringLiteral("musicPath")).toString().trimmed();
    if (musicPath.isEmpty()) {
        musicPath = item.value(QStringLiteral("path")).toString().trimmed();
    }
    QString playPath = item.value(QStringLiteral("playPath")).toString().trimmed();
    if (playPath.isEmpty()) {
        playPath = musicPath;
    }
    const QString title = item.value(QStringLiteral("title")).toString().trimmed().isEmpty()
        ? QFileInfo(musicPath).completeBaseName()
        : item.value(QStringLiteral("title")).toString().trimmed();
    const QString artist = item.value(QStringLiteral("artist")).toString().trimmed().isEmpty()
        ? QStringLiteral("未知艺术家")
        : item.value(QStringLiteral("artist")).toString().trimmed();
    if (!item.contains(QStringLiteral("trackId")) ||
        item.value(QStringLiteral("trackId")).toString().trimmed().isEmpty()) {
        item.insert(QStringLiteral("trackId"), fallbackTrackId(musicPath, title, artist));
    }
    item.insert(QStringLiteral("musicPath"), musicPath);
    item.insert(QStringLiteral("path"), musicPath);
    item.insert(QStringLiteral("playPath"), playPath);
    item.insert(QStringLiteral("title"), title);
    item.insert(QStringLiteral("artist"), artist);
    item.insert(QStringLiteral("sourceType"), sourceType);
    if (playlistId > 0) {
        item.insert(QStringLiteral("playlistId"), playlistId);
    }
    return item;
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
    m_clientAutomationHostService = new ClientAutomationHostService(this, m_viewModel, this);

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
    searchBox->setFixedHeight(50);
    searchBox->setMinimumWidth(280);
    searchBox->setMaximumWidth(460);
    searchBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // userWidget = new UserWidget(this);

    userWidgetQml = new UserWidgetQml(this);
    userWidgetQml->setFixedHeight(40);
    userWidgetQml->setMinimumWidth(120);
    userWidgetQml->setMaximumWidth(150);

    aiAssistantTopButton = new QPushButton(QStringLiteral(u"AI 助手"), this);
    aiAssistantTopButton->setFixedHeight(36);
    aiAssistantTopButton->setMinimumWidth(116);
    aiAssistantTopButton->setObjectName("TopAgentButton");
    aiAssistantTopButton->setIcon(QIcon(QStringLiteral(":/qml/assets/ai/icons/run.svg")));
    aiAssistantTopButton->setIconSize(QSize(16, 16));
    aiAssistantTopButton->setVisible(kAiAssistantTopEntryEnabled);

    menuButton = new QPushButton(QStringLiteral(u"\u83dc\u5355"), this);
    menuButton->setFixedHeight(42);
    menuButton->setMinimumWidth(84);
    menuButton->setObjectName("MainMenuButton");
    menuButton->setIcon(QIcon(QStringLiteral(":/qml/assets/ai/icons/main-menu.svg")));
    menuButton->setIconSize(QSize(16, 16));

    mainMenu = nullptr;

    QHBoxLayout* widget_op_layout = new QHBoxLayout(topWidget);

    widget_op_layout->addWidget(searchBox);
    if (kAiAssistantTopEntryEnabled) {
        widget_op_layout->addWidget(aiAssistantTopButton);
    }
    widget_op_layout->addStretch();
    widget_op_layout->addWidget(menuButton);
    widget_op_layout->addWidget(maximizeButton);
    widget_op_layout->addWidget(minimizeButton);
    widget_op_layout->addWidget(closeButton);
    widget_op_layout->setSpacing(10);
    widget_op_layout->setContentsMargins(236, 10, 14, 8);

    topWidget->setLayout(widget_op_layout);
    topWidget->setGeometry(0, 0, this->width(), 74);
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
    brandWidget->setObjectName("SidebarBrandWidget");

    QLabel* icolabel = new QLabel(brandWidget);
    icolabel->setObjectName("SidebarBrandIcon");
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
    layout_text->setContentsMargins(10, 0, 10, 0);
    layout_text->setSpacing(10);
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
    if (kAiAssistantTopEntryEnabled) {
        connect(aiAssistantTopButton, &QPushButton::clicked, this,
                &MainWidget::handleAiAssistantClicked);
    }
    connect(&SettingsManager::instance(), &SettingsManager::agentSettingsChanged, this,
            &MainWidget::updateAiAssistantButtonState);
    connect(sidebarOwnedTabButton, &QPushButton::clicked, this,
            &MainWidget::handleSidebarOwnedTabClicked);
    connect(sidebarSubscribedTabButton, &QPushButton::clicked, this,
            &MainWidget::handleSidebarSubscribedTabClicked);
    connect(sidebarPlaylistAddButton, &QPushButton::clicked, this,
            &MainWidget::handleSidebarCreatePlaylistClicked);

    localButton->setChecked(true);
    updateSidebarPlaylistTabs();
    updateAiAssistantButtonState();

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

    if (m_viewModel) {
        m_viewModel->registerPluginHostService(QStringLiteral("mainWidget"), this);
        if (m_clientAutomationHostService) {
            m_viewModel->registerPluginHostService(QStringLiteral("clientAutomationHost"),
                                                   m_clientAutomationHostService);
        }
        m_viewModel->registerPluginHostService(QStringLiteral("httpRequestV2"),
                                               m_viewModel->requestGateway());
        m_viewModel->registerPluginHostService(QStringLiteral("audioService"),
                                               m_viewModel->audioServiceObject());
        if (m_playbackStateManager) {
            m_viewModel->registerPluginHostService(QStringLiteral("playbackStateManager"),
                                                   m_playbackStateManager);
        }
    }

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

QVariantMap MainWidget::agentVideoWindowState() const {
    if (!videoPlayerWindow) {
        return {{QStringLiteral("available"), false}};
    }

    QVariantMap snapshot = videoPlayerWindow->snapshot();
    snapshot.insert(QStringLiteral("available"), true);
    return snapshot;
}

bool MainWidget::agentPlayVideo(const QString& source) {
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

bool MainWidget::agentPauseVideo() {
    if (!videoPlayerWindow) {
        return false;
    }
    videoPlayerWindow->pausePlayback();
    return true;
}

bool MainWidget::agentResumeVideo() {
    return videoPlayerWindow ? videoPlayerWindow->resumePlayback() : false;
}

bool MainWidget::agentSeekVideo(qint64 positionMs) {
    return videoPlayerWindow ? videoPlayerWindow->seekToPosition(positionMs) : false;
}

bool MainWidget::agentSetVideoFullScreen(bool enabled) {
    return videoPlayerWindow ? videoPlayerWindow->setFullScreenEnabled(enabled) : false;
}

bool MainWidget::agentSetVideoPlaybackRate(double rate) {
    return videoPlayerWindow ? videoPlayerWindow->setPlaybackRateValue(rate) : false;
}

bool MainWidget::agentSetVideoQualityPreset(const QString& preset) {
    return videoPlayerWindow ? videoPlayerWindow->setQualityPresetValue(preset) : false;
}

bool MainWidget::agentCloseVideoWindow() {
    if (!videoPlayerWindow) {
        return false;
    }
    videoPlayerWindow->close();
    return true;
}

QVariantMap MainWidget::agentDesktopLyricsState() const {
    return w ? w->desktopLyricSnapshot() : QVariantMap{{QStringLiteral("available"), false}};
}

bool MainWidget::agentSetDesktopLyricsVisible(bool visible) {
    return w ? w->setDesktopLyricVisible(visible) : false;
}

bool MainWidget::agentSetDesktopLyricsStyle(const QVariantMap& style) {
    return w ? w->setDesktopLyricStyleFromMap(style) : false;
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
        rootItem->setProperty(
            "placeholderText",
            m_localOnlyMode ? QStringLiteral(" 离线模式仅支持本地内容")
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

void MainWidget::updateAiAssistantButtonState() {
    if (!aiAssistantTopButton || !kAiAssistantTopEntryEnabled) {
        return;
    }

    const SettingsManager& settings = SettingsManager::instance();
    const bool assistantMode =
        settings.agentMode().trimmed().compare(QStringLiteral("assistant"), Qt::CaseInsensitive) ==
        0;
    const QString localModelName = settings.agentLocalModelName().trimmed();

    aiAssistantTopButton->setText(assistantMode ? QStringLiteral("AI·助手")
                                                : QStringLiteral("AI·控制"));

    QStringList tooltipLines;
    if (assistantMode) {
        tooltipLines << QStringLiteral("当前为助手模式：仅解释，不直接执行写操作。");
        tooltipLines << (settings.agentRemoteFallbackEnabled()
                             ? QStringLiteral("远程兜底已启用。")
                             : QStringLiteral("远程兜底未启用，仅可进行解释性回复。"));
    } else {
        tooltipLines << QStringLiteral("当前为控制模式：优先使用本地模型控制软件。");
        tooltipLines << QStringLiteral("本地模型：%1")
                            .arg(localModelName.isEmpty() ? QStringLiteral("未配置")
                                                          : localModelName);
    }

    if (m_localOnlyMode) {
        tooltipLines
            << QStringLiteral("当前处于离线直进模式，AI 助手仍可用于本地控制与状态解释。");
    }

    aiAssistantTopButton->setToolTip(tooltipLines.join(QLatin1Char('\n')));
}

void MainWidget::applyLocalOnlyModeUi() {
    updateSearchBoxForMode();
    updateAiAssistantButtonState();

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
        userWidgetQml->setUserInfo(QStringLiteral("本地模式"),
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

bool MainWidget::agentOfflineMode() const
{
    return m_localOnlyMode;
}

QString MainWidget::agentCurrentPageKey() const
{
    if (settingsWidget && settingsWidget->isVisible()) {
        return QStringLiteral("settings");
    }
    if (userProfileWidget && userProfileWidget->isVisible()) {
        return QStringLiteral("user_profile");
    }
    if (playlistWidget && playlistWidget->isVisible()) {
        return QStringLiteral("playlists");
    }
    if (favoriteMusicWidget && favoriteMusicWidget->isVisible()) {
        return QStringLiteral("favorites");
    }
    if (playHistoryWidget && playHistoryWidget->isVisible()) {
        return QStringLiteral("history");
    }
    if (net_list && net_list->isVisible()) {
        return QStringLiteral("online_music");
    }
    if (videoListWidget && videoListWidget->isVisible()) {
        return QStringLiteral("video");
    }
    if (recommendMusicWidget && recommendMusicWidget->isVisible()) {
        return QStringLiteral("recommend");
    }
    if (localAndDownloadWidget && localAndDownloadWidget->isVisible()) {
        return QStringLiteral("local_download");
    }
    return QString();
}

QString MainWidget::agentLocalDownloadSubTabKey() const
{
    return localAndDownloadWidget ? localAndDownloadWidget->currentSubTabKey()
                                  : QStringLiteral("local_music");
}

QString MainWidget::agentSidebarPlaylistTabKey() const
{
    if (m_sidebarSelectedPlaylistId > 0) {
        for (const QVariant& value : m_subscribedSidebarPlaylists) {
            if (parsePlaylistCollectionId(value.toMap()) == m_sidebarSelectedPlaylistId) {
                return QStringLiteral("playlist_list_subscribed");
            }
        }
    }
    return QStringLiteral("playlist_list_owned");
}

QString MainWidget::agentCurrentMusicTabKey() const
{
    const QString pageKey = agentCurrentPageKey();
    if (pageKey == QLatin1String("local_download")) {
        return agentLocalDownloadSubTabKey();
    }
    if (pageKey == QLatin1String("playlists")) {
        const QVariantMap detail = playlistWidget ? playlistWidget->currentPlaylistDetailSnapshot()
                                                  : QVariantMap{};
        if (!detail.isEmpty()) {
            return QStringLiteral("playlist_detail");
        }
        return agentSidebarPlaylistTabKey();
    }
    if (pageKey == QLatin1String("recommend") || pageKey == QLatin1String("online_music") ||
        pageKey == QLatin1String("history") || pageKey == QLatin1String("favorites")) {
        return pageKey;
    }
    return QString();
}

QVariantList MainWidget::agentOwnedPlaylistEntries() const
{
    QVariantList items;
    items.reserve(m_ownedSidebarPlaylists.size());
    for (const QVariant& value : m_ownedSidebarPlaylists) {
        items.push_back(normalizePlaylistEntry(value.toMap(), QStringLiteral("owned")));
    }
    return items;
}

QVariantList MainWidget::agentSubscribedPlaylistEntries() const
{
    QVariantList items;
    items.reserve(m_subscribedSidebarPlaylists.size());
    for (const QVariant& value : m_subscribedSidebarPlaylists) {
        items.push_back(normalizePlaylistEntry(value.toMap(), QStringLiteral("subscribed")));
    }
    return items;
}

QVariantMap MainWidget::agentSelectedPlaylistSnapshot() const
{
    QVariantMap snapshot;

    if (playlistWidget) {
        const QVariantMap detail = playlistWidget->currentPlaylistDetailSnapshot();
        const qint64 detailPlaylistId = parsePlaylistCollectionId(detail);
        if (detailPlaylistId > 0 ||
            !detail.value(QStringLiteral("name")).toString().trimmed().isEmpty()) {
            const QVariantList items = detail.value(QStringLiteral("items")).toList();
            snapshot.insert(QStringLiteral("playlistId"), detailPlaylistId);
            snapshot.insert(QStringLiteral("name"),
                            detail.value(QStringLiteral("name")).toString().trimmed());
            snapshot.insert(QStringLiteral("trackCount"),
                            detail.contains(QStringLiteral("track_count"))
                                ? detail.value(QStringLiteral("track_count")).toInt()
                                : items.size());
            snapshot.insert(QStringLiteral("description"),
                            detail.value(QStringLiteral("description")).toString().trimmed());
            snapshot.insert(QStringLiteral("coverUrl"),
                            detail.value(QStringLiteral("cover_url")).toString().trimmed());
            return snapshot;
        }
    }

    const qint64 selectedPlaylistId = m_sidebarSelectedPlaylistId;
    if (selectedPlaylistId <= 0) {
        return snapshot;
    }

    const auto buildSidebarSnapshot = [selectedPlaylistId](const QVariantList& playlists)
        -> QVariantMap {
        for (const QVariant& value : playlists) {
            const QVariantMap raw = value.toMap();
            if (parsePlaylistCollectionId(raw) != selectedPlaylistId) {
                continue;
            }

            QVariantMap item;
            item.insert(QStringLiteral("playlistId"), selectedPlaylistId);
            item.insert(QStringLiteral("name"), raw.value(QStringLiteral("name")).toString().trimmed());
            item.insert(QStringLiteral("trackCount"), raw.value(QStringLiteral("track_count")).toInt());
            item.insert(QStringLiteral("description"),
                        raw.value(QStringLiteral("description")).toString().trimmed());
            item.insert(QStringLiteral("coverUrl"),
                        raw.value(QStringLiteral("cover_url")).toString().trimmed());
            return item;
        }
        return {};
    };

    snapshot = buildSidebarSnapshot(m_ownedSidebarPlaylists);
    if (!snapshot.isEmpty()) {
        return snapshot;
    }
    return buildSidebarSnapshot(m_subscribedSidebarPlaylists);
}

QVariantList MainWidget::agentSelectedTrackIdsSnapshot() const
{
    if (!playlistWidget || !playlistWidget->isVisible()) {
        return {};
    }

    QVariantList normalized;
    const QVariantList rawTrackIds = playlistWidget->currentPlaylistTrackIds();
    normalized.reserve(rawTrackIds.size());
    for (const QVariant& rawId : rawTrackIds) {
        const QString text = rawId.toString().trimmed();
        if (!text.isEmpty()) {
            normalized.push_back(text);
        }
    }
    return normalized;
}

QVariantMap MainWidget::agentCurrentTrackSnapshot() const
{
    QVariantMap track;
    if (!w || !w->playbackViewModel()) {
        track.insert(QStringLiteral("playing"), false);
        return track;
    }

    PlaybackViewModel* playbackVm = w->playbackViewModel();
    const QString filePath = playbackVm->currentFilePath().trimmed();
    if (filePath.isEmpty()) {
        track.insert(QStringLiteral("playing"), false);
        return track;
    }

    const QString title = playbackVm->currentTitle().trimmed().isEmpty()
        ? QFileInfo(filePath).completeBaseName()
        : playbackVm->currentTitle().trimmed();
    const QString artist = playbackVm->currentArtist().trimmed().isEmpty()
        ? QStringLiteral("未知艺术家")
        : playbackVm->currentArtist().trimmed();

    track.insert(QStringLiteral("musicPath"), filePath);
    track.insert(QStringLiteral("title"), title);
    track.insert(QStringLiteral("artist"), artist);
    track.insert(QStringLiteral("coverUrl"), playbackVm->currentAlbumArt().trimmed());
    track.insert(QStringLiteral("durationMs"), qMax<qint64>(0, playbackVm->duration()));
    track.insert(QStringLiteral("positionMs"), qMax<qint64>(0, playbackVm->position()));
    track.insert(QStringLiteral("playing"), playbackVm->isPlaying());
    track.insert(QStringLiteral("paused"), playbackVm->isPaused());
    track.insert(QStringLiteral("isLocal"), agentPathIsLocal(filePath));
    return track;
}

QVariantMap MainWidget::agentQueueSnapshot() const
{
    QVariantMap snapshot;
    QVariantList items;

    if (w && w->playbackViewModel()) {
        PlaybackViewModel* playbackVm = w->playbackViewModel();
        const QVariantList rawItems = playbackVm->playlistSnapshot();
        const QVariantMap currentTrack = agentCurrentTrackSnapshot();
        const QString currentPath = currentTrack.value(QStringLiteral("musicPath")).toString();

        items.reserve(rawItems.size());
        for (int index = 0; index < rawItems.size(); ++index) {
            const QVariantMap raw = rawItems.at(index).toMap();
            const QString filePath = raw.value(QStringLiteral("filePath")).toString().trimmed();

            QVariantMap item;
            item.insert(QStringLiteral("musicPath"), filePath);
            item.insert(QStringLiteral("title"), raw.value(QStringLiteral("title")).toString().trimmed());
            item.insert(QStringLiteral("artist"), raw.value(QStringLiteral("artist")).toString().trimmed());
            item.insert(QStringLiteral("coverUrl"), raw.value(QStringLiteral("cover")).toString().trimmed());
            item.insert(QStringLiteral("isCurrent"), raw.value(QStringLiteral("isCurrent")).toBool());
            item.insert(QStringLiteral("isLocal"), agentPathIsLocal(filePath));
            item.insert(QStringLiteral("index"), index);

            qint64 durationMs = raw.value(QStringLiteral("durationMs")).toLongLong();
            if (durationMs <= 0 && raw.value(QStringLiteral("duration")).isValid()) {
                durationMs = agentDurationMsFromText(raw.value(QStringLiteral("duration")).toString());
            }
            if (durationMs <= 0 && filePath == currentPath) {
                durationMs = currentTrack.value(QStringLiteral("durationMs")).toLongLong();
            }
            if (durationMs <= 0 && item.value(QStringLiteral("isLocal")).toBool() && m_viewModel) {
                durationMs =
                    static_cast<qint64>(m_viewModel->localMusicCacheDurationSeconds(filePath)) * 1000;
            }
            item.insert(QStringLiteral("durationMs"), qMax<qint64>(0, durationMs));
            items.push_back(item);
        }

        snapshot.insert(QStringLiteral("items"), items);
        snapshot.insert(QStringLiteral("count"), items.size());
        snapshot.insert(QStringLiteral("currentIndex"), playbackVm->currentIndex());
        snapshot.insert(QStringLiteral("playing"), playbackVm->isPlaying());
        snapshot.insert(QStringLiteral("paused"), playbackVm->isPaused());
        snapshot.insert(QStringLiteral("volume"), playbackVm->volume());
        snapshot.insert(QStringLiteral("playMode"), playbackVm->playModeValue());
        return snapshot;
    }

    snapshot.insert(QStringLiteral("items"), items);
    snapshot.insert(QStringLiteral("count"), 0);
    snapshot.insert(QStringLiteral("currentIndex"), -1);
    snapshot.insert(QStringLiteral("playing"), false);
    snapshot.insert(QStringLiteral("paused"), false);
    snapshot.insert(QStringLiteral("volume"), 50);
    snapshot.insert(QStringLiteral("playMode"), 0);
    return snapshot;
}

QVariantMap MainWidget::agentUiOverviewSnapshot() const
{
    QVariantMap snapshot;
    snapshot.insert(QStringLiteral("currentPage"), agentCurrentPageKey());
    snapshot.insert(QStringLiteral("offlineMode"), agentOfflineMode());
    snapshot.insert(QStringLiteral("loggedIn"), isUserLoggedIn());
    snapshot.insert(QStringLiteral("currentMusicTab"), agentCurrentMusicTabKey());
    snapshot.insert(QStringLiteral("localDownloadSubTab"), agentLocalDownloadSubTabKey());
    snapshot.insert(QStringLiteral("sidebarPlaylistTab"), agentSidebarPlaylistTabKey());
    snapshot.insert(QStringLiteral("selectedPlaylist"), agentSelectedPlaylistSnapshot());
    snapshot.insert(QStringLiteral("selectedTrackIds"), agentSelectedTrackIdsSnapshot());
    snapshot.insert(QStringLiteral("currentTrack"), agentCurrentTrackSnapshot());
    snapshot.insert(QStringLiteral("queueSummary"), agentQueueSnapshot());
    return snapshot;
}

QVariantMap MainWidget::agentUiPageStateSnapshot(const QString& pageKey) const
{
    const QString page = pageKey.trimmed().toLower();
    QVariantMap state{
        {QStringLiteral("page"), page},
        {QStringLiteral("currentPage"), agentCurrentPageKey()},
        {QStringLiteral("visible"), page == agentCurrentPageKey()}
    };

    if (page == QLatin1String("recommend")) {
        const QVariantList items =
            recommendMusicWidget ? recommendMusicWidget->recommendationItemsSnapshot(50)
                                 : QVariantList{};
        state.insert(QStringLiteral("items"), items);
        state.insert(QStringLiteral("count"), items.size());
        state.insert(QStringLiteral("meta"),
                     recommendMusicWidget ? recommendMusicWidget->recommendationMetaSnapshot()
                                          : QVariantMap{});
        return state;
    }
    if (page == QLatin1String("local_download")) {
        state.insert(QStringLiteral("currentMusicTab"), agentCurrentMusicTabKey());
        state.insert(QStringLiteral("localDownloadSubTab"), agentLocalDownloadSubTabKey());
        state.insert(QStringLiteral("localMusicCount"),
                     localAndDownloadWidget
                         ? localAndDownloadWidget->localMusicItemsSnapshot().size()
                         : 0);
        state.insert(QStringLiteral("downloadedMusicCount"),
                     localAndDownloadWidget
                         ? localAndDownloadWidget->downloadedMusicItemsSnapshot().size()
                         : 0);
        state.insert(QStringLiteral("downloadingTaskCount"),
                     localAndDownloadWidget
                         ? localAndDownloadWidget->downloadingTaskItemsSnapshot().size()
                         : 0);
        return state;
    }
    if (page == QLatin1String("online_music")) {
        const QVariantList items = net_list ? net_list->currentItemsSnapshot(100) : QVariantList{};
        state.insert(QStringLiteral("items"), items);
        state.insert(QStringLiteral("count"), items.size());
        return state;
    }
    if (page == QLatin1String("history")) {
        const QVariantList items =
            playHistoryWidget ? playHistoryWidget->historyItemsSnapshot(100) : QVariantList{};
        state.insert(QStringLiteral("items"), items);
        state.insert(QStringLiteral("count"), items.size());
        return state;
    }
    if (page == QLatin1String("favorites")) {
        const QVariantList items =
            favoriteMusicWidget ? favoriteMusicWidget->favoriteItemsSnapshot(100) : QVariantList{};
        state.insert(QStringLiteral("items"), items);
        state.insert(QStringLiteral("count"), items.size());
        return state;
    }
    if (page == QLatin1String("playlists")) {
        const QVariantList owned = agentOwnedPlaylistEntries();
        const QVariantList subscribed = agentSubscribedPlaylistEntries();
        const QVariantMap selected = agentSelectedPlaylistSnapshot();
        const QVariantMap detail =
            playlistWidget ? playlistWidget->currentPlaylistDetailSnapshot() : QVariantMap{};
        state.insert(QStringLiteral("sidebarPlaylistTab"), agentSidebarPlaylistTabKey());
        state.insert(QStringLiteral("selectedPlaylist"), selected);
        state.insert(QStringLiteral("ownedPlaylists"), owned);
        state.insert(QStringLiteral("subscribedPlaylists"), subscribed);
        state.insert(QStringLiteral("ownedCount"), owned.size());
        state.insert(QStringLiteral("subscribedCount"), subscribed.size());
        state.insert(QStringLiteral("detailCount"),
                     detail.value(QStringLiteral("items")).toList().size());
        return state;
    }
    if (page == QLatin1String("user_profile")) {
        state.insert(QStringLiteral("profile"), agentUserProfileSnapshot());
        return state;
    }
    if (page == QLatin1String("video")) {
        state.insert(QStringLiteral("videoWindow"), agentVideoWindowState());
        return state;
    }
    if (page == QLatin1String("settings")) {
        SettingsManager& settings = SettingsManager::instance();
        state.insert(QStringLiteral("agentMode"), settings.agentMode());
        state.insert(QStringLiteral("localModelPath"), settings.agentLocalModelPath());
        state.insert(QStringLiteral("localModelName"), settings.agentLocalModelName());
        state.insert(QStringLiteral("localContextSize"), settings.agentLocalContextSize());
        state.insert(QStringLiteral("localThreadCount"), settings.agentLocalThreadCount());
        state.insert(QStringLiteral("remoteFallbackEnabled"),
                     settings.agentRemoteFallbackEnabled());
        return state;
    }

    return state;
}

QVariantList MainWidget::agentMusicTabItemsSnapshot(const QString& tabKey,
                                                    const QVariantMap& options) const
{
    const QString tab = tabKey.trimmed().toLower();
    const int limit = qMax(1, options.value(QStringLiteral("limit"), 200).toInt());

    if (tab == QLatin1String("recommend")) {
        QVariantList items =
            recommendMusicWidget ? recommendMusicWidget->recommendationItemsSnapshot(limit)
                                 : QVariantList{};
        for (QVariant& value : items) {
            value = normalizeTrackItem(value.toMap(), QStringLiteral("recommend"));
        }
        return items;
    }
    if (tab == QLatin1String("local_music")) {
        return localAndDownloadWidget ? localAndDownloadWidget->localMusicItemsSnapshot(limit)
                                      : QVariantList{};
    }
    if (tab == QLatin1String("downloaded_music")) {
        return localAndDownloadWidget
            ? localAndDownloadWidget->downloadedMusicItemsSnapshot(limit)
            : QVariantList{};
    }
    if (tab == QLatin1String("downloading_tasks")) {
        return localAndDownloadWidget
            ? localAndDownloadWidget->downloadingTaskItemsSnapshot(limit)
            : QVariantList{};
    }
    if (tab == QLatin1String("online_music")) {
        return net_list ? net_list->currentItemsSnapshot(limit) : QVariantList{};
    }
    if (tab == QLatin1String("history")) {
        QVariantList items =
            playHistoryWidget ? playHistoryWidget->historyItemsSnapshot(limit) : QVariantList{};
        for (QVariant& value : items) {
            value = normalizeTrackItem(value.toMap(), QStringLiteral("history"));
        }
        return items;
    }
    if (tab == QLatin1String("favorites")) {
        QVariantList items =
            favoriteMusicWidget ? favoriteMusicWidget->favoriteItemsSnapshot(limit) : QVariantList{};
        for (QVariant& value : items) {
            value = normalizeTrackItem(value.toMap(), QStringLiteral("favorite"));
        }
        return items;
    }
    if (tab == QLatin1String("playlist_list_owned")) {
        return agentOwnedPlaylistEntries();
    }
    if (tab == QLatin1String("playlist_list_subscribed")) {
        return agentSubscribedPlaylistEntries();
    }
    if (tab == QLatin1String("playlist_detail")) {
        const QVariantMap detail =
            playlistWidget ? playlistWidget->currentPlaylistDetailSnapshot() : QVariantMap{};
        const qint64 playlistId = parsePlaylistCollectionId(detail);
        QVariantList items = detail.value(QStringLiteral("items")).toList();
        QVariantList normalized;
        const int bounded = qMin(limit, items.size());
        normalized.reserve(bounded);
        for (int i = 0; i < bounded; ++i) {
            normalized.push_back(
                normalizeTrackItem(items.at(i).toMap(), QStringLiteral("playlist"), playlistId));
        }
        return normalized;
    }

    return {};
}

QVariantMap MainWidget::agentResolveMusicTabItem(const QString& tabKey,
                                                 const QVariantMap& selector) const
{
    const QString tab = tabKey.trimmed().toLower();
    QVariantMap options;
    if (selector.contains(QStringLiteral("playlistId"))) {
        options.insert(QStringLiteral("playlistId"), selector.value(QStringLiteral("playlistId")));
    }
    if (selector.contains(QStringLiteral("playlistName"))) {
        options.insert(QStringLiteral("playlistName"), selector.value(QStringLiteral("playlistName")));
    }

    const QVariantList items = agentMusicTabItemsSnapshot(tab, options);
    const QString trackId = selector.value(QStringLiteral("trackId")).toString().trimmed();
    const QString musicPath = selector.value(QStringLiteral("musicPath")).toString().trimmed();
    const qint64 playlistId = selector.value(QStringLiteral("playlistId")).toLongLong();
    const QString playlistName = selector.value(QStringLiteral("playlistName")).toString().trimmed();

    for (const QVariant& value : items) {
        const QVariantMap item = value.toMap();
        if (!trackId.isEmpty() &&
            item.value(QStringLiteral("trackId")).toString().trimmed() == trackId) {
            return item;
        }
        if (!musicPath.isEmpty() &&
            item.value(QStringLiteral("musicPath")).toString().trimmed() == musicPath) {
            return item;
        }
        if ((tab == QLatin1String("playlist_list_owned") ||
             tab == QLatin1String("playlist_list_subscribed")) &&
            playlistId > 0 &&
            item.value(QStringLiteral("playlistId")).toLongLong() == playlistId) {
            return item;
        }
        if ((tab == QLatin1String("playlist_list_owned") ||
             tab == QLatin1String("playlist_list_subscribed")) &&
            !playlistName.isEmpty() &&
            item.value(QStringLiteral("name")).toString().trimmed().compare(
                playlistName, Qt::CaseInsensitive) == 0) {
            return item;
        }
    }
    return {};
}

QVariantMap MainWidget::agentInvokeSongAction(const QString& action,
                                              const QVariantMap& songData)
{
    QVariantMap payload = songData;
    const QString trimmedAction = action.trimmed();
    if (trimmedAction.isEmpty() || payload.isEmpty()) {
        return {{QStringLiteral("accepted"), false},
                {QStringLiteral("errorCode"), QStringLiteral("invalid_args")}};
    }

    if (trimmedAction == QLatin1String("add_to_playlist")) {
        qint64 playlistId = payload.value(QStringLiteral("playlistId")).toLongLong();
        if (playlistId <= 0) {
            const QString playlistName =
                payload.value(QStringLiteral("playlistName")).toString().trimmed();
            if (!playlistName.isEmpty()) {
                for (const QVariant& value : m_ownedSidebarPlaylists) {
                    const QVariantMap item = value.toMap();
                    if (item.value(QStringLiteral("name")).toString().trimmed().compare(
                            playlistName, Qt::CaseInsensitive) == 0) {
                        playlistId = parsePlaylistCollectionId(item);
                        break;
                    }
                }
            }
        }
        if (playlistId <= 0) {
            return {{QStringLiteral("accepted"), false},
                    {QStringLiteral("errorCode"), QStringLiteral("playlist_context_required")}};
        }
        payload.insert(QStringLiteral("playlistId"), playlistId);
    }

    handleSongActionRequested(trimmedAction, payload);
    return {
        {QStringLiteral("accepted"), true},
        {QStringLiteral("action"), trimmedAction},
        {QStringLiteral("item"), payload}
    };
}

QVariantMap MainWidget::agentUserProfileSnapshot() const
{
    QVariantMap profile = cachedUserProfileSnapshot();
    profile.insert(QStringLiteral("loggedIn"), isUserLoggedIn());
    profile.insert(QStringLiteral("offlineMode"), m_localOnlyMode);
    profile.insert(QStringLiteral("favoritesCount"), m_profileFavoritesCount);
    profile.insert(QStringLiteral("historyCount"), m_profileHistoryCount);
    profile.insert(QStringLiteral("ownedPlaylistsCount"), m_profileOwnedPlaylistsCount);
    return profile;
}

bool MainWidget::agentRefreshUserProfile()
{
    if (m_localOnlyMode || !m_viewModel || !isUserLoggedIn()) {
        return false;
    }
    m_viewModel->requestUserProfile();
    refreshUserProfileStats();
    return true;
}

bool MainWidget::agentUpdateUsername(const QString& username)
{
    if (m_localOnlyMode || !m_viewModel || !isUserLoggedIn()) {
        return false;
    }

    const QString trimmed = username.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    m_viewModel->updateCurrentUsername(trimmed);
    return true;
}

bool MainWidget::agentUploadAvatar(const QString& filePath)
{
    if (m_localOnlyMode || !m_viewModel || !isUserLoggedIn()) {
        return false;
    }

    const QString trimmed = filePath.trimmed();
    if (trimmed.isEmpty() || !QFileInfo::exists(trimmed)) {
        return false;
    }
    m_viewModel->uploadCurrentAvatar(trimmed);
    return true;
}

bool MainWidget::agentLogoutUser()
{
    if (m_localOnlyMode || !m_viewModel || !isUserLoggedIn()) {
        return false;
    }
    handleUserLogoutRequested();
    return true;
}

bool MainWidget::agentReturnToWelcome()
{
    handleSettingsReturnToWelcomeRequested();
    return true;
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
    button->setGeometry(10, navStartY + row * itemHeight, panelWidth - 20, itemHeight);
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
    const int topBarHeight = 74;
    const int leftEdge = leftWidget ? (leftWidget->geometry().right() + 18) : 228;
    const int bottomReserved = (w && !w->isUp) ? (w->collapsedPlaybackHeight() + 16) : 104;

    const int x = leftEdge;
    const int y = topBarHeight + 12;
    const int widthValue = qMax(300, width() - x - 16);
    const int heightValue = qMax(180, height() - y - bottomReserved);
    return QRect(x, y, widthValue, heightValue);
}

void MainWidget::updateSideNavLayout() {
    if (!leftWidget || !brandWidget) {
        return;
    }

    const int panelWidth = leftWidget->width();
    const int itemHeight = 48;
    const int brandTop = 14;
    const int brandHeight = 52;
    brandWidget->setGeometry(8, brandTop, panelWidth - 16, brandHeight);

    const int navStartY = brandTop + brandHeight + 18;
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
                                             const QString& updatedAt, bool* found) const
{
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

    const QString normalizedSource =
        normalizeSidebarPlaylistCoverSource(entry.value(QStringLiteral("normalizedSource")).toString());
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
                                         const QString& coverUrl)
{
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

void MainWidget::clearPlaylistCoverCacheForPlaylist(qint64 playlistId)
{
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

QIcon MainWidget::sidebarPlaylistCoverIcon(const QString& coverUrl)
{
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

QString MainWidget::normalizePlaylistCoverUrl(const QString& coverUrl) const
{
    return collapseDuplicateUploads(coverUrl);
}

void MainWidget::applySidebarPlaylistButtonIcon(QPushButton* button, const QString& coverUrl)
{
    if (!button) {
        return;
    }

    const QString source = normalizeSidebarPlaylistCoverSource(coverUrl);
    button->setProperty("coverUrl", coverUrl);
    button->setProperty("sidebarCoverSource", source);
    button->setIcon(sidebarPlaylistCoverIcon(coverUrl));
    button->setIconSize(QSize(18, 18));
}

void MainWidget::requestSidebarPlaylistCoverIcon(const QString& coverUrl)
{
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

void MainWidget::refreshSidebarPlaylistCoverIcon(qint64 playlistId, const QString& coverUrl)
{
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
    const int topBarHeight = 74;
    const int leftPanelMargin = 14;
    const int leftPanelTop = topBarHeight + 10;
    const int leftWidth = qBound(208, width() / 5, 260);

    if (topWidget) {
        topWidget->setGeometry(0, 0, width(), topBarHeight);
        topWidget->raise();
        if (auto* layout = qobject_cast<QHBoxLayout*>(topWidget->layout())) {
            layout->setContentsMargins(leftPanelMargin + leftWidth + 18, 10, 14, 8);
        }
    }

    if (userWidgetQml) {
        const int avatarWidth = qMin(160, qMax(128, leftWidth - 8));
        userWidgetQml->setGeometry(leftPanelMargin, 16, avatarWidth, 42);
        userWidgetQml->raise();
    }

    if (searchBox) {
        const int searchWidth = qBound(280, width() / 4, 500);
        searchBox->setMinimumWidth(searchWidth);
        searchBox->setMaximumWidth(searchWidth);
    }

    if (leftWidget) {
        leftWidget->setGeometry(leftPanelMargin, leftPanelTop, leftWidth,
                                qMax(200, height() - leftPanelTop - 12));
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
