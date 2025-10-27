#ifndef USERWIDGET_QML_H
#define USERWIDGET_QML_H

#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlError>
#include <QQmlEngine>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QDebug>
#include <QCursor>
#include <QScreen>
#include <QGuiApplication>

/**
 * @brief QML 版本的 UserWidget 包装类
 * 使用独立窗口显示弹出菜单，避免 QQuickWidget 裁剪问题
 */
class UserWidgetQml : public QQuickWidget
{
    Q_OBJECT

private:
    QQmlApplicationEngine *popupEngine;  // 独立窗口引擎
    QObject *popupWindow;                 // 弹出窗口对象
    bool isLoggedIn_;                     // 登录状态
    QString username_;                     // 用户名

public:
    explicit UserWidgetQml(QWidget *parent = nullptr)
        : QQuickWidget(parent), popupEngine(nullptr), popupWindow(nullptr),
          isLoggedIn_(false), username_("未登录")
    {
        qDebug() << "UserWidgetQml: Initializing...";
        
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
        
        qDebug() << "UserWidgetQml: QML loaded successfully";
        
        // 设置背景透明
        setClearColor(Qt::transparent);
        setAttribute(Qt::WA_AlwaysStackOnTop);
        setAttribute(Qt::WA_TranslucentBackground);
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        
        // 初始化弹出窗口引擎
        initPopupWindow();
        
        qDebug() << "UserWidgetQml: Initialization complete";
    }
    
    ~UserWidgetQml() override {
        if (popupEngine) {
            delete popupEngine;
        }
    }

private:
    void initPopupWindow() {
        qDebug() << "UserWidgetQml: Initializing popup window...";
        
        popupEngine = new QQmlApplicationEngine(this);
        popupEngine->load(QUrl("qrc:/qml/components/UserPopupWindow.qml"));
        
        if (popupEngine->rootObjects().isEmpty()) {
            qWarning() << "UserWidgetQml: Failed to load UserPopupWindow.qml";
            return;
        }
        
        popupWindow = popupEngine->rootObjects().first();
        qDebug() << "UserWidgetQml: Popup window loaded";
        
        // 连接弹出窗口的信号
        connect(popupWindow, SIGNAL(loginRequested()), 
                this, SIGNAL(loginRequested()));
        connect(popupWindow, SIGNAL(logoutRequested()), 
                this, SIGNAL(logoutRequested()));
        
        qDebug() << "UserWidgetQml: Popup window signals connected";
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        QQuickWidget::mousePressEvent(event);
        
        // 点击时显示弹出窗口
        qDebug() << "UserWidgetQml: Mouse pressed, showing popup";
        showPopupWindow();
    }

signals:
    void loginRequested();
    void logoutRequested();

public slots:
    void showPopupWindow() {
        if (!popupWindow) {
            qWarning() << "UserWidgetQml: Popup window not initialized";
            return;
        }
        
        qDebug() << "UserWidgetQml: Showing popup window...";
        
        // 更新弹出窗口的状态
        popupWindow->setProperty("isLoggedIn", isLoggedIn_);
        popupWindow->setProperty("username", username_);
        
        // 计算弹出窗口位置（在主控件右下方）
        QPoint globalPos = mapToGlobal(QPoint(0, height()));
        int popupX = globalPos.x() + width() - 180;  // 右对齐
        int popupY = globalPos.y() + 5;
        
        qDebug() << "UserWidgetQml: Setting popup position to" << popupX << popupY;
        
        popupWindow->setProperty("x", popupX);
        popupWindow->setProperty("y", popupY);
        
        // 显示窗口
        QMetaObject::invokeMethod(popupWindow, "show");
        QMetaObject::invokeMethod(popupWindow, "raise");
        QMetaObject::invokeMethod(popupWindow, "requestActivate");
        
        qDebug() << "UserWidgetQml: Popup window shown";
    }
    
    void setUserInfo(const QString &username, const QPixmap &avatar) {
        username_ = username;
        
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            QMetaObject::invokeMethod(rootItem, "setUserInfo",
                Q_ARG(QVariant, username),
                Q_ARG(QVariant, "qrc:/new/prefix1/icon/denglu.png"));
        }
        
        if (popupWindow) {
            popupWindow->setProperty("username", username);
        }
    }
    
    void setLoginState(bool loggedIn) {
        isLoggedIn_ = loggedIn;
        
        QQuickItem *rootItem = rootObject();
        if (rootItem) {
            rootItem->setProperty("isLoggedIn", loggedIn);
            if (!loggedIn) {
                username_ = "未登录";
                QMetaObject::invokeMethod(rootItem, "setLoginState",
                    Q_ARG(QVariant, false));
            }
        }
        
        if (popupWindow) {
            popupWindow->setProperty("isLoggedIn", loggedIn);
        }
    }
};

#endif // USERWIDGET_QML_H
