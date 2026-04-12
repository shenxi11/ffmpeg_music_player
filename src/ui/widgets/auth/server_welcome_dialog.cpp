#include "server_welcome_dialog.h"

#include "viewmodels/ServerWelcomeViewModel.h"

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QShortcut>
#include <QShowEvent>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

namespace {

QFont buildWelcomeFont(int pixelSize, int weight = QFont::Normal) {
    QFont font;
    font.setFamilies({QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Microsoft YaHei"),
                      QStringLiteral("PingFang SC"), QStringLiteral("Segoe UI")});
    font.setPixelSize(pixelSize);
    font.setWeight(weight);
    font.setStyleStrategy(QFont::PreferAntialias);
    return font;
}

QLabel* createIconLabel(const QString& iconPath, const QSize& size, QWidget* parent) {
    auto* label = new QLabel(parent);
    label->setFixedSize(size);

    const QIcon icon(iconPath);
    const QPixmap pixmap = icon.pixmap(size);
    if (!pixmap.isNull()) {
        label->setPixmap(pixmap);
        label->setAlignment(Qt::AlignCenter);
    }

    return label;
}

QFrame* createHeroTipCard(const QString& iconPath, const QString& title, const QString& text,
                          QWidget* parent) {
    auto* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("HeroTipCard"));

    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(12);

    auto* iconWrap = new QFrame(card);
    iconWrap->setObjectName(QStringLiteral("HeroTipIconWrap"));
    iconWrap->setFixedSize(38, 38);

    auto* iconLabel = createIconLabel(iconPath, QSize(20, 20), iconWrap);
    iconLabel->move((iconWrap->width() - iconLabel->width()) / 2,
                    (iconWrap->height() - iconLabel->height()) / 2);

    auto* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(3);

    auto* titleLabel = new QLabel(title, card);
    titleLabel->setObjectName(QStringLiteral("HeroTipTitle"));
    titleLabel->setFont(buildWelcomeFont(15, QFont::DemiBold));

    auto* textLabel = new QLabel(text, card);
    textLabel->setObjectName(QStringLiteral("HeroTipText"));
    textLabel->setWordWrap(true);
    textLabel->setFont(buildWelcomeFont(13));

    textLayout->addWidget(titleLabel);
    textLayout->addWidget(textLabel);

    layout->addWidget(iconWrap, 0, Qt::AlignTop);
    layout->addLayout(textLayout, 1);

    return card;
}

} // namespace

