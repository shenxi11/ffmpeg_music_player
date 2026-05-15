#ifndef VIDEOLISTVIEWMODEL_H
#define VIDEOLISTVIEWMODEL_H

#include "BaseViewModel.h"
#include "httprequest_v2.h"

/**
 * @brief 视频列表视图模型。
 *
 * 负责在线视频列表加载与播放地址解析，
 * 让视频页面视图只负责列表展示与播放器窗口交互。
 */
class VideoListViewModel : public BaseViewModel
{
    Q_OBJECT

public:
    explicit VideoListViewModel(QObject* parent = nullptr);

    void refresh();
    void resolveVideoStream(const QString& videoPath, const QString& videoName);

signals:
    void videoListUpdated(const QVariantList& videoList);
    void videoStreamResolved(const QString& videoUrl, const QString& videoName,
                             const QVariantList& playbackSources);

private slots:
    void onVideoListReceived(const QVariantList& videoList);
    void onVideoStreamUrlReceived(const QString& videoUrl);
    void onVideoPlaybackInfoReceived(const QVariantMap& playbackInfo);

private:
    QVariantList buildPlaybackSources(const QVariantMap& playbackInfo) const;

    HttpRequestV2 m_request;
    QString m_selectedVideoPath;
    QString m_selectedVideoName;
};

#endif // VIDEOLISTVIEWMODEL_H
