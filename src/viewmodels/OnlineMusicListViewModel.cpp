#include "OnlineMusicListViewModel.h"

#include "download_manager.h"
#include "settings_manager.h"

OnlineMusicListViewModel::OnlineMusicListViewModel(QObject* parent)
    : BaseViewModel(parent)
    , m_request(this)
{
    setupConnections();
}

void OnlineMusicListViewModel::resolveStreamUrl(const QString& relativePath,
                                                const QString& artist,
                                                const QString& cover)
{
    m_currentArtist = artist;
    m_currentCover = cover;
    m_request.getMusicData(relativePath);
}

void OnlineMusicListViewModel::downloadMusic(const QString& relativePath,
                                             const QString& coverUrl)
{
    m_request.download(relativePath,
                       SettingsManager::instance().downloadPath(),
                       SettingsManager::instance().downloadLyrics(),
                       coverUrl);
}

void OnlineMusicListViewModel::onStreamUrlReady(bool success, const QString& url)
{
    if (!success) {
        emit streamResolveFailed();
        return;
    }
    emit streamReady(url, m_currentArtist, m_currentCover);
}
