#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H

#include <QWidget>

class Test_Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Test_Widget(QWidget *parent = nullptr);

signals:

};

#endif // TEST_WIDGET_H
