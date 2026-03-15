#include "download_task_model.h"

/*
模块名称: DownloadTaskModel 连接配置
功能概述: 统一管理下载管理器到下载任务模型的事件绑定。
对外接口: DownloadTaskModel::setupConnections()
维护说明: 仅维护连接关系，不包含模型更新业务实现。
*/

void DownloadTaskModel::setupConnections()
{
    connect(&DownloadManager::instance(), &DownloadManager::downloadAdded,
            this, &DownloadTaskModel::onDownloadAdded);
    connect(&DownloadManager::instance(), &DownloadManager::downloadStarted,
            this, &DownloadTaskModel::onDownloadStarted);
    connect(&DownloadManager::instance(), &DownloadManager::downloadProgress,
            this, &DownloadTaskModel::onDownloadProgress);
    connect(&DownloadManager::instance(), &DownloadManager::downloadFinished,
            this, &DownloadTaskModel::onDownloadFinished);
    connect(&DownloadManager::instance(), &DownloadManager::downloadFailed,
            this, &DownloadTaskModel::onDownloadFailed);
    connect(&DownloadManager::instance(), &DownloadManager::downloadPaused,
            this, &DownloadTaskModel::onDownloadPaused);
    connect(&DownloadManager::instance(), &DownloadManager::downloadCancelled,
            this, &DownloadTaskModel::onDownloadCancelled);
    connect(&DownloadManager::instance(), &DownloadManager::taskRemoved,
            this, &DownloadTaskModel::onTaskRemoved);
}
