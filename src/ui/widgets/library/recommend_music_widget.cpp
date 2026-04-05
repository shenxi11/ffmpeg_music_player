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
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }

    QMetaObject::invokeMethod(root, "loadRecommendations",
                              Q_ARG(QVariant, QVariant::fromValue(meta)),
                              Q_ARG(QVariant, QVariant::fromValue(recommendationData)));
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
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    QMetaObject::invokeMethod(root, "clearRecommendations");
}
