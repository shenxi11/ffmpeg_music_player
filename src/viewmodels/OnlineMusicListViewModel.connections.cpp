#include "OnlineMusicListViewModel.h"

#include "download_manager.h"

/*
模块名称: OnlineMusicListViewModel 连接配置
功能概述: 统一管理在线音乐流解析与下载任务事件绑定。
对外接口: OnlineMusicListViewModel::setupConnections()
维护说明: 连接关系集中维护，避免构造函数堆叠。
*/

void OnlineMusicListViewModel::setupConnections()
{
    connect(&m_request, &HttpRequestV2::signalStreamurl,
            this, &OnlineMusicListViewModel::onStreamUrlReady);
    connect(&DownloadManager::instance(), &DownloadManager::downloadStarted,
            this, &OnlineMusicListViewModel::downloadStarted);
    connect(&DownloadManager::instance(), &DownloadManager::downloadProgress,
            this, &OnlineMusicListViewModel::downloadProgress);
    connect(&DownloadManager::instance(), &DownloadManager::downloadFinished,
            this, &OnlineMusicListViewModel::downloadFinished);
    connect(&DownloadManager::instance(), &DownloadManager::downloadFailed,
            this, &OnlineMusicListViewModel::downloadFailed);
}
