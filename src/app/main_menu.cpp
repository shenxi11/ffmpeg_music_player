#include "main_menu.h"

MainMenu::MainMenu(QWidget *parent)
    : QWidget(parent)
    , menuLayout(nullptr)
    , diagnosticsBtn(nullptr)
    , settingsBtn(nullptr)
    , aboutBtn(nullptr)
    , hideTimer(nullptr)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    hideTimer->setInterval(200);
    connect(hideTimer, &QTimer::timeout, this, &MainMenu::hideMenu);

    setupUI();
    createPluginButtons();

    const int buttonCount = pluginButtons.size() + 4; // 插件 + 标题 + 诊断 + 设置 + 关于
    setFixedSize(220, 60 + buttonCount * 48);
}

void MainMenu::setupUI()
{
    menuLayout = new QVBoxLayout(this);
    menuLayout->setContentsMargins(15, 15, 15, 15);
    menuLayout->setSpacing(8);

    QLabel* titleLabel = new QLabel(QStringLiteral("工具菜单"), this);
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
    for (QPushButton* btn : pluginButtons) {
        menuLayout->removeWidget(btn);
        delete btn;
    }
    pluginButtons.clear();

    PluginManager& manager = PluginManager::instance();
    const QVector<PluginInfo> plugins = manager.getPluginInfos();

    qDebug() << "Creating buttons for" << plugins.size() << "plugins";

    for (const PluginInfo& info : plugins) {
        QPushButton* btn = new QPushButton(this);

        if (!info.icon.isNull()) {
            btn->setIcon(info.icon);
            btn->setText(info.name);
        } else {
            btn->setText(info.name);
        }

        btn->setFixedHeight(40);
        btn->setStyleSheet(createButtonStyle());
        btn->setProperty("pluginId", info.id);
        btn->setProperty("pluginName", info.name);

        connect(btn, &QPushButton::clicked, this, &MainMenu::onPluginButtonClicked);

        menuLayout->addWidget(btn);
        pluginButtons.append(btn);

        qDebug() << "Added plugin button:" << info.name << "id:" << info.id;
    }

    if (!plugins.isEmpty()) {
        QFrame* line = new QFrame(this);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setStyleSheet("background-color: #ddd;");
        menuLayout->addWidget(line);
    }

    diagnosticsBtn = new QPushButton(QStringLiteral("插件诊断"), this);
    diagnosticsBtn->setFixedHeight(40);
    diagnosticsBtn->setStyleSheet(createButtonStyle());
    connect(diagnosticsBtn, &QPushButton::clicked, this, &MainMenu::onPluginDiagnosticsClicked);
    menuLayout->addWidget(diagnosticsBtn);

    settingsBtn = new QPushButton(QStringLiteral("设置"), this);
    settingsBtn->setFixedHeight(40);
    settingsBtn->setStyleSheet(createButtonStyle());
    connect(settingsBtn, &QPushButton::clicked, this, &MainMenu::onSettingsClicked);
    menuLayout->addWidget(settingsBtn);

    aboutBtn = new QPushButton(QStringLiteral("关于"), this);
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

    const QRect bgRect = rect().adjusted(5, 5, -5, -5);
    QPainterPath path;
    path.addRoundedRect(bgRect, 10, 10);

    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0, QColor(255, 255, 255, 240));
    gradient.setColorAt(1, QColor(245, 245, 245, 240));

    painter.fillPath(path, QBrush(gradient));
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
    if (!btn) {
        return;
    }

    const QString pluginId = btn->property("pluginId").toString();
    const QString pluginName = btn->property("pluginName").toString();
    qDebug() << "Plugin button clicked, id:" << pluginId << "name:" << pluginName;
    emit pluginRequested(pluginId.isEmpty() ? pluginName : pluginId);
    hide();
}

void MainMenu::onPluginDiagnosticsClicked()
{
    emit pluginDiagnosticsRequested();
    hide();
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

    const int buttonCount = pluginButtons.size() + 4;
    setFixedSize(220, 60 + buttonCount * 48);
}

void MainMenu::showMenu(const QPoint& position)
{
    qDebug() << "MainMenu::showMenu called with position:" << position;

    move(position);
    show();
    raise();
    activateWindow();

    hideTimer->start(5000);
}
