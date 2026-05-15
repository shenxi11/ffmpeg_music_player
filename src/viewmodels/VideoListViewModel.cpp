#include "VideoListViewModel.h"

VideoListViewModel::VideoListViewModel(QObject* parent)
    : BaseViewModel(parent)
    , m_request(this)
{
    connect(&m_request, &HttpRequestV2::signalVideoList,
            this, &VideoListViewModel::onVideoListReceived);
    connect(&m_request, &HttpRequestV2::signalVideoStreamUrl,
            this, &VideoListViewModel::onVideoStreamUrlReceived);
    connect(&m_request, &HttpRequestV2::signalVideoPlaybackInfo,
            this, &VideoListViewModel::onVideoPlaybackInfoReceived);
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
    m_selectedVideoPath = videoPath;
    m_selectedVideoName = videoName;
    m_request.getVideoPlaybackInfo(videoPath);
}

void VideoListViewModel::onVideoListReceived(const QVariantList& videoList)
{
    setIsBusy(false);
    emit videoListUpdated(videoList);
}

void VideoListViewModel::onVideoStreamUrlReceived(const QString& videoUrl)
{
    setIsBusy(false);
    QVariantMap source;
    source.insert(QStringLiteral("id"), QStringLiteral("direct"));
    source.insert(QStringLiteral("label"), QStringLiteral("原画"));
    source.insert(QStringLiteral("url"), videoUrl);
    source.insert(QStringLiteral("direct"), true);
    emit videoStreamResolved(videoUrl, m_selectedVideoName, QVariantList{source});
}

void VideoListViewModel::onVideoPlaybackInfoReceived(const QVariantMap& playbackInfo)
{
    const QString directUrl = playbackInfo.value(QStringLiteral("direct_url")).toString().trimmed();
    if (playbackInfo.isEmpty() || directUrl.isEmpty()) {
        m_request.getVideoStreamUrl(m_selectedVideoPath);
        return;
    }

    QVariantList sources = buildPlaybackSources(playbackInfo);
    const QString selectedUrl = directUrl;

    setIsBusy(false);
    emit videoStreamResolved(selectedUrl, m_selectedVideoName, sources);
}

QVariantList VideoListViewModel::buildPlaybackSources(const QVariantMap& playbackInfo) const
{
    QVariantList sources;

    const QString directUrl = playbackInfo.value(QStringLiteral("direct_url")).toString().trimmed();
    if (!directUrl.isEmpty()) {
        QVariantMap source;
        source.insert(QStringLiteral("id"), QStringLiteral("direct"));
        source.insert(QStringLiteral("label"), QStringLiteral("原画"));
        source.insert(QStringLiteral("url"), directUrl);
        source.insert(QStringLiteral("direct"), true);
        sources.append(source);
    }

    const bool hlsReady = playbackInfo.value(QStringLiteral("hls_ready")).toBool();
    if (hlsReady) {
        const QVariantList variants = playbackInfo.value(QStringLiteral("variants")).toList();
        for (auto it = variants.crbegin(); it != variants.crend(); ++it) {
            QVariantMap variant = it->toMap();
            const QString url = variant.value(QStringLiteral("url")).toString().trimmed();
            if (url.isEmpty()) {
                continue;
            }
            QVariantMap source;
            source.insert(QStringLiteral("id"), variant.value(QStringLiteral("id")).toString());
            source.insert(QStringLiteral("label"), variant.value(QStringLiteral("label")).toString());
            source.insert(QStringLiteral("url"), url);
            source.insert(QStringLiteral("direct"), false);
            sources.append(source);
        }
    }

    return sources;
}
