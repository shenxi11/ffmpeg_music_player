#include "AgentChatWindow.h"

#include "AgentChatViewModel.h"

#include <QCloseEvent>
#include <QCursor>
#include <QGuiApplication>
#include <QMessageBox>
#include <QQmlError>
#include <QQmlContext>
#include <QQuickWidget>
#include <QScreen>
#include <QUrl>
#include <QVBoxLayout>

AgentChatWindow::AgentChatWindow(AgentChatViewModel* viewModel, QWidget* parent)
    : QWidget(parent)
    , m_viewModel(viewModel)
{
    setWindowFlag(Qt::Window, true);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowTitle(QStringLiteral("AI 助手"));
    setMinimumSize(820, 560);

    QScreen* targetScreen = QGuiApplication::screenAt(QCursor::pos());
    if (!targetScreen) {
        targetScreen = QGuiApplication::primaryScreen();
    }

    if (targetScreen) {
        const QRect available = targetScreen->availableGeometry();
        const int maxWidth = qMax(minimumWidth(), available.width() - 40);
        const int maxHeight = qMax(minimumHeight(), available.height() - 40);

        const double preferredWidth = 1180.0;
        const double preferredHeight = 760.0;
        const double scale = qMin(1.0, qMin(maxWidth / preferredWidth, maxHeight / preferredHeight));

        int targetWidth = static_cast<int>(preferredWidth * scale);
        int targetHeight = static_cast<int>(preferredHeight * scale);
        targetWidth = qBound(minimumWidth(), targetWidth, maxWidth);
        targetHeight = qBound(minimumHeight(), targetHeight, maxHeight);
        resize(targetWidth, targetHeight);

        const QPoint topLeft = available.center() - QPoint(width() / 2, height() / 2);
        move(topLeft);
    } else {
        resize(1180, 760);
    }

    buildUi();
}

void AgentChatWindow::closeEvent(QCloseEvent* event)
{
    if (m_viewModel) {
        m_viewModel->handleWindowClosed();
    }

    if (event) {
        event->ignore();
    }
    hide();
}

void AgentChatWindow::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_quickWidget = new QQuickWidget(this);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->setClearColor(QColor("#F4F6FA"));

    if (m_viewModel) {
        m_quickWidget->rootContext()->setContextProperty(QStringLiteral("agentChatVM"), m_viewModel);
    }

    const QUrl qmlUrl(QStringLiteral("qrc:/qml/components/agent/AgentChatWindow.qml"));
    m_quickWidget->setSource(qmlUrl);

    if (m_quickWidget->status() == QQuickWidget::Error) {
        const auto errors = m_quickWidget->errors();
        QStringList lines;
        for (const auto& e : errors) {
            lines << e.toString();
        }
        QMessageBox::warning(this,
                             QStringLiteral("AI 助手"),
                             QStringLiteral("聊天页面加载失败：\n%1").arg(lines.join(QStringLiteral("\n"))));
    }

    rootLayout->addWidget(m_quickWidget);
}
