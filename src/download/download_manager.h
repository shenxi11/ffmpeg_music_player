#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QMap>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QSettings>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QWaitCondition>

/**
 * @brief 下载任务状态
 */
enum class DownloadState {
    Waiting,      // 等待中
    Downloading,  // 下载中
    Paused,       // 已暂停
    Completed,    // 已完成
    Failed,       // 失败
    Cancelled     // 已取消
};

/**
 * @brief 下载任务信息
 */
struct DownloadTask {
    QString taskId;           // 任务唯一ID
    QString url;              // 下载URL
    QString filename;         // 文件名（可能包含子目录）
    QString savePath;         // 完整保存路径
    QByteArray postData;      // POST请求数据（可选）
    qint64 totalSize;         // 总大小
    qint64 downloadedSize;    // 已下载大小
    DownloadState state;      // 下载状态
    QDateTime createTime;     // 创建时间
    QString errorMsg;         // 错误信息
    int lastReportedProgress; // 上次报告的进度百分比（用于限流）
    QString coverUrl;         // 专辑图片URL
    
    DownloadTask() 
        : totalSize(0)
        , downloadedSize(0)
        , state(DownloadState::Waiting)
        , createTime(QDateTime::currentDateTime())
        , lastReportedProgress(-1)
    {}
    
    // 计算下载进度百分比
    int progress() const {
        if (totalSize <= 0) return 0;
        return static_cast<int>((downloadedSize * 100) / totalSize);
    }
};

/**
 * @brief 下载线程任务（在线程池中执行）
 */
class DownloadThread : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit DownloadThread(const QString& taskId, const QString& url, 
                           const QString& savePath, const QByteArray& postData,
                           qint64 startByte = 0);
    
    void run() override;
    void abort();

signals:
    void started(const QString& taskId);
    void progressUpdated(const QString& taskId, qint64 bytesReceived, qint64 bytesTotal);
    void finished(const QString& taskId, bool success, const QString& errorMsg);

private:
    void handleReplyDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleReplyReadyRead();

    QString m_taskId;
    QString m_url;
    QString m_savePath;
    QByteArray m_postData;
    qint64 m_startByte;
    QAtomicInt m_aborted;
    QNetworkReply* m_currentReply = nullptr;
    QFile* m_currentFile = nullptr;
};

/**
 * @brief 下载管理器（单例模式）
 * 负责管理所有下载任务，支持队列、并发控制、进度跟踪
 */
class DownloadManager : public QObject
{
    Q_OBJECT

public:
    static DownloadManager& instance()
    {
        static DownloadManager instance;
        return instance;
    }

    /**
     * @brief 添加下载任务
     * @param url 下载URL
     * @param filename 文件名（可能包含子目录，如 "歌手名/歌曲.mp3"）
     * @param downloadFolder 下载根目录
     * @param postData POST请求数据（可选，用于需要POST的下载接口）
     * @param coverUrl 专辑图片URL（可选）
     * @return 任务ID
     */
    QString addDownload(const QString& url, const QString& filename, const QString& downloadFolder, const QByteArray& postData = QByteArray(), const QString& coverUrl = QString());

    /**
     * @brief 暂停下载
     * @param taskId 任务ID
     */
    void pauseDownload(const QString& taskId);

    /**
     * @brief 恢复下载
     * @param taskId 任务ID
     */
    void resumeDownload(const QString& taskId);

    /**
     * @brief 取消下载
     * @param taskId 任务ID
     */
    void cancelDownload(const QString& taskId);

    /**
     * @brief 删除任务记录（从内存和持久化存储中删除）
     * @param taskId 任务ID
     */
    void removeTask(const QString& taskId);

    /**
     * @brief 获取所有下载任务
     */
    QList<DownloadTask> getAllTasks() const;

    /**
     * @brief 获取活跃的下载任务
     */
    QList<DownloadTask> getActiveTasks() const;

