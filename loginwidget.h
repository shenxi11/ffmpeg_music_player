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

private:
    QLineEdit* account;
    QLineEdit* password;
    QLineEdit* username;
    QPushButton* login;
    QLabel* Register;

    HttpRequest* request;
    bool isLogin = true;
};

#endif // LOGINWIDGET_H
