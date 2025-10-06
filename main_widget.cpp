#include "main_widget.h"
#include "plugin_manager.h"
#include <QCoreApplication>

MainWidget::MainWidget(QWidget *parent) : QWidget(parent)
  ,w(nullptr)
  ,list(nullptr)
{
    resize(1000,600);
    setWindowFlags(Qt::CustomizeWindowHint);
    
    // 初始化插件管理器，加载所有插件
    PluginManager& pluginManager = PluginManager::instance();
    QString pluginPath = QCoreApplication::applicationDirPath() + "/plugin";
    int loadedCount = pluginManager.loadPlugins(pluginPath);
    qDebug() << "Loaded" << loadedCount << "plugins from" << pluginPath;

    topWidget = new QWidget(this);
    QPushButton* minimizeButton = new QPushButton(topWidget);
    minimizeButton->setStyleSheet("QPushButton {"
                                  "    border-image: url(:/new/prefix1/icon/square_unselected.png);"
                                  "}");
    minimizeButton->setFixedSize(30, 30);

    QPushButton* maximizeButton = new QPushButton(topWidget);
    maximizeButton->setStyleSheet("QPushButton {"
                                  "    border-image: url(:/new/prefix1/icon/minus_sign.png);"
                                  "}");
    maximizeButton->setFixedSize(30, 30);

    QPushButton* closeButton = new QPushButton(topWidget);
    closeButton->setStyleSheet("QPushButton {"
                               "    border-image: url(:/new/prefix1/icon/close1.png);"
                               "}");
    closeButton->setFixedSize(30, 30);

    connect(minimizeButton, &QPushButton::clicked, this, [=](){
        if(isMaximized())
            showNormal();
        else
            showMaximized();
    });
    connect(maximizeButton, &QPushButton::clicked, this, &MainWidget::showMinimized);
    connect(closeButton, &QPushButton::clicked, this, &MainWidget::close);

    SearchBox* searchBox = new SearchBox(this);
    searchBox->setFixedSize(250, 60); // 进一步增加搜索框高度以完整显示音符

    // 使用新的用户控件
    userWidget = new UserWidget(this);
    userWidget->setFixedSize(110, 50); // 进一步增加用户控件尺寸以完整显示"未登录"
    
    // 创建主菜单按钮
    menuButton = new QPushButton("☰", this);
    menuButton->setFixedSize(50, 50);
    menuButton->setStyleSheet(
        "QPushButton {"
        "    background: transparent;"
        "    border: none;"
        "    font-size: 20px;"
        "    color: #333;"
        "    border-radius: 25px;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(0, 122, 204, 0.1);"
        "}"
        "QPushButton:pressed {"
        "    background: rgba(0, 122, 204, 0.2);"
        "}"
    );
    
    // 初始化菜单指针为空，按需创建
    mainMenu = nullptr;

    QHBoxLayout* widget_op_layout = new QHBoxLayout(this);

    widget_op_layout->addSpacing(210);
    
    widget_op_layout->addWidget(searchBox);
    widget_op_layout->addStretch();
    widget_op_layout->addWidget(userWidget);
    widget_op_layout->addWidget(menuButton);
    widget_op_layout->addWidget(maximizeButton);
    widget_op_layout->addWidget(minimizeButton);
    widget_op_layout->addWidget(closeButton);
    widget_op_layout->setSpacing(10);
    widget_op_layout->setContentsMargins(10, 5, 10, 5);


    topWidget->setLayout(widget_op_layout);
    topWidget->setGeometry(0, 0, this->width(), 60);
    topWidget->raise();

    loginWidget = new LoginWidget();
    loginWidget->setWindowTitle("登陆");
    loginWidget->setWindowFlags(loginWidget->windowFlags() | Qt::WindowStaysOnTopHint);
    loginWidget->close();

    main_list = new MusicListWidgetLocal(this);
    main_list->setFixedSize(800,400);
    main_list->move((this->width()-800),this->height()- 500);
    main_list->show();
    main_list->setObjectName("local");


    list = new MusicListWidget(this);
    list->setFixedSize(200,300);
    list->move(this->width() - 200, 60);
    list->clear();
    list->close();

    net_list = new MusicListWidgetNet(this);
    net_list->setFixedSize(800, 400);
    net_list->move(main_list->pos());
    net_list->hide();
    net_list->setObjectName("net");

    translateWidget = new TranslateWidget(this);
    translateWidget->setFixedSize(800, 400);
    translateWidget->move(main_list->pos());
    translateWidget->hide();

    QWidget* leftWidget = new QWidget(this);
    leftWidget->setFixedSize(200, this->height() - 100);
    leftWidget->setStyleSheet("background-color: #F0F3F6;");

    QPushButton* localList = new QPushButton("本地音乐", leftWidget);
    localList->setFixedSize(200,50);
    localList->move(0,this->height()- 500);
    localList->setCheckable(true);

    QPushButton* NetList = new QPushButton("在线音乐", leftWidget);
    NetList->setFixedSize(200,50);
    NetList->move(0,this->height()- 450);
    NetList->setCheckable(true);
    NetList->setStyleSheet(
                "background-color: transparent;"
                "color: black;"
                "border: none;"
                );

    QPushButton* translateBtn = new QPushButton("语音转换", leftWidget);
    translateBtn->setFixedSize(200,50);
    translateBtn->move(0,this->height()- 400);
    translateBtn->setCheckable(true);
    translateBtn->setStyleSheet(
                "background-color: transparent;"
                "color: black;"
                "border: none;"
                );

    QButtonGroup* leftButtons = new QButtonGroup(this);
    leftButtons->addButton(localList);
    leftButtons->addButton(NetList);
    leftButtons->addButton(translateBtn);
    leftButtons->setExclusive(true);

    QWidget* textWidget = new QWidget(leftWidget);

    QLabel* icolabel = new QLabel(textWidget);
    icolabel->setPixmap(QPixmap(":/new/prefix1/icon/netease.ico"));
    icolabel->setScaledContents(true);
    icolabel->setFixedSize(40, 40);

    QLabel *textLabel = new QLabel("网易云音乐", textWidget);
    QFont font;
    font.setFamily("Brush Script MT");
    font.setPointSize(20);
    textLabel->setFont(font);
    textLabel->adjustSize();

    QHBoxLayout* layout_text = new QHBoxLayout(textWidget);
    layout_text->addWidget(icolabel);
    layout_text->addWidget(textLabel);

    textWidget->setLayout(layout_text);
    textWidget->move(5, 10);

    connect(localList, &QPushButton::toggled, this, [=](bool checked) {
        if (checked) {
            main_list->show();
            net_list->hide();
            translateWidget->hide();
            localList->setStyleSheet(
                        "background-color: rgba(44, 210, 126, 0.8);"
                        "color: white;"
                        "border: none;"
                        );
        } else {
            localList->setStyleSheet(
                        "background-color: transparent;"
                        "color: black;"
                        "border: none;"
                        );
        }
    });

    connect(NetList, &QPushButton::toggled, this, [=](bool checked){
        if(checked)
        {
            net_list->show();
            main_list->hide();
            translateWidget->hide();
            NetList->setStyleSheet(
                        "background-color: rgba(44, 210, 126, 0.8);"
                        "color: white;"
                        "border: none;"
                        );
        }
        else
        {
            NetList->setStyleSheet(
                        "background-color: transparent;"
                        "color: black;"
                        "border: none;"
                        );
        }
    });

    connect(translateBtn, &QPushButton::toggled, this, [=](bool checked){
        if(checked)
        {
            translateWidget->show();
            main_list->hide();
            net_list->hide();
            translateBtn->setStyleSheet(
                        "background-color: rgba(44, 210, 126, 0.8);"
                        "color: white;"
                        "border: none;"
                        );
        }
        else
        {
            translateBtn->setStyleSheet(
                        "background-color: transparent;"
                        "color: black;"
                        "border: none;"
                        );
        }
    });
    localList->setChecked(true);



    w = new PlayWidget(this);
    w->setFixedSize(1000,600);
    QRegion region(0, 500, w->width(), 500);
    w->setMask(region);
    w->move(0, this->height()-w->height());

    request = HttpRequestPool::getInstance().getRequest();


    connect(searchBox, &SearchBox::search, request, &HttpRequest::getMusic);
    connect(searchBox, &SearchBox::searchAll, request, &HttpRequest::getAllFiles);
    connect(request, &HttpRequest::signal_addSong_list, net_list, &MusicListWidgetNet::signal_add_songlist);
    connect(request, &HttpRequest::signal_addSong_list, this, [=](){NetList->setChecked(true);});

    // 连接本地音乐列表的翻译按钮
    connect(main_list, &MusicListWidgetLocal::signal_translate_button_clicked, this, [=](){
        translateBtn->setChecked(true);
    });

    // 连接网络音乐列表的翻译按钮
    connect(net_list, &MusicListWidgetNet::signal_translate_button_clicked, this, [=](){
        translateBtn->setChecked(true);
    });

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
                // TODO: 实现设置页面
                // settingWidget->show();
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
    connect(userWidget, &UserWidget::loginRequested, this, [=](){
        // 现在这个信号应该来自弹出窗口中的登录按钮，而不是直接点击头像
        loginWidget->isVisible = !loginWidget->isVisible;

        this->loginWidget->setVisible(loginWidget->isVisible);
        if(loginWidget->isVisible)
        {
            QScreen *screen = QGuiApplication::primaryScreen();
            QRect screenGeometry = screen->geometry();

            int x = (screenGeometry.width() - loginWidget->width()) / 2;
            int y = (screenGeometry.height() - loginWidget->height()) / 2;

            loginWidget->move(x, y);
        }
    });

    connect(userWidget, &UserWidget::logoutRequested, this, [=](){
        // 处理退出登录逻辑
        userWidget->setLoginState(false);
        // 这里可以添加清除用户数据的代码
    });

    connect(loginWidget, &LoginWidget::login_, this, [=](QString username){
        // 登录成功后更新用户控件
        QPixmap userAvatar(":/new/prefix1/icon/denglu.png"); // 可以根据需要设置用户头像
        userWidget->setUserInfo(username, userAvatar);
        userWidget->setLoginState(true);
        loginWidget->close();
    });
    connect(main_list, &MusicListWidgetLocal::signal_add_button_clicked, w, &PlayWidget::openfile);


    connect(w, &PlayWidget::signal_Last, main_list, &MusicListWidgetLocal::signal_last);
    connect(w, &PlayWidget::signal_Next, main_list, &MusicListWidgetLocal::signal_next);

    connect(w, &PlayWidget::signal_Last, net_list, &MusicListWidgetNet::signal_last);
    connect(w, &PlayWidget::signal_Next, net_list, &MusicListWidgetNet::signal_next);

    connect(w,&PlayWidget::signal_add_song,main_list,&MusicListWidgetLocal::on_signal_add_song);
    connect(w, &PlayWidget::signal_play_button_click,main_list,&MusicListWidgetLocal::on_signal_play_button_click);
    connect(w, &PlayWidget::signal_play_button_click, net_list, &MusicListWidgetNet::on_signal_play_button_click);

    connect(main_list, &MusicListWidgetLocal::signal_play_click, w, [=](const QString name, bool flag){
        w->set_play_net(flag);
        w->_play_click(name);

    });
    connect(main_list, &MusicListWidgetLocal::signal_remove_click, w, &PlayWidget::_remove_click);

    connect(net_list, &MusicListWidgetNet::signal_play_click, w, [=](const QString name, bool flag){
        w->set_play_net(flag);
        w->_play_click(name);
    });
    connect(net_list, &MusicListWidgetNet::signal_choose_download_dir, this, &MainWidget::on_signal_choose_download_dir);

    connect(w,&PlayWidget::signal_big_clicked,this,[=](bool checked){
        if(checked)
        {
            w->raise();
            w->clearMask();
            w->set_isUp(true);
            update();
        }
        else
        {
            w->lower();
            QRegion region1(0, 500, w->width(), 500);
            w->setMask(region1);
            w->set_isUp(false);
            update();
            w->setPianWidgetEnable(false);
        }
    });
}
void MainWidget::on_signal_choose_download_dir()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, "选择文件夹", QString(),
                                                            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(folderPath.size() > 0)
    {
        net_list->on_signal_set_down_dir(folderPath);
    }
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

}

