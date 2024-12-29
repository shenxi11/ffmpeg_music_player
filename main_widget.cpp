#include "main_widget.h"

MainWidget::MainWidget(QWidget *parent) : QWidget(parent)
  ,w(nullptr)
  ,list(nullptr)
{
    resize(1000,600);
    setWindowFlags(Qt::CustomizeWindowHint);

    QWidget* topWidget = new QWidget(this);
    QPushButton* minimizeButton = new QPushButton(topWidget);
    minimizeButton->setStyleSheet("QPushButton {"
                                  "    border-image: url(:/new/prefix1/icon/方形未选中.png);"
                                  "}");
    minimizeButton->setFixedSize(30, 30);

    QPushButton* maximizeButton = new QPushButton(topWidget);
    maximizeButton->setStyleSheet("QPushButton {"
                                  "    border-image: url(:/new/prefix1/icon/减号.png);"
                                  "}");
    maximizeButton->setFixedSize(30, 30);

    QPushButton* closeButton = new QPushButton(topWidget);
    closeButton->setStyleSheet("QPushButton {"
                               "    border-image: url(:/new/prefix1/icon/关闭1.png);"
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
    searchBox->setFixedSize(200,30);

    Login = new QPushButton(this);
    //Login->setFixedSize(100,50);
    Login->setText("未登录");
    Login->setStyleSheet(
                "QPushButton "
                "{ border-radius: 15px; border: 2px solid black; }"
                );

    QLabel* head = new QLabel(this);
    head->setStyleSheet("QLabel {"
                        "    border-image: url(:/new/prefix1/icon/denglu.png);"
                        "}");
    head->setFixedSize(30,30);

    QHBoxLayout* login_layout = new QHBoxLayout(this);
    login_layout->addWidget(head);
    login_layout->addWidget(Login);

    QWidget* lWidget = new QWidget(this);
    lWidget->setLayout(login_layout);

    QHBoxLayout* widget_op_layout = new QHBoxLayout(this);
    widget_op_layout->addWidget(searchBox);
    widget_op_layout->addWidget(lWidget);
    widget_op_layout->addWidget(maximizeButton);
    widget_op_layout->addWidget(minimizeButton);
    widget_op_layout->addWidget(closeButton);


    topWidget->setLayout(widget_op_layout);
    topWidget->move(1000 - 440, 0);

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
    list->move(this->width() - 200,0);
    list->clear();
    list->close();

    net_list = new MusicListWidgetNet(this);
    net_list->setFixedSize(800, 400);
    net_list->move(main_list->pos());
    net_list->hide();
    net_list->setObjectName("net");

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

    QButtonGroup* leftButtons = new QButtonGroup(this);
    leftButtons->addButton(localList);
    leftButtons->addButton(NetList);
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
    localList->setChecked(true);



    w = new PlayWidget(this);
    w->setFixedSize(1000,600);
    QRegion region(0, 500, w->width(), 500);
    w->setMask(region);
    w->move(0, this->height()-w->height());

    auto request = HttpRequest::getInstance();


    connect(searchBox, &SearchBox::search, request, &HttpRequest::getMusic);
    connect(searchBox, &SearchBox::searchAll, request, &HttpRequest::getAllFiles);
    connect(request, &HttpRequest::signal_addSong_list, net_list, &MusicListWidgetNet::signal_add_songlist);
    connect(request, &HttpRequest::signal_addSong_list, this, [=](){NetList->setChecked(true);});

    connect(Login, &QPushButton::clicked, this, [=](){
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

    connect(loginWidget, &LoginWidget::login_, this, [=](QString username){
        Login->setText(username);
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
            w->isUp = true;
            update();
        }
        else
        {
            w->lower();
            QRegion region1(0, 500, w->width(), 500);
            w->setMask(region1);
            w->isUp = false;
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
MainWidget::~MainWidget()
{

}

