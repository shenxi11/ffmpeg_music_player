#ifndef RECOMMEND_MUSIC_WIDGET_H
#define RECOMMEND_MUSIC_WIDGET_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QVariantList>
#include <QVariantMap>

class RecommendMusicWidget : public QQuickWidget
{
    Q_OBJECT

public:
    explicit RecommendMusicWidget(QWidget* parent = nullptr);

    void setLoggedIn(bool loggedIn, const QString& userAccount = QString());
    void loadRecommendations(const QVariantMap& meta, const QVariantList& recommendationData);
    void setCurrentPlayingPath(const QString& filePath);
    void setPlayingState(const QString& filePath, bool playing);
    void clearRecommendations();

signals:
    void playMusicWithMetadata(const QString& filePath,
                               const QString& title,
                               const QString& artist,
                               const QString& cover,
                               const QString& duration,
                               const QString& songId,
                               const QString& requestId,
                               const QString& modelVersion,
                               const QString& scene);
    void addToFavorite(const QString& filePath,
                       const QString& title,
                       const QString& artist,
                       const QString& duration,
                       bool isLocal);
    void feedbackEvent(const QString& songId,
                       const QString& eventType,
                       int playMs,
                       int durationMs,
                       const QString& scene,
                       const QString& requestId,
                       const QString& modelVersion);
    void loginRequested();
    void refreshRequested();
};

#endif // RECOMMEND_MUSIC_WIDGET_H
