#include "music_list_widget_net.h"

#include <QDebug>
#include <QMetaObject>
#include <QVBoxLayout>

MusicListWidgetNet::MusicListWidgetNet(QWidget* parent)
    : QWidget(parent)
{
    listWidget = new MusicListWidgetNetQml(this);
    m_viewModel = new OnlineMusicListViewModel(this);

    QVBoxLayout* v_layout = new QVBoxLayout(this);
    v_layout->setContentsMargins(0, 0, 0, 0);
    v_layout->addWidget(listWidget);
    setLayout(v_layout);

    setupConnections();
}

void MusicListWidgetNet::setAvailablePlaylists(const QVariantList& playlists)
{
    if (!listWidget || !listWidget->rootObject()) {
        return;
    }
    listWidget->rootObject()->setProperty("availablePlaylists", QVariant::fromValue(playlists));
}

void MusicListWidgetNet::setFavoritePaths(const QStringList& favoritePaths)
{
    if (!listWidget || !listWidget->rootObject()) {
        return;
    }
    listWidget->rootObject()->setProperty("favoritePaths", QVariant::fromValue(favoritePaths));
}

void MusicListWidgetNet::onPlayClick(const QString name, const QString artist, const QString cover)
{
    currentSongArtist = artist;
    currentSongCover = cover;
    if (m_viewModel) {
        m_viewModel->resolveStreamUrl(name, artist, cover);
    }
}

void MusicListWidgetNet::onDownloadMusic(QString songName)
{
    if (mainWidget) {
        bool isLoggedIn = false;
        QMetaObject::invokeMethod(mainWidget,
                                  "isUserLoggedIn",
                                  Qt::DirectConnection,
                                  Q_RETURN_ARG(bool, isLoggedIn));

        if (!isLoggedIn) {
            qDebug() << "[MusicListWidgetNet] 下载需要登录，显示登录窗口";
            emit loginRequired();
            return;
        }
    }

    const QString coverUrl = song_cover.value(songName, QString());
    qDebug() << "[MusicListWidgetNet] Download requested for:" << songName;
    qDebug() << "  Cover URL:" << coverUrl;

    if (m_viewModel) {
        m_viewModel->downloadMusic(songName, coverUrl);
    }
}

void MusicListWidgetNet::onRemoveClick(const QString name)
{
    Q_UNUSED(name);
}

void MusicListWidgetNet::onPlayButtonClick(bool flag, const QString filename)
{
    qDebug() << "[PLAY_STATE] MusicListWidgetNet::onPlayButtonClick flag="
             << flag << ", filename=" << filename;
    emit signalPlayButtonClick(flag, filename);
}

void MusicListWidgetNet::onTranslateButtonClicked()
{
    emit signalTranslateButtonClicked();
}

void MusicListWidgetNet::resolveSongAction(const QString& action, const QVariantMap& songData)
{
    if (!m_viewModel) {
        emit songActionRequested(action, songData);
        return;
    }

    const QString relativePath = songData.value(QStringLiteral("path")).toString().trimmed();
    if (relativePath.isEmpty()) {
        emit songActionRequested(action, songData);
        return;
    }

    m_pendingResolvedAction = action;
    m_pendingResolvedSongData = songData;
    m_viewModel->resolveStreamUrl(relativePath,
                                  songData.value(QStringLiteral("artist")).toString(),
                                  songData.value(QStringLiteral("cover")).toString());
}
