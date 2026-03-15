#include "VideoListViewModel.h"

VideoListViewModel::VideoListViewModel(QObject* parent)
    : BaseViewModel(parent)
    , m_request(this)
{
    connect(&m_request, &HttpRequestV2::signalVideoList,
            this, &VideoListViewModel::onVideoListReceived);
    connect(&m_request, &HttpRequestV2::signalVideoStreamUrl,
            this, &VideoListViewModel::onVideoStreamUrlReceived);
}

void VideoListViewModel::refresh()
{
    clearError();
    setIsBusy(true);
    m_request.getVideoList();
}

void VideoListViewModel::resolveVideoStream(const QString& videoPath, const QString& videoName)
{
    clearError();
    setIsBusy(true);
    m_selectedVideoName = videoName;
    m_request.getVideoStreamUrl(videoPath);
}

void VideoListViewModel::onVideoListReceived(const QVariantList& videoList)
{
    setIsBusy(false);
    emit videoListUpdated(videoList);
}

void VideoListViewModel::onVideoStreamUrlReceived(const QString& videoUrl)
{
    setIsBusy(false);
    emit videoStreamResolved(videoUrl, m_selectedVideoName);
}
