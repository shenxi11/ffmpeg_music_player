#ifndef ONLINEMUSICLISTVIEWMODEL_H
#define ONLINEMUSICLISTVIEWMODEL_H

#include "BaseViewModel.h"
#include "httprequest_v2.h"

#include <QtGlobal>

/**
 * @brief 在线音乐列表视图模型。
 *
 * 负责在线歌曲播放地址解析、下载任务发起与下载进度事件转发，
 * 让在线音乐列表视图只关注展示和用户交互。
 */
class OnlineMusicListViewModel : public BaseViewModel
{
    Q_OBJECT

public:
    explicit OnlineMusicListViewModel(QObject* parent = nullptr);

    void resolveStreamUrl(const QString& relativePath,
                          const QString& artist,
                          const QString& cover);
    void downloadMusic(const QString& relativePath,
                       const QString& coverUrl);

signals:
    void streamReady(const QString& streamUrl,
                     const QString& artist,
                     const QString& cover);
    void downloadStarted(const QString& taskId, const QString& filename);
    void downloadProgress(const QString& taskId,
                          const QString& filename,
                          qint64 bytesReceived,
                          qint64 bytesTotal);
    void downloadFinished(const QString& taskId,
                          const QString& filename,
                          const QString& savePath);
    void downloadFailed(const QString& taskId,
                        const QString& filename,
                        const QString& error);

private slots:
    void onStreamUrlReady(bool success, const QString& url);

private:
    // 连接拆分：集中管理网关与下载管理器信号绑定。
    void setupConnections();

    HttpRequestV2 m_request;
    QString m_currentArtist;
    QString m_currentCover;
};

#endif // ONLINEMUSICLISTVIEWMODEL_H
