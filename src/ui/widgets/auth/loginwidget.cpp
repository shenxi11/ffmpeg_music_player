#include "loginwidget.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

LoginWidget::LoginWidget(QWidget* parent)
    : QWidget(parent)
    , m_viewModel(new LoginViewModel(this))
{
    this->resize(450, 380);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    isLogin = true;

    QWidget* mainContainer = new QWidget(this);
    mainContainer->setStyleSheet(
        "QWidget {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "               stop:0 #667eea, stop:1 #764ba2);"
        "    border-radius: 15px;"
        "    border: 1px solid rgba(255, 255, 255, 0.2);"
        "}");

    QWidget* titleBar = new QWidget(mainContainer);
    titleBar->setFixedHeight(50);
    titleBar->setStyleSheet(
        "QWidget {"
        "    background: rgba(255, 255, 255, 0.1);"
        "    border-top-left-radius: 15px;"
        "    border-top-right-radius: 15px;"
        "    border-bottom: 1px solid rgba(255, 255, 255, 0.2);"
        "}");

    QLabel* titleLabel = new QLabel("🎵 云音乐", titleBar);
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: white;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    background: transparent;"
        "}");
    titleLabel->setAlignment(Qt::AlignCenter);

    QPushButton* closeButton = new QPushButton("✕", titleBar);
    closeButton->setFixedSize(35, 35);
    closeButton->setStyleSheet(
        "QPushButton {"
        "    background: rgba(255, 255, 255, 0.1);"
        "    color: white;"
        "    font-size: 16px;"
        "    border: none;"
        "    border-radius: 17px;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(255, 71, 102, 0.8);"
        "}"
        "QPushButton:pressed {"
        "    background: rgba(255, 71, 102, 1);"
        "}");
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);

    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(closeButton);
    titleLayout->setContentsMargins(15, 0, 15, 0);

    QWidget* contentArea = new QWidget(mainContainer);
    contentArea->setStyleSheet("background: transparent;");

    QLabel* logoLabel = new QLabel("🎵", contentArea);
    logoLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 48px;"
        "    color: white;"
        "    background: transparent;"
        "}");
    logoLabel->setAlignment(Qt::AlignCenter);

    QLabel* welcomeLabel = new QLabel("欢迎登录", contentArea);
    welcomeLabel->setStyleSheet(
        "QLabel {"
        "    color: white;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    background: transparent;"
        "    margin-bottom: 10px;"
        "}");
    welcomeLabel->setAlignment(Qt::AlignCenter);

    QString inputStyle =
        "QLineEdit {"
        "    background: rgba(255, 255, 255, 0.9);"
        "    border: 2px solid rgba(255, 255, 255, 0.3);"
        "    border-radius: 8px;"
        "    padding: 12px 15px;"
        "    font-size: 14px;"
        "    color: #333;"
        "}"
        "QLineEdit:focus {"
        "    border: 2px solid rgba(255, 71, 102, 0.8);"
        "    background: rgba(255, 255, 255, 1);"
        "}"
        "QLineEdit::placeholder {"
        "    color: #999;"
        "}";

    account = new QLineEdit(contentArea);
    account->setPlaceholderText("📧 请输入账号");
    account->setFixedHeight(45);
    account->setStyleSheet(inputStyle);

    password = new QLineEdit(contentArea);
    password->setEchoMode(QLineEdit::Password);
    password->setPlaceholderText("🔒 请输入密码");
    password->setFixedHeight(45);
    password->setStyleSheet(inputStyle);

    username = new QLineEdit(contentArea);
    username->setPlaceholderText("👤 请输入用户名");
    username->setFixedHeight(45);
    username->setStyleSheet(inputStyle);
    username->setVisible(false);

    login = new QPushButton("登录", contentArea);
    login->setFixedHeight(45);
    login->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "               stop:0 #ff6b6b, stop:1 #ee5a52);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "               stop:0 #ff5252, stop:1 #e53935);"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "               stop:0 #e53935, stop:1 #d32f2f);"
        "}");

    Register = new QLabel("<a href=\"#\" style='color: white; text-decoration: none;'>还没有账号？点击注册</a>", contentArea);
    Register->setStyleSheet(
        "QLabel {"
        "    color: rgba(255, 255, 255, 0.8);"
        "    font-size: 13px;"
        "    background: transparent;"
        "    padding: 10px;"
        "}"
        "QLabel:hover {"
        "    color: white;"
        "}");
    Register->setTextFormat(Qt::RichText);
    Register->setTextInteractionFlags(Qt::TextBrowserInteraction);
    Register->setOpenExternalLinks(false);
    Register->setAlignment(Qt::AlignCenter);

    QVBoxLayout* contentLayout = new QVBoxLayout(contentArea);
    contentLayout->addWidget(logoLabel);
    contentLayout->addWidget(welcomeLabel);
    contentLayout->addSpacing(20);
    contentLayout->addWidget(account);
    contentLayout->addWidget(password);
    contentLayout->addWidget(username);
    contentLayout->addSpacing(10);
    contentLayout->addWidget(login);
    contentLayout->addSpacing(10);
    contentLayout->addWidget(Register);
    contentLayout->setSpacing(15);
    contentLayout->setContentsMargins(30, 20, 30, 30);

    QVBoxLayout* mainLayout = new QVBoxLayout(mainContainer);
    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(contentArea);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->addWidget(mainContainer);
    outerLayout->setContentsMargins(10, 10, 10, 10);
    setLayout(outerLayout);

    setupAuthConnections(welcomeLabel);
}

void LoginWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        mousePressed = true;
        mouseStartPoint = event->globalPos();
        windowStartPoint = this->frameGeometry().topLeft();
    }
}

void LoginWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (mousePressed) {
        const QPoint distance = event->globalPos() - mouseStartPoint;
        this->move(windowStartPoint + distance);
    }
}

void LoginWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    mousePressed = false;
}
