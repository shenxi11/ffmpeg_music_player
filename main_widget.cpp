#include "main_widget.h"
#include "plugin_manager.h"
#include "searchbox_qml.h"  // 使用 QML 版本的 SearchBox
#include "AudioService.h"   // 添加 AudioService 以同步播放列表
#include "VideoPlayerWindow.h"
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
    
    // 移除深色背景，使用 paintEvent 的渐变背景
    setObjectName("MainWidget");
    
    // 初始化插件管理器，加载所有插件
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

    // 使用 QML 版本的 SearchBox ⭐
    SearchBoxQml* searchBox = new SearchBoxQml(this);
    searchBox->setFixedSize(250, 60);

    // 使用新的用户控件（注释掉旧版本，使用 QML 版本）
    // userWidget = new UserWidget(this);
    // userWidget->setFixedSize(110, 50); // 进一步增加用户控件尺寸以完整显示"未登录"
    
    // 使用 QML 版本的 UserWidget ⭐
    userWidgetQml = new UserWidgetQml(this);
    userWidgetQml->setFixedSize(150, 40);
    
    // 创建主菜单按钮
    menuButton = new QPushButton("☰", this);
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
    
    // 初始化菜单指针为空，按需创建
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
    loginWidget->setWindowTitle("登陆");

    main_list = new MusicListWidgetLocal(this);
    main_list->setFixedSize(800,400);
    main_list->move((this->width()-800),this->height()- 500);
    main_list->show();
    main_list->setObjectName("local");

    // 创建新的本地和下载页面（多Tab）
    localAndDownloadWidget = new LocalAndDownloadWidget(this);
    localAndDownloadWidget->setFixedSize(800, 400);
    localAndDownloadWidget->move((this->width()-800), this->height()- 500);
    localAndDownloadWidget->hide();  // 默认隐藏，通过按钮切换
    localAndDownloadWidget->setObjectName("localAndDownload");


    list = new MusicListWidget(this);
    list->setFixedSize(200,300);
    list->move(this->width() - 200, 60);
    list->clear();
    list->close();

    net_list = new MusicListWidgetNet(this);
    net_list->setMainWidget(this);  // 设置MainWidget指针用于登录检查
    net_list->setFixedSize(800, 400);
    net_list->move(main_list->pos());
    net_list->hide();
    net_list->setObjectName("net");
    
    // 创建播放历史widget
    playHistoryWidget = new PlayHistoryWidget(this);
    playHistoryWidget->setFixedSize(800, 400);
    playHistoryWidget->move(main_list->pos());
    playHistoryWidget->hide();
    playHistoryWidget->setObjectName("playHistory");
    
    // 创建喜欢音乐widget
    favoriteMusicWidget = new FavoriteMusicWidget(this);
    favoriteMusicWidget->setFixedSize(800, 400);
    favoriteMusicWidget->move(main_list->pos());
    favoriteMusicWidget->hide();
    favoriteMusicWidget->setObjectName("favoriteMusic");

    // 注意：PlayWidget (w) 必须在此之前创建
    // 创建在线视频列表（内嵌控件）- 先占位，稍后在w创建后初始化
    videoListWidget = nullptr;


    QWidget* leftWidget = new QWidget(this);
    leftWidget->setFixedSize(200, this->height() - 100);
    leftWidget->setStyleSheet(
        "QWidget {"
        "    background: #F2F3F5;"
        "    border-radius: 0px;"
        "}"
    );

    QPushButton* localList = new QPushButton("🎵 本地和下载", leftWidget);
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

    QPushButton* NetList = new QPushButton("🌐 在线音乐", leftWidget);
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

    // 播放历史按钮
    QPushButton* PlayHistoryBtn = new QPushButton("⌚ 最近播放", leftWidget);
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
    
    // 喜欢音乐按钮
    QPushButton* FavoriteMusicBtn = new QPushButton("♥ 喜欢音乐", leftWidget);
    FavoriteMusicBtn->setFixedSize(200, 50);
    FavoriteMusicBtn->move(0, this->height() - 350);
    FavoriteMusicBtn->setCheckable(true);
    FavoriteMusicBtn->setObjectName("FavoriteMusicBtn");
    FavoriteMusicBtn->setVisible(false);  // 默认隐藏，登录后才显示
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
    
    // 视频播放按钮（非互斥按钮，独立弹出窗口）
    QPushButton* VideoPlayerBtn = new QPushButton("🎬 视频播放", leftWidget);
    VideoPlayerBtn->setFixedSize(200, 50);
    VideoPlayerBtn->setObjectName("VideoPlayerBtn");
    VideoPlayerBtn->move(0, this->height() - 350);  // 默认位置（未登录时）
    VideoPlayerBtn->setCheckable(false);  // 不设置为可选中，每次点击都触发
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
    
    // 连接视频播放按钮点击事件 - 显示内嵌的在线视频列表
    connect(VideoPlayerBtn, &QPushButton::clicked, this, [=]() {
        qDebug() << "[MainWidget] Showing online video list";
        
        // 隐藏音乐列表，显示视频列表
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

    QLabel *textLabel = new QLabel("网易云音乐", textWidget);
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
            main_list->hide();  // 隐藏旧的本地音乐列表
            localAndDownloadWidget->show();  // 显示新的本地和下载页面
            net_list->hide();
            playHistoryWidget->hide();
            favoriteMusicWidget->hide();
            if (videoListWidget) videoListWidget->hide();  // 隐藏视频列表（检查指针）
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
            if (videoListWidget) videoListWidget->hide();  // 隐藏视频列表（检查指针）
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
    
    // 播放历史按钮切换事件
    connect(PlayHistoryBtn, &QPushButton::toggled, this, [=](bool checked){
        if(checked)
        {
            playHistoryWidget->show();
            main_list->hide();
            localAndDownloadWidget->hide();
            net_list->hide();
            favoriteMusicWidget->hide();
            if (videoListWidget) videoListWidget->hide();
            
            // 刷新播放历史数据
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
    
    // 喜欢音乐按钮切换事件
    connect(FavoriteMusicBtn, &QPushButton::toggled, this, [=](bool checked){
        if(checked)
        {
            favoriteMusicWidget->show();
            main_list->hide();
            localAndDownloadWidget->hide();
            net_list->hide();
            playHistoryWidget->hide();
            if (videoListWidget) videoListWidget->hide();
            
            // 刷新喜欢音乐数据
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

    w = new PlayWidget(this, this);  // 传入this作为mainWidget
    w->setFixedSize(1000,600);
    QRegion region(0, 500, w->width(), 500);
    w->setMask(region);
    w->move(0, this->height()-w->height());

    qDebug() << "[MainWidget] PlayWidget created successfully";

    // 连接音乐播放信号，当音乐开始播放时暂停视频
    connect(w, &PlayWidget::signal_playState, this, [=](ProcessSliderQml::State state){
        if (state == ProcessSliderQml::Play && videoPlayerWindow) {
            // 音乐开始播放，暂停视频
            qDebug() << "[MainWidget] Music started, pausing video";
            videoPlayerWindow->pausePlayback();
        }
    });

    qDebug() << "[MainWidget] Creating VideoListWidget...";

    // 创建在线视频列表（内嵌控件）- 现在w已经创建，可以传入引用
    videoListWidget = new VideoListWidget(w, this);
    videoListWidget->setFixedSize(800, 400);
    videoListWidget->move(main_list->pos());
    videoListWidget->hide();
    videoListWidget->setObjectName("videoList");

    qDebug() << "[MainWidget] VideoListWidget created successfully";

    request = HttpRequestPool::getInstance().getRequest();

    // 统一搜索流程：使用 /music/search 接口，智能搜索标题、歌手、专辑、路径
    connect(searchBox, &SearchBoxQml::search, this, [=](const QString& keyword) {
        QString trimmedKeyword = keyword.trimmed();
        
        if (trimmedKeyword.isEmpty()) {
            // 空白内容时显示提示
            QMessageBox::information(this, "提示", "请输入要搜索的内容");
            return;
        }
        
        qDebug() << "[MainWidget] Search keyword:" << trimmedKeyword;
        // 清空在线音乐列表
        net_list->clearList();
        // 直接调用统一搜索接口，服务器会智能匹配并按相关性排序
        request->getMusic(trimmedKeyword);
    });
    
    connect(request, &HttpRequest::signal_addSong_list, net_list, &MusicListWidgetNet::signal_add_songlist);
    connect(request, &HttpRequest::signal_addSong_list, this, [=](){NetList->setChecked(true);});

    // 连接菜单按钮
    connect(menuButton, &QPushButton::clicked, this, [=](){
        // 创建主菜单（如果不存在）
        if (!mainMenu) {
            mainMenu = new MainMenu(this);
            
            // 连接菜单的插件请求信号
            connect(mainMenu, &MainMenu::pluginRequested, this, [=](const QString& pluginName){
                qDebug() << "Plugin requested:" << pluginName;
                
                // 通过插件管理器获取插件实例
                PluginManager& pluginManager = PluginManager::instance();
                PluginInterface* plugin = pluginManager.getPlugin(pluginName);
                
                if (plugin) {
                    // 创建插件窗口
                    QWidget* pluginWidget = plugin->createWidget(this);
                    if (pluginWidget) {
                        pluginWidget->show();
                        pluginWidget->raise();
                        pluginWidget->activateWindow();
                    }
                } else {
                    qWarning() << "Plugin not found:" << pluginName;
                    QMessageBox::warning(this, "错误", "插件 \"" + pluginName + "\" 未找到或加载失败");
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
                // TODO: 实现关于页面
                QMessageBox::about(this, "关于", "FFmpeg 音乐播放器 v1.0\n集成音频转换和语音翻译功能");
            });
        }
        
        // 获取菜单按钮的全局位置，显示在按钮下方
        QPoint globalPos = menuButton->mapToGlobal(QPoint(0, menuButton->height() + 5));
        
        // 调试输出
        qDebug() << "Menu button position:" << globalPos;
        qDebug() << "Showing main menu...";
        
        // 显示菜单
        mainMenu->showMenu(globalPos);
    });

    // 连接新的用户控件 - 现在只处理弹出窗口中的登录按钮点击
    // 旧版本的信号连接（已注释）
    /*
    connect(userWidget, &UserWidget::loginRequested, this, [=](){
        // 切换登录窗口显示状态
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
        // 处理退出登录逻辑
        userWidget->setLoginState(false);
        // 这里可以添加清除用户数据的代码
    });
    */
    
    // 连接 QML 版本的 UserWidget ⭐
    connect(userWidgetQml, &UserWidgetQml::loginRequested, this, [=](){
        // 切换登录窗口显示状态
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
        // 处理退出登录逻辑
        userWidgetQml->setLoginState(false);
        // 这里可以添加清除用户数据的代码
    });

    connect(loginWidget, &LoginWidgetQml::login_, this, [=](QString username){
        // 登录成功后更新用户控件（仅使用 QML 版本）
        QPixmap userAvatar(":/new/prefix1/icon/denglu.png"); // 可以根据需要设置用户头像
        // userWidget->setUserInfo(username, userAvatar);  // 已注释：使用 QML 版本
        // userWidget->setLoginState(true);                 // 已注释：使用 QML 版本
        userWidgetQml->setUserInfo(username, userAvatar);
        userWidgetQml->setLoginState(true);
        loginWidget->close();
    });
    
    // 连接网络音乐列表的登录请求信号
    connect(net_list, &MusicListWidgetNet::loginRequired, this, [=](){
        qDebug() << "[MainWidget] 下载需要登录，显示登录窗口";
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
    
    // 连接本地音乐添加信号到缓存
    connect(w, &PlayWidget::signal_add_song, [=](const QString fileName, const QString path){
        qDebug() << "[LocalMusicCache] Adding music:" << fileName << path;
        LocalMusicInfo info;
        info.filePath = path;
        info.fileName = fileName;
        LocalMusicCache::instance().addMusic(info);
    });
    connect(w, &PlayWidget::signal_play_button_click,main_list,&MusicListWidgetLocal::on_signal_play_button_click);
    connect(w, &PlayWidget::signal_play_button_click, net_list, &MusicListWidgetNet::on_signal_play_button_click);
    
    // 连接下载列表的播放状态更新
    connect(w, &PlayWidget::signal_play_button_click, [=](bool playing, const QString& filename) {
        // 如果播放的是下载的音乐（非网络音乐），更新下载列表的播放状态
        if (!w->get_net_flag() && !filename.isEmpty()) {
            localAndDownloadWidget->setCurrentPlayingPath(playing ? filename : "");
        }
    });
    
    // 连接元数据更新信号（专辑图片和时长）
    connect(w, &PlayWidget::signal_metadata_updated, main_list, &MusicListWidgetLocal::on_signal_update_metadata);
    
    // 连接元数据更新到本地音乐缓存
    connect(w, &PlayWidget::signal_metadata_updated, [=](const QString& filePath, const QString& coverUrl, const QString& duration) {
        qDebug() << "[LocalMusicCache] Updating metadata:" << filePath << coverUrl << duration;
        LocalMusicCache::instance().updateMetadata(filePath, coverUrl, duration);
    });

    connect(main_list, &MusicListWidgetLocal::signal_play_click, w, [=](const QString name, bool flag){
        // 如果之前是网络模式，先清除网络列表的播放状态
        if (w->get_net_flag()) {
            qDebug() << "[切换播放源] 从网络音乐切换到本地音乐，清除网络列表播放状态";
            net_list->signal_play_button_click(false, "");
        }
        // 清除下载列表的播放状态
        localAndDownloadWidget->setCurrentPlayingPath("");
        w->set_play_net(flag);
        
        // 清除网络音乐元数据缓存（播放本地音乐）
        m_networkMusicArtist.clear();
        m_networkMusicCover.clear();
        
        // 注意：播放列表现在是自动管理的播放历史，不需要手动同步
        // 每次play()会自动添加到历史列表
        
        w->_play_click(name);

    });
    connect(main_list, &MusicListWidgetLocal::signal_remove_click, w, &PlayWidget::_remove_click);

    // 连接本地和下载页面的播放和删除信号
    connect(localAndDownloadWidget, &LocalAndDownloadWidget::playMusic, w, [=](const QString filename){
        qDebug() << "[LocalAndDownloadWidget] Play music:" << filename;
        // 清除其他列表的播放状态
        if (w->get_net_flag()) {
            net_list->signal_play_button_click(false, "");
        }
        // 清除本地列表的播放状态
        main_list->signal_play_button_click(false, "");
        // 设置当前播放路径（用于高亮显示）
        localAndDownloadWidget->setCurrentPlayingPath(filename);
        w->set_play_net(false);  // 下载的音乐是本地文件
        w->_play_click(filename);
    });
    
    // 连接本地音乐添加请求信号
    connect(localAndDownloadWidget, &LocalAndDownloadWidget::addLocalMusicRequested, w, &PlayWidget::openfile);
    
    connect(localAndDownloadWidget, &LocalAndDownloadWidget::deleteMusic, [=](const QString filename){
        qDebug() << "[LocalAndDownloadWidget] Delete music:" << filename;
        
        // 从本地音乐缓存中移除
        LocalMusicCache::instance().removeMusic(filename);
        
        QFile file(filename);
        if (file.exists()) {
            // 删除文件
            if (file.remove()) {
                qDebug() << "[LocalAndDownloadWidget] File deleted successfully:" << filename;
                
                // 尝试删除同名文件夹（如果是下载的歌曲，文件夹名和文件名相同）
                QFileInfo fileInfo(filename);
                QString folderPath = fileInfo.dir().absolutePath();
                QDir parentDir = fileInfo.dir();
                parentDir.cdUp();
                
                // 检查父目录中是否有同名文件夹
                QString baseName = fileInfo.completeBaseName(); // 不带扩展名的文件名
                QString sameFolderPath = parentDir.absoluteFilePath(baseName);
                QDir sameFolder(sameFolderPath);
                
                if (sameFolder.exists() && folderPath.contains(baseName)) {
                    // 删除整个文件夹及其内容
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
        
        // 如果之前是本地模式，先清除本地列表的播放状态
        if (!w->get_net_flag()) {
            qDebug() << "[切换播放源] 从本地音乐切换到网络音乐，清除本地列表播放状态";
            main_list->signal_play_button_click(false, "");
        }
        // 清除下载列表的播放状态
        localAndDownloadWidget->setCurrentPlayingPath("");
        w->set_play_net(flag);
        
        // 设置网络音乐的元数据（artist和cover），以便添加到播放历史时使用
        w->setNetworkMetadata(artist, cover);
        
        // 保存网络音乐元数据到MainWidget，供playbackStarted使用
        m_networkMusicArtist = artist;
        m_networkMusicCover = cover;
        
        // 注意：播放列表现在是自动管理的播放历史，不需要手动同步
        // 每次play()会自动添加到历史列表
        
        w->_play_click(name);
    });
    
    // ============ 播放历史 widget 信号连接 ============
    
    // 播放历史 - 播放音乐
    connect(playHistoryWidget, &PlayHistoryWidget::playMusic, w, [=](const QString filePath){
        qDebug() << "[PlayHistoryWidget] Play music:" << filePath;
        // 清除其他列表的播放状态
        if (w->get_net_flag()) {
            net_list->signal_play_button_click(false, "");
        }
        main_list->signal_play_button_click(false, "");
        localAndDownloadWidget->setCurrentPlayingPath("");
        
        // 判断是本地还是在线音乐
        bool isLocal = !filePath.startsWith("http");
        w->set_play_net(!isLocal);
        w->_play_click(filePath);
    });
    
    // 播放历史 - 删除历史记录（批量）
    connect(playHistoryWidget, &PlayHistoryWidget::deleteHistory, this, [=](const QStringList& paths){
        qDebug() << "[PlayHistoryWidget] Delete history, count:" << paths.size();
        
        // 调用删除播放历史的API
        QString userAccount = User::getInstance()->get_account();
        if (!userAccount.isEmpty()) {
            request->removePlayHistory(userAccount, paths);
        } else {
            qWarning() << "[PlayHistoryWidget] Cannot delete history: user not logged in";
        }
    });
    
    // 删除播放历史结果
    connect(request, &HttpRequest::signal_removeHistoryResult, this, [=](bool success){
        if (success) {
            qDebug() << "[PlayHistoryWidget] Delete history success, refreshing list";
            // 删除成功，刷新列表
            QString userAccount = User::getInstance()->get_account();
            if (!userAccount.isEmpty()) {
                request->getPlayHistory(userAccount, 50);
            }
        } else {
            qWarning() << "[PlayHistoryWidget] Delete history failed";
        }
    });
    
    // 播放历史 - 需要登录
    connect(playHistoryWidget, &PlayHistoryWidget::loginRequested, this, [=](){
        qDebug() << "[PlayHistoryWidget] Login requested";
        showLoginWindow();
    });
    
    // 播放历史 - 刷新
    connect(playHistoryWidget, &PlayHistoryWidget::refreshRequested, this, [=](){
        qDebug() << "[PlayHistoryWidget] Refresh requested";
        if (isUserLoggedIn()) {
            QString userAccount = User::getInstance()->get_account();
            request->getPlayHistory(userAccount, 50);
        }
    });
    
    // HttpRequest - 播放历史列表响应
    connect(request, &HttpRequest::signal_historyList, playHistoryWidget, &PlayHistoryWidget::loadHistory);
    
    // ============ 喜欢音乐 widget 信号连接 ============
    
    // 喜欢音乐 - 播放音乐
    connect(favoriteMusicWidget, &FavoriteMusicWidget::playMusic, w, [=](const QString filePath){
        qDebug() << "[FavoriteMusicWidget] Play music:" << filePath;
        // 清除其他列表的播放状态
        if (w->get_net_flag()) {
            net_list->signal_play_button_click(false, "");
        }
        main_list->signal_play_button_click(false, "");
        localAndDownloadWidget->setCurrentPlayingPath("");
        
        // 判断是本地还是在线音乐
        bool isLocal = !filePath.startsWith("http");
        w->set_play_net(!isLocal);
        w->_play_click(filePath);
    });
    
    // 喜欢音乐 - 移除喜欢（批量）
    connect(favoriteMusicWidget, &FavoriteMusicWidget::removeFavorite, this, [=](const QStringList& paths){
        qDebug() << "[FavoriteMusicWidget] Remove favorite, count:" << paths.size();
        QString userAccount = User::getInstance()->get_account();
        request->removeFavorite(userAccount, paths);
    });
    
    // 喜欢音乐 - 刷新
    connect(favoriteMusicWidget, &FavoriteMusicWidget::refreshRequested, this, [=](){
        qDebug() << "[FavoriteMusicWidget] Refresh requested";
        QString userAccount = User::getInstance()->get_account();
        request->getFavorites(userAccount);
    });
    
    // HttpRequest - 喜欢音乐列表响应
    connect(request, &HttpRequest::signal_favoritesList, favoriteMusicWidget, &FavoriteMusicWidget::loadFavorites);
    
    // HttpRequest - 移除喜欢结果响应
    connect(request, &HttpRequest::signal_removeFavoriteResult, this, [=](bool success){
        if (success) {
            qDebug() << "[MainWidget] Remove favorite success, refreshing list";
            QString userAccount = User::getInstance()->get_account();
            request->getFavorites(userAccount);
        } else {
            qWarning() << "[MainWidget] Remove favorite failed";
        }
    });
    
    // ============ 添加到喜欢音乐功能 ============
    
    // 本地和下载列表 - 添加到喜欢
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
    
    // 在线音乐列表 - 添加到喜欢
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
    
    // HttpRequest - 添加喜欢结果响应
    connect(request, &HttpRequest::signal_addFavoriteResult, this, [=](bool success){
        if (success) {
            qDebug() << "[MainWidget] Add to favorite success";
            // 可以显示提示消息
        } else {
            qWarning() << "[MainWidget] Add to favorite failed";
        }
    });
    
    // ============ 登录状态变化时更新喜欢音乐按钮显示 ============
    connect(userWidgetQml, &UserWidgetQml::loginStateChanged, this, [=](bool loggedIn){
        qDebug() << "[MainWidget] Login state changed:" << loggedIn;
        
        // 更新播放历史widget的登录状态
        QString userAccount = loggedIn ? User::getInstance()->get_account() : "";
        playHistoryWidget->setLoggedIn(loggedIn, userAccount);
        favoriteMusicWidget->setUserAccount(userAccount);
        
        // 查找喜欢音乐按钮并更新可见性
        QPushButton* favBtn = findChild<QPushButton*>("FavoriteMusicBtn");
        QPushButton* videoBtn = findChild<QPushButton*>("VideoPlayerBtn");
        
        if (favBtn) {
            favBtn->setVisible(loggedIn);
            qDebug() << "[MainWidget] Favorite music button visibility:" << loggedIn;
        }
        
        // 调整视频播放按钮位置
        if (videoBtn) {
            if (loggedIn) {
                // 登录后，视频播放按钮下移，为喜欢音乐按钮让出空间
                videoBtn->move(0, this->height() - 300);
            } else {
                // 登出后，视频播放按钮上移
                videoBtn->move(0, this->height() - 350);
            }
        }
        
        // 如果登出，清空喜欢音乐列表
        if (!loggedIn) {
            favoriteMusicWidget->clearFavorites();
            playHistoryWidget->clearHistory();
        }
    });
    
    // ============ 监听播放事件，自动添加到播放历史 ============
    connect(&AudioService::instance(), &AudioService::playbackStarted, this, [=](const QString& sessionId, const QUrl& url) {
        qDebug() << "[MainWidget] playbackStarted signal received! sessionId:" << sessionId << "url:" << url;
        
        QString filePath = url.toLocalFile();
        if (filePath.isEmpty()) {
            filePath = url.toString(); // 在线音乐使用URL
        }
        
        qDebug() << "[MainWidget] Extracted filePath:" << filePath;
        
        // 同步播放状态到播放历史和喜欢音乐列表（高亮显示）
        qDebug() << "[MainWidget] About to call setCurrentPlayingPath on both widgets...";
        playHistoryWidget->setCurrentPlayingPath(filePath);
        favoriteMusicWidget->setCurrentPlayingPath(filePath);
        qDebug() << "[MainWidget] setCurrentPlayingPath calls completed";
        
        // 只在登录状态下添加到服务端播放历史（通过检查account是否为空判断）
        QString userAccount = User::getInstance()->get_account();
        if (userAccount.isEmpty()) {
            qDebug() << "[MainWidget] User not logged in, skipping history add";
            return;
        }
        
        // 从AudioService获取当前播放的元数据
        AudioSession* currentSession = AudioService::instance().currentSession();
        if (!currentSession) {
            return;
        }
        
        QString title = currentSession->title();
        QString artist = currentSession->artist();
        QString album = "";  // AudioSession没有album字段，使用空字符串
        qint64 durationMs = currentSession->duration();
        
        // 判断是本地还是在线音乐
        bool isLocal = !filePath.startsWith("http");
        
        // 对于网络音乐，使用保存的元数据（AudioSession的元数据可能还未解析完成）
        if (!isLocal && !m_networkMusicArtist.isEmpty()) {
            artist = m_networkMusicArtist;
            qDebug() << "[MainWidget] Using cached network artist:" << artist;
        }
        
        // 如果没有标题，从文件路径提取文件名
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
        
        // 调用API添加到播放历史
        request->addPlayHistory(userAccount, filePath, title, artist, album, 
                                QString::number(durationMs / 1000), isLocal);
    });
    
    // ============ 监听播放停止事件，清除播放状态高亮 ============
    connect(&AudioService::instance(), &AudioService::playbackStopped, this, [=]() {
        qDebug() << "[MainWidget] playbackStopped signal received, clearing currentPlayingPath";
        playHistoryWidget->setCurrentPlayingPath("");
        favoriteMusicWidget->setCurrentPlayingPath("");
    });

    connect(w,&PlayWidget::signal_big_clicked,this,[=](bool checked){
        if(checked)
        {
            // 歌词页面展开 - 只隐藏 QML 控件（QWidget 会被 PlayWidget 遮罩自然遮住）
            searchBox->hide();        // 隐藏搜索框 QML 控件
            userWidgetQml->hide();    // 隐藏用户头像 QML 控件
            
            w->raise();
            w->clearMask();
            w->set_isUp(true);
            update();
        }
        else
        {
            // 歌词页面关闭 - 显示 QML 控件
            searchBox->show();        // 显示搜索框 QML 控件
            userWidgetQml->show();    // 显示用户头像 QML 控件
            
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
    
    // 确保topWidget始终占据整个顶部区域
    if (topWidget) {
        topWidget->setGeometry(0, 0, this->width(), 60);
        topWidget->raise(); // 确保始终在最前面
    }
}

MainWidget::~MainWidget()
{
    qDebug() << "MainWidget::~MainWidget() - Starting cleanup...";
    
    // 1. 清理播放器窗口（会触发 PlayWidget 的析构，停止所有线程）
    if(w) {
        qDebug() << "MainWidget: Deleting PlayWidget...";
        w->deleteLater();
        w = nullptr;
    }
    
    // 2. 等待 Qt 事件循环处理 deleteLater
    QCoreApplication::processEvents();
    QThread::msleep(100);
    
    // 3. 清理其他窗口
    if(list) {
        list->deleteLater();
        list = nullptr;
    }
    
    if(loginWidget) {
        loginWidget->close();
        loginWidget->deleteLater();
        loginWidget = nullptr;
    }
    
    // 清理视频播放窗口
    if(videoPlayerWindow) {
        qDebug() << "MainWidget: Deleting VideoPlayerWindow...";
        videoPlayerWindow->close();
        videoPlayerWindow->deleteLater();
        videoPlayerWindow = nullptr;
    }
    
    // 4. 等待所有 QThreadPool 任务完成
    qDebug() << "MainWidget: Waiting for thread pool...";
    QThreadPool::globalInstance()->waitForDone(3000);  // 等待最多3秒
    
    // 5. 卸载所有插件
    qDebug() << "MainWidget: Unloading plugins...";
    PluginManager::instance().unloadAllPlugins();
    
    qDebug() << "MainWidget::~MainWidget() - Cleanup complete";
}