ServerWelcomeDialog::ServerWelcomeDialog(bool autoVerifyOnShow, QWidget* parent)
    : QDialog(parent), m_viewModel(new ServerWelcomeViewModel(this)),
      m_autoVerifyOnShow(autoVerifyOnShow) {
    setObjectName(QStringLiteral("ServerWelcomeDialog"));
    setWindowTitle(QStringLiteral(u"欢迎使用 云音乐"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setModal(true);
    setFixedSize(720, 500);
    setFont(buildWelcomeFont(13));

    setStyleSheet(
        QStringLiteral("QDialog#ServerWelcomeDialog {"
                       "  background: transparent;"
                       "}"
                       "QFrame#MainCard {"
                       "  background: transparent;"
                       "  border: none;"
                       "}"
                       "QFrame#HeroPanel {"
                       "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
                       "              stop:0 #FFF2F2, stop:0.58 #FFF8F8, stop:1 #FFFFFF);"
                       "  border: 1px solid #F3D9D9;"
                       "  border-radius: 16px;"
                       "}"
                       "QFrame#BrandLogoWrap {"
                       "  background: #EC4141;"
                       "  border: none;"
                       "  border-radius: 16px;"
                       "}"
                       "QFrame#HeroTipCard {"
                       "  background: rgba(255, 255, 255, 0.92);"
                       "  border: 1px solid #F1DFDF;"
                       "  border-radius: 14px;"
                       "}"
                       "QFrame#HeroTipIconWrap {"
                       "  background: #FFF1F1;"
                       "  border: 1px solid #F4D4D4;"
                       "  border-radius: 12px;"
                       "}"
                       "QFrame#FormPanel {"
                       "  background: #FFFFFF;"
                       "  border: 1px solid #ECECEC;"
                       "  border-radius: 16px;"
                       "}"
                       "QFrame#StatusPanel {"
                       "  background: #FFF4F4;"
                       "  border: 1px solid #F3C9C9;"
                       "  border-radius: 12px;"
                       "}"
                       "QDialog#ServerWelcomeDialog QLabel {"
                       "  background: transparent;"
                       "}"
                       "QLabel#TitleLabel {"
                       "  color: #1F2329;"
                       "}"
                       "QLabel#SubTitleLabel {"
                       "  color: #6F7785;"
                       "}"
                       "QLabel#BadgeLabel {"
                       "  color: #EC4141;"
                       "  background: rgba(255, 255, 255, 0.92);"
                       "  border: 1px solid #F4D4D4;"
                       "  border-radius: 11px;"
                       "  padding: 4px 12px;"
                       "}"
                       "QLabel#HeroTipTitle {"
                       "  color: #1F2329;"
                       "}"
                       "QLabel#HeroTipText, QLabel#FieldHintLabel, QLabel#SecondaryLinkLabel {"
                       "  color: #7A7D85;"
                       "}"
                       "QLabel#SectionTitle {"
                       "  color: #4F5663;"
                       "}"
                       "QLabel#StatusLabel {"
                       "  color: #D64848;"
                       "}"
                       "QLabel#SecondaryLinkLabel[disabled=\"true\"] {"
                       "  color: #C8CDD7;"
                       "}"
                       "QLabel#SecondaryLinkLabel a {"
                       "  color: #EC4141;"
                       "  text-decoration: none;"
                       "}"
                       "QLabel#SecondaryLinkLabel a:hover {"
                       "  text-decoration: underline;"
                       "}"
                       "QLineEdit, QSpinBox {"
                       "  min-height: 44px;"
                       "  border: 1px solid #E4E5EA;"
                       "  border-radius: 12px;"
                       "  padding: 0 14px;"
                       "  background: #FFFFFF;"
                       "  color: #1F2329;"
                       "  font-size: 14px;"
                       "}"
                       "QLineEdit:hover, QSpinBox:hover {"
                       "  border-color: #F2B0B0;"
                       "}"
                       "QLineEdit:focus, QSpinBox:focus {"
                       "  border-color: #EC4141;"
                       "}"
                       "QSpinBox::up-button, QSpinBox::down-button {"
                       "  width: 0px;"
                       "  border: none;"
                       "}"
                       "QPushButton {"
                       "  min-height: 44px;"
                       "  border-radius: 12px;"
                       "  padding: 0 18px;"
                       "  font-size: 14px;"
                       "  font-weight: 600;"
                       "}"
                       "QPushButton#CancelButton {"
                       "  border: 1px solid #E5E5E5;"
                       "  background: #FFFFFF;"
                       "  color: #4A515C;"
                       "}"
                       "QPushButton#CancelButton:hover {"
                       "  background: #F8F8F8;"
                       "}"
                       "QPushButton#CancelButton:pressed {"
                       "  background: #F3F3F3;"
                       "}"
                       "QPushButton#PrimaryButton {"
                       "  border: none;"
                       "  background: #EC4141;"
                       "  color: #FFFFFF;"
                       "}"
                       "QPushButton#PrimaryButton:hover {"
                       "  background: #F15A5A;"
                       "}"
                       "QPushButton#PrimaryButton:pressed {"
                       "  background: #D83939;"
                       "}"
                       "QPushButton#PrimaryButton:disabled {"
                       "  background: #F3A1A1;"
                       "  color: #FFF5F5;"
                       "}"
                       "QPushButton#CloseButton, QPushButton#CenterButton {"
                       "  min-height: 28px;"
                       "  max-height: 28px;"
                       "  min-width: 28px;"
                       "  max-width: 28px;"
                       "  border: none;"
                       "  border-radius: 14px;"
                       "  padding: 0;"
                       "  background: transparent;"
                       "}"
                       "QPushButton#CloseButton {"
                       "  color: #7A818D;"
                       "  font-size: 20px;"
                       "  font-weight: 400;"
                       "}"
                       "QPushButton#CloseButton:hover {"
                       "  background: #FFE8E8;"
                       "  color: #EC4141;"
                       "}"
                       "QPushButton#CloseButton:pressed {"
                       "  background: #FFD9D9;"
                       "  color: #D83939;"
                       "}"
                       "QPushButton#CenterButton {"
                       "  color: #A2A7B1;"
                       "  font-size: 11px;"
                       "  font-weight: 700;"
                       "}"
                       "QPushButton#CenterButton:hover {"
                       "  background: #F1F4FB;"
                       "  color: #616A79;"
                       "}"
                       "QPushButton#CenterButton:pressed {"
                       "  background: #E7EDF9;"
                       "  color: #4B5361;"
                       "}"));

    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("MainCard"));

    auto* heroPanel = new QFrame(card);
    heroPanel->setObjectName(QStringLiteral("HeroPanel"));

    auto* logoWrap = new QFrame(heroPanel);
    logoWrap->setObjectName(QStringLiteral("BrandLogoWrap"));
    logoWrap->setFixedSize(60, 60);

    auto* logoLabel = new QLabel(logoWrap);
    logoLabel->setFixedSize(36, 36);
    logoLabel->move((logoWrap->width() - logoLabel->width()) / 2,
                    (logoWrap->height() - logoLabel->height()) / 2);
    const QPixmap logo(QStringLiteral(
        ":/design/design_exports/netease_ui_pack_20260309/icon/ui/brand/brand_logo_light@2x.png"));
    if (!logo.isNull()) {
        logoLabel->setPixmap(logo.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    auto* titleLabel = new QLabel(QStringLiteral(u"开始连接你的音乐空间"), heroPanel);
    titleLabel->setObjectName(QStringLiteral("TitleLabel"));
    titleLabel->setFont(buildWelcomeFont(30, QFont::Bold));

    auto* subtitleLabel = new QLabel(
        QStringLiteral(u"输入服务器地址即可继续，连接成功后会自动进入主界面。"), heroPanel);
    subtitleLabel->setObjectName(QStringLiteral("SubTitleLabel"));
    subtitleLabel->setWordWrap(true);
    subtitleLabel->setFont(buildWelcomeFont(15));

    auto* badgeLabel = new QLabel(QStringLiteral(u"欢迎使用"), heroPanel);
    badgeLabel->setObjectName(QStringLiteral("BadgeLabel"));
    badgeLabel->setFont(buildWelcomeFont(12, QFont::DemiBold));
    badgeLabel->setMinimumHeight(30);
    badgeLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    badgeLabel->setAlignment(Qt::AlignCenter);

    m_centerButton = new QPushButton(QStringLiteral("◎"), heroPanel);
    m_centerButton->setObjectName(QStringLiteral("CenterButton"));
    m_centerButton->setToolTip(QStringLiteral(u"恢复居中"));
    m_centerButton->setCursor(Qt::PointingHandCursor);
    m_centerButton->setFocusPolicy(Qt::NoFocus);

    m_closeButton = new QPushButton(QStringLiteral(u"×"), heroPanel);
    m_closeButton->setObjectName(QStringLiteral("CloseButton"));
    m_closeButton->setToolTip(QStringLiteral(u"关闭"));
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setFocusPolicy(Qt::NoFocus);

    auto* serverTipCard = createHeroTipCard(
        QStringLiteral(":/qml/assets/ai/icons/welcome-server.svg"), QStringLiteral(u"连接服务器"),
        QStringLiteral(u"支持直接输入常用地址，首次连接后会自动记住。"), heroPanel);

    auto* offlineTipCard = createHeroTipCard(
        QStringLiteral(":/qml/assets/ai/icons/welcome-offline.svg"), QStringLiteral(u"离线进入"),
        QStringLiteral(u"暂时不连接也可以先进入，照常管理本地音乐。"), heroPanel);

    auto* actionRow = new QHBoxLayout();
    actionRow->setContentsMargins(0, 0, 0, 0);
    actionRow->addStretch();
    actionRow->addWidget(m_centerButton, 0, Qt::AlignRight | Qt::AlignTop);
    actionRow->addWidget(m_closeButton, 0, Qt::AlignRight | Qt::AlignTop);

    auto* brandRow = new QHBoxLayout();
    brandRow->setSpacing(14);
    brandRow->addWidget(logoWrap, 0, Qt::AlignTop);

    auto* brandTextLayout = new QVBoxLayout();
    brandTextLayout->setContentsMargins(0, 0, 0, 0);
    brandTextLayout->setSpacing(8);
    brandTextLayout->addWidget(titleLabel);
    brandTextLayout->addWidget(subtitleLabel);

    auto* badgeRow = new QHBoxLayout();
    badgeRow->setContentsMargins(0, 0, 0, 0);
    badgeRow->addWidget(badgeLabel, 0, Qt::AlignLeft);
    badgeRow->addStretch();
    brandTextLayout->addLayout(badgeRow);

    brandRow->addLayout(brandTextLayout, 1);

    auto* tipsRow = new QHBoxLayout();
    tipsRow->setSpacing(12);
    tipsRow->addWidget(serverTipCard, 1);
    tipsRow->addWidget(offlineTipCard, 1);

    auto* heroLayout = new QVBoxLayout(heroPanel);
    heroLayout->setContentsMargins(20, 18, 20, 18);
    heroLayout->setSpacing(16);
    heroLayout->addLayout(actionRow);
    heroLayout->addLayout(brandRow);
    heroLayout->addLayout(tipsRow);

    auto* formPanel = new QFrame(card);
    formPanel->setObjectName(QStringLiteral("FormPanel"));

    auto* hostLabel = new QLabel(QStringLiteral(u"服务器地址"), formPanel);
    hostLabel->setObjectName(QStringLiteral("SectionTitle"));
    hostLabel->setFont(buildWelcomeFont(13, QFont::DemiBold));

    m_hostEdit = new QLineEdit(formPanel);
    m_hostEdit->setPlaceholderText(
        QStringLiteral(u"例如：192.168.1.208、music.local 或 host:port"));
    m_hostEdit->setText(m_viewModel ? m_viewModel->serverHost() : QString());
    m_hostEdit->setClearButtonEnabled(true);
    m_hostEdit->setFont(buildWelcomeFont(14));

    auto* portLabel = new QLabel(QStringLiteral(u"端口"), formPanel);
    portLabel->setObjectName(QStringLiteral("SectionTitle"));
    portLabel->setFont(buildWelcomeFont(13, QFont::DemiBold));

    m_portSpin = new QSpinBox(formPanel);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(m_viewModel ? m_viewModel->serverPort() : 8080);
    m_portSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_portSpin->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_portSpin->setFont(buildWelcomeFont(14));

    auto* fieldHintLabel =
        new QLabel(QStringLiteral(u"支持直接输入 host:port，端口会自动识别。"), formPanel);
    fieldHintLabel->setObjectName(QStringLiteral("FieldHintLabel"));
    fieldHintLabel->setWordWrap(true);
    fieldHintLabel->setFont(buildWelcomeFont(13));

    m_statusFrame = new QFrame(formPanel);
    m_statusFrame->setObjectName(QStringLiteral("StatusPanel"));
    auto* statusLayout = new QVBoxLayout(m_statusFrame);
    statusLayout->setContentsMargins(14, 12, 14, 12);
    statusLayout->setSpacing(0);

    m_statusLabel = new QLabel(m_statusFrame);
    m_statusLabel->setObjectName(QStringLiteral("StatusLabel"));
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setFont(buildWelcomeFont(13, QFont::Medium));
    statusLayout->addWidget(m_statusLabel);
    m_statusFrame->hide();

    m_verifyButton = new QPushButton(QStringLiteral(u"连接并进入"), formPanel);
    m_verifyButton->setObjectName(QStringLiteral("PrimaryButton"));
    m_verifyButton->setDefault(true);
    m_verifyButton->setMinimumWidth(152);
    m_verifyButton->setFont(buildWelcomeFont(15, QFont::DemiBold));

    m_cancelButton = new QPushButton(QStringLiteral(u"退出"), formPanel);
    m_cancelButton->setObjectName(QStringLiteral("CancelButton"));
    m_cancelButton->setMinimumWidth(96);
    m_cancelButton->setFont(buildWelcomeFont(14, QFont::Medium));

    m_localOnlyEntryLabel = new QLabel(formPanel);
    m_localOnlyEntryLabel->setObjectName(QStringLiteral("SecondaryLinkLabel"));
    m_localOnlyEntryLabel->setText(
        QStringLiteral("暂时不连接？<a href=\"localOnly\">进入离线模式</a>"));
    m_localOnlyEntryLabel->setTextFormat(Qt::RichText);
    m_localOnlyEntryLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_localOnlyEntryLabel->setCursor(Qt::PointingHandCursor);
    m_localOnlyEntryLabel->setOpenExternalLinks(false);
    m_localOnlyEntryLabel->setFont(buildWelcomeFont(13));

    auto* formGrid = new QGridLayout();
    formGrid->setHorizontalSpacing(12);
    formGrid->setVerticalSpacing(10);
    formGrid->addWidget(hostLabel, 0, 0);
    formGrid->addWidget(portLabel, 0, 1);
    formGrid->addWidget(m_hostEdit, 1, 0);
    formGrid->addWidget(m_portSpin, 1, 1);
    formGrid->setColumnStretch(0, 3);
    formGrid->setColumnStretch(1, 1);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(10);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_verifyButton);

    auto* localOnlyLayout = new QHBoxLayout();
    localOnlyLayout->setContentsMargins(0, 0, 0, 0);
    localOnlyLayout->addStretch();
    localOnlyLayout->addWidget(m_localOnlyEntryLabel, 0, Qt::AlignRight);

    auto* formLayout = new QVBoxLayout(formPanel);
    formLayout->setContentsMargins(20, 18, 20, 18);
    formLayout->setSpacing(14);
    formLayout->addLayout(formGrid);
    formLayout->addWidget(fieldHintLabel);
    formLayout->addWidget(m_statusFrame);
    formLayout->addStretch();
    formLayout->addLayout(buttonLayout);
    formLayout->addLayout(localOnlyLayout);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(12);
    cardLayout->addWidget(heroPanel);
    cardLayout->addWidget(formPanel, 1);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(card);

    if (m_viewModel && m_viewModel->hasWindowPos()) {
        move(adjustedWindowPos(m_viewModel->windowPos(), false));
    }

    setupInteractionConnections();
}

void ServerWelcomeDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);

    if (m_hostEdit && !m_hostEdit->hasFocus()) {
        m_hostEdit->setFocus();
        m_hostEdit->selectAll();
    }

    if (!m_autoVerifyOnShow || m_autoVerifyTriggered || !m_viewModel) {
        return;
    }

    const QString cachedHost = m_viewModel->serverHost().trimmed();
    if (cachedHost.isEmpty()) {
        return;
    }

    m_autoVerifyTriggered = true;
    QTimer::singleShot(0, this, [this]() {
        if (isVisible()) {
            onVerifyClicked();
        }
    });
}

