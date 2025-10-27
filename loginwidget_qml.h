#ifndef LOGINWIDGET_QML_H
#define LOGINWIDGET_QML_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QDebug>
#include "httprequest.h"

/**
 * @brief QML 版本的 LoginWidget 包装类
 * 提供与原 LoginWidget 相同的接口，用于无缝替换
 */
class LoginWidgetQml : public QObject
{
    Q_OBJECT

public:
    explicit LoginWidgetQml(QObject *parent = nullptr)
        : QObject(parent)
        , m_window(nullptr)
        , m_engine(new QQmlApplicationEngine(this))
        , isVisible(false)
    {
        qDebug() << "LoginWidgetQml: Initializing...";
        
        // 获取 HttpRequest 实例
        request = HttpRequestPool::getInstance().getRequest();
        
        // 加载 QML 文件
        m_engine->load(QUrl("qrc:/qml/components/LoginWindow.qml"));
        
        if (m_engine->rootObjects().isEmpty()) {
            qWarning() << "LoginWidgetQml: Failed to load LoginWindow.qml";
            return;
        }
        
        // 获取窗口对象
        m_window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
        
        if (!m_window) {
            qWarning() << "LoginWidgetQml: Failed to get window object";
            return;
        }
        
        qDebug() << "LoginWidgetQml: QML loaded successfully";
        
        // 连接 QML 信号到 C++ 槽
        connect(m_window, SIGNAL(loginRequested(QString, QString)), 
                this, SLOT(onLoginRequested(QString, QString)));
        
        connect(m_window, SIGNAL(registerRequested(QString, QString, QString)), 
                this, SLOT(onRegisterRequested(QString, QString, QString)));
        
        // 连接 HttpRequest 信号
        connect(request, &HttpRequest::signal_getusername, this, &LoginWidgetQml::onUsernameReceived);
        connect(request, &HttpRequest::signal_Registerflag, this, &LoginWidgetQml::onRegisterResult);
        
        // 监听窗口可见性变化
        connect(m_window, &QQuickWindow::visibleChanged, this, [this]() {
            isVisible = m_window->isVisible();
        });
        
        qDebug() << "LoginWidgetQml: Initialization complete";
    }
    
    ~LoginWidgetQml() override = default;
    
    // 显示/隐藏窗口
    void show() {
        if (m_window) {
            m_window->show();
            m_window->raise();
            m_window->requestActivate();
            isVisible = true;
        }
    }
    
    void close() {
        if (m_window) {
            m_window->close();
            isVisible = false;
        }
    }
    
    void setWindowTitle(const QString &title) {
        if (m_window) {
            m_window->setTitle(title);
        }
    }
    
    // 与原 LoginWidget 兼容的属性
    bool isVisible;

signals:
    // 与原 LoginWidget 保持一致的信号
    void login_(QString username);

private slots:
    /**
     * @brief 处理登录请求
     */
    void onLoginRequested(const QString &account, const QString &password) {
        qDebug() << "LoginWidgetQml: Login requested for account:" << account;
        request->Login(account, password);
    }
    
    /**
     * @brief 处理注册请求
     */
    void onRegisterRequested(const QString &account, const QString &password, const QString &username) {
        qDebug() << "LoginWidgetQml: Register requested for account:" << account << "username:" << username;
        request->Register(account, password, username);
    }
    
    /**
     * @brief 处理服务器返回的用户名
     */
    void onUsernameReceived(const QString &username) {
        if (username.size() > 0) {
            emit login_(username);
            qDebug() << "LoginWidgetQml: Login successful, username:" << username;
            close(); // 登录成功后关闭窗口
            
            // 清空输入框
            if (m_window) {
                QMetaObject::invokeMethod(m_window, "clearInputs");
            }
        } else {
            qWarning() << "LoginWidgetQml: Login failed - invalid username";
            // 这里可以显示错误消息
        }
    }
    
    /**
     * @brief 处理注册结果
     */
    void onRegisterResult(bool success) {
        if (success) {
            qDebug() << "LoginWidgetQml: Registration successful";
            
            // 切换到登录模式
            if (m_window) {
                QMetaObject::invokeMethod(m_window, "switchToLoginMode");
            }
        } else {
            qWarning() << "LoginWidgetQml: Registration failed";
        }
    }

private:
    QQuickWindow *m_window;
    QQmlApplicationEngine *m_engine;
    HttpRequest *request;
};

#endif // LOGINWIDGET_QML_H
