#include "main_widget.h"
#include "plugin_manager.h"
#include "searchbox_qml.h"
#include "AudioService.h"
#include "AudioPlayer.h"
#include "VideoPlayerWindow.h"
#include "online_presence_manager.h"
#include "playback_state_manager.h"
#include "user.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QIcon>
#include <QTimer>

namespace {
int parseDurationToSeconds(const QString& durationText)
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
            const int minutes = parts[0].toInt(&okMin);
            const int seconds = parts[1].toInt(&okSec);
            if (okMin && okSec) {
                return qMax(0, minutes * 60 + seconds);
            }
        }
    }

    bool ok = false;
    const int seconds = trimmed.toInt(&ok);
    return ok ? qMax(0, seconds) : 0;
}

int durationSecondsFromLocalCache(const QString& filePath)
{
    for (const LocalMusicInfo& info : LocalMusicCache::instance().getMusicList()) {
        if (info.filePath == filePath) {
            return parseDurationToSeconds(info.duration);
        }
    }
    return 0;
}

QString normalizeArtistForHistory(const QString& artist)
{
    const QString trimmed = artist.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    const QString lower = trimmed.toLower();
    if (trimmed == QStringLiteral("未知艺术家") ||
        trimmed == QStringLiteral("未知歌手") ||
        lower == QStringLiteral("unknown artist") ||
        lower == QStringLiteral("unknown") ||
        lower == QStringLiteral("<unknown>")) {
        return QString();
    }
    return trimmed;
}
}