    /**
     * @brief 获取已完成的下载任务
     */
    QList<DownloadTask> getCompletedTasks() const;

    /**
     * @brief 获取指定任务信息
     */
    DownloadTask getTask(const QString& taskId) const;

    /**
     * @brief 设置最大并发下载数
     * @param max 最大并发数
     */
    void setMaxConcurrent(int max);

    /**
     * @brief 获取当前下载队列大小
     */
    int queueSize() const { return m_downloadQueue.size(); }

    /**
     * @brief 获取当前活跃下载数
     */
    int activeDownloads() const { return m_activeThreads.size(); }

signals:
    /**
     * @brief 任务添加信号（立即发出，让UI可以显示等待状态）
     * @param taskId 任务ID
     * @param filename 文件名
     */
    void downloadAdded(const QString& taskId, const QString& filename);
    
    /**
     * @brief 下载开始信号
     * @param taskId 任务ID
     * @param filename 文件名
     */
    void downloadStarted(const QString& taskId, const QString& filename);

    /**
     * @brief 下载进度信号
     * @param taskId 任务ID
     * @param filename 文件名
     * @param bytesReceived 已接收字节数
     * @param bytesTotal 总字节数
     */
    void downloadProgress(const QString& taskId, const QString& filename, qint64 bytesReceived, qint64 bytesTotal);

    /**
     * @brief 下载完成信号
     * @param taskId 任务ID
     * @param filename 文件名
     * @param savePath 保存路径
     */
    void downloadFinished(const QString& taskId, const QString& filename, const QString& savePath);

    /**
     * @brief 下载失败信号
     * @param taskId 任务ID
     * @param filename 文件名
     * @param error 错误信息
     */
    void downloadFailed(const QString& taskId, const QString& filename, const QString& error);

    /**
     * @brief 下载暂停信号
     * @param taskId 任务ID
     * @param filename 文件名
     */
    void downloadPaused(const QString& taskId, const QString& filename);

    /**
     * @brief 下载取消信号
     * @param taskId 任务ID
     * @param filename 文件名
     */
    void downloadCancelled(const QString& taskId, const QString& filename);

    /**
     * @brief 任务被删除信号
     * @param taskId 任务ID
     * @param filename 文件名
     */
    void taskRemoved(const QString& taskId, const QString& filename);

private:
    DownloadManager();
    ~DownloadManager();
    DownloadManager(const DownloadManager&) = delete;
    DownloadManager& operator=(const DownloadManager&) = delete;

private slots:
    void onThreadStarted(const QString& taskId);
    void onThreadProgressUpdated(const QString& taskId, qint64 bytesReceived, qint64 bytesTotal);
    void onThreadFinished(const QString& taskId, bool success, const QString& errorMsg);

    /**
     * @brief 处理下载队列，启动下一个任务
     */
    void processQueue();

    /**
     * @brief 开始下载任务（支持断点续传）
     */
    void startDownload(DownloadTask& task, bool isResume = false);

    /**
     * @brief 创建必要的目录
     */
    bool createDirectories(const QString& filePath);

    /**
     * @brief 保存任务信息到持久化存储
     */
    void saveTaskInfo(const DownloadTask& task);

    /**
     * @brief 加载任务信息
     */
    void loadTaskInfo();

    /**
     * @brief 生成唯一任务ID
     */
    QString generateTaskId() const;

private:
    QMap<QString, DownloadTask> m_allTasks;         // 所有任务（包括历史）
    QQueue<QString> m_downloadQueue;                 // 等待下载的任务ID队列
    QMap<QString, DownloadThread*> m_activeThreads;  // 活跃的下载线程
    QThreadPool* m_threadPool;                       // 线程池
    QMutex m_tasksMutex;                             // 任务映射的线程安全保护
    int m_maxConcurrent;                             // 最大并发下载数
    QSettings* m_settings;                           // 持久化存储
};

#endif // DOWNLOAD_MANAGER_H
