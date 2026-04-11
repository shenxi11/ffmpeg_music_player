#ifndef PLAY_HISTORY_WIDGET_H
#define PLAY_HISTORY_WIDGET_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlContext>
#include <QDebug>
#include <QVariant>
#include <QVariantMap>

class PlayHistoryWidget : public QQuickWidget
{
    Q_OBJECT
public:
    explicit PlayHistoryWidget(QWidget *parent = nullptr);
    
    // 设置登录状态
    void setLoggedIn(bool loggedIn, const QString& userAccount = "");
    
    // 加载播放历史数据
    void loadHistory(const QVariantList& historyData);
    
    // 设置当前播放路径（用于高亮显示）
    void setCurrentPlayingPath(const QString& filePath);

    // 更新播放状态（播放/暂停），用于同步图标样式
    void setPlayingState(const QString& filePath, bool playing);
    
    // 清空历史
    void clearHistory();
    QVariantList historyItemsSnapshot(int limit = -1) const;

signals:
    void playMusic(const QString& filePath);
    void playMusicWithMetadata(const QString& filePath, const QString& title,
                               const QString& artist, const QString& cover);
    void addToFavorite(const QString& filePath, const QString& title,
                       const QString& artist, const QString& duration, bool isLocal);
    void deleteHistory(const QStringList& paths);
    void loginRequested();
    void refreshRequested();
    void songActionRequested(const QString& action, const QVariantMap& songData);

private slots:
    void handleDeleteHistory(const QVariant& selectedPaths);
    void handleSongActionRequested(const QString& action, const QVariant& payload);

private:
    QVariantList m_lastHistoryItems;
};

#endif // PLAY_HISTORY_WIDGET_H
