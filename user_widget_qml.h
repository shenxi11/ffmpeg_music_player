#ifndef USERWIDGET_QML_H
#define USERWIDGET_QML_H

#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlError>
#include <QQmlEngine>
#include <QQuickItem>
#include <QDebug>
#include <QPixmap>

/**
 * @brief QML 版本的 UserWidget 包装类
 * 提供与原 UserWidget 相同的接口，用于无缝替换
 */
class UserWidgetQml : public QQuickWidget
{
    Q_OBJECT

public:
    explicit UserWidgetQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        qDebug() << "UserWidgetQml: Initializing...";
        
        // 配置 QML 引擎的导入路径
        engine()->addImportPath("qrc:/");
        engine()->addImportPath(":/");
        
        // 设置 QML 引擎属性
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        // 加载 QML 文件
        qDebug() << "UserWidgetQml: Loading QML from qrc:/qml/components/UserWidget.qml";
        setSource(QUrl("qrc:/qml/components/UserWidget.qml"));
        
        // 检查加载是否成功
        if (status() == QQuickWidget::Error) {
            qWarning() << "UserWidgetQml: Failed to load UserWidget.qml";
            QList<QQmlError> errors = this->errors();
            for (const QQmlError &error : errors) {
                qWarning() << "  Error:" << error.toString();
            }
            return;
        }
        
        qDebug() << "UserWidgetQml: QML loaded successfully, status =" << status();
        
        // 连接 QML 信号到 C++ 信号
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            qDebug() << "UserWidgetQml: Root item found";
            
            // 连接 loginRequested() 信号
            connect(rootItem, SIGNAL(loginRequested()), 
                    this, SIGNAL(loginRequested()));
            
            // 连接 logoutRequested() 信号
            connect(rootItem, SIGNAL(logoutRequested()), 
                    this, SIGNAL(logoutRequested()));
            
            qDebug() << "UserWidgetQml: QML signals connected successfully";
        } else {
            qWarning() << "UserWidgetQml: Failed to get root object";
        }
        
        // 设置背景透明
        setClearColor(Qt::transparent);
        setAttribute(Qt::WA_TranslucentBackground);
        
        qDebug() << "UserWidgetQml: Initialization complete";
    }
    
    ~UserWidgetQml() override = default;

signals:
    // 与原 UserWidget 保持一致的信号
    void loginRequested();
    void logoutRequested();

public slots:
    /**
     * @brief 设置用户信息
     */
    void setUserInfo(const QString &username, const QPixmap &avatar) {
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            // 设置用户名
            rootItem->setProperty("username", username);
            
            // 如果需要设置头像，可以将 QPixmap 保存为临时文件或使用 base64
            // 这里简化处理，只设置用户名
            qDebug() << "UserWidgetQml: Set user info -" << username;
            
            // 如果有头像，调用 QML 方法
            if (!avatar.isNull()) {
                // 可以将头像保存到临时位置，或者使用 base64
                // 这里简化处理
                QMetaObject::invokeMethod(rootItem, "setUserInfo",
                    Q_ARG(QVariant, username),
                    Q_ARG(QVariant, "qrc:/new/prefix1/icon/denglu.png"));
            } else {
                QMetaObject::invokeMethod(rootItem, "setUserInfo",
                    Q_ARG(QVariant, username),
                    Q_ARG(QVariant, "qrc:/new/prefix1/icon/denglu.png"));
            }
        }
    }
    
    /**
     * @brief 设置登录状态
     */
    void setLoginState(bool loggedIn) {
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            rootItem->setProperty("isLoggedIn", loggedIn);
            
            // 调用 QML 方法
            QMetaObject::invokeMethod(rootItem, "setLoginState",
                Q_ARG(QVariant, loggedIn));
            
            qDebug() << "UserWidgetQml: Set login state -" << loggedIn;
        }
    }
};

#endif // USERWIDGET_QML_H
