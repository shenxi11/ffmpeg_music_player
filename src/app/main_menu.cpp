#include "main_menu.h"

#include <QGuiApplication>
#include <QScreen>

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
    setupBaseConnections();

    setupUI();
    createPluginButtons();
    refreshPlugins();
}

void MainMenu::setupUI()
{
    menuLayout = new QVBoxLayout(this);
    menuLayout->setContentsMargins(12, 12, 12, 12);
    menuLayout->setSpacing(6);

    QLabel* titleLabel = new QLabel(QStringLiteral("工具菜单"), this);
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #6F7785;"
        "    font-size: 12px;"
        "    font-weight: 600;"
        "    padding: 2px 2px 8px 2px;"
        "}"
    );
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    menuLayout->addWidget(titleLabel);
}

void MainMenu::createPluginButtons()
{
    if (!menuLayout) {
        return;
    }

    // 仅保留第 0 项标题，其余动态项（插件按钮/分隔线/功能按钮/stretch）全部清理，
    // 避免每次刷新时重复叠加。
    while (menuLayout->count() > 1) {
        QLayoutItem* item = menuLayout->takeAt(1);
        if (!item) {
            break;
        }
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    pluginButtons.clear();
    diagnosticsBtn = nullptr;
    settingsBtn = nullptr;
    aboutBtn = nullptr;

    const QVector<PluginInfo> plugins = m_pluginInfos;

    qDebug() << "Creating buttons for" << plugins.size() << "plugins";

    for (const PluginInfo& info : plugins) {
        QPushButton* btn = new QPushButton(this);

        if (!info.icon.isNull()) {
            btn->setIcon(info.icon);
            btn->setText(info.name);
        } else {
            btn->setText(info.name);
        }

        btn->setFixedHeight(38);
        btn->setStyleSheet(createButtonStyle());
        btn->setProperty("pluginId", info.id);
        btn->setProperty("pluginName", info.name);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFocusPolicy(Qt::NoFocus);

        connectPluginButton(btn);

        menuLayout->addWidget(btn);
        pluginButtons.append(btn);

        qDebug() << "Added plugin button:" << info.name << "id:" << info.id;
    }

    if (!plugins.isEmpty()) {
        QFrame* line = new QFrame(this);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setStyleSheet("background-color: #EEF1F5;");
        menuLayout->addWidget(line);
    }

    diagnosticsBtn = new QPushButton(QStringLiteral("插件诊断"), this);
    diagnosticsBtn->setFixedHeight(38);
    diagnosticsBtn->setStyleSheet(createButtonStyle());
    diagnosticsBtn->setCursor(Qt::PointingHandCursor);
    diagnosticsBtn->setFocusPolicy(Qt::NoFocus);
    menuLayout->addWidget(diagnosticsBtn);

    settingsBtn = new QPushButton(QStringLiteral("设置"), this);
    settingsBtn->setFixedHeight(38);
    settingsBtn->setStyleSheet(createButtonStyle());
    settingsBtn->setCursor(Qt::PointingHandCursor);
    settingsBtn->setFocusPolicy(Qt::NoFocus);
    menuLayout->addWidget(settingsBtn);

    aboutBtn = new QPushButton(QStringLiteral("关于"), this);
    aboutBtn->setFixedHeight(38);
    aboutBtn->setStyleSheet(createButtonStyle());
    aboutBtn->setCursor(Qt::PointingHandCursor);
    aboutBtn->setFocusPolicy(Qt::NoFocus);
    menuLayout->addWidget(aboutBtn);

    connectActionButtons();

    menuLayout->addStretch();
}

QString MainMenu::createButtonStyle()
{
    return QString(
        "QPushButton {"
        "    background: transparent;"
        "    border: 1px solid transparent;"
        "    border-radius: 0px;"
        "    padding: 8px 12px;"
        "    font-size: 13px;"
        "    font-weight: 500;"
        "    color: #2F3542;"
        "    text-align: left;"
        "}"
        "QPushButton:hover {"
        "    background: #F6F7FA;"
        "    border: 1px solid #ECEFF5;"
        "    color: #111827;"
        "}"
        "QPushButton:pressed {"
        "    background: #F0F2F7;"
        "    border: 1px solid #E2E7F0;"
        "}"
    );
}

void MainMenu::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRect bgRect = rect().adjusted(2, 2, -2, -2);
    QPainterPath path;
    path.addRect(bgRect);

    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0, QColor(255, 255, 255, 250));
    gradient.setColorAt(1, QColor(250, 251, 253, 248));

    painter.fillPath(path, QBrush(gradient));
    painter.setPen(QPen(QColor(228, 232, 239, 220), 1));
    painter.drawPath(path);
}

void MainMenu::enterEvent(QEvent *event)
{
    QWidget::enterEvent(event);
}

void MainMenu::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
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
    setFixedWidth(268);
    layout()->activate();
    setFixedHeight(sizeHint().height());
}

void MainMenu::setPluginInfos(const QVector<PluginInfo>& plugins)
{
    auto samePlugins = [](const QVector<PluginInfo>& a, const QVector<PluginInfo>& b) -> bool {
        if (a.size() != b.size()) {
            return false;
        }
        for (int i = 0; i < a.size(); ++i) {
            if (a[i].id != b[i].id || a[i].name != b[i].name || a[i].version != b[i].version) {
                return false;
            }
        }
        return true;
    };

    if (samePlugins(m_pluginInfos, plugins)) {
        return;
    }

    m_pluginInfos = plugins;
    refreshPlugins();
}

void MainMenu::showMenu(const QPoint& position)
{
    qDebug() << "MainMenu::showMenu called with position:" << position;

    adjustSize();
    const QScreen* screen = QGuiApplication::screenAt(position);
    const QRect available =
        screen ? screen->availableGeometry() : QGuiApplication::primaryScreen()->availableGeometry();
    const QPoint boundedPos(qBound(available.left(), position.x(),
                                   available.right() - width()),
                            qBound(available.top(), position.y(),
                                   available.bottom() - height()));
    move(boundedPos);
    show();
    raise();
    activateWindow();
}

void MainMenu::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    emit menuVisibilityChanged(true);
}

void MainMenu::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    emit menuVisibilityChanged(false);
}
