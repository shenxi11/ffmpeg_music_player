#include "video_list_widget.h"

#include <QStackedLayout>

VideoListWidget::VideoListWidget(QWidget* parent)
    : QWidget(parent), m_viewModel(new VideoListViewModel(this)) {
    listWidget = new VideoListWidgetQml(this);
    m_playerPage = new VideoPlayerPage(this);

    m_pageLayout = new QStackedLayout(this);
    m_pageLayout->setContentsMargins(0, 0, 0, 0);
    m_pageLayout->addWidget(listWidget);
    m_pageLayout->addWidget(m_playerPage);
    m_pageLayout->setCurrentWidget(listWidget);
    setLayout(m_pageLayout);

    setupConnections();
    setupVideoPlayerConnections();
}

VideoListWidget::~VideoListWidget() {
    qDebug() << "[VideoListWidget] Destroyed";
}

void VideoListWidget::onRefreshRequested() {
    qDebug() << "[VideoListWidget] Refresh requested, fetching video list...";
    m_videoEntriesByPath.clear();
    listWidget->clearAll();
    m_viewModel->refresh();
}

void VideoListWidget::onVideoListReceived(const QVariantList& videoList) {
    qDebug() << "[VideoListWidget] Received" << videoList.size() << "videos";
    m_videoEntriesByPath.clear();
    listWidget->clearAll();

    for (const QVariant& value : videoList) {
        const QVariantMap item = value.toMap();
        const QString path = item.value(QStringLiteral("path")).toString().trimmed();
        if (!path.isEmpty()) {
            m_videoEntriesByPath.insert(path, item);
        }
    }

    listWidget->addVideoList(videoList);
}

void VideoListWidget::onVideoSelected(const QString& videoPath, const QString& videoName) {
    qDebug() << "[VideoListWidget] Video selected:" << videoName << "(" << videoPath << ")";

    m_selectedVideoPath = videoPath.trimmed();
    m_selectedVideoName = videoName.trimmed();
    m_selectedVideoSize =
        m_videoEntriesByPath.value(m_selectedVideoPath).value(QStringLiteral("size")).toLongLong();
    m_viewModel->resolveVideoStream(m_selectedVideoPath, m_selectedVideoName);
}

void VideoListWidget::pauseVideoPlayback() {
    if (m_playerPage) {
        m_playerPage->pausePlayback();
    }
}

void VideoListWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    onRefreshRequested();
}

void VideoListWidget::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    pauseVideoPlayback();
}

void VideoListWidget::showListPage() {
    if (m_pageLayout) {
        m_pageLayout->setCurrentWidget(listWidget);
    }
}

void VideoListWidget::showPlayerPage() {
    if (m_pageLayout) {
        m_pageLayout->setCurrentWidget(m_playerPage);
    }
}
