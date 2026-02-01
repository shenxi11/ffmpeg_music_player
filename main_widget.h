#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H
#include"play_widget.h"
#include "music_list_widget.h"
#include "music_list_widget_local.h"
#include "music_list_widget_net.h"
#include "loginwidget_qml.h"
#include "searchbox.h"
#include "user_widget.h"
#include "userwidget_qml.h"
#include "main_menu.h"
#include "httprequest.h"
#include "plugin_manager.h"
#include "VideoPlayerWindow.h"
#include "video_list_widget.h"
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
    LoginWidgetQml* loginWidget;
    UserWidget* userWidget;
    UserWidgetQml* userWidgetQml;
    QWidget* topWidget;
    MainMenu* mainMenu;
    QPushButton* menuButton;
    VideoPlayerWindow* videoPlayerWindow;
    VideoListWidget* videoListWidget;  // 在线视频列表窗口

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

        // QQ音乐风格：浅灰白色渐变背景
        QLinearGradient gradient(0, 0, 0, height());
        gradient.setColorAt(0, QColor("#F5F5F7"));   // 浅灰白色
        gradient.setColorAt(0.5, QColor("#FAFAFA")); // 几乎白色
        gradient.setColorAt(1, QColor("#F0F0F2"));   // 浅灰色
        painter.fillRect(rect(), gradient);

     }

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};

#endif // TEST_WIDGET_H
