#include "download_manager.h"
#include <QUuid>
#include <QEventLoop>

// ========== DownloadThread 实现 ==========

DownloadThread::DownloadThread(const QString& taskId, const QString& url,
                               const QString& savePath, const QByteArray& postData,
                               qint64 startByte)
    : m_taskId(taskId)
    , m_url(url)
    , m_savePath(savePath)
    , m_postData(postData)
    , m_startByte(startByte)
    , m_aborted(0)
{
    setAutoDelete(true); // 线程执行完自动删除
}

void DownloadThread::abort()
{
    m_aborted.storeRelease(1);
}

void DownloadThread::handleReplyDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (m_aborted.loadAcquire()) {
        if (m_currentReply) {
            m_currentReply->abort();
        }
        return;
    }

    const qint64 totalSize = bytesTotal + m_startByte;
    const qint64 receivedSize = bytesReceived + m_startByte;
    emit progressUpdated(m_taskId, receivedSize, totalSize);
}

void DownloadThread::handleReplyReadyRead()
{
    if (m_aborted.loadAcquire()) {
        if (m_currentReply) {
            m_currentReply->abort();
        }
        return;
    }

    if (!m_currentReply || !m_currentFile) {
        return;
    }
    m_currentFile->write(m_currentReply->readAll());
}

