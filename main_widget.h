#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H
#include"play_widget.h"
#include "music_list_widget.h"
#include "music_list_widget_local.h"
#include "music_list_widget_net.h"
#include "local_and_download_widget.h"
#include "local_music_cache.h"
#include "loginwidget_qml.h"
#include "searchbox.h"
#include "user_widget.h"
#include "userwidget_qml.h"
#include "main_menu.h"
#include "httprequest_v2.h"
#include "plugin_manager.h"
#include "VideoPlayerWindow.h"
#include "video_list_widget.h"
#include "settings_widget.h"
#include "settings_manager.h"
#include "play_history_widget.h"
#include "favorite_music_widget.h"
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
    
    // 检查登录状态 (Q_INVOKABLE用于QMetaObject::invokeMethod调用)
    Q_INVOKABLE bool isUserLoggedIn() const { 
        return userWidgetQml ? userWidgetQml->getLoginState() : false; 
    }
    
    // 显示登录窗口
    void showLoginWindow() {
        if (loginWidget) {
            loginWidget->isVisible = true;
            loginWidget->show();
        }
    }
    
signals:
    void loginRequired();  // 需要登录的信号
    
private:
    PlayWidget* w;
    MusicListWidget* list;
    MusicListWidgetLocal* main_list;
    LocalAndDownloadWidget* localAndDownloadWidget;  // 新的本地和下载页面
    MusicListWidgetNet* net_list;
    PlayHistoryWidget* playHistoryWidget;  // 播放历史widget
    FavoriteMusicWidget* favoriteMusicWidget;  // 喜欢音乐widget
    LoginWidgetQml* loginWidget;
    UserWidget* userWidget;
    UserWidgetQml* userWidgetQml;
    QWidget* topWidget;
    MainMenu* mainMenu;
    QPushButton* menuButton;
    VideoPlayerWindow* videoPlayerWindow;
    VideoListWidget* videoListWidget;  // 在线视频列表窗口
    SettingsWidget* settingsWidget;    // 设置窗口

    QPushButton* Login;
    HttpRequestV2* request;
    
    // 网络音乐元数据缓存（用于添加播放历史）
    QString m_networkMusicArtist;
    QString m_networkMusicCover;

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
