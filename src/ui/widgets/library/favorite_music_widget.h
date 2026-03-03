#ifndef FAVORITE_MUSIC_WIDGET_H
#define FAVORITE_MUSIC_WIDGET_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlContext>
#include <QDebug>

class FavoriteMusicWidget : public QQuickWidget
{
    Q_OBJECT
public:
    explicit FavoriteMusicWidget(QWidget *parent = nullptr);
    
    // 设置用户账号
    void setUserAccount(const QString& userAccount);
    
    // 加载喜欢音乐数据
    void loadFavorites(const QVariantList& favoritesData);
    
    // 设置当前播放路径（用于高亮显示）
    void setCurrentPlayingPath(const QString& filePath);
    
    // 清空喜欢列表
    void clearFavorites();

signals:
    void playMusic(const QString& filePath);
    void removeFavorite(const QStringList& paths);
    void refreshRequested();

private slots:
    void handleRemoveFavorite(const QVariant& selectedPaths);
};

#endif // FAVORITE_MUSIC_WIDGET_H
