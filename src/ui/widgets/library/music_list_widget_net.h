#ifndef MUSICLISTWIDGETNET_H
#define MUSICLISTWIDGETNET_H

#include "music.h"
#include "music_list_widget_net_qml.h"
#include "viewmodels/OnlineMusicListViewModel.h"

#include <QMap>
#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QWidget>

class MainWidget;

class MusicListWidgetNet : public QWidget {
    Q_OBJECT
  public:
    explicit MusicListWidgetNet(QWidget* parent = nullptr);

    MusicListWidgetNetQml* getListWidget() const {
        return listWidget;
    }

    void clearList() {
        if (listWidget) {
            listWidget->clearAll();
        }
        m_currentItems.clear();
    }

    void onPlayClick(const QString name, const QString artist, const QString cover);
    void onRemoveClick(const QString name);
    void onDownloadMusic(QString songName);
    void onTranslateButtonClicked();
    void setAvailablePlaylists(const QVariantList& playlists);
    void setFavoritePaths(const QStringList& favoritePaths);
    void setPlayingState(const QString& filePath, bool playing);
    void resolveSongAction(const QString& action, const QVariantMap& songData);
    QVariantList currentItemsSnapshot(int limit = -1) const;

  signals:
    void signalAddSonglist(const QList<Music>& musicList);
    void signalPlayClick(const QString songName, const QString artist, const QString cover,
                         bool net);
    void signalLast(QString songName);
    void signalNext(QString songName);
    void signalTranslateButtonClicked();
    void loginRequired();
    void addToFavorite(const QString& path, const QString& title, const QString& artist,
                       const QString& duration);
    void songActionRequested(const QString& action, const QVariantMap& songData);

  public:
    void setMainWidget(QWidget* widget) {
        mainWidget = widget;
    }

  private slots:
    void onSongActionRequested(const QString& action, const QVariant& payload);

  private:
    // 连接拆分：将在线列表信号绑定集中到独立实现中维护。
    void setupConnections();
    bool hasPendingResolvedAction() const;
    void clearPendingResolvedAction();

    MusicListWidgetNetQml* listWidget = nullptr;
    QMap<QString, double> song_duration;
    QMap<QString, QString> song_cover;
    OnlineMusicListViewModel* m_viewModel = nullptr;
    QString m_pendingResolvedAction;
    QVariantMap m_pendingResolvedSongData;
    QVariantList m_currentItems;
    QString currentSongArtist;
    QString currentSongCover;
    QWidget* mainWidget = nullptr;
};

#endif // MUSICLISTWIDGETNET_H
