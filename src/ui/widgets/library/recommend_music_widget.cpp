#include "recommend_music_widget.h"

#include <QMetaObject>
#include <QDebug>

RecommendMusicWidget::RecommendMusicWidget(QWidget* parent)
    : QQuickWidget(parent)
{
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/components/library/RecommendMusicWidget.qml"));
    setAttribute(Qt::WA_TranslucentBackground);

    QQuickItem* root = rootObject();
    if (!root) {
        qWarning() << "[RecommendMusicWidget] rootObject is null";
        return;
    }

    connect(root, SIGNAL(playMusicWithMetadata(QString,QString,QString,QString,QString,QString,QString,QString,QString)),
            this, SIGNAL(playMusicWithMetadata(QString,QString,QString,QString,QString,QString,QString,QString,QString)));
    connect(root, SIGNAL(addToFavorite(QString,QString,QString,QString,bool)),
            this, SIGNAL(addToFavorite(QString,QString,QString,QString,bool)));
    connect(root, SIGNAL(feedbackEvent(QString,QString,int,int,QString,QString,QString)),
            this, SIGNAL(feedbackEvent(QString,QString,int,int,QString,QString,QString)));
    connect(root, SIGNAL(loginRequested()), this, SIGNAL(loginRequested()));
    connect(root, SIGNAL(refreshRequested()), this, SIGNAL(refreshRequested()));
    connect(root, SIGNAL(songActionRequested(QString,QVariant)),
            this, SLOT(onSongActionRequested(QString,QVariant)));
}

void RecommendMusicWidget::setLoggedIn(bool loggedIn, const QString& userAccount)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    root->setProperty("isLoggedIn", loggedIn);
    root->setProperty("userAccount", userAccount);
}

void RecommendMusicWidget::loadRecommendations(const QVariantMap& meta, const QVariantList& recommendationData)
{
    m_lastRecommendationMeta = meta;
    m_lastRecommendationItems = recommendationData;
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }

    QMetaObject::invokeMethod(root, "loadRecommendations",
                              Q_ARG(QVariant, QVariant::fromValue(meta)),
                              Q_ARG(QVariant, QVariant::fromValue(recommendationData)));
}

void RecommendMusicWidget::setAvailablePlaylists(const QVariantList& playlists)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    root->setProperty("availablePlaylists", QVariant::fromValue(playlists));
}

void RecommendMusicWidget::setFavoritePaths(const QStringList& favoritePaths)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    root->setProperty("favoritePaths", QVariant::fromValue(favoritePaths));
}

void RecommendMusicWidget::setCurrentPlayingPath(const QString& filePath)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    qDebug() << "[RecommendMusicWidget] Setting currentPlayingPath to:" << filePath;
    root->setProperty("currentPlayingPath", filePath);
}

void RecommendMusicWidget::setPlayingState(const QString& filePath, bool playing)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    qDebug() << "[RecommendMusicWidget] Setting playing state - path:" << filePath
             << "playing:" << playing;
    QMetaObject::invokeMethod(root, "setPlayingState",
                              Q_ARG(QVariant, filePath),
                              Q_ARG(QVariant, playing));
}

void RecommendMusicWidget::clearRecommendations()
{
    m_lastRecommendationMeta.clear();
    m_lastRecommendationItems.clear();
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    QMetaObject::invokeMethod(root, "clearRecommendations");
}

void RecommendMusicWidget::onSongActionRequested(const QString& action, const QVariant& payload)
{
    emit songActionRequested(action, payload.toMap());
}

QVariantMap RecommendMusicWidget::recommendationMetaSnapshot() const
{
    return m_lastRecommendationMeta;
}

QVariantList RecommendMusicWidget::recommendationItemsSnapshot(int limit) const
{
    if (limit <= 0 || limit >= m_lastRecommendationItems.size()) {
        return m_lastRecommendationItems;
    }

    QVariantList items;
    items.reserve(limit);
    for (int i = 0; i < limit; ++i) {
        items.push_back(m_lastRecommendationItems.at(i));
    }
    return items;
}
