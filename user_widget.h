#ifndef USERWIDGET_H
#define USERWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QBitmap>
#include <QGuiApplication>
#include <QApplication>
#include <QScreen>
#include "loginwidget.h"

class UserPopupWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UserPopupWidget(QWidget *parent = nullptr);
    void setUserInfo(const QString &username, const QPixmap &avatar);
    void setLoginState(bool loggedIn);

protected:
    void paintEvent(QPaintEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void loginRequested();
    void logoutRequested();

private slots:
    void onActionButtonClicked();

private:
    QLabel* avatarLabel;
    QLabel* usernameLabel;
    QLabel* statusLabel;
    QPushButton* actionButton;
    bool isLoggedIn;
    
    void setupUI();
    void updateContent();
};

class UserWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UserWidget(QWidget *parent = nullptr);
    void setUserInfo(const QString &username, const QPixmap &avatar);
    void setLoginState(bool loggedIn);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

signals:
    void loginRequested();
    void logoutRequested();

private slots:
    void showPopup();
    void hidePopup();
    void onPopupActionRequested();

private:
    QLabel* avatarLabel;
    QLabel* usernameLabel;
    UserPopupWidget* popup;
    QTimer* hoverTimer;
    QTimer* hideTimer;
    
    QString currentUsername;
    QPixmap currentAvatar;
    bool isLoggedIn;
    
    void setupUI();
    void updateDisplay();
    QPixmap createCircularAvatar(const QPixmap &source, int size);
};

#endif // USERWIDGET_H
