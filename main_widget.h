#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H
#include"play_widget.h"
#include "music_list_widget.h"
#include "music_list_widget_local.h"
#include "music_list_widget_net.h"
#include "loginwidget.h"
#include "searchbox.h"
#include "translate_widget.h"
#include "user_widget.h"
#include "main_menu.h"
#include "httprequest.h"
#include <QWidget>
#include <QButtonGroup>
#include <QScreen>
#include <QGuiApplication>

class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget();
    void Update_paint();
public slots:
    void on_signal_choose_download_dir();
signals:
private:
    PlayWidget* w;
    MusicListWidget* list;
    MusicListWidgetLocal* main_list;
    MusicListWidgetNet* net_list;
    LoginWidget* loginWidget;
    TranslateWidget* translateWidget;
    UserWidget* userWidget;
    QWidget* topWidget;
    MainMenu* mainMenu;
    QPushButton* menuButton;

    QPushButton* Login;
    HttpRequest* request;

    QPoint pos_ = QPoint(0, 0);
    bool dragging = false;
protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);

        painter.setRenderHint(QPainter::Antialiasing);

        QColor backgroundColor("#F7F9FC");
        painter.fillRect(this->rect(), backgroundColor);

     }

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};

#endif // TEST_WIDGET_H
