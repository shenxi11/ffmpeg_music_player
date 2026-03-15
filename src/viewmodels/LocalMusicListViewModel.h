#ifndef LOCALMUSICLISTVIEWMODEL_H
#define LOCALMUSICLISTVIEWMODEL_H

#include "BaseViewModel.h"
#include "httprequest_v2.h"

#include <QStringList>

/**
 * @brief 本地音乐列表视图模型。
 *
 * 负责处理本地音乐新增与登录后本地音乐路径刷新，
 * 避免本地音乐列表视图直接持有网络请求对象或用户对象。
 */
class LocalMusicListViewModel : public BaseViewModel
{
    Q_OBJECT

public:
    explicit LocalMusicListViewModel(QObject* parent = nullptr);

    void addMusic(const QString& path);
    void refreshLocalMusicPaths();

signals:
    void localMusicPathsReady(const QStringList& paths);

private:
    void handleUserSongsChanged();

    HttpRequestV2 m_request;
};

#endif // LOCALMUSICLISTVIEWMODEL_H