void DownloadThread::run()
{
    qDebug() << "[DownloadThread]" << m_taskId << "started in thread" << QThread::currentThreadId();
    
    emit started(m_taskId);
    
    // 创建独立的 NetworkAccessManager（线程安全）
    QNetworkAccessManager networkManager;
    
    QNetworkRequest request(m_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // 如果是续传，设置 Range 头
    if (m_startByte > 0) {
        QString rangeHeader = QString("bytes=%1-").arg(m_startByte);
        request.setRawHeader("Range", rangeHeader.toUtf8());
        qDebug() << "[DownloadThread]" << m_taskId << "Resume with Range:" << rangeHeader;
    }
    
    // 发起请求
    QNetworkReply* reply = !m_postData.isEmpty() ?
        networkManager.post(request, m_postData) :
        networkManager.get(request);
    m_currentReply = reply;
    
    // 打开文件用于写入
    QFile file(m_savePath);
    QIODevice::OpenMode openMode = m_startByte > 0 ? 
        QIODevice::Append : QIODevice::WriteOnly;
    
    if (!file.open(openMode)) {
        qDebug() << "[DownloadThread]" << m_taskId << "Failed to open file:" << file.errorString();
        reply->abort();
        reply->deleteLater();
        emit finished(m_taskId, false, "Failed to open file: " + file.errorString());
        m_currentReply = nullptr;
        return;
    }
    m_currentFile = &file;
    
    QObject::connect(reply, &QNetworkReply::downloadProgress,
                     this, &DownloadThread::handleReplyDownloadProgress,
                     Qt::DirectConnection);
    QObject::connect(reply, &QNetworkReply::readyRead,
                     this, &DownloadThread::handleReplyReadyRead,
                     Qt::DirectConnection);
    
    // 使用事件循环等待下载完成
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    // 检查是否被中止
    if (m_aborted.loadAcquire()) {
        file.close();
        reply->deleteLater();
        m_currentReply = nullptr;
        m_currentFile = nullptr;
        emit finished(m_taskId, false, "Download aborted by user");
        return;
    }
    
    // 检查网络错误。续传命中 HTTP 416 时，说明服务端判定本地文件已完整。
    if (reply->error() != QNetworkReply::NoError) {
        const int httpStatus =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const qint64 localFileSize = file.size();

        if (m_startByte > 0 && httpStatus == 416 && localFileSize > 0) {
            qDebug() << "[DownloadThread]" << m_taskId
                     << "Resume hit HTTP 416, treat existing file as completed. size:"
                     << localFileSize;
            file.flush();
            file.close();
            reply->deleteLater();
            m_currentReply = nullptr;
            m_currentFile = nullptr;
            emit progressUpdated(m_taskId, localFileSize, localFileSize);
            emit finished(m_taskId, true, QString());
            return;
        }

        QString errorMsg = reply->errorString();
        qDebug() << "[DownloadThread]" << m_taskId << "Network error:" << errorMsg
                 << "HTTP status:" << httpStatus;
        file.close();
        reply->deleteLater();
        m_currentReply = nullptr;
        m_currentFile = nullptr;
        emit finished(m_taskId, false, errorMsg);
        return;
    }
    
    // 写入剩余数据
    QByteArray remainingData = reply->readAll();
    if (!remainingData.isEmpty()) {
        file.write(remainingData);
    }
    
    file.flush();
    file.close();
    reply->deleteLater();
    m_currentReply = nullptr;
    m_currentFile = nullptr;
    
    qDebug() << "[DownloadThread]" << m_taskId << "completed successfully";
    emit finished(m_taskId, true, QString());
}

// ========== DownloadManager 实现 ==========

DownloadManager::DownloadManager()
    : m_threadPool(new QThreadPool(this))
    , m_maxConcurrent(3)
    , m_settings(new QSettings("FFmpegMusicPlayer", "Downloads", this))
{
    m_threadPool->setMaxThreadCount(m_maxConcurrent);
    qDebug() << "[DownloadManager] Initialized with thread pool, max threads:" << m_maxConcurrent;
    loadTaskInfo();
}

DownloadManager::~DownloadManager()
{
    // 中止所有活跃的下载线程
    QMutexLocker locker(&m_tasksMutex);
    for (auto thread : m_activeThreads.values()) {
        if (thread) {
            thread->abort();
        }
    }
    locker.unlock();
    
    // 等待线程池中的所有任务完成（最多等待5秒）
    m_threadPool->waitForDone(5000);
}

QString DownloadManager::generateTaskId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void DownloadManager::setMaxConcurrent(int max)
{
    if (max < 1) max = 1;
    if (max > 10) max = 10; // 限制最大并发数，避免过多线程
    
    m_maxConcurrent = max;
    if (m_threadPool) {
        m_threadPool->setMaxThreadCount(max);
        qDebug() << "[DownloadManager] Max concurrent downloads set to:" << max;
    }
}

QString DownloadManager::addDownload(const QString& url, const QString& filename, 
                                      const QString& downloadFolder, const QByteArray& postData,
                                      const QString& coverUrl)
{
    QString savePath = downloadFolder + "/" + filename;
    
    // 检查是否已有相同文件的任务（正在下载或等待中）
    for (const auto& existingTask : m_allTasks.values()) {
        if (existingTask.savePath == savePath) {
            if (existingTask.state == DownloadState::Downloading || 
                existingTask.state == DownloadState::Waiting ||
                existingTask.state == DownloadState::Paused) {
                qDebug() << "[DownloadManager] Task already exists for:" << filename;
                return existingTask.taskId;
            }
            // 如果是已完成的任务，删除旧任务记录
            if (existingTask.state == DownloadState::Completed) {
                qDebug() << "[DownloadManager] Removing old completed task for:" << filename;
                removeTask(existingTask.taskId);
            }
        }
    }
    
    DownloadTask task;
    task.taskId = generateTaskId();
    task.url = url;
    task.filename = filename;
    task.savePath = savePath;
    task.postData = postData;
    task.coverUrl = coverUrl;
    task.state = DownloadState::Waiting;

    QFile existingFile(task.savePath);
    if (existingFile.exists()) {
        qint64 existingSize = existingFile.size();
        task.downloadedSize = existingSize;
        qDebug() << "[DownloadManager] Found existing file, size:" << existingSize;
    }

    qDebug() << "[DownloadManager] Adding download task:" << task.taskId << task.filename;

    m_allTasks.insert(task.taskId, task);
    m_downloadQueue.enqueue(task.taskId);
    saveTaskInfo(task);
    
    // 立即通知UI任务已添加
    emit downloadAdded(task.taskId, task.filename);
    
    processQueue();
    
    return task.taskId;
}

void DownloadManager::pauseDownload(const QString& taskId)
{
    QMutexLocker locker(&m_tasksMutex);
    if (!m_allTasks.contains(taskId)) return;
    DownloadTask& task = m_allTasks[taskId];
    if (task.state != DownloadState::Downloading) return;

    qDebug() << "[DownloadManager] Pausing:" << task.filename;
    
    // 中止下载线程
    if (m_activeThreads.contains(taskId)) {
        m_activeThreads[taskId]->abort();
        m_activeThreads.remove(taskId);
    }
    
    task.state = DownloadState::Paused;
    saveTaskInfo(task);
    emit downloadPaused(taskId, task.filename);
}

void DownloadManager::resumeDownload(const QString& taskId)
{
    if (!m_allTasks.contains(taskId)) return;
    DownloadTask& task = m_allTasks[taskId];
    if (task.state != DownloadState::Paused && task.state != DownloadState::Failed) return;

    qDebug() << "[DownloadManager] Resuming:" << task.filename;
    task.state = DownloadState::Waiting;
    m_downloadQueue.enqueue(taskId);
    saveTaskInfo(task);
    processQueue();
}

void DownloadManager::cancelDownload(const QString& taskId)
{
    if (!m_allTasks.contains(taskId)) return;
    DownloadTask& task = m_allTasks[taskId];

    qDebug() << "[DownloadManager] Cancelling:" << task.filename;
    
    QMutexLocker locker(&m_tasksMutex);
    if (m_activeThreads.contains(taskId)) {
        m_activeThreads[taskId]->abort();
        m_activeThreads.remove(taskId);
    }
    locker.unlock();
    if (task.state != DownloadState::Completed) {
        QFile file(task.savePath);
        if (file.exists()) file.remove();
    }
    task.state = DownloadState::Cancelled;
    saveTaskInfo(task);
    emit downloadCancelled(taskId, task.filename);
    processQueue();
}

void DownloadManager::removeTask(const QString& taskId)
{
    if (!m_allTasks.contains(taskId)) return;
    
    DownloadTask task = m_allTasks[taskId];
    qDebug() << "[DownloadManager] Removing task:" << task.filename;
    
    // 从内存中删除
    m_allTasks.remove(taskId);
    
    // 从持久化存储中删除
    m_settings->remove(taskId);
    
    emit taskRemoved(taskId, task.filename);
}

QList<DownloadTask> DownloadManager::getAllTasks() const
{
    return m_allTasks.values();
}

QList<DownloadTask> DownloadManager::getActiveTasks() const
{
    QList<DownloadTask> result;
    for (const auto& task : m_allTasks.values()) {
        if (task.state == DownloadState::Downloading || 
            task.state == DownloadState::Waiting ||
            task.state == DownloadState::Paused) {
            // 过滤掉歌词文件，只显示音频/视频文件
            if (!task.filename.endsWith(".lrc", Qt::CaseInsensitive)) {
                result.append(task);
            }
        }
    }
    return result;
}

QList<DownloadTask> DownloadManager::getCompletedTasks() const
{
    QList<DownloadTask> result;
    for (const auto& task : m_allTasks.values()) {
        if (task.state == DownloadState::Completed) {
            // 过滤掉歌词文件，只显示音频/视频文件
            if (!task.filename.endsWith(".lrc", Qt::CaseInsensitive)) {
                result.append(task);
            }
        }
    }
    return result;
}

DownloadTask DownloadManager::getTask(const QString& taskId) const
{
    return m_allTasks.value(taskId);
}

void DownloadManager::processQueue()
{
    QMutexLocker locker(&m_tasksMutex);
    
    int activeCount = m_activeThreads.size();
    if (m_downloadQueue.isEmpty() || activeCount >= m_maxConcurrent) return;

    QString taskId = m_downloadQueue.dequeue();
    if (!m_allTasks.contains(taskId)) {
        locker.unlock();
        processQueue();
        return;
    }

    DownloadTask& task = m_allTasks[taskId];
    if (task.state == DownloadState::Cancelled) {
        locker.unlock();
        processQueue();
        return;
    }

    startDownload(task, task.downloadedSize > 0);
}

void DownloadManager::startDownload(DownloadTask& task, bool isResume)
{
    qDebug() << "[DownloadManager] Starting:" << task.filename << "Resume:" << isResume;

    if (!createDirectories(task.savePath)) {
        task.state = DownloadState::Failed;
        task.errorMsg = "Failed to create directories";
        saveTaskInfo(task);
        emit downloadFailed(task.taskId, task.filename, task.errorMsg);
        processQueue();
        return;
    }

    // 创建下载线程
    DownloadThread* thread = new DownloadThread(
        task.taskId, 
        task.url, 
        task.savePath, 
        task.postData,
        isResume ? task.downloadedSize : 0
    );
    
    // 连接信号
    connect(thread, &DownloadThread::started,
            this, &DownloadManager::onThreadStarted, Qt::QueuedConnection);
    connect(thread, &DownloadThread::progressUpdated,
            this, &DownloadManager::onThreadProgressUpdated, Qt::QueuedConnection);
    connect(thread, &DownloadThread::finished,
            this, &DownloadManager::onThreadFinished, Qt::QueuedConnection);
    
    // 添加到活跃线程列表
    m_activeThreads.insert(task.taskId, thread);
    
    // 提交到线程池执行
    m_threadPool->start(thread);
    
    qDebug() << "[DownloadManager] Thread submitted to pool, active threads:" << m_activeThreads.size();
}

void DownloadManager::onThreadStarted(const QString& taskId)
{
    QMutexLocker locker(&m_tasksMutex);
    if (!m_allTasks.contains(taskId)) {
        return;
    }

    DownloadTask& task = m_allTasks[taskId];
    task.state = DownloadState::Downloading;
    saveTaskInfo(task);
    emit downloadStarted(taskId, task.filename);
}

void DownloadManager::onThreadProgressUpdated(const QString& taskId,
                                              qint64 bytesReceived,
                                              qint64 bytesTotal)
{
    QMutexLocker locker(&m_tasksMutex);
    if (!m_allTasks.contains(taskId)) {
        return;
    }

    DownloadTask& task = m_allTasks[taskId];
    task.downloadedSize = bytesReceived;
    if (bytesTotal > 0) {
        task.totalSize = bytesTotal;
    }

    const int currentProgress = task.progress();
    if (currentProgress > task.lastReportedProgress) {
        task.lastReportedProgress = currentProgress;
        saveTaskInfo(task);
        emit downloadProgress(taskId, task.filename, bytesReceived, bytesTotal);
    }
}

void DownloadManager::onThreadFinished(const QString& taskId,
                                       bool success,
                                       const QString& errorMsg)
{
    QMutexLocker locker(&m_tasksMutex);
    m_activeThreads.remove(taskId);

    if (!m_allTasks.contains(taskId)) {
        locker.unlock();
        processQueue();
        return;
    }

    DownloadTask& task = m_allTasks[taskId];
    if (success) {
        task.state = DownloadState::Completed;
        saveTaskInfo(task);
        emit downloadFinished(taskId, task.filename, task.savePath);
    } else {
        task.state = DownloadState::Failed;
        task.errorMsg = errorMsg;
        saveTaskInfo(task);
        emit downloadFailed(taskId, task.filename, errorMsg);
    }

    locker.unlock();
    processQueue();
}

bool DownloadManager::createDirectories(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        if (dir.mkpath(".")) {
            qDebug() << "[DownloadManager] Created dir:" << dir.absolutePath();
            return true;
        }
        return false;
    }
    return true;
}

