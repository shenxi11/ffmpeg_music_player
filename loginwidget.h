#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include "httprequest.h"
#include <QObject>
#include <QWidget>
#include <QDebug>
#include <QLabel>
#include <QLabel>
#include <QPushButton>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>

class LoginWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWidget(QWidget *parent = nullptr);

    bool isVisible = false;
signals:
    void login_(QString username);
protected:
    void closeEvent(QCloseEvent *event) override
    {
        this->isVisible = false;
    };
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
private:
    QLineEdit* account;
    QLineEdit* password;
    QLineEdit* username;
    QPushButton* login;
    QLabel* Register;

    bool isLogin = true;

    bool mousePressed = false;

    QPoint mouseStartPoint = QPoint(0, 0);
    QPoint windowStartPoint = QPoint(0, 0);

    HttpRequest* request;
};

#endif // LOGINWIDGET_H
