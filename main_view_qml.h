#ifndef MAIN_VIEW_QML_H
#define MAIN_VIEW_QML_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlContext>
#include <QDebug>

class MainViewQml : public QQuickWidget
{
    Q_OBJECT
public:
    explicit MainViewQml(QWidget *parent = nullptr)
        : QQuickWidget(parent)
    {
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        
        qDebug() << "MainViewQml: Loading QML from qrc:/qml/MainView.qml";
        setSource(QUrl("qrc:/qml/MainView.qml"));
        
        if (status() == QQuickWidget::Error) {
            qCritical() << "MainViewQml: Failed to load QML!";
            qCritical() << "Errors:" << errors();
            return;
        }
        
        QQuickItem* root = rootObject();
        if (root) {
            qDebug() << "MainViewQml: QML loaded successfully, connecting signals";
            // 连接 QML 信号到 C++ 槽
            connect(root, SIGNAL(minimizeClicked()), this, SIGNAL(minimizeClicked()));
            connect(root, SIGNAL(maximizeClicked()), this, SIGNAL(maximizeClicked()));
            connect(root, SIGNAL(closeClicked()), this, SIGNAL(closeClicked()));
            connect(root, SIGNAL(menuClicked()), this, SIGNAL(menuClicked()));
            connect(root, SIGNAL(localMusicClicked()), this, SIGNAL(localMusicClicked()));
            connect(root, SIGNAL(onlineMusicClicked()), this, SIGNAL(onlineMusicClicked()));
            connect(root, SIGNAL(playHistoryClicked()), this, SIGNAL(playHistoryClicked()));
            connect(root, SIGNAL(favoriteMusicClicked()), this, SIGNAL(favoriteMusicClicked()));
        } else {
            qCritical() << "MainViewQml: Failed to get root object!";
        }
    }
    
    void setMaximizedState(bool maximized)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setMaximizedState",
                Q_ARG(QVariant, maximized));
        }
    }
    
    void setCurrentView(const QString& view)
    {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "setCurrentView",
                Q_ARG(QVariant, view));
        }
    }
    
    void setUserLoggedIn(bool loggedIn)
    {
        QQuickItem* root = rootObject();
        if (root) {
            root->setProperty("isUserLoggedIn", loggedIn);
        }
    }
    
signals:
    void minimizeClicked();
    void maximizeClicked();
    void closeClicked();
    void menuClicked();
    void localMusicClicked();
    void onlineMusicClicked();
    void playHistoryClicked();
    void favoriteMusicClicked();
};

#endif // MAIN_VIEW_QML_H
