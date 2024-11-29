#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H
#include"play_widget.h"
#include "music_list_widget.h"
#include "loginwidget.h"
#include "searchbox.h"
#include <QWidget>
#include <QButtonGroup>
#include <QScreen>
#include <QGuiApplication>

class Main_Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Main_Widget(QWidget *parent = nullptr);
    ~Main_Widget();
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
    HttpRequest* request;

    QThread* a;
    QPoint pos_ = QPoint(0, 0);
    bool dragging = false;
protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);

//        painter.setBrush(QColor(246, 246, 246));

//        painter.drawRect(this->rect());

        painter.setRenderHint(QPainter::Antialiasing);

        QColor backgroundColor("#F7F9FC");
        painter.fillRect(this->rect(), backgroundColor);

     }

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};

#endif // TEST_WIDGET_H
