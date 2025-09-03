#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H
#include"play_widget.h"
#include "music_list_widget.h"
#include "music_list_widget_local.h"
#include "music_list_widget_net.h"
#include "loginwidget.h"
#include "searchbox.h"
#include "translate_widget.h"
#include <QWidget>
#include <QButtonGroup>
#include <QScreen>
#include <QGuiApplication>
#include "cplaywidget.h"

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
    void signal_close();
    void signal_startDecode(QString url);
private:
    PlayWidget* w;
    MusicListWidget* list;
    MusicListWidgetLocal* main_list;
    MusicListWidgetNet* net_list;
    TranslateWidget* translate_widget;
    LoginWidget* loginWidget;

    QPushButton* Login;

//    CPlayWidget* glwidget;
//    QPushButton* openGlWidget_btn;
//    MovieDecoder* decoder;

    std::thread thread_ ;
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
    void closeEvent(QCloseEvent *event) override{
        if(w)
            w->close();
        if(net_list)
            net_list->close();
        if(translate_widget)
            translate_widget->close();
        if(loginWidget)
            loginWidget->close();
        if(main_list)
            main_list->close();
        if(list)
            list->close();
        emit signal_close();
    }
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};

#endif // TEST_WIDGET_H
