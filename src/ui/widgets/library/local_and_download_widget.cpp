#include "local_and_download_widget.h"
#include "local_music_cache.h"
#include <QDebug>
#include <QQuickItem>

LocalAndDownloadWidget::LocalAndDownloadWidget(QWidget *parent)
    : QQuickWidget(parent)
    , m_downloadTaskModel(new DownloadTaskModel(this))
    , m_localMusicModel(new LocalMusicModel(this))
{
    qDebug() << "[LocalAndDownloadWidget] Initializing...";
    
    // 补充 qrc 导入路径，确保 QML 组件可解析。
    engine()->addImportPath("qrc:/");
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    // 将数据模型暴露给 QML 层。
    rootContext()->setContextProperty("downloadTaskModel", m_downloadTaskModel);
    rootContext()->setContextProperty("localMusicModel", m_localMusicModel);
    
    // 转发“添加本地音乐”请求到上层业务。
    connect(m_localMusicModel, &LocalMusicModel::addMusicRequested,
            this, &LocalAndDownloadWidget::addLocalMusicRequested);
    
    // 启动时先刷新一次下载任务状态。
    qDebug() << "[LocalAndDownloadWidget] Initial refresh of active tasks";
    m_downloadTaskModel->refresh(false);
    
    // 加载本地与下载页面 QML。
    setSource(QUrl("qrc:/qml/components/library/LocalAndDownloadWidget.qml"));
    
    if (status() == QQuickWidget::Error) {
        qWarning() << "[LocalAndDownloadWidget] Failed to load QML";
        for (const auto& error : errors()) {
            qWarning() << "  " << error.toString();
        }
    }
    
    // 根对象用于连接 QML 事件。
    QQuickItem* root = rootObject();
    if (root) {
        qDebug() << "[LocalAndDownloadWidget] Root object loaded successfully";
        
        // 本地列表播放事件。
        connect(root, SIGNAL(playMusic(QString)), this, SLOT(onPlayMusic(QString)));
        // 本地列表删除事件。
        connect(root, SIGNAL(deleteMusic(QString)), this, SLOT(onDeleteMusic(QString)));
        // 收藏动作透传给业务层。
        connect(root, SIGNAL(addToFavorite(QString,QString,QString,QString)), 
                this, SIGNAL(addToFavorite(QString,QString,QString,QString)));
        connect(root, SIGNAL(songActionRequested(QString,QVariant)),
                this, SLOT(onSongActionRequested(QString,QVariant)));
        
        qDebug() << "[LocalAndDownloadWidget] Signals connected";
    } else {
        qWarning() << "[LocalAndDownloadWidget] Root object is null";
    }
    
    qDebug() << "[LocalAndDownloadWidget] Initialized successfully";
}

void LocalAndDownloadWidget::onPlayMusic(const QString& filename)
{
    qDebug() << "[LocalAndDownloadWidget] Play music:" << filename;
    emit playMusic(filename);
}

void LocalAndDownloadWidget::onDeleteMusic(const QString& filename)
{
    qDebug() << "[LocalAndDownloadWidget] Delete music:" << filename;
    emit deleteMusic(filename);
}

void LocalAndDownloadWidget::setCurrentPlayingPath(const QString& path)
{
    if (m_downloadTaskModel) {
        m_downloadTaskModel->setCurrentPlayingPath(path);
    }
    if (m_localMusicModel) {
        m_localMusicModel->setCurrentPlayingPath(path);
    }
    if (QQuickItem* root = rootObject()) {
        root->setProperty("currentPlayingPath", path);
    }
}

void LocalAndDownloadWidget::setPlayingState(const QString& filePath, bool playing)
{
    setCurrentPlayingPath(filePath);
    if (QQuickItem* root = rootObject()) {
        root->setProperty("isPlaying", playing);
    }
}

void LocalAndDownloadWidget::setAvailablePlaylists(const QVariantList& playlists)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    root->setProperty("availablePlaylists", QVariant::fromValue(playlists));
}

void LocalAndDownloadWidget::setFavoritePaths(const QStringList& favoritePaths)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    root->setProperty("favoritePaths", QVariant::fromValue(favoritePaths));
}

void LocalAndDownloadWidget::onSongActionRequested(const QString& action, const QVariant& payload)
{
    emit songActionRequested(action, payload.toMap());
}

