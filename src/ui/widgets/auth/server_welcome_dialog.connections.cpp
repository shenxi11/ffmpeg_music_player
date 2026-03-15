#include "server_welcome_dialog.h"

#include "viewmodels/ServerWelcomeViewModel.h"

#include <QGuiApplication>
#include <QKeySequence>
#include <QLineEdit>
#include <QPushButton>
#include <QScreen>
#include <QShortcut>

/*
模块名称: ServerWelcomeDialog 交互连接
功能概述: 统一管理欢迎验证页的按钮行为、快捷键与回车触发逻辑。
对外接口: setupInteractionConnections / moveToScreenCenter
维护说明: 仅负责连接与交互动作，不承载 UI 构建代码。
*/

void ServerWelcomeDialog::setupInteractionConnections()
{
    connect(m_verifyButton, &QPushButton::clicked, this, &ServerWelcomeDialog::onVerifyClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &ServerWelcomeDialog::reject);
    connect(m_centerButton, &QPushButton::clicked, this, &ServerWelcomeDialog::moveToScreenCenter);
    connect(m_closeButton, &QPushButton::clicked, this, &ServerWelcomeDialog::reject);
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this),
            &QShortcut::activated,
            this,
            &ServerWelcomeDialog::reject);
    connect(m_hostEdit, &QLineEdit::returnPressed, this, &ServerWelcomeDialog::onVerifyClicked);
}

void ServerWelcomeDialog::moveToScreenCenter()
{
    QScreen* screen = QGuiApplication::screenAt(frameGeometry().center());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return;
    }

    const QRect area = screen->availableGeometry();
    const QPoint centered(area.left() + (area.width() - width()) / 2,
                          area.top() + (area.height() - height()) / 2);
    const QPoint adjusted = adjustedWindowPos(centered, false);
    move(adjusted);
    if (m_viewModel) {
        m_viewModel->setWindowPos(adjusted);
    }
}
