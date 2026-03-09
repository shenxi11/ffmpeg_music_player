#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H
#include"play_widget.h"
#include "music_list_widget.h"
#include "music_list_widget_local.h"
#include "music_list_widget_net.h"
#include "local_and_download_widget.h"
#include "local_music_cache.h"
#include "loginwidget_qml.h"
#include "searchbox_qml.h"
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
#include "recommend_music_widget.h"
#include <QWidget>
#include <QButtonGroup>
#include <QScreen>
#include <QGuiApplication>
#include <QStringList>
#include <QPainter>
#include <QLinearGradient>
#include <QMouseEvent>

class PlaybackStateManager;
class QCloseEvent;
class QTimer;

class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget();
    void Update_paint();
    
    // 检查登录状态（Q_INVOKABLE 供 QMetaObject::invokeMethod 调用）
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
    void loginRequired();  // 需要登录时发出的信号
    
private:
    PlayWidget* w;
    MusicListWidget* list;
    MusicListWidgetLocal* main_list;
    LocalAndDownloadWidget* localAndDownloadWidget;  // 本地与下载页面组件
    MusicListWidgetNet* net_list;
    PlayHistoryWidget* playHistoryWidget;  // 最近播放页面组件
    FavoriteMusicWidget* favoriteMusicWidget;  // 喜欢音乐页面组件
    RecommendMusicWidget* recommendMusicWidget;  // 推荐音乐页面组件
    LoginWidgetQml* loginWidget;
    UserWidget* userWidget;
    UserWidgetQml* userWidgetQml;
    SearchBoxQml* searchBox = nullptr;
    QWidget* topWidget;
    QWidget* leftWidget = nullptr;
    QWidget* brandWidget = nullptr;
    MainMenu* mainMenu;
    QPushButton* menuButton;
    QPushButton* recommendButton = nullptr;
    QPushButton* localButton = nullptr;
    QPushButton* netButton = nullptr;
    QPushButton* historyButton = nullptr;
    QPushButton* favoriteButton = nullptr;
    QPushButton* videoButton = nullptr;
    VideoPlayerWindow* videoPlayerWindow;
    VideoListWidget* videoListWidget;  // 在线视频列表窗口
    SettingsWidget* settingsWidget;    // 设置窗口

    QPushButton* Login;
    HttpRequestV2* request;
    
    // 在线音乐元数据缓存（用于追加最近播放记录）
    QString m_networkMusicArtist;
    QString m_networkMusicCover;
    PlaybackStateManager* m_playbackStateManager = nullptr;
    QTimer* m_pluginErrorDialogTimer = nullptr;
    QStringList m_pendingPluginErrors;

    QPoint pos_ = QPoint(0, 0);
    bool dragging = false;
protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // QQ 音乐风格：浅灰白色渐变背景
        QLinearGradient gradient(0, 0, 0, height());
        gradient.setColorAt(0, QColor("#F5F5F7"));   // 浅灰白色
        gradient.setColorAt(0.5, QColor("#FAFAFA")); // 接近纯白
        gradient.setColorAt(1, QColor("#F0F0F2"));   // 浅灰色
        painter.fillRect(rect(), gradient);

     }

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void submitPlayHistoryWithRetry(const QString& sessionId,
                                    const QString& filePath,
                                    const QString& userAccount,
                                    int retryCount);
    QRect computeContentRect() const;
    void updateAdaptiveLayout();
    void updateSideNavLayout();
    void enqueuePluginLoadError(const QString& pluginFilePath, const QString& reason);
    void showPluginDiagnosticsDialog();
};

#endif // TEST_WIDGET_H



