#include "favorite_music_widget.h"

FavoriteMusicWidget::FavoriteMusicWidget(QWidget *parent)
    : QQuickWidget(parent)
{
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/components/playback/FavoriteMusicList.qml"));
    
    setAttribute(Qt::WA_TranslucentBackground);
    
    qDebug() << "FavoriteMusicWidget: Created";
    
    // 杩炴帴QML淇″彿
    QQuickItem* root = rootObject();
    if (root) {
        connect(root, SIGNAL(playMusic(QString)),
                this, SIGNAL(playMusic(QString)));
        connect(root, SIGNAL(removeFavorite(QVariant)),
                this, SLOT(handleRemoveFavorite(QVariant)));
        connect(root, SIGNAL(refreshRequested()),
                this, SIGNAL(refreshRequested()));
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

