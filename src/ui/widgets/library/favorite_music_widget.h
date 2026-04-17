#ifndef FAVORITE_MUSIC_WIDGET_H
#define FAVORITE_MUSIC_WIDGET_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlContext>
#include <QDebug>
#include <QVariantList>
#include <QVariantMap>
#include <QStringList>

class FavoriteMusicWidget : public QQuickWidget
{
    Q_OBJECT
public:
    explicit FavoriteMusicWidget(QWidget *parent = nullptr);

    // 设置用户账号
    void setUserAccount(const QString& userAccount);

    // 加载喜欢音乐数据
    void loadFavorites(const QVariantList& favoritesData);
    void setAvailablePlaylists(const QVariantList& playlists);
    void setFavoritePaths(const QStringList& favoritePaths);

    // 设置当前播放路径（用于高亮显示）
    void setCurrentPlayingPath(const QString& filePath);
    void setPlayingState(const QString& filePath, bool playing);

    // 清空喜欢列表
    void clearFavorites();
    QVariantList favoriteItemsSnapshot(int limit = -1) const;

signals:
    void playMusic(const QString& filePath);
    void removeFavorite(const QStringList& paths);
    void refreshRequested();
    void songActionRequested(const QString& action, const QVariantMap& songData);

private slots:
    void handleRemoveFavorite(const QVariant& selectedPaths);
    void handleSongActionRequested(const QString& action, const QVariant& payload);

private:
    QVariantList m_lastFavoriteItems;
};

#endif // FAVORITE_MUSIC_WIDGET_H
