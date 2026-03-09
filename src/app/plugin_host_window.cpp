#include "plugin_host_window.h"

#include "settings_manager.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

PluginHostWindow::PluginHostWindow(const Meta& meta, QWidget* parent)
    : QDialog(parent, Qt::Window)
    , m_meta(meta)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setObjectName(QStringLiteral("PluginHostWindow"));

    setWindowTitle(m_meta.name.trimmed().isEmpty() ? QStringLiteral("插件") : m_meta.name);
    if (!m_meta.icon.isNull()) {
        setWindowIcon(m_meta.icon);
    }

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);

    auto* headerCard = new QWidget(this);
    headerCard->setObjectName(QStringLiteral("PluginHeaderCard"));
    headerCard->setProperty("card", true);

    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 10, 14, 10);
    headerLayout->setSpacing(10);

    auto* iconLabel = new QLabel(headerCard);
    iconLabel->setObjectName(QStringLiteral("PluginHeaderIcon"));
    iconLabel->setFixedSize(24, 24);
    const QIcon effectiveIcon = m_meta.icon.isNull() ? windowIcon() : m_meta.icon;
    if (!effectiveIcon.isNull()) {
        iconLabel->setPixmap(effectiveIcon.pixmap(24, 24));
    }

    auto* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    auto* titleLabel = new QLabel(windowTitle(), headerCard);
    titleLabel->setObjectName(QStringLiteral("PluginHeaderTitle"));

    QString subtitle = m_meta.description.trimmed();
    if (!m_meta.version.trimmed().isEmpty()) {
        if (!subtitle.isEmpty()) {
            subtitle += QStringLiteral(" · ");
        }
        subtitle += QStringLiteral("v") + m_meta.version.trimmed();
    }
    auto* subtitleLabel = new QLabel(subtitle, headerCard);
    subtitleLabel->setObjectName(QStringLiteral("PluginHeaderSubtitle"));
    subtitleLabel->setProperty("secondary", true);

    textLayout->addWidget(titleLabel);
    textLayout->addWidget(subtitleLabel);

    auto* helpButton = new QPushButton(QStringLiteral("帮助"), headerCard);
    helpButton->setObjectName(QStringLiteral("PluginHeaderHelpButton"));
    helpButton->setVisible(!m_meta.description.trimmed().isEmpty());
    connect(helpButton, &QPushButton::clicked, this, &PluginHostWindow::showHelpDialog);

    headerLayout->addWidget(iconLabel);
    headerLayout->addLayout(textLayout, 1);
    headerLayout->addWidget(helpButton);

    m_contentCard = new QWidget(this);
    m_contentCard->setObjectName(QStringLiteral("PluginContentCard"));
    m_contentCard->setProperty("card", true);
    m_contentLayout = new QVBoxLayout(m_contentCard);
    m_contentLayout->setContentsMargins(8, 8, 8, 8);
    m_contentLayout->setSpacing(0);

    rootLayout->addWidget(headerCard);
    rootLayout->addWidget(m_contentCard, 1);

    restoreWindowGeometry();
    if (size().isEmpty()) {
        resize(960, 760);
    }
}

void PluginHostWindow::setPluginContent(QWidget* content)
{
    if (!content || !m_contentLayout) {
        return;
    }

    while (QLayoutItem* item = m_contentLayout->takeAt(0)) {
        if (QWidget* oldWidget = item->widget()) {
            oldWidget->deleteLater();
        }
        delete item;
    }

    content->setParent(m_contentCard);
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    content->setWindowFlags(content->windowFlags() & ~Qt::Window);
    m_contentLayout->addWidget(content);

    if (width() < 720 || height() < 540) {
        const QSize suggested = content->sizeHint().expandedTo(QSize(880, 680));
        resize(suggested);
    }
}

void PluginHostWindow::closeEvent(QCloseEvent* event)
{
    persistWindowGeometry();
    QDialog::closeEvent(event);
}

void PluginHostWindow::showHelpDialog()
{
    QString message;
    if (!m_meta.description.trimmed().isEmpty()) {
        message += QStringLiteral("功能说明：\n") + m_meta.description.trimmed();
    }
    if (!m_meta.version.trimmed().isEmpty()) {
        if (!message.isEmpty()) {
            message += QStringLiteral("\n\n");
        }
        message += QStringLiteral("版本：v") + m_meta.version.trimmed();
    }
    if (!m_meta.pluginId.trimmed().isEmpty()) {
        if (!message.isEmpty()) {
            message += QStringLiteral("\n");
        }
        message += QStringLiteral("插件ID：") + m_meta.pluginId.trimmed();
    }
    if (message.isEmpty()) {
        message = QStringLiteral("暂无帮助信息。");
    }

    QMessageBox::information(this, QStringLiteral("插件帮助"), message);
}

void PluginHostWindow::restoreWindowGeometry()
{
    if (m_meta.pluginId.trimmed().isEmpty()) {
        resize(960, 760);
        return;
    }

    const QByteArray geometry = SettingsManager::instance().pluginWindowGeometry(m_meta.pluginId);
    if (!geometry.isEmpty() && restoreGeometry(geometry)) {
        return;
    }

    resize(960, 760);
}

void PluginHostWindow::persistWindowGeometry() const
{
    if (m_meta.pluginId.trimmed().isEmpty()) {
        return;
    }
    SettingsManager::instance().setPluginWindowGeometry(m_meta.pluginId, saveGeometry());
}