void ServerWelcomeDialog::mousePressEvent(QMouseEvent* event) {
    if (!event || event->button() != Qt::LeftButton) {
        QDialog::mousePressEvent(event);
        return;
    }

    QWidget* hit = childAt(event->pos());
    if (hit && (qobject_cast<QAbstractButton*>(hit) || qobject_cast<QLineEdit*>(hit) ||
                qobject_cast<QSpinBox*>(hit) || hit->inherits("QAbstractSpinBox"))) {
        QDialog::mousePressEvent(event);
        return;
    }

    m_dragging = true;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
#else
    m_dragOffset = event->globalPos() - frameGeometry().topLeft();
#endif
    event->accept();
}

void ServerWelcomeDialog::mouseMoveEvent(QMouseEvent* event) {
    if (!event || !m_dragging || !(event->buttons() & Qt::LeftButton)) {
        QDialog::mouseMoveEvent(event);
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    move(adjustedWindowPos(event->globalPosition().toPoint() - m_dragOffset, false));
#else
    move(adjustedWindowPos(event->globalPos() - m_dragOffset, false));
#endif
    event->accept();
}

void ServerWelcomeDialog::mouseReleaseEvent(QMouseEvent* event) {
    if (m_dragging) {
        const QPoint snapped = adjustedWindowPos(pos(), true);
        if (snapped != pos()) {
            move(snapped);
        }
        if (m_viewModel) {
            m_viewModel->setWindowPos(snapped);
        }
    }
    m_dragging = false;
    QDialog::mouseReleaseEvent(event);
}

QPoint ServerWelcomeDialog::adjustedWindowPos(const QPoint& desiredTopLeft,
                                              bool snapToEdges) const {
    QScreen* screen = QGuiApplication::screenAt(desiredTopLeft + QPoint(width() / 2, height() / 2));
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return desiredTopLeft;
    }

    const QRect area = screen->availableGeometry();
    const int maxX = area.left() + area.width() - width();
    const int maxY = area.top() + area.height() - height();

    int x = qBound(area.left(), desiredTopLeft.x(), maxX);
    int y = qBound(area.top(), desiredTopLeft.y(), maxY);

    if (snapToEdges) {
        constexpr int kSnapDistance = 18;
        if (qAbs(x - area.left()) <= kSnapDistance) {
            x = area.left();
        } else if (qAbs(maxX - x) <= kSnapDistance) {
            x = maxX;
        }

        if (qAbs(y - area.top()) <= kSnapDistance) {
            y = area.top();
        } else if (qAbs(maxY - y) <= kSnapDistance) {
            y = maxY;
        }
    }

    return QPoint(x, y);
}

