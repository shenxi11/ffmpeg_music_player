#include "LocalMusicListViewModel.h"

#include "user.h"

LocalMusicListViewModel::LocalMusicListViewModel(QObject* parent)
    : BaseViewModel(parent)
    , m_request(this)
{
    auto* user = User::getInstance();
    connect(user, &User::signalAddSongs, this, &LocalMusicListViewModel::handleUserSongsChanged);
}

void LocalMusicListViewModel::addMusic(const QString& path)
{
    m_request.addMusic(path);
}

void LocalMusicListViewModel::refreshLocalMusicPaths()
{
    auto* user = User::getInstance();
    emit localMusicPathsReady(user->getMusicPath());
}

void LocalMusicListViewModel::handleUserSongsChanged()
{
    auto* user = User::getInstance();
    emit localMusicPathsReady(user->getMusicPath());
}
