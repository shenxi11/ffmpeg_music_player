#include "main_menu.h"

MainMenu::MainMenu(QWidget *parent)
    : QWidget(parent), menuLayout(nullptr)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    // 初始化定时器
    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    hideTimer->setInterval(200);
    connect(hideTimer, &QTimer::timeout, this, &MainMenu::hideMenu);
    
    setupUI();
    createPluginButtons(); // 创建插件按钮
    
    // 调整大小以适应插件数量
    int buttonCount = pluginButtons.size() + 3; // 插件数 + 标题 + 设置 + 关于
    setFixedSize(200, 60 + buttonCount * 48);
}

void MainMenu::setupUI()
{
    menuLayout = new QVBoxLayout(this);
    menuLayout->setContentsMargins(15, 15, 15, 15);
    menuLayout->setSpacing(8);
    
    // 标题
    QLabel* titleLabel = new QLabel("工具菜单", this);
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #333;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    padding: 5px 0px;"
        "}"
    );
    titleLabel->setAlignment(Qt::AlignCenter);
    menuLayout->addWidget(titleLabel);
}

void MainMenu::createPluginButtons()
{
    // 清除旧的插件按钮
    for (QPushButton* btn : pluginButtons) {
        menuLayout->removeWidget(btn);
        delete btn;
    }
    pluginButtons.clear();
    
    // 获取所有插件信息
    PluginManager& manager = PluginManager::instance();
    QVector<PluginInfo> plugins = manager.getPluginInfos();
    
    qDebug() << "Creating buttons for" << plugins.size() << "plugins";
    
    // 为每个插件创建按钮
    for (const PluginInfo& info : plugins) {
        QPushButton* btn = new QPushButton(this);
        
        // 设置按钮文本和图标
        if (!info.icon.isNull()) {
            btn->setIcon(info.icon);
            btn->setText(info.name);
        } else {
            btn->setText("🔧 " + info.name);
        }
        
        btn->setFixedHeight(40);
        btn->setStyleSheet(createButtonStyle());
        btn->setProperty("pluginName", info.name); // 存储插件名称
        
        // 连接信号
        connect(btn, &QPushButton::clicked, this, &MainMenu::onPluginButtonClicked);
        
        menuLayout->addWidget(btn);
        pluginButtons.append(btn);
        
        qDebug() << "Added plugin button:" << info.name;
    }
    
    // 添加分隔线
    if (!plugins.isEmpty()) {
        QFrame* line = new QFrame(this);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setStyleSheet("background-color: #ddd;");
        menuLayout->addWidget(line);
    }
    
    // 设置按钮
    settingsBtn = new QPushButton("⚙️ 设置", this);
    settingsBtn->setFixedHeight(40);
    settingsBtn->setStyleSheet(createButtonStyle());
    connect(settingsBtn, &QPushButton::clicked, this, &MainMenu::onSettingsClicked);
    menuLayout->addWidget(settingsBtn);
    
    // 关于按钮
    aboutBtn = new QPushButton("ℹ️ 关于", this);
    aboutBtn->setFixedHeight(40);
    aboutBtn->setStyleSheet(createButtonStyle());
    connect(aboutBtn, &QPushButton::clicked, this, &MainMenu::onAboutClicked);
    menuLayout->addWidget(aboutBtn);
    
    menuLayout->addStretch();
}

QString MainMenu::createButtonStyle()
{
    return QString(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #ffffff, stop:1 #f5f5f5);"
        "    border: 1px solid #ddd;"
        "    border-radius: 8px;"
        "    padding: 8px 15px;"
        "    font-size: 14px;"
        "    color: #333;"
        "    text-align: left;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #e8f4fd, stop:1 #d1e7f0);"
        "    border: 1px solid #007acc;"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #d1e7f0, stop:1 #b3d9f2);"
        "    border: 1px solid #005fa3;"
        "}"
    );
}

void MainMenu::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    QRect bgRect = rect().adjusted(5, 5, -5, -5);
    QPainterPath path;
    path.addRoundedRect(bgRect, 10, 10);
    
    // 背景渐变
    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0, QColor(255, 255, 255, 240));
    gradient.setColorAt(1, QColor(245, 245, 245, 240));
    
    painter.fillPath(path, QBrush(gradient));
    
    // 边框
    painter.setPen(QPen(QColor(200, 200, 200, 180), 1));
    painter.drawPath(path);
}

void MainMenu::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    hideTimer->stop();
}

void MainMenu::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    hideTimer->start();
}

void MainMenu::hideMenu()
{
    if (!underMouse()) {
        hide();
    }
}

void MainMenu::onPluginButtonClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        QString pluginName = btn->property("pluginName").toString();
        qDebug() << "Plugin button clicked:" << pluginName;
        emit pluginRequested(pluginName);
        hide();
    }
}

void MainMenu::onSettingsClicked()
{
    emit settingsRequested();
    hide();
}

void MainMenu::onAboutClicked()
{
    emit aboutRequested();
    hide();
}

void MainMenu::refreshPlugins()
{
    qDebug() << "Refreshing plugin list...";
    createPluginButtons();
    
    // 调整窗口大小
    int buttonCount = pluginButtons.size() + 3; // 插件数 + 标题 + 设置 + 关于
    setFixedSize(200, 60 + buttonCount * 48);
}

void MainMenu::showMenu(const QPoint& position)
{
    qDebug() << "MainMenu::showMenu called with position:" << position;
    
    // 设置菜单位置
    move(position);
    
    qDebug() << "Menu size:" << size();
    qDebug() << "Menu geometry after move:" << geometry();
    
    // 显示菜单
    show();
    raise();
    activateWindow();
    
    qDebug() << "Menu visibility:" << isVisible();
    
    // 启动自动隐藏定时器（延长时间）
    hideTimer->start(5000); // 5秒后自动隐藏
}