MainWidget::MainWidget(QWidget *parent) : QWidget(parent)
  ,w(nullptr)
  ,list(nullptr)
  ,videoPlayerWindow(nullptr)
  ,videoListWidget(nullptr)
  ,settingsWidget(nullptr)
{
    resize(1000,600);
    setWindowFlags(Qt::CustomizeWindowHint);
    setAttribute(Qt::WA_QuitOnClose, true);
    setWindowTitle(QStringLiteral(u"\u4e91\u97f3\u4e50"));
    setWindowIcon(QIcon("qrc:/new/prefix1/icon/netease.ico"));
    
    setObjectName("MainWidget");

    m_playbackStateManager = new PlaybackStateManager(this);
    connect(m_playbackStateManager, &PlaybackStateManager::pauseAudioRequested, this, []() {
        if (AudioService::instance().isPlaying()) {
            AudioService::instance().pause();
        }
    });
    connect(m_playbackStateManager, &PlaybackStateManager::pauseVideoRequested, this, [this]() {
        if (videoListWidget) {
            videoListWidget->pauseVideoPlayback();
        } else if (videoPlayerWindow) {
            videoPlayerWindow->pausePlayback();
        }
    });
    
    PluginManager& pluginManager = PluginManager::instance();
    QString pluginPath = QCoreApplication::applicationDirPath() + "/plugin";
    int loadedCount = pluginManager.loadPlugins(pluginPath);
    qDebug() << "Loaded" << loadedCount << "plugins from" << pluginPath;

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

    connect(minimizeButton, &QPushButton::clicked, this, [=](){
        if(isMaximized())
            showNormal();
        else
            showMaximized();
    });
    connect(maximizeButton, &QPushButton::clicked, this, &MainWidget::showMinimized);
    connect(closeButton, &QPushButton::clicked, this, &MainWidget::close);

    SearchBoxQml* searchBox = new SearchBoxQml(this);
    searchBox->setFixedSize(250, 60);

    // userWidget = new UserWidget(this);
    
    userWidgetQml = new UserWidgetQml(this);
    userWidgetQml->setFixedSize(150, 40);
    
    menuButton = new QPushButton(QStringLiteral(u"\u83dc\u5355"), this);
    menuButton->setFixedSize(50, 50);
    menuButton->setObjectName("MainMenuButton");
    
    mainMenu = nullptr;

    QHBoxLayout* widget_op_layout = new QHBoxLayout(this);

    widget_op_layout->addSpacing(210);
    
    widget_op_layout->addWidget(searchBox);
    widget_op_layout->addStretch();
    widget_op_layout->addWidget(userWidgetQml);
    widget_op_layout->addWidget(menuButton);
    widget_op_layout->addWidget(maximizeButton);
    widget_op_layout->addWidget(minimizeButton);
    widget_op_layout->addWidget(closeButton);
    widget_op_layout->setSpacing(10);
    widget_op_layout->setContentsMargins(10, 5, 10, 5);


    topWidget->setLayout(widget_op_layout);
    topWidget->setGeometry(0, 0, this->width(), 60);
    topWidget->raise();

    loginWidget = new LoginWidgetQml(this);
    loginWidget->setWindowTitle(QStringLiteral(u"\u767b\u5f55"));
    loginWidget->setSavedAccount(SettingsManager::instance().cachedAccount(),
                                 SettingsManager::instance().cachedPassword(),
                                 SettingsManager::instance().cachedUsername());

    main_list = new MusicListWidgetLocal(this);
    main_list->setFixedSize(800,400);
    main_list->move((this->width()-800),this->height()- 500);
    main_list->show();
    main_list->setObjectName("local");

    localAndDownloadWidget = new LocalAndDownloadWidget(this);
    localAndDownloadWidget->setFixedSize(800, 400);
    localAndDownloadWidget->move((this->width()-800), this->height()- 500);
    localAndDownloadWidget->hide();
    localAndDownloadWidget->setObjectName("localAndDownload");


    list = new MusicListWidget(this);
    list->setFixedSize(200,300);
    list->move(this->width() - 200, 60);
    list->clear();
    list->close();

    net_list = new MusicListWidgetNet(this);
    net_list->setMainWidget(this);
    net_list->setFixedSize(800, 400);
    net_list->move(main_list->pos());
    net_list->hide();
    net_list->setObjectName("net");
    
    playHistoryWidget = new PlayHistoryWidget(this);
    playHistoryWidget->setFixedSize(800, 400);
    playHistoryWidget->move(main_list->pos());
    playHistoryWidget->hide();
    playHistoryWidget->setObjectName("playHistory");
    
    favoriteMusicWidget = new FavoriteMusicWidget(this);
    favoriteMusicWidget->setFixedSize(800, 400);
    favoriteMusicWidget->move(main_list->pos());
    favoriteMusicWidget->hide();
    favoriteMusicWidget->setObjectName("favoriteMusic");

    videoListWidget = nullptr;


    QWidget* leftWidget = new QWidget(this);
    leftWidget->setFixedSize(200, this->height() - 100);
    leftWidget->setObjectName("MainLeftPanel");

    QPushButton* localList = new QPushButton(QStringLiteral(u"\u672c\u5730\u4e0e\u4e0b\u8f7d"), leftWidget);
    localList->setFixedSize(200,50);
    localList->move(0,this->height()- 500);
    localList->setCheckable(true);
    localList->setObjectName("SideNavButton");
    localList->setProperty("sideNav", true);

    QPushButton* NetList = new QPushButton(QStringLiteral(u"\u5728\u7ebf\u97f3\u4e50"), leftWidget);
    NetList->setFixedSize(200,50);
    NetList->move(0,this->height()- 450);
    NetList->setCheckable(true);
    NetList->setObjectName("SideNavButton");
    NetList->setProperty("sideNav", true);

    QPushButton* PlayHistoryBtn = new QPushButton(QStringLiteral(u"\u6700\u8fd1\u64ad\u653e"), leftWidget);
    PlayHistoryBtn->setFixedSize(200, 50);
    PlayHistoryBtn->move(0, this->height() - 400);
    PlayHistoryBtn->setCheckable(true);
    PlayHistoryBtn->setObjectName("SideNavButton");
    PlayHistoryBtn->setProperty("sideNav", true);
    
    QPushButton* FavoriteMusicBtn = new QPushButton(QStringLiteral(u"\u6211\u559c\u6b22\u7684\u97f3\u4e50"), leftWidget);
    FavoriteMusicBtn->setFixedSize(200, 50);
    FavoriteMusicBtn->move(0, this->height() - 350);
    FavoriteMusicBtn->setCheckable(true);
    FavoriteMusicBtn->setObjectName("FavoriteMusicBtn");
    FavoriteMusicBtn->setVisible(false);
    FavoriteMusicBtn->setProperty("sideNav", true);
    
    QPushButton* VideoPlayerBtn = new QPushButton(QStringLiteral(u"\u89c6\u9891\u64ad\u653e"), leftWidget);
    VideoPlayerBtn->setFixedSize(200, 50);
    VideoPlayerBtn->setObjectName("VideoPlayerBtn");
    VideoPlayerBtn->move(0, this->height() - 350);
    VideoPlayerBtn->setCheckable(true);
    VideoPlayerBtn->setProperty("sideNav", true);
    
    QButtonGroup* leftButtons = new QButtonGroup(this);
    leftButtons->addButton(localList);
    leftButtons->addButton(NetList);
    leftButtons->addButton(PlayHistoryBtn);
    leftButtons->addButton(FavoriteMusicBtn);
    leftButtons->addButton(VideoPlayerBtn);
    leftButtons->setExclusive(true);

    QWidget* textWidget = new QWidget(leftWidget);

    QLabel* icolabel = new QLabel(textWidget);
    icolabel->setPixmap(QPixmap(":/new/prefix1/icon/netease.ico"));
    icolabel->setScaledContents(true);
    icolabel->setFixedSize(40, 40);

    QLabel *textLabel = new QLabel(QStringLiteral(u"\u4e91\u97f3\u4e50"), textWidget);
    QFont font;
    font.setFamily("Microsoft YaHei");
    font.setPointSize(16);
    font.setBold(true);
    textLabel->setFont(font);
    textLabel->setObjectName("BrandTitleLabel");
    textLabel->adjustSize();

    QHBoxLayout* layout_text = new QHBoxLayout(textWidget);
    layout_text->addWidget(icolabel);
    layout_text->addWidget(textLabel);

    textWidget->setLayout(layout_text);
    textWidget->move(5, 10);

    connect(localList, &QPushButton::toggled, this, [=](bool checked) {
        if (checked) {
            main_list->hide();
            localAndDownloadWidget->show();
            net_list->hide();
            playHistoryWidget->hide();
            favoriteMusicWidget->hide();
            if (videoListWidget) videoListWidget->hide();
        }
    });

    connect(NetList, &QPushButton::toggled, this, [=](bool checked){
        if(checked)
        {
            net_list->show();
            main_list->hide();
            localAndDownloadWidget->hide();
            playHistoryWidget->hide();
            favoriteMusicWidget->hide();
            if (videoListWidget) videoListWidget->hide();
        }
    });
    
    connect(PlayHistoryBtn, &QPushButton::toggled, this, [=](bool checked){
        if(checked)
        {
            playHistoryWidget->show();
            main_list->hide();
            localAndDownloadWidget->hide();
            net_list->hide();
            favoriteMusicWidget->hide();
            if (videoListWidget) videoListWidget->hide();
            
            if (isUserLoggedIn()) {
                QString userAccount = User::getInstance()->get_account();
                request->getPlayHistory(userAccount, 50);
            }
        }
    });
    
    connect(FavoriteMusicBtn, &QPushButton::toggled, this, [=](bool checked){
        if(checked)
        {
            favoriteMusicWidget->show();
            main_list->hide();
            localAndDownloadWidget->hide();
            net_list->hide();
            playHistoryWidget->hide();
            if (videoListWidget) videoListWidget->hide();
            
            QString userAccount = User::getInstance()->get_account();
            request->getFavorites(userAccount);
        }
    });

    connect(VideoPlayerBtn, &QPushButton::toggled, this, [=](bool checked) {
        if (checked) {
            qDebug() << "[MainWidget] Showing online video list";

            main_list->hide();
            localAndDownloadWidget->hide();
            net_list->hide();
            playHistoryWidget->hide();
            favoriteMusicWidget->hide();
            if (videoListWidget) {
                videoListWidget->show();
                videoListWidget->raise();
            }
        }
    });

    localList->setChecked(true);

    qDebug() << "[MainWidget] Creating PlayWidget...";

    w = new PlayWidget(this, this);
    w->setFixedSize(1000,600);
    QRegion region(0, 500, w->width(), 500);
    w->setMask(region);
    w->move(0, this->height()-w->height());

    qDebug() << "[MainWidget] PlayWidget created successfully";

    connect(w, &PlayWidget::signal_playState, this, [this](ProcessSliderQml::State state){
        if (!m_playbackStateManager) {
            return;
        }

        if (state == ProcessSliderQml::Play) {
            m_playbackStateManager->onAudioPlayIntent();
        } else if (state == ProcessSliderQml::Stop) {
            m_playbackStateManager->onAudioInactive();
        }
    });

    qDebug() << "[MainWidget] Creating VideoListWidget...";

    videoListWidget = new VideoListWidget(w, this);
    videoListWidget->setFixedSize(800, 400);
    videoListWidget->move(main_list->pos());
    videoListWidget->hide();
    videoListWidget->setObjectName("videoList");
    connect(videoListWidget, &VideoListWidget::videoPlayerWindowReady, this, [this](VideoPlayerWindow* window) {
        videoPlayerWindow = window;
        qDebug() << "[MainWidget] VideoPlayerWindow linked from VideoListWidget:" << window;
    });
    connect(videoListWidget, &VideoListWidget::videoPlaybackStateChanged, this, [this](bool isPlaying) {
        if (!m_playbackStateManager) {
            return;
        }
        if (isPlaying) {
            m_playbackStateManager->onVideoPlayIntent();
        } else {
            m_playbackStateManager->onVideoInactive();
        }
    });

    qDebug() << "[MainWidget] VideoListWidget created successfully";

    request = new HttpRequestV2(this);

    connect(searchBox, &SearchBoxQml::search, this, [=](const QString& keyword) {
        QString trimmedKeyword = keyword.trimmed();
        
        if (trimmedKeyword.isEmpty()) {
            QMessageBox::information(this,
                                     QStringLiteral(u"\u63d0\u793a"),
                                     QStringLiteral(u"\u8bf7\u8f93\u5165\u641c\u7d22\u5173\u952e\u8bcd\u3002"));
            return;
        }
        
        qDebug() << "[MainWidget] Search keyword:" << trimmedKeyword;
        net_list->clearList();
        request->getMusic(trimmedKeyword);
    });
    
    connect(request, &HttpRequestV2::signal_addSong_list, net_list, &MusicListWidgetNet::signal_add_songlist);
    connect(request, &HttpRequestV2::signal_addSong_list, this, [=](){NetList->setChecked(true);});

    connect(menuButton, &QPushButton::clicked, this, [=](){
        if (!mainMenu) {
            mainMenu = new MainMenu(this);
            
            connect(mainMenu, &MainMenu::pluginRequested, this, [=](const QString& pluginName){
                qDebug() << "Plugin requested:" << pluginName;
                
                PluginManager& pluginManager = PluginManager::instance();
                PluginInterface* plugin = pluginManager.getPlugin(pluginName);
                
                if (plugin) {
                    QWidget* pluginWidget = plugin->createWidget(this);
                    if (pluginWidget) {
                        pluginWidget->show();
                        pluginWidget->raise();
                        pluginWidget->activateWindow();
                    }
                } else {
                    qWarning() << "Plugin not found:" << pluginName;
                    QMessageBox::warning(this,
                                         QStringLiteral(u"\u9519\u8bef"),
                                         QStringLiteral(u"\u63d2\u4ef6\u201c") + pluginName
                                         + QStringLiteral(u"\u201d\u672a\u627e\u5230\u6216\u52a0\u8f7d\u5931\u8d25\u3002"));
                }
            });

            connect(mainMenu, &MainMenu::settingsRequested, this, [=](){
                qDebug() << "Settings requested";
                if (!settingsWidget) {
                    // Use a top-level settings window to avoid being embedded into MainWidget.
                    settingsWidget = new SettingsWidget(nullptr);
                }
                settingsWidget->show();
                settingsWidget->raise();
                settingsWidget->activateWindow();
            });

            connect(mainMenu, &MainMenu::aboutRequested, this, [=](){
                QMessageBox::about(this,
                                   QStringLiteral(u"\u5173\u4e8e"),
                                   QStringLiteral(u"FFmpeg \u97f3\u4e50\u64ad\u653e\u5668 v1.0\\n\u5df2\u96c6\u6210\u97f3\u9891\u8f6c\u6362\u4e0e\u8bed\u97f3\u7ffb\u8bd1\u3002"));
            });
        }
        
        QPoint globalPos = menuButton->mapToGlobal(QPoint(0, menuButton->height() + 5));
        
        qDebug() << "Menu button position:" << globalPos;
        qDebug() << "Showing main menu...";
        
        mainMenu->showMenu(globalPos);
    });

    /*
    connect(userWidget, &UserWidget::loginRequested, this, [=](){
        loginWidget->isVisible = !loginWidget->isVisible;

        if(loginWidget->isVisible)
        {
            loginWidget->show();
        }
        else
        {
            loginWidget->close();
        }
    });

    connect(userWidget, &UserWidget::logoutRequested, this, [=](){
        userWidget->setLoginState(false);
    });
    */
    
    connect(userWidgetQml, &UserWidgetQml::loginRequested, this, [=](){
        loginWidget->isVisible = !loginWidget->isVisible;

        if(loginWidget->isVisible)
        {
            loginWidget->show();
        }
        else
        {
            loginWidget->close();
        }
    });

    connect(userWidgetQml, &UserWidgetQml::logoutRequested, this, [=](){
        OnlinePresenceManager::instance().logoutAndClear(false);
        SettingsManager::instance().setAutoLoginEnabled(false);
        User::getInstance()->set_account("");
        User::getInstance()->set_password("");
        User::getInstance()->set_username("");
        userWidgetQml->setLoginState(false);
    });

    connect(&OnlinePresenceManager::instance(), &OnlinePresenceManager::sessionExpired, this, [=]() {
        qWarning() << "[MainWidget] online session expired, forcing logout";
        SettingsManager::instance().setAutoLoginEnabled(false);
        User::getInstance()->set_account("");
        User::getInstance()->set_password("");
        User::getInstance()->set_username("");
        userWidgetQml->setLoginState(false);
        QMessageBox::warning(this,
                             QStringLiteral("登录状态失效"),
                             QStringLiteral("在线会话已失效，请重新登录。"));
    });

    connect(loginWidget, &LoginWidgetQml::login_, this, [=](QString username){
        QPixmap userAvatar(":/new/prefix1/icon/denglu.png");
        userWidgetQml->setUserInfo(username, userAvatar);
        userWidgetQml->setLoginState(true);
        loginWidget->close();
        OnlinePresenceManager::instance().ensureSessionForUser(User::getInstance()->get_account(), username);

        SettingsManager::instance().saveAccountCache(
            User::getInstance()->get_account(),
            User::getInstance()->get_password(),
            username,
            true
        );
        loginWidget->setSavedAccount(SettingsManager::instance().cachedAccount(),
                                     SettingsManager::instance().cachedPassword(),
                                     SettingsManager::instance().cachedUsername());
    });

    const QString cachedAccount = SettingsManager::instance().cachedAccount();
    const QString cachedPassword = SettingsManager::instance().cachedPassword();
    if (SettingsManager::instance().autoLoginEnabled()
            && !cachedAccount.isEmpty()
            && !cachedPassword.isEmpty()) {
        QTimer::singleShot(0, this, [this, cachedAccount, cachedPassword]() {
            qDebug() << "[MainWidget] Auto login with cached account:" << cachedAccount;
            loginWidget->requestLogin(cachedAccount, cachedPassword, true);
        });
    }
    
    connect(net_list, &MusicListWidgetNet::loginRequired, this, [=](){
        qDebug() << "[MainWidget] Download requires login, showing login window";
        showLoginWindow();
    });
    connect(main_list, &MusicListWidgetLocal::signal_add_button_clicked, w, &PlayWidget::openfile);


    connect(w, &PlayWidget::signal_Last, main_list, [this](QString songName, bool net_flag){
        if(net_flag) emit net_list->signal_last(songName);
        else           emit main_list->signal_last(songName);
    });
    connect(w, &PlayWidget::signal_Next, main_list, [this](QString songName, bool net_flag){
        if(net_flag) emit net_list->signal_next(songName);
        else           emit main_list->signal_next(songName);
    });
    connect(w, &PlayWidget::signal_netFlagChanged,[this](bool net_flag){
        //if(net_flag)
    });
    connect(w,&PlayWidget::signal_add_song,main_list,&MusicListWidgetLocal::on_signal_add_song);
    
    connect(w, &PlayWidget::signal_add_song, [=](const QString fileName, const QString path){
        qDebug() << "[LocalMusicCache] Adding music:" << fileName << path;
        LocalMusicInfo info;
        info.filePath = path;
        info.fileName = fileName;
        LocalMusicCache::instance().addMusic(info);
    });
    connect(w, &PlayWidget::signal_play_button_click,main_list,&MusicListWidgetLocal::on_signal_play_button_click);
    connect(w, &PlayWidget::signal_play_button_click, net_list, &MusicListWidgetNet::on_signal_play_button_click);
    
    connect(w, &PlayWidget::signal_play_button_click, [=](bool playing, const QString& filename) {
        playHistoryWidget->setPlayingState(filename, playing);
        if (!w->get_net_flag() && !filename.isEmpty()) {
            localAndDownloadWidget->setCurrentPlayingPath(playing ? filename : "");
        }
    });
    
    connect(w, &PlayWidget::signal_metadata_updated, main_list, &MusicListWidgetLocal::on_signal_update_metadata);
    
    connect(w, &PlayWidget::signal_metadata_updated, [=](const QString& filePath, const QString& coverUrl, const QString& duration) {
        qDebug() << "[LocalMusicCache] Updating metadata:" << filePath << coverUrl << duration;
        LocalMusicCache::instance().updateMetadata(filePath, coverUrl, duration);
    });

    connect(main_list, &MusicListWidgetLocal::signal_play_click, w, [=](const QString name, bool flag){
        if (w->get_net_flag()) {
            qDebug() << "[Switch source] Network music -> local music, clear network playing state";
            net_list->signal_play_button_click(false, "");
        }
        localAndDownloadWidget->setCurrentPlayingPath("");
        w->set_play_net(flag);
        
        m_networkMusicArtist.clear();
        m_networkMusicCover.clear();
        
        
        w->_play_click(name);

    });
    connect(main_list, &MusicListWidgetLocal::signal_remove_click, w, &PlayWidget::_remove_click);

    connect(localAndDownloadWidget, &LocalAndDownloadWidget::playMusic, w, [=](const QString filename){
        qDebug() << "[LocalAndDownloadWidget] Play music:" << filename;
        if (w->get_net_flag()) {
            net_list->signal_play_button_click(false, "");
        }
        main_list->signal_play_button_click(false, "");
        localAndDownloadWidget->setCurrentPlayingPath(filename);
        w->set_play_net(false);
        w->_play_click(filename);
    });
    
    connect(localAndDownloadWidget, &LocalAndDownloadWidget::addLocalMusicRequested, w, &PlayWidget::openfile);
    
    connect(localAndDownloadWidget, &LocalAndDownloadWidget::deleteMusic, [=](const QString filename){
        qDebug() << "[LocalAndDownloadWidget] Delete music:" << filename;
        
        LocalMusicCache::instance().removeMusic(filename);
        
        QFile file(filename);
        if (file.exists()) {
            if (file.remove()) {
                qDebug() << "[LocalAndDownloadWidget] File deleted successfully:" << filename;
                
                QFileInfo fileInfo(filename);
                QString folderPath = fileInfo.dir().absolutePath();
                QDir parentDir = fileInfo.dir();
                parentDir.cdUp();
                
                QString baseName = fileInfo.completeBaseName();
                QString sameFolderPath = parentDir.absoluteFilePath(baseName);
                QDir sameFolder(sameFolderPath);
                
                if (sameFolder.exists() && folderPath.contains(baseName)) {
                    if (sameFolder.removeRecursively()) {
                        qDebug() << "[LocalAndDownloadWidget] Folder deleted successfully:" << sameFolderPath;
                    } else {
                        qWarning() << "[LocalAndDownloadWidget] Failed to delete folder:" << sameFolderPath;
                    }
                }
            } else {
                qWarning() << "[LocalAndDownloadWidget] Failed to delete file:" << filename;
            }
        } else {
            qWarning() << "[LocalAndDownloadWidget] File not found:" << filename;
        }
    });

    connect(net_list, &MusicListWidgetNet::signal_play_click, w, [=](const QString name, const QString artist, const QString cover, bool flag){
        qDebug() << "[MainWidget] ========== NET MUSIC PLAY SIGNAL ==========";
        qDebug() << "[MainWidget] name:" << name;
        qDebug() << "[MainWidget] artist:" << artist;
        qDebug() << "[MainWidget] cover:" << cover;
        qDebug() << "[MainWidget] flag:" << flag;
        qDebug() << "[MainWidget] ===============================================";
        
        if (!w->get_net_flag()) {
            qDebug() << "[Switch source] Local music -> network music, clear local playing state";
            main_list->signal_play_button_click(false, "");
        }
        localAndDownloadWidget->setCurrentPlayingPath("");
        w->set_play_net(flag);
        
        const QString normalizedArtist = normalizeArtistForHistory(artist);
        w->setNetworkMetadata(normalizedArtist, cover);

        m_networkMusicArtist = normalizedArtist;
        m_networkMusicCover = cover;
        
        
        w->_play_click(name);
    });
    
    
    connect(playHistoryWidget, &PlayHistoryWidget::playMusic, w, [=](const QString filePath){
        qDebug() << "[PlayHistoryWidget] Play music:" << filePath;
        if (w->get_net_flag()) {
            net_list->signal_play_button_click(false, "");
        }
        main_list->signal_play_button_click(false, "");
        localAndDownloadWidget->setCurrentPlayingPath("");
        
        bool isLocal = !filePath.startsWith("http");
        w->set_play_net(!isLocal);
        w->_play_click(filePath);
    });

    connect(playHistoryWidget, &PlayHistoryWidget::playMusicWithMetadata, w,
            [=](const QString filePath, const QString title, const QString artist, const QString cover){
        qDebug() << "[PlayHistoryWidget] Play music with metadata:" << filePath
                 << "title:" << title << "artist:" << artist << "cover:" << cover;

        if (w->get_net_flag()) {
            net_list->signal_play_button_click(false, "");
        }
        main_list->signal_play_button_click(false, "");
        localAndDownloadWidget->setCurrentPlayingPath("");

        const bool isLocal = !filePath.startsWith("http");
        w->set_play_net(!isLocal);
        // Preload metadata so playlist history can render correct title/artist/cover
        // before decoder album-art callback finishes.
        const QString normalizedArtist = normalizeArtistForHistory(artist);
        w->setNetworkMetadata(title, normalizedArtist, cover);
        if (!isLocal) {
            m_networkMusicArtist = normalizedArtist;
            m_networkMusicCover = cover;
        }
        w->_play_click(filePath);
    });
    
    connect(playHistoryWidget, &PlayHistoryWidget::deleteHistory, this, [=](const QStringList& paths){
        qDebug() << "[PlayHistoryWidget] Delete history, count:" << paths.size();
        
        QString userAccount = User::getInstance()->get_account();
        if (!userAccount.isEmpty()) {
            request->removePlayHistory(userAccount, paths);
        } else {
            qWarning() << "[PlayHistoryWidget] Cannot delete history: user not logged in";
        }
    });
    
    connect(request, &HttpRequestV2::signal_removeHistoryResult, this, [=](bool success){
        if (success) {
            qDebug() << "[PlayHistoryWidget] Delete history success, refreshing list";
            QString userAccount = User::getInstance()->get_account();
            if (!userAccount.isEmpty()) {
                request->getPlayHistory(userAccount, 50, false);
            }
        } else {
            qWarning() << "[PlayHistoryWidget] Delete history failed";
        }
    });

    connect(playHistoryWidget, &PlayHistoryWidget::addToFavorite, this,
            [=](const QString& path, const QString& title, const QString& artist, const QString& duration, bool isLocal){
        qDebug() << "[PlayHistoryWidget] Add to favorite:" << title << "path:" << path << "isLocal:" << isLocal;
        if (!isUserLoggedIn()) {
            showLoginWindow();
            return;
        }
        QString userAccount = User::getInstance()->get_account();
        request->addFavorite(userAccount, path, title, artist, duration, isLocal);
    });
    
    connect(playHistoryWidget, &PlayHistoryWidget::loginRequested, this, [=](){
        qDebug() << "[PlayHistoryWidget] Login requested";
        showLoginWindow();
    });
    
    connect(playHistoryWidget, &PlayHistoryWidget::refreshRequested, this, [=](){
        qDebug() << "[PlayHistoryWidget] Refresh requested";
        if (isUserLoggedIn()) {
            QString userAccount = User::getInstance()->get_account();
            request->getPlayHistory(userAccount, 50, false);
        }
    });
    
    connect(request, &HttpRequestV2::signal_historyList, playHistoryWidget, &PlayHistoryWidget::loadHistory);
    
    
    connect(favoriteMusicWidget, &FavoriteMusicWidget::playMusic, w, [=](const QString filePath){
        qDebug() << "[FavoriteMusicWidget] Play music:" << filePath;
        if (w->get_net_flag()) {
            net_list->signal_play_button_click(false, "");
        }
        main_list->signal_play_button_click(false, "");
        localAndDownloadWidget->setCurrentPlayingPath("");
        
        bool isLocal = !filePath.startsWith("http");
        w->set_play_net(!isLocal);
        w->_play_click(filePath);
    });
    
    connect(favoriteMusicWidget, &FavoriteMusicWidget::removeFavorite, this, [=](const QStringList& paths){
        qDebug() << "[FavoriteMusicWidget] Remove favorite, count:" << paths.size();
        QString userAccount = User::getInstance()->get_account();
        request->removeFavorite(userAccount, paths);
    });
    
    connect(favoriteMusicWidget, &FavoriteMusicWidget::refreshRequested, this, [=](){
        qDebug() << "[FavoriteMusicWidget] Refresh requested";
        QString userAccount = User::getInstance()->get_account();
        request->getFavorites(userAccount);
    });
    
    connect(request, &HttpRequestV2::signal_favoritesList, favoriteMusicWidget, &FavoriteMusicWidget::loadFavorites);
    
    connect(request, &HttpRequestV2::signal_removeFavoriteResult, this, [=](bool success){
        if (success) {
            qDebug() << "[MainWidget] Remove favorite success, refreshing list";
            QString userAccount = User::getInstance()->get_account();
            request->getFavorites(userAccount, false);
        } else {
            qWarning() << "[MainWidget] Remove favorite failed";
        }
    });
    
    
    connect(localAndDownloadWidget, &LocalAndDownloadWidget::addToFavorite, 
            this, [=](const QString& path, const QString& title, const QString& artist, const QString& duration){
        qDebug() << "[MainWidget] Add to favorite from local/download:" << title;
        if (!isUserLoggedIn()) {
            showLoginWindow();
            return;
        }
        QString userAccount = User::getInstance()->get_account();
        request->addFavorite(userAccount, path, title, artist, duration, true);  // is_local = true
    });
    
    connect(net_list, &MusicListWidgetNet::addToFavorite,
            this, [=](const QString& path, const QString& title, const QString& artist, const QString& duration){
        qDebug() << "[MainWidget] Add to favorite from online:" << title;
        if (!isUserLoggedIn()) {
            showLoginWindow();
            return;
        }
        QString userAccount = User::getInstance()->get_account();
        request->addFavorite(userAccount, path, title, artist, duration, false);  // is_local = false
    });
    
    connect(request, &HttpRequestV2::signal_addFavoriteResult, this, [=](bool success){
        if (success) {
            qDebug() << "[MainWidget] Add to favorite success, refreshing list";
        } else {
            qWarning() << "[MainWidget] Add to favorite failed, try refreshing list to sync latest state";
        }

        QString userAccount = User::getInstance()->get_account();
        if (!userAccount.isEmpty()) {
            request->getFavorites(userAccount, false);
        }
    });
    
    connect(userWidgetQml, &UserWidgetQml::loginStateChanged, this, [=](bool loggedIn){
        qDebug() << "[MainWidget] Login state changed:" << loggedIn;
        
        QString userAccount = loggedIn ? User::getInstance()->get_account() : "";
        playHistoryWidget->setLoggedIn(loggedIn, userAccount);
        favoriteMusicWidget->setUserAccount(userAccount);
        
        QPushButton* favBtn = findChild<QPushButton*>("FavoriteMusicBtn");
        QPushButton* videoBtn = findChild<QPushButton*>("VideoPlayerBtn");
        
        if (favBtn) {
            favBtn->setVisible(loggedIn);
            qDebug() << "[MainWidget] Favorite music button visibility:" << loggedIn;
        }
        
        if (videoBtn) {
            if (loggedIn) {
                videoBtn->move(0, this->height() - 300);
            } else {
                videoBtn->move(0, this->height() - 350);
            }
        }
        
        if (!loggedIn) {
            favoriteMusicWidget->clearFavorites();
            playHistoryWidget->clearHistory();
        }
    });
    
    connect(&AudioService::instance(), &AudioService::playbackStarted, this, [=](const QString& sessionId, const QUrl& url) {
        if (m_playbackStateManager) {
            m_playbackStateManager->onAudioPlayIntent();
        }
        qDebug() << "[MainWidget] playbackStarted signal received! sessionId:" << sessionId << "url:" << url;
        
        QString filePath = url.toLocalFile();
        if (filePath.isEmpty()) {
            filePath = url.toString();
        }
        
        qDebug() << "[MainWidget] Extracted filePath:" << filePath;
        
        qDebug() << "[MainWidget] About to call setCurrentPlayingPath on both widgets...";
        playHistoryWidget->setCurrentPlayingPath(filePath);
        favoriteMusicWidget->setCurrentPlayingPath(filePath);
        qDebug() << "[MainWidget] setCurrentPlayingPath calls completed";
        
        QString userAccount = User::getInstance()->get_account();
        if (userAccount.isEmpty()) {
            qDebug() << "[MainWidget] User not logged in, skipping history add";
            return;
        }
        
        submitPlayHistoryWithRetry(sessionId, filePath, userAccount, 1);
    });
    
    connect(&AudioService::instance(), &AudioService::playbackPaused, this, [this]() {
        if (m_playbackStateManager) {
            m_playbackStateManager->onAudioInactive();
        }
    });

    connect(&AudioService::instance(), &AudioService::playbackResumed, this, [this]() {
        if (m_playbackStateManager) {
            m_playbackStateManager->onAudioPlayIntent();
        }
    });

    connect(&AudioService::instance(), &AudioService::playbackStopped, this, [=]() {
        if (m_playbackStateManager) {
            m_playbackStateManager->onAudioInactive();
        }
        qDebug() << "[MainWidget] playbackStopped signal received, clearing currentPlayingPath";
        playHistoryWidget->setCurrentPlayingPath("");
        favoriteMusicWidget->setCurrentPlayingPath("");
    });

    connect(w,&PlayWidget::signal_big_clicked,this,[=](bool checked){
        if(checked)
        {
            searchBox->hide();
            userWidgetQml->hide();
            
            w->raise();
            w->clearMask();
            w->set_isUp(true);
            update();
        }
        else
        {
            searchBox->show();
            userWidgetQml->show();
            
            w->lower();
            QRegion region1(0, 500, w->width(), 500);
            w->setMask(region1);
            w->set_isUp(false);
            update();
            w->setPianWidgetEnable(false);
        }
    });
}