void DownloadManager::saveTaskInfo(const DownloadTask& task)
{
    m_settings->beginGroup(task.taskId);
    m_settings->setValue("url", task.url);
    m_settings->setValue("filename", task.filename);
    m_settings->setValue("savePath", task.savePath);
    m_settings->setValue("postData", task.postData);
    m_settings->setValue("totalSize", task.totalSize);
    m_settings->setValue("downloadedSize", task.downloadedSize);
    m_settings->setValue("state", static_cast<int>(task.state));
    m_settings->setValue("createTime", task.createTime);
    m_settings->setValue("errorMsg", task.errorMsg);
    m_settings->setValue("coverUrl", task.coverUrl);
    m_settings->endGroup();
    m_settings->sync();
}

void DownloadManager::loadTaskInfo()
{
    QStringList taskIds = m_settings->childGroups();
    for (const QString& taskId : taskIds) {
        m_settings->beginGroup(taskId);
        DownloadTask task;
        task.taskId = taskId;
        task.url = m_settings->value("url").toString();
        task.filename = m_settings->value("filename").toString();
        task.savePath = m_settings->value("savePath").toString();
        task.postData = m_settings->value("postData").toByteArray();
        task.totalSize = m_settings->value("totalSize").toLongLong();
        task.downloadedSize = m_settings->value("downloadedSize").toLongLong();
        task.state = static_cast<DownloadState>(m_settings->value("state").toInt());
        task.createTime = m_settings->value("createTime").toDateTime();
        task.errorMsg = m_settings->value("errorMsg").toString();
        task.coverUrl = m_settings->value("coverUrl").toString();
        m_settings->endGroup();
        m_allTasks.insert(taskId, task);
    }
    qDebug() << "[DownloadManager] Loaded" << m_allTasks.size() << "tasks";
}