void ServerWelcomeDialog::onVerifyClicked() {
    m_enterLocalOnly = false;
    setUiBusy(true);
    setStatusMessage(QString(), false);

    QString errorMessage;
    QString normalizedHost;
    int normalizedPort = m_portSpin ? m_portSpin->value() : 8080;
    if (m_viewModel &&
        m_viewModel->verifyServer(m_hostEdit ? m_hostEdit->text() : QString(), normalizedPort,
                                  &normalizedHost, &normalizedPort, &errorMessage)) {
        if (m_hostEdit) {
            m_hostEdit->setText(normalizedHost);
        }
        if (m_portSpin) {
            m_portSpin->setValue(normalizedPort);
        }
        accept();
        return;
    }

    setStatusMessage(
        errorMessage.isEmpty()
            ? QStringLiteral(u"暂时无法连接到该服务器，请检查地址、端口和服务状态后重试。")
            : errorMessage,
        true);
    setUiBusy(false);
}

void ServerWelcomeDialog::onEnterLocalOnlyClicked() {
    if (m_verifyButton && !m_verifyButton->isEnabled()) {
        return;
    }

    m_enterLocalOnly = true;
    accept();
}

void ServerWelcomeDialog::setUiBusy(bool busy) {
    if (m_hostEdit) {
        m_hostEdit->setEnabled(!busy);
    }
    if (m_portSpin) {
        m_portSpin->setEnabled(!busy);
    }
    if (m_verifyButton) {
        m_verifyButton->setEnabled(!busy);
        m_verifyButton->setText(busy ? QStringLiteral(u"连接中...")
                                     : QStringLiteral(u"连接并进入"));
    }
    if (m_cancelButton) {
        m_cancelButton->setEnabled(!busy);
    }
    if (m_localOnlyEntryLabel) {
        m_localOnlyEntryLabel->setEnabled(!busy);
    }
}

void ServerWelcomeDialog::setStatusMessage(const QString& message, bool isError) {
    if (!m_statusFrame || !m_statusLabel) {
        return;
    }

    const QString trimmed = message.trimmed();
    if (trimmed.isEmpty()) {
        m_statusLabel->clear();
        m_statusFrame->hide();
        return;
    }

    if (isError) {
        m_statusFrame->setStyleSheet(QStringLiteral("QFrame#StatusPanel {"
                                                    "  background: #FFF4F4;"
                                                    "  border: 1px solid #F3C9C9;"
                                                    "  border-radius: 12px;"
                                                    "}"));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #D64848;"));
    } else {
        m_statusFrame->setStyleSheet(QStringLiteral("QFrame#StatusPanel {"
                                                    "  background: #F4FAF6;"
                                                    "  border: 1px solid #CFE9D7;"
                                                    "  border-radius: 12px;"
                                                    "}"));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #1F8E49;"));
    }

    m_statusLabel->setText(trimmed);
    m_statusFrame->show();
}
