#include "favorite_music_widget.h"

FavoriteMusicWidget::FavoriteMusicWidget(QWidget *parent)
    : QQuickWidget(parent)
{
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/components/playback/FavoriteMusicList.qml"));
    
    setAttribute(Qt::WA_TranslucentBackground);
    
    qDebug() << "FavoriteMusicWidget: Created";
    
    // 获取 QML 根对象并连接收藏列表交互信号。
    QQuickItem* root = rootObject();
    if (root) {
        connect(root, SIGNAL(playMusic(QString)),
                this, SIGNAL(playMusic(QString)));
        connect(root, SIGNAL(removeFavorite(QVariant)),
                this, SLOT(handleRemoveFavorite(QVariant)));
        connect(root, SIGNAL(refreshRequested()),
                this, SIGNAL(refreshRequested()));
        connect(root, SIGNAL(songActionRequested(QString,QVariant)),
                this, SLOT(handleSongActionRequested(QString,QVariant)));
        qDebug() << "FavoriteMusicWidget: Signals connected";
    } else {
        qDebug() << "FavoriteMusicWidget: ERROR - Root object is null!";
    }
}

void FavoriteMusicWidget::setUserAccount(const QString& userAccount)
{
    QQuickItem* root = rootObject();
    if (root) {
        root->setProperty("userAccount", userAccount);
        qDebug() << "FavoriteMusicWidget: Set user account:" << userAccount;
    }
}

void FavoriteMusicWidget::loadFavorites(const QVariantList& favoritesData)
{
    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "loadFavorites",
            Q_ARG(QVariant, QVariant::fromValue(favoritesData)));
        qDebug() << "FavoriteMusicWidget: Loaded" << favoritesData.size() << "favorite items";
    }
}

void FavoriteMusicWidget::setAvailablePlaylists(const QVariantList& playlists)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    root->setProperty("availablePlaylists", QVariant::fromValue(playlists));
}

void FavoriteMusicWidget::setFavoritePaths(const QStringList& favoritePaths)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    root->setProperty("favoritePaths", QVariant::fromValue(favoritePaths));
}

void FavoriteMusicWidget::setCurrentPlayingPath(const QString& filePath)
{
    QQuickItem* root = rootObject();
    if (root) {
        qDebug() << "[FavoriteMusicWidget] Setting currentPlayingPath to:" << filePath;
        root->setProperty("currentPlayingPath", filePath);
        QString currentValue = root->property("currentPlayingPath").toString();
        qDebug() << "[FavoriteMusicWidget] Verified currentPlayingPath is now:" << currentValue;
    } else {
        qDebug() << "[FavoriteMusicWidget] WARNING: root object is null!";
    }
}

void FavoriteMusicWidget::setPlayingState(const QString& filePath, bool playing)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    QMetaObject::invokeMethod(root, "setPlayingState",
                              Q_ARG(QVariant, filePath),
                              Q_ARG(QVariant, playing));
}

void FavoriteMusicWidget::clearFavorites()
{
    QQuickItem* root = rootObject();
    if (root) {
        QMetaObject::invokeMethod(root, "clearFavorites");
    }
}

void FavoriteMusicWidget::handleRemoveFavorite(const QVariant& selectedPaths)
{
    QStringList paths;
    QVariantList variantList = selectedPaths.toList();
    for (const QVariant& item : variantList) {
        paths.append(item.toString());
    }
    emit removeFavorite(paths);
}

void FavoriteMusicWidget::handleSongActionRequested(const QString& action, const QVariant& payload)
{
    emit songActionRequested(action, payload.toMap());
}

