#include "main_widget.h"
#include "plugin_manager.h"
#include "searchbox_qml.h"
#include "AudioService.h"
#include "VideoPlayerWindow.h"
#include "user.h"
#include <QCoreApplication>

MainWidget::MainWidget(QWidget *parent) : QWidget(parent)
  ,w(nullptr)
  ,list(nullptr)
  ,videoPlayerWindow(nullptr)
  ,videoListWidget(nullptr)
  ,settingsWidget(nullptr)
{
    resize(1000,600);
    setWindowFlags(Qt::CustomizeWindowHint);
    
    setObjectName("MainWidget");
    
    PluginManager& pluginManager = PluginManager::instance();
    QString pluginPath = QCoreApplication::applicationDirPath() + "/plugin";
    int loadedCount = pluginManager.loadPlugins(pluginPath);
    qDebug() << "Loaded" << loadedCount << "plugins from" << pluginPath;

    topWidget = new QWidget(this);
    topWidget->setStyleSheet("QWidget { background: transparent; }");
    
    QPushButton* minimizeButton = new QPushButton(topWidget);
    minimizeButton->setStyleSheet(
        "QPushButton {"
        "    border-image: url(:/new/prefix1/icon/square_unselected.png);"
        "    background: transparent;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(63, 81, 181, 0.1);"
        "    border-radius: 15px;"
        "}"
    );
    minimizeButton->setFixedSize(30, 30);

    QPushButton* maximizeButton = new QPushButton(topWidget);
    maximizeButton->setStyleSheet(
        "QPushButton {"
        "    border-image: url(:/new/prefix1/icon/minus_sign.png);"
        "    background: transparent;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(63, 81, 181, 0.1);"
        "    border-radius: 15px;"
        "}"
    );
    maximizeButton->setFixedSize(30, 30);

    QPushButton* closeButton = new QPushButton(topWidget);
    closeButton->setStyleSheet(
        "QPushButton {"
        "    border-image: url(:/new/prefix1/icon/close1.png);"
        "    background: transparent;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(244, 67, 54, 0.2);"
        "    border-radius: 15px;"
        "}"
    );
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
    menuButton->setStyleSheet(
        "QPushButton {"
        "    background: transparent;"
        "    border: none;"
        "    font-size: 22px;"
        "    color: #666666;"
        "    border-radius: 25px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(49, 194, 124, 0.1);"
        "    color: #31C27C;"
        "}"
        "QPushButton:pressed {"
        "    background: rgba(49, 194, 124, 0.2);"
        "}"
    );
    
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
    leftWidget->setStyleSheet(
        "QWidget {"
        "    background: #F2F3F5;"
        "    border-radius: 0px;"
        "}"
    );

    QPushButton* localList = new QPushButton(QStringLiteral(u"\u672c\u5730\u4e0e\u4e0b\u8f7d"), leftWidget);
    localList->setFixedSize(200,50);
    localList->move(0,this->height()- 500);
    localList->setCheckable(true);
    localList->setStyleSheet(
        "QPushButton {"
        "    background: transparent;"
        "    color: #333333;"
        "    border: none;"
        "    text-align: left;"
        "    padding-left: 20px;"
        "    font-size: 14px;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(0, 0, 0, 0.03);"
        "}"
    );

    QPushButton* NetList = new QPushButton(QStringLiteral(u"\u5728\u7ebf\u97f3\u4e50"), leftWidget);
    NetList->setFixedSize(200,50);
    NetList->move(0,this->height()- 450);
    NetList->setCheckable(true);
    NetList->setStyleSheet(
        "QPushButton {"
        "    background: transparent;"
        "    color: #333333;"
        "    border: none;"
        "    text-align: left;"
        "    padding-left: 20px;"
        "    font-size: 14px;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(0, 0, 0, 0.03);"
        "}"
    );

    QPushButton* PlayHistoryBtn = new QPushButton(QStringLiteral(u"\u6700\u8fd1\u64ad\u653e"), leftWidget);
    PlayHistoryBtn->setFixedSize(200, 50);
    PlayHistoryBtn->move(0, this->height() - 400);
    PlayHistoryBtn->setCheckable(true);
    PlayHistoryBtn->setStyleSheet(
        "QPushButton {"
        "    background: transparent;"
        "    color: #333333;"
        "    border: none;"
        "    text-align: left;"
        "    padding-left: 20px;"
        "    font-size: 14px;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(0, 0, 0, 0.03);"
        "}"
    );
    
    QPushButton* FavoriteMusicBtn = new QPushButton(QStringLiteral(u"\u6211\u559c\u6b22\u7684\u97f3\u4e50"), leftWidget);
    FavoriteMusicBtn->setFixedSize(200, 50);
    FavoriteMusicBtn->move(0, this->height() - 350);
    FavoriteMusicBtn->setCheckable(true);
    FavoriteMusicBtn->setObjectName("FavoriteMusicBtn");
    FavoriteMusicBtn->setVisible(false);
    FavoriteMusicBtn->setStyleSheet(
        "QPushButton {"
        "    background: transparent;"
        "    color: #333333;"
        "    border: none;"
        "    text-align: left;"
        "    padding-left: 20px;"
        "    font-size: 14px;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(0, 0, 0, 0.03);"
        "}"
    );
    
    QPushButton* VideoPlayerBtn = new QPushButton(QStringLiteral(u"\u89c6\u9891\u64ad\u653e"), leftWidget);
    VideoPlayerBtn->setFixedSize(200, 50);
    VideoPlayerBtn->setObjectName("VideoPlayerBtn");
    VideoPlayerBtn->move(0, this->height() - 350);
    VideoPlayerBtn->setCheckable(false);
    VideoPlayerBtn->setStyleSheet(
        "QPushButton {"
        "    background: transparent;"
        "    color: #333333;"
        "    border: none;"
        "    text-align: left;"
        "    padding-left: 20px;"
        "    font-size: 14px;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(0, 0, 0, 0.03);"
        "}"
        "QPushButton:pressed {"
        "    background: rgba(49, 194, 124, 0.1);"
        "}"
    );
    
    connect(VideoPlayerBtn, &QPushButton::clicked, this, [=]() {
        qDebug() << "[MainWidget] Showing online video list";
        
        main_list->hide();
        localAndDownloadWidget->hide();
        net_list->hide();
        playHistoryWidget->hide();
        favoriteMusicWidget->hide();
        videoListWidget->show();
        videoListWidget->raise();
    });

    QButtonGroup* leftButtons = new QButtonGroup(this);
    leftButtons->addButton(localList);
    leftButtons->addButton(NetList);
    leftButtons->addButton(PlayHistoryBtn);
    leftButtons->addButton(FavoriteMusicBtn);
    leftButtons->setExclusive(true);

    QWidget* textWidget = new QWidget(leftWidget);

    QLabel* icolabel = new QLabel(textWidget);
    icolabel->setPixmap(QPixmap(":/new/prefix1/icon/netease.ico"));
    icolabel->setScaledContents(true);
    icolabel->setFixedSize(40, 40);

    QLabel *textLabel = new QLabel(QStringLiteral(u"\u7f51\u6613\u4e91\u97f3\u4e50"), textWidget);
    QFont font;
    font.setFamily("Microsoft YaHei");
    font.setPointSize(16);
    font.setBold(true);
    textLabel->setFont(font);
    textLabel->setStyleSheet("color: #333333;");
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
            localList->setStyleSheet(
                "QPushButton {"
                "    background: rgba(49, 194, 124, 0.15);"
                "    color: #31C27C;"
                "    border: none;"
                "    border-left: 3px solid #31C27C;"
                "    text-align: left;"
                "    padding-left: 17px;"
                "    font-size: 14px;"
                "    font-weight: 600;"
                "}"
            );
        } else {
            localList->setStyleSheet(
                "QPushButton {"
                "    background: transparent;"
                "    color: #333333;"
                "    border: none;"
                "    text-align: left;"
                "    padding-left: 20px;"
                "    font-size: 14px;"
                "    font-weight: 500;"
                "}"
                "QPushButton:hover {"
                "    background: rgba(0, 0, 0, 0.03);"
                "}"
            );
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
            NetList->setStyleSheet(
                "QPushButton {"
                "    background: rgba(49, 194, 124, 0.15);"
                "    color: #31C27C;"
                "    border: none;"
                "    border-left: 3px solid #31C27C;"
                "    text-align: left;"
                "    padding-left: 17px;"
                "    font-size: 14px;"
                "    font-weight: 600;"
                "}"
            );
        }
        else
        {
            NetList->setStyleSheet(
                "QPushButton {"
                "    background: transparent;"
                "    color: #333333;"
                "    border: none;"
                "    text-align: left;"
                "    padding-left: 20px;"
                "    font-size: 14px;"
                "    font-weight: 500;"
                "}"
                "QPushButton:hover {"
                "    background: rgba(0, 0, 0, 0.03);"
                "}"
            );
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
            
            PlayHistoryBtn->setStyleSheet(
                "QPushButton {"
                "    background: rgba(49, 194, 124, 0.15);"
                "    color: #31C27C;"
                "    border: none;"
                "    border-left: 3px solid #31C27C;"
                "    text-align: left;"
                "    padding-left: 17px;"
                "    font-size: 14px;"
                "    font-weight: 600;"
                "}"
            );
        }
        else
        {
            PlayHistoryBtn->setStyleSheet(
                "QPushButton {"
                "    background: transparent;"
                "    color: #333333;"
                "    border: none;"
                "    text-align: left;"
                "    padding-left: 20px;"
                "    font-size: 14px;"
                "    font-weight: 500;"
                "}"
                "QPushButton:hover {"
                "    background: rgba(0, 0, 0, 0.03);"
                "}"
            );
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
            
            FavoriteMusicBtn->setStyleSheet(
                "QPushButton {"
                "    background: rgba(49, 194, 124, 0.15);"
                "    color: #31C27C;"
                "    border: none;"
                "    border-left: 3px solid #31C27C;"
                "    text-align: left;"
                "    padding-left: 17px;"
                "    font-size: 14px;"
                "    font-weight: 600;"
                "}"
            );
        }
        else
        {
            FavoriteMusicBtn->setStyleSheet(
                "QPushButton {"
                "    background: transparent;"
                "    color: #333333;"
                "    border: none;"
                "    text-align: left;"
                "    padding-left: 20px;"
                "    font-size: 14px;"
                "    font-weight: 500;"
                "}"
                "QPushButton:hover {"
                "    background: rgba(0, 0, 0, 0.03);"
                "}"
            );
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

    connect(w, &PlayWidget::signal_playState, this, [=](ProcessSliderQml::State state){
        if (state == ProcessSliderQml::Play) {
            qDebug() << "[MainWidget] Music started, pausing video";
            if (videoListWidget) {
                videoListWidget->pauseVideoPlayback();
            } else if (videoPlayerWindow) {
                videoPlayerWindow->pausePlayback();
            }
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
                    settingsWidget = new SettingsWidget(this);
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
        userWidgetQml->setLoginState(false);
    });

    connect(loginWidget, &LoginWidgetQml::login_, this, [=](QString username){
        QPixmap userAvatar(":/new/prefix1/icon/denglu.png");
        userWidgetQml->setUserInfo(username, userAvatar);
        userWidgetQml->setLoginState(true);
        loginWidget->close();
    });
    
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
        
        w->setNetworkMetadata(artist, cover);
        
        m_networkMusicArtist = artist;
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
        w->setNetworkMetadata(title, artist, cover);
        if (!isLocal) {
            m_networkMusicArtist = artist;
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
                request->getPlayHistory(userAccount, 50);
            }
        } else {
            qWarning() << "[PlayHistoryWidget] Delete history failed";
        }
    });
    
    connect(playHistoryWidget, &PlayHistoryWidget::loginRequested, this, [=](){
        qDebug() << "[PlayHistoryWidget] Login requested";
        showLoginWindow();
    });
    
    connect(playHistoryWidget, &PlayHistoryWidget::refreshRequested, this, [=](){
        qDebug() << "[PlayHistoryWidget] Refresh requested";
        if (isUserLoggedIn()) {
            QString userAccount = User::getInstance()->get_account();
            request->getPlayHistory(userAccount, 50);
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
            request->getFavorites(userAccount);
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
            qDebug() << "[MainWidget] Add to favorite success";
        } else {
            qWarning() << "[MainWidget] Add to favorite failed";
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
        
        AudioSession* currentSession = AudioService::instance().currentSession();
        if (!currentSession) {
            return;
        }
        
        QString title = currentSession->title();
        QString artist = currentSession->artist();
        QString album = "";
        qint64 durationMs = currentSession->duration();
        
        bool isLocal = !filePath.startsWith("http");
        
        if (!isLocal && !m_networkMusicArtist.isEmpty()) {
            artist = m_networkMusicArtist;
            qDebug() << "[MainWidget] Using cached network artist:" << artist;
        }
        
        if (title.isEmpty()) {
            QFileInfo fileInfo(filePath);
            title = fileInfo.completeBaseName();
        }
        
        qDebug() << "[MainWidget] ========== ADDING TO PLAY HISTORY ==========";
        qDebug() << "[MainWidget] userAccount:" << userAccount;
        qDebug() << "[MainWidget] filePath:" << filePath;
        qDebug() << "[MainWidget] title:" << title;
        qDebug() << "[MainWidget] artist:" << artist;
        qDebug() << "[MainWidget] album:" << album;
        qDebug() << "[MainWidget] duration:" << QString::number(durationMs / 1000) << "seconds";
        qDebug() << "[MainWidget] isLocal:" << isLocal;
        qDebug() << "[MainWidget] =============================================";
        
        request->addPlayHistory(userAccount, filePath, title, artist, album, 
                                QString::number(durationMs / 1000), isLocal);
    });
    
    connect(&AudioService::instance(), &AudioService::playbackStopped, this, [=]() {
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

MainWidget::~MainWidget()
{
    qDebug() << "MainWidget::~MainWidget() - Starting cleanup...";
    
    if(w) {
        qDebug() << "MainWidget: Deleting PlayWidget...";
        w->deleteLater();
        w = nullptr;
    }
    
    QCoreApplication::processEvents();
    QThread::msleep(100);
    
    if(list) {
        list->deleteLater();
        list = nullptr;
    }
    
    if(loginWidget) {
        loginWidget->close();
        loginWidget->deleteLater();
        loginWidget = nullptr;
    }
    
    if(videoPlayerWindow) {
        qDebug() << "MainWidget: Deleting VideoPlayerWindow...";
        videoPlayerWindow->close();
        videoPlayerWindow->deleteLater();
        videoPlayerWindow = nullptr;
    }
    
    qDebug() << "MainWidget: Waiting for thread pool...";
    QThreadPool::globalInstance()->waitForDone(3000);
    
    qDebug() << "MainWidget: Unloading plugins...";
    PluginManager::instance().unloadAllPlugins();
    
    qDebug() << "MainWidget::~MainWidget() - Cleanup complete";
}
