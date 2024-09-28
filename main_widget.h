#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H
#include"play_widget.h"
#include <QWidget>

class Main_Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Main_Widget(QWidget *parent = nullptr);

    void show_widget();
signals:

private:
    Play_Widget*w;

};

#endif // TEST_WIDGET_H
