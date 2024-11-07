#include "loginwidget.h"

LoginWidget::LoginWidget(QWidget *parent) : QWidget(parent)
{
    this->resize(300, 200);

    request = new HttpRequest(this);

    QWidget* accountWidget = new QWidget(this);
    QLabel* accountLabel = new QLabel(this);
    accountLabel->setText("账号:");
    account = new QLineEdit(this);
    QHBoxLayout* alayout = new QHBoxLayout(this);
    alayout->addWidget(accountLabel);
    alayout->addWidget(account);
    accountWidget->setLayout(alayout);

    QWidget* passwordWidget = new QWidget(this);
    QLabel* passwordLabel = new QLabel(this);
    passwordLabel->setText("密码:");
    password = new QLineEdit(this);
    password->setEchoMode(QLineEdit::Password);
    QHBoxLayout* playout = new QHBoxLayout(this);
    playout->addWidget(passwordLabel);
    playout->addWidget(password);
    passwordWidget->setLayout(playout);

    QWidget* usernameWidget = new QWidget(this);
    QLabel* usernameLabel = new QLabel(this);
    usernameLabel->setText("用户名:");
    username = new QLineEdit(this);
    QHBoxLayout* usLayout = new QHBoxLayout(this);
    usLayout->addWidget(usernameLabel);
    usLayout->addWidget(username);
    usernameWidget->setLayout(usLayout);
    usernameWidget->close();

    QWidget* buttonWidget = new QWidget(this);
    login = new QPushButton(this);
    login->setText("登陆");
    Register = new QLabel("<a href=\"#\">注册</a>",this);
    Register->setTextFormat(Qt::RichText);
    Register->setTextInteractionFlags(Qt::TextBrowserInteraction);
    Register->setOpenExternalLinks(false);
    QHBoxLayout* llayout = new QHBoxLayout(this);
    llayout->addWidget(login);
    llayout->addSpacerItem(new QSpacerItem(10, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    llayout->addWidget(Register);
    buttonWidget->setLayout(llayout);

    QVBoxLayout* vlayout = new QVBoxLayout(this);
    vlayout->addWidget(accountWidget);
    vlayout->addWidget(passwordWidget);
    vlayout->addWidget(buttonWidget);

    setLayout(vlayout);

    connect(request, &HttpRequest::Registerflag, this, [=](bool flag){
        if(!isLogin && flag)
        {
            login->setText("登陆");
            Register->show();
            vlayout->removeWidget(usernameWidget);
            usernameWidget->setVisible(isLogin);
            isLogin = true;
        }
    });

    connect(login, &QPushButton::clicked, this, [=](){
        if(account->text().size() > 0 && password->text().size() > 0)
        {
            if(isLogin)
            {
                QString Account = account->text();
                QString Password = password->text();

                request->Login(Account, Password);
            }
            else if(!isLogin && username->text().size() > 0)
            {
                QString Account = account->text();
                QString Password = password->text();
                QString Username = username->text();

                request->Register(Account, Password, Username);
            }
        }
    });
    connect(Register, &QLabel::linkActivated, this, [=](){
        if(isLogin)
        {
            login->setText("注册");
            Register->close();
            vlayout->insertWidget(2, usernameWidget);
            usernameWidget->setVisible(isLogin);
            isLogin = false;
        }
    });
}