void MainWidget::submitPlayHistoryWithRetry(const QString& sessionId,
                                            const QString& filePath,
                                            const QString& userAccount,
                                            int retryCount)
{
    if (!request || userAccount.trimmed().isEmpty()) {
        return;
    }

    AudioSession* session = AudioService::instance().getSession(sessionId);
    if (!session) {
        session = AudioService::instance().currentSession();
    }
    if (!session) {
        qDebug() << "[MainWidget] Skip history add: session not available for" << filePath;
        return;
    }

    QString title = session->title();
    const QString sessionArtist = normalizeArtistForHistory(session->artist());
    const QString cachedArtist = normalizeArtistForHistory(m_networkMusicArtist);
    QString artist = !sessionArtist.isEmpty() ? sessionArtist : cachedArtist;
    QString album = "";
    qint64 durationMs = session->duration();
    const bool isLocal = !filePath.startsWith("http");

    if (title.isEmpty()) {
        QFileInfo fileInfo(filePath);
        title = fileInfo.completeBaseName();
    }

    if (artist.isEmpty()) {
        artist = QStringLiteral("未知艺术家");
    }

    if (durationMs <= 0 && retryCount > 0) {
        qDebug() << "[MainWidget] History add delayed: duration not ready for" << filePath
                 << "retry left:" << retryCount;
        QTimer::singleShot(1200, this, [this, sessionId, filePath, userAccount, retryCount]() {
            submitPlayHistoryWithRetry(sessionId, filePath, userAccount, retryCount - 1);
        });
        return;
    }

    if (durationMs <= 0 && isLocal) {
        const int cachedSeconds = durationSecondsFromLocalCache(filePath);
        if (cachedSeconds > 0) {
            durationMs = static_cast<qint64>(cachedSeconds) * 1000;
            qDebug() << "[MainWidget] History duration filled from local cache:" << cachedSeconds << "s";
        }
    }

    const qint64 durationSec = qMax<qint64>(0, durationMs / 1000);
    qDebug() << "[MainWidget] Add history:" << title
             << "durationSec:" << durationSec
             << "isLocal:" << isLocal;

    request->addPlayHistory(userAccount, filePath, title, artist, album,
                            QString::number(durationSec), isLocal);
}

void MainWidget::Update_paint()
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
    
    if (topWidget) {
        topWidget->setGeometry(0, 0, this->width(), 60);
        topWidget->raise();
    }
}

void MainWidget::closeEvent(QCloseEvent *event)
{
    qDebug() << "[MainWidget] closeEvent: start shutdown";
    OnlinePresenceManager::instance().logoutAndClear(true, 1200);

    if (mainMenu) {
        mainMenu->close();
    }
    if (settingsWidget) {
        settingsWidget->close();
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

    if (event->isAccepted()) {
        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
    }
}

MainWidget::~MainWidget()
{
    qDebug() << "MainWidget::~MainWidget() - Starting cleanup...";

    if (videoListWidget) {
        videoListWidget->pauseVideoPlayback();
    }
    if (videoPlayerWindow) {
        videoPlayerWindow->pausePlayback();
    }
    AudioService::instance().stop();
    AudioPlayer::instance().stop();
    AudioPlayer::instance().clearWriteOwner();
    AudioPlayer::instance().resetBuffer();
    
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
    PluginManager::instance().unloadAllPlugins();
    
    qDebug() << "MainWidget::~MainWidget() - Cleanup complete";
}

