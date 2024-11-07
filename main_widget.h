#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H
#include"play_widget.h"
#include "music_list_widget.h"
#include "loginwidget.h"
#include <QWidget>

class Main_Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Main_Widget(QWidget *parent = nullptr);

    void Update_paint();
signals:

private:
    Play_Widget* w;
    MusicListWidget* list;
    MusicListWidget* main_list;
    LoginWidget* loginWidget;

    QPushButton* play;
    QPushButton* add;
    QPushButton* Login;
protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);

        painter.setBrush(QColor(246, 246, 246));

        painter.drawRect(this->rect());
    }
};

#endif // TEST_WIDGET_H
