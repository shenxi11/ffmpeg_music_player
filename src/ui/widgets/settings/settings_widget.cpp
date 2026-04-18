#include "settings_widget.h"

SettingsWidget::SettingsWidget(QWidget* parent)
    : QQuickWidget(parent)
    , m_viewModel(new SettingsViewModel(this))
{
    qDebug() << "[SettingsWidget] Initializing...";

    engine()->addImportPath("qrc:/");
    rootContext()->setContextProperty(QStringLiteral("settingsViewModel"), m_viewModel);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/components/settings/Settings.qml"));

    if (status() == QQuickWidget::Error) {
        qWarning() << "[SettingsWidget] Failed to load Settings.qml";
        return;
    }

    qDebug() << "[SettingsWidget] QML loaded successfully";

    QQuickItem* root = rootObject();
    if (root) {
        setupRootConnections(root);
    }

    setupRefreshTimer();

    setWindowTitle(QStringLiteral("设置"));
    resize(1080, 760);
    setMinimumSize(820, 520);
    setAttribute(Qt::WA_StyledBackground, true);
    setClearColor(Qt::transparent);
    setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    if (parent) {
        setWindowFlags(Qt::Widget);
    } else {
        setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint |
                       Qt::WindowMinMaxButtonsHint);
    }
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

void SettingsWidget::onRefreshPresenceRequested()
{
    m_viewModel->refreshPresence();
}

void SettingsWidget::onReturnToWelcomeRequested()
{
    emit returnToWelcomeRequested();
}
