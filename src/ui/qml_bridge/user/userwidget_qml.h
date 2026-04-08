#ifndef USERWIDGET_QML_H
#define USERWIDGET_QML_H

#include <QCursor>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QScreen>

/**
 * @brief QML 版本的 UserWidget 包装类
 * 使用独立窗口显示弹出菜单，避免 QQuickWidget 裁剪问题
 */
class UserWidgetQml : public QQuickWidget {
    Q_OBJECT

  private:
    static QString defaultAvatarSource() {
        return QStringLiteral("qrc:/qml/assets/ai/icons/default-user-avatar.svg");
    }

    QQmlApplicationEngine* popupEngine; // 独立窗口引擎
    QObject* popupWindow;               // 弹出窗口对象
    bool isLoggedIn_;                   // 登录状态
    QString username_;                  // 用户名
    QString avatarSource_;              // 头像资源
    bool popupBlocked_ = false;        // 是否阻止弹出菜单

  public:
    explicit UserWidgetQml(QWidget* parent = nullptr)
        : QQuickWidget(parent), popupEngine(nullptr), popupWindow(nullptr), isLoggedIn_(false),
          username_(QStringLiteral("未登录")), avatarSource_(defaultAvatarSource()) {
        qDebug() << "UserWidgetQml: Initializing...";

        // 设置 QML 引擎属性
        setResizeMode(QQuickWidget::SizeRootObjectToView);

        // 加载 QML 文件
        qDebug() << "UserWidgetQml: Loading QML from qrc:/qml/components/user/UserWidget.qml";
        setSource(QUrl("qrc:/qml/components/user/UserWidget.qml"));

        // 检查加载是否成功
        if (status() == QQuickWidget::Error) {
            qWarning() << "UserWidgetQml: Failed to load UserWidget.qml";
            QList<QQmlError> errors = this->errors();
            for (const QQmlError& error : errors) {
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
        popupEngine->load(QUrl("qrc:/qml/components/user/UserPopupWindow.qml"));

        if (popupEngine->rootObjects().isEmpty()) {
            qWarning() << "UserWidgetQml: Failed to load UserPopupWindow.qml";
            return;
        }

        popupWindow = popupEngine->rootObjects().first();
        qDebug() << "UserWidgetQml: Popup window loaded";

        // 连接弹出窗口的信号
        connect(popupWindow, SIGNAL(loginRequested()), this, SIGNAL(loginRequested()));
        connect(popupWindow, SIGNAL(profileRequested()), this, SIGNAL(profileRequested()));
        connect(popupWindow, SIGNAL(logoutRequested()), this, SIGNAL(logoutRequested()));

        qDebug() << "UserWidgetQml: Popup window signals connected";
    }

  protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (popupBlocked_) {
            emit blockedInteractionRequested();
            event->accept();
            return;
        }

        QQuickWidget::mousePressEvent(event);

        // 点击时显示弹出窗口
        qDebug() << "UserWidgetQml: Mouse pressed, showing popup";
        showPopupWindow();
    }

  signals:
    void loginRequested();
    void profileRequested();
    void logoutRequested();
    void blockedInteractionRequested();
    void loginStateChanged(bool loggedIn); // 新增：登录状态变化信号

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
        popupWindow->setProperty("avatarSource", avatarSource_);

        // 计算弹出窗口位置（在主控件右下方）
        QPoint globalPos = mapToGlobal(QPoint(0, height()));
        int popupX = globalPos.x() + width() - 180; // 右对齐
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

    void setUserInfo(const QString& username, const QString& avatarSource = QString()) {
        username_ = username.trimmed().isEmpty() ? QStringLiteral("未登录") : username.trimmed();
        avatarSource_ =
            avatarSource.trimmed().isEmpty() ? defaultAvatarSource() : avatarSource.trimmed();

        QQuickItem* rootItem = rootObject();
        if (rootItem) {
            QMetaObject::invokeMethod(rootItem, "setUserInfo", Q_ARG(QVariant, username_),
                                      Q_ARG(QVariant, avatarSource_));
        }

        if (popupWindow) {
            popupWindow->setProperty("username", username_);
            popupWindow->setProperty("avatarSource", avatarSource_);
        }
    }

    void setLoginState(bool loggedIn) {
        if (isLoggedIn_ != loggedIn) {
            isLoggedIn_ = loggedIn;
            emit loginStateChanged(loggedIn); // 发送登录状态变化信号
        }

        QQuickItem* rootItem = rootObject();
        if (rootItem) {
            rootItem->setProperty("isLoggedIn", loggedIn);
            if (!loggedIn) {
                username_ = QStringLiteral("未登录");
                avatarSource_ = defaultAvatarSource();
                QMetaObject::invokeMethod(rootItem, "setLoginState", Q_ARG(QVariant, false));
            }
        }

        if (popupWindow) {
            popupWindow->setProperty("isLoggedIn", loggedIn);
            popupWindow->setProperty("username", username_);
            popupWindow->setProperty("avatarSource", avatarSource_);
        }
    }

    void setPopupBlocked(bool blocked) {
        popupBlocked_ = blocked;
    }

    // 获取登录状态
    bool getLoginState() const {
        return isLoggedIn_;
    }
};

#endif // USERWIDGET_QML_H
