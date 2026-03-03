#include "user_widget.h"

// UserPopupWidget 实现
UserPopupWidget::UserPopupWidget(QWidget *parent)
    : QWidget(parent), isLoggedIn(false)
{
    // 修改窗口标志，避免渲染问题
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::NoFocus); // 避免焦点问题
    setFixedSize(180, 140); // 减小弹出窗口尺寸
    
    setupUI();
    
    // 移除阴影效果以避免渲染问题
    // QGraphicsDropShadowEffect 在某些情况下会导致 UpdateLayeredWindowIndirect 错误
}

void UserPopupWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);
    
    // 头像
    avatarLabel = new QLabel(this);
    avatarLabel->setFixedSize(50, 50); // 减小头像尺寸
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setScaledContents(true);
    // 移除样式，使用纯图片显示
    
    // 用户名
    usernameLabel = new QLabel("未登录", this);
    usernameLabel->setAlignment(Qt::AlignCenter);
    usernameLabel->setStyleSheet(
        "QLabel {"
        "    color: #333;"
        "    font-size: 13px;" // 调整字体大小
        "    font-weight: bold;"
        "}"
    );
    
    // 状态标签
    statusLabel = new QLabel("点击下方按钮登录", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet(
        "QLabel {"
        "    color: #666;"
        "    font-size: 10px;" // 调整字体大小
        "}"
    );
    statusLabel->setWordWrap(true);
    
    // 操作按钮
    actionButton = new QPushButton("我要登录/注册", this);
    actionButton->setFixedHeight(28); // 减小按钮高度
    actionButton->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #007acc, stop:1 #005fa3);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 16px;"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "    padding: 6px 12px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #0086e6, stop:1 #006bb3);"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 #005fa3, stop:1 #004d82);"
        "}"
    );
    
    mainLayout->addWidget(avatarLabel, 0, Qt::AlignHCenter);
    mainLayout->addWidget(usernameLabel);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(actionButton);
    
    connect(actionButton, &QPushButton::clicked, this, &UserPopupWidget::onActionButtonClicked);
}

void UserPopupWidget::setUserInfo(const QString &username, const QPixmap &avatar)
{
    usernameLabel->setText(username);
    if (!avatar.isNull()) {
        // 直接使用原始图片，不做圆形裁剪
        QPixmap scaledAvatar = avatar.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        avatarLabel->setPixmap(scaledAvatar);
    } else {
        // 设置默认头像，也不做圆形裁剪
        QPixmap defaultAvatar(":/new/prefix1/icon/denglu.png");
        if (!defaultAvatar.isNull()) {
            QPixmap scaledAvatar = defaultAvatar.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            avatarLabel->setPixmap(scaledAvatar);
        }
    }
}

void UserPopupWidget::setLoginState(bool loggedIn)
{
    isLoggedIn = loggedIn;
    updateContent();
}

void UserPopupWidget::updateContent()
{
    if (isLoggedIn) {
        statusLabel->setText("已登录");
        actionButton->setText("退出登录");
        actionButton->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "                              stop:0 #dc3545, stop:1 #c82333);"
            "    color: white;"
            "    border: none;"
            "    border-radius: 16px;"
            "    font-size: 12px;"
            "    font-weight: bold;"
            "    padding: 6px 12px;"
            "}"
            "QPushButton:hover {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "                              stop:0 #e74c3c, stop:1 #d62c1a);"
            "}"
            "QPushButton:pressed {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "                              stop:0 #c82333, stop:1 #a71e2a);"
            "}"
        );
    } else {
        usernameLabel->setText("未登录");
        statusLabel->setText("点击下方按钮登录");
        actionButton->setText("我要登录/注册");
        actionButton->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "                              stop:0 #007acc, stop:1 #005fa3);"
            "    color: white;"
            "    border: none;"
            "    border-radius: 16px;"
            "    font-size: 12px;"
            "    font-weight: bold;"
            "    padding: 6px 12px;"
            "}"
            "QPushButton:hover {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "                              stop:0 #0086e6, stop:1 #006bb3);"
            "}"
            "QPushButton:pressed {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "                              stop:0 #005fa3, stop:1 #004d82);"
            "}"
        );
        
        // 设置默认头像
        QPixmap defaultAvatar(":/new/prefix1/icon/denglu.png");
        if (!defaultAvatar.isNull()) {
            setUserInfo("未登录", defaultAvatar);
        }
    }
}

void UserPopupWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    QRect bgRect = rect().adjusted(5, 5, -5, -5);
    QPainterPath path;
    path.addRoundedRect(bgRect, 12, 12);
    
    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0, QColor(255, 255, 255, 250));
    gradient.setColorAt(1, QColor(248, 250, 252, 250));
    
    painter.fillPath(path, QBrush(gradient));
    
    // 绘制边框
    painter.setPen(QPen(QColor(220, 220, 220), 1));
    painter.drawPath(path);
}

void UserPopupWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    // 添加小延迟，避免鼠标快速移动时的闪烁
    QTimer::singleShot(100, this, [this]() {
        if (!underMouse()) {
            hide();
        }
    });
}

void UserPopupWidget::onActionButtonClicked()
{
    if (isLoggedIn) {
        emit logoutRequested();
    } else {
        emit loginRequested();
    }
    hide();
}

