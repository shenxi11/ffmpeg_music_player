#include "settings_widget.h"

SettingsWidget::SettingsWidget(QWidget* parent)
    : QQuickWidget(parent)
    , m_viewModel(new SettingsViewModel(this))
{
    qDebug() << "[SettingsWidget] Initializing...";

    engine()->addImportPath("qrc:/");
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/components/settings/Settings.qml"));

    if (status() == QQuickWidget::Error) {
        qWarning() << "[SettingsWidget] Failed to load Settings.qml";
        return;
    }

    qDebug() << "[SettingsWidget] QML loaded successfully";

    QQuickItem* root = rootObject();
    if (root) {
        syncViewModelToRoot();
        syncPresenceToRoot();
        setupRootConnections(root);
    }

    setupViewModelConnections();
    setupRefreshTimer();

    setWindowTitle(QStringLiteral("设置"));
    resize(860, 700);
    setMinimumSize(720, 560);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
}

void SettingsWidget::onChooseDownloadPath()
{
    m_viewModel->chooseDownloadPath(this);
}

void SettingsWidget::onChooseAudioCachePath()
{
    m_viewModel->chooseAudioCachePath(this);
}

void SettingsWidget::onChooseLogPath()
{
    m_viewModel->chooseLogPath(this);
}

void SettingsWidget::onClearLocalCacheRequested()
{
    m_viewModel->clearLocalCache(this);
}

void SettingsWidget::onDownloadPathChanged()
{
    if (QQuickItem* root = rootObject()) {
        m_viewModel->setDownloadPath(root->property("downloadPath").toString());
    }
}

void SettingsWidget::onDownloadLyricsChanged()
{
    if (QQuickItem* root = rootObject()) {
        m_viewModel->setDownloadLyrics(root->property("downloadLyrics").toBool());
    }
}

void SettingsWidget::onDownloadCoverChanged()
{
    if (QQuickItem* root = rootObject()) {
        m_viewModel->setDownloadCover(root->property("downloadCover").toBool());
    }
}

void SettingsWidget::onAudioCachePathChanged()
{
    if (QQuickItem* root = rootObject()) {
        m_viewModel->setAudioCachePath(root->property("audioCachePath").toString());
    }
}

void SettingsWidget::onLogPathChanged()
{
    if (QQuickItem* root = rootObject()) {
        m_viewModel->setLogPath(root->property("logPath").toString());
    }
}

void SettingsWidget::onRefreshPresenceRequested()
{
    m_viewModel->refreshPresence();
}

void SettingsWidget::syncViewModelToRoot()
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }

    QMetaObject::invokeMethod(root, "setDownloadPath", Q_ARG(QVariant, m_viewModel->downloadPath()));
    QMetaObject::invokeMethod(root, "setDownloadLyrics", Q_ARG(QVariant, m_viewModel->downloadLyrics()));
    QMetaObject::invokeMethod(root, "setDownloadCover", Q_ARG(QVariant, m_viewModel->downloadCover()));
    QMetaObject::invokeMethod(root, "setAudioCachePath", Q_ARG(QVariant, m_viewModel->audioCachePath()));
    QMetaObject::invokeMethod(root, "setLogPath", Q_ARG(QVariant, m_viewModel->logPath()));
}

void SettingsWidget::syncPresenceToRoot()
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }

    QMetaObject::invokeMethod(root,
                              "setPresenceSnapshot",
                              Q_ARG(QVariant, QVariant(m_viewModel->presenceAccount())),
                              Q_ARG(QVariant, QVariant(m_viewModel->presenceSessionToken())),
                              Q_ARG(QVariant, QVariant(m_viewModel->presenceOnline())),
                              Q_ARG(QVariant, QVariant(m_viewModel->presenceHeartbeatIntervalSec())),
                              Q_ARG(QVariant, QVariant(m_viewModel->presenceOnlineTtlSec())),
                              Q_ARG(QVariant, QVariant(m_viewModel->presenceTtlRemainingSec())),
                              Q_ARG(QVariant, QVariant(m_viewModel->presenceStatusMessage())),
                              Q_ARG(QVariant, QVariant(m_viewModel->presenceLastSeenText())));
}