// UserWidget 实现
UserWidget::UserWidget(QWidget *parent)
    : QWidget(parent), isLoggedIn(false)
{
    setFixedSize(150, 40);
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
    
    setupUI();
    
    // 初始化定时器
    hoverTimer = new QTimer(this);
    hoverTimer->setSingleShot(true);
    hoverTimer->setInterval(300); // 300ms 延迟显示
    
    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    hideTimer->setInterval(200); // 200ms 延迟隐藏
    
    connect(hoverTimer, &QTimer::timeout, this, &UserWidget::showPopup);
    connect(hideTimer, &QTimer::timeout, this, &UserWidget::hidePopup);
    
    // 创建弹出窗口
    popup = new UserPopupWidget(this);
    popup->hide();
    
    connect(popup, &UserPopupWidget::loginRequested, this, &UserWidget::onPopupActionRequested);
    connect(popup, &UserPopupWidget::logoutRequested, this, [this](){
        setLoginState(false);
        emit logoutRequested();
    });
    
    // 设置初始状态
    setLoginState(false);
}

void UserWidget::setupUI()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 6, 10, 6); // 增加内边距，特别是垂直方向
    layout->setSpacing(10); // 增加控件间距
    
    // 头像
    avatarLabel = new QLabel(this);
    avatarLabel->setFixedSize(32, 32);
    avatarLabel->setScaledContents(true);
    // 移除样式，使用纯图片显示
    
    // 用户名
    usernameLabel = new QLabel("未登录", this);
    usernameLabel->setStyleSheet(
        "QLabel {"
        "    color: #333;"
        "    font-size: 14px;" // 进一步增加字体大小
        "    font-weight: bold;"
        "    padding: 4px;" // 增加内边距
        "}"
    );
    
    layout->addWidget(avatarLabel);
    layout->addWidget(usernameLabel);
    layout->addStretch();
    
    setLayout(layout);
}

void UserWidget::setUserInfo(const QString &username, const QPixmap &avatar)
{
    currentUsername = username;
    currentAvatar = avatar;
    updateDisplay();
    
    // 同步到弹出窗口
    popup->setUserInfo(username, avatar);
}

void UserWidget::setLoginState(bool loggedIn)
{
    isLoggedIn = loggedIn;
    popup->setLoginState(loggedIn);
    updateDisplay();
}

void UserWidget::updateDisplay()
{
    if (isLoggedIn && !currentUsername.isEmpty()) {
        usernameLabel->setText(currentUsername);
        if (!currentAvatar.isNull()) {
            // 直接使用原始图片，不做圆形裁剪
            QPixmap scaledAvatar = currentAvatar.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            avatarLabel->setPixmap(scaledAvatar);
        }
    } else {
        usernameLabel->setText("未登录");
        QPixmap defaultAvatar(":/new/prefix1/icon/denglu.png");
        if (!defaultAvatar.isNull()) {
            // 直接使用原始图片，不做圆形裁剪
            QPixmap scaledAvatar = defaultAvatar.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            avatarLabel->setPixmap(scaledAvatar);
        }
    }
}

QPixmap UserWidget::createCircularAvatar(const QPixmap &source, int size)
{
    if (source.isNull()) return QPixmap();
    
    QPixmap scaled = source.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap result(size, size);
    result.fill(Qt::transparent);
    
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);
    
    // 居中绘制
    int x = (size - scaled.width()) / 2;
    int y = (size - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
    
    return result;
}

void UserWidget::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    hideTimer->stop();
    hoverTimer->start();
    
    // 更新样式
    setStyleSheet(
        "UserWidget {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                              stop:0 rgba(0, 122, 204, 0.1), "
        "                              stop:1 rgba(0, 122, 204, 0.05));"
        "    border: 1px solid rgba(0, 122, 204, 0.3);"
        "    border-radius: 20px;"
        "}"
    );
}

void UserWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    hoverTimer->stop();
    hideTimer->start();
    
    // 恢复样式
    // 移除背景样式，使用透明背景让图片更清晰
    setStyleSheet(
        "UserWidget {"
        "    background: transparent;"
        "    border: none;"
        "}"
        "UserWidget:hover {"
        "    background: rgba(0, 122, 204, 0.1);"
        "    border-radius: 10px;"
        "}"
    );
}

void UserWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 只显示弹出窗口，不直接触发登录操作
        if (popup->isVisible()) {
            hidePopup();
        } else {
            showPopup();
        }
    }
    QWidget::mousePressEvent(event);
}

void UserWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 基础样式已通过 stylesheet 处理
    QWidget::paintEvent(event);
}

void UserWidget::showPopup()
{
    if (popup->isVisible()) return;
    
    // 计算弹出位置
    QPoint globalPos = mapToGlobal(QPoint(0, height() + 5));
    
    // 确保弹出窗口在屏幕内
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    if (globalPos.x() + popup->width() > screenGeometry.right()) {
        globalPos.setX(screenGeometry.right() - popup->width());
    }
    if (globalPos.y() + popup->height() > screenGeometry.bottom()) {
        globalPos.setY(mapToGlobal(QPoint(0, -popup->height() - 5)).y());
    }
    
    popup->move(globalPos);
    popup->show();
    popup->raise();
}

void UserWidget::hidePopup()
{
    if (!popup->underMouse() && !this->underMouse()) {
        popup->hide();
    }
}

void UserWidget::onPopupActionRequested()
{
    if (isLoggedIn) {
        setLoginState(false);
        emit logoutRequested();
    } else {
        emit loginRequested();
    }
}
