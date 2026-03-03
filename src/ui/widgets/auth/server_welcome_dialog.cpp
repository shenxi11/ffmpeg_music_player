#include "server_welcome_dialog.h"

#include "settings_manager.h"

#include <QAbstractSpinBox>
#include <QEventLoop>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPushButton>
#include <QShortcut>
#include <QSpinBox>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QString extractResponseMessage(const QJsonObject& root)
{
    const QString message = root.value("message").toString().trimmed();
    if (!message.isEmpty()) {
        return message;
    }
    const QJsonObject data = root.value("data").toObject();
    return data.value("message").toString().trimmed();
}

} // namespace

ServerWelcomeDialog::ServerWelcomeDialog(QWidget* parent)
    : QDialog(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    setObjectName(QStringLiteral("ServerWelcomeDialog"));
    setWindowTitle(QStringLiteral(u"\u6b22\u8fce\u4f7f\u7528 \u4e91\u97f3\u4e50"));
    setModal(true);
    setFixedSize(760, 500);

    setStyleSheet(QStringLiteral(
        "QDialog#ServerWelcomeDialog {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "              stop:0 #fff7f7, stop:0.45 #fffafa, stop:1 #f8f9fc);"
        "}"
        "QFrame#MainCard {"
        "  background: #ffffff;"
        "  border: 1px solid #f1d8d8;"
        "  border-radius: 16px;"
        "}"
        "QFrame#BrandLogoWrap {"
        "  background: #ec4141;"
        "  border: none;"
        "  border-radius: 14px;"
        "}"
        "QFrame#InfoPanel {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "              stop:0 #fff1f1, stop:1 #ffe9e9);"
        "  border: 1px solid #f8cdcd;"
        "  border-radius: 14px;"
        "}"
        "QFrame#StatusPanel {"
        "  background: #fff4f4;"
        "  border: 1px solid #f8cdcd;"
        "  border-radius: 12px;"
        "}"
        "QDialog#ServerWelcomeDialog QLabel {"
        "  background: transparent;"
        "}"
        "QLabel#TitleLabel {"
        "  color: #1f2329;"
        "  font-size: 24px;"
        "  font-weight: 700;"
        "}"
        "QLabel#SubTitleLabel {"
        "  color: #6f7785;"
        "  font-size: 13px;"
        "}"
        "QLabel#BadgeLabel {"
        "  color: #ffffff;"
        "  background: #ec4141;"
        "  border-radius: 10px;"
        "  padding: 3px 10px;"
        "  font-size: 11px;"
        "  font-weight: 700;"
        "}"
        "QLabel#SectionTitle {"
        "  color: #4f5663;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "}"
        "QLabel#TipLabel {"
        "  color: #6f7785;"
        "  font-size: 12px;"
        "}"
        "QLineEdit, QSpinBox {"
        "  min-height: 44px;"
        "  border: 1px solid #f1d8d8;"
        "  border-radius: 12px;"
        "  padding: 0 14px;"
        "  background: #fff8f8;"
        "  color: #1f2329;"
        "  font-size: 14px;"
        "}"
        "QLineEdit:hover, QSpinBox:hover {"
        "  border: 1px solid #efbebe;"
        "}"
        "QLineEdit:focus, QSpinBox:focus {"
        "  border: 1px solid #ec4141;"
        "}"
        "QSpinBox::up-button, QSpinBox::down-button {"
        "  width: 0px;"
        "  border: none;"
        "}"
        "QPushButton {"
        "  min-height: 46px;"
        "  border-radius: 12px;"
        "  padding: 0 18px;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "}"
        "QPushButton#CancelButton {"
        "  border: 1px solid #e1e1e1;"
        "  background: #ffffff;"
        "  color: #4a515c;"
        "}"
        "QPushButton#CancelButton:hover {"
        "  background: #f8f8f8;"
        "}"
        "QPushButton#CancelButton:pressed {"
        "  background: #f3f3f3;"
        "}"
        "QPushButton#PrimaryButton {"
        "  border: none;"
        "  background: #ec4141;"
        "  color: #ffffff;"
        "}"
        "QPushButton#PrimaryButton:hover {"
        "  background: #f15a5a;"
        "}"
        "QPushButton#PrimaryButton:pressed {"
        "  background: #d83939;"
        "}"
        "QPushButton#PrimaryButton:disabled {"
        "  background: #f3a1a1;"
        "  color: #fff5f5;"
        "}"
    ));

    QFrame* card = new QFrame(this);
    card->setObjectName(QStringLiteral("MainCard"));

    QFrame* logoWrap = new QFrame(card);
    logoWrap->setObjectName(QStringLiteral("BrandLogoWrap"));
    logoWrap->setFixedSize(56, 56);

    QLabel* logoLabel = new QLabel(logoWrap);
    logoLabel->setFixedSize(34, 34);
    logoLabel->move((logoWrap->width() - logoLabel->width()) / 2,
                    (logoWrap->height() - logoLabel->height()) / 2);
    const QPixmap logo(QStringLiteral(":/new/prefix1/icon/netease.png"));
    if (!logo.isNull()) {
        logoLabel->setPixmap(logo.scaled(34, 34, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    QLabel* titleLabel = new QLabel(QStringLiteral(u"\u4e91\u97f3\u4e50 \u79c1\u57df\u6d41\u5a92\u4f53\u5ba2\u6237\u7aef"), card);
    titleLabel->setObjectName(QStringLiteral("TitleLabel"));

    QLabel* subtitleLabel = new QLabel(
        QStringLiteral(u"\u9996\u6b21\u542f\u52a8\u8bf7\u5148\u9a8c\u8bc1\u670d\u52a1\u5668\u53ef\u8fde\u901a\u6027\uff0c\u9a8c\u8bc1\u901a\u8fc7\u540e\u518d\u8fdb\u5165\u4e3b\u754c\u9762\u3002"), card);
    subtitleLabel->setObjectName(QStringLiteral("SubTitleLabel"));
    subtitleLabel->setWordWrap(true);

    QLabel* badgeLabel = new QLabel(QStringLiteral(u"\u7f51\u6613\u4e91\u98ce\u683c"), card);
    badgeLabel->setObjectName(QStringLiteral("BadgeLabel"));
    badgeLabel->setFixedHeight(22);
    badgeLabel->setAlignment(Qt::AlignCenter);

    QFrame* infoPanel = new QFrame(card);
    infoPanel->setObjectName(QStringLiteral("InfoPanel"));
    infoPanel->setFixedWidth(250);
    QVBoxLayout* infoLayout = new QVBoxLayout(infoPanel);
    infoLayout->setContentsMargins(14, 14, 14, 14);
    infoLayout->setSpacing(8);
    QLabel* infoTitle = new QLabel(QStringLiteral(u"\u4f7f\u7528\u63d0\u793a"), infoPanel);
    infoTitle->setObjectName(QStringLiteral("SectionTitle"));
    QLabel* infoLine1 = new QLabel(QStringLiteral(u"\u2022 \u652f\u6301 IP\u3001\u57df\u540d\u6216 host:port \u8f93\u5165"), infoPanel);
    QLabel* infoLine2 = new QLabel(QStringLiteral(u"\u2022 \u5c06\u4f9d\u6b21\u9a8c\u8bc1 /client/ping \u4e0e /client/bootstrap"), infoPanel);
    QLabel* infoLine3 = new QLabel(QStringLiteral(u"\u2022 \u9a8c\u8bc1\u6210\u529f\u540e\u81ea\u52a8\u4fdd\u5b58\u670d\u52a1\u7aef\u914d\u7f6e"), infoPanel);
    infoLine1->setObjectName(QStringLiteral("TipLabel"));
    infoLine2->setObjectName(QStringLiteral("TipLabel"));
    infoLine3->setObjectName(QStringLiteral("TipLabel"));
    infoLine1->setWordWrap(true);
    infoLine2->setWordWrap(true);
    infoLine3->setWordWrap(true);
    infoLayout->addWidget(infoTitle);
    infoLayout->addWidget(infoLine1);
    infoLayout->addWidget(infoLine2);
    infoLayout->addWidget(infoLine3);
    infoLayout->addStretch();

    QLabel* hostLabel = new QLabel(QStringLiteral(u"\u670d\u52a1\u5668\u5730\u5740"), card);
    hostLabel->setObjectName(QStringLiteral("SectionTitle"));
    m_hostEdit = new QLineEdit(card);
    m_hostEdit->setPlaceholderText(QStringLiteral(u"\u4f8b\u5982\uff1a192.168.1.208 \u6216 music.local"));
    m_hostEdit->setText(SettingsManager::instance().serverHost());

    QLabel* portLabel = new QLabel(QStringLiteral(u"\u7aef\u53e3"), card);
    portLabel->setObjectName(QStringLiteral("SectionTitle"));
    m_portSpin = new QSpinBox(card);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(SettingsManager::instance().serverPort());
    m_portSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_portSpin->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QLabel* tipLabel = new QLabel(
        QStringLiteral(u"\u53ef\u76f4\u63a5\u8f93\u5165 host:port\uff0c\u7cfb\u7edf\u4f1a\u81ea\u52a8\u62c6\u5206\u5e76\u586b\u5145\u7aef\u53e3\u3002"), card);
    tipLabel->setObjectName(QStringLiteral("TipLabel"));
    tipLabel->setWordWrap(true);

    QFrame* statusFrame = new QFrame(card);
    statusFrame->setObjectName(QStringLiteral("StatusPanel"));
    QVBoxLayout* statusLayout = new QVBoxLayout(statusFrame);
    statusLayout->setContentsMargins(14, 12, 14, 12);
    statusLayout->setSpacing(4);
    statusFrame->setMinimumHeight(72);
    m_statusLabel = new QLabel(
        QStringLiteral(u"\u51c6\u5907\u5b8c\u6210\u8fde\u901a\u9a8c\u8bc1\uff1a/client/ping \u2192 /client/bootstrap"), statusFrame);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setObjectName(QStringLiteral("TipLabel"));
    statusLayout->addWidget(m_statusLabel);

    m_verifyButton = new QPushButton(QStringLiteral(u"\u9a8c\u8bc1\u5e76\u8fdb\u5165"), card);
    m_verifyButton->setObjectName(QStringLiteral("PrimaryButton"));
    m_verifyButton->setDefault(true);
    m_cancelButton = new QPushButton(QStringLiteral(u"\u9000\u51fa"), card);
    m_cancelButton->setObjectName(QStringLiteral("CancelButton"));
    m_verifyButton->setMinimumWidth(138);
    m_cancelButton->setMinimumWidth(104);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_verifyButton);

    QHBoxLayout* brandLayout = new QHBoxLayout();
    brandLayout->setSpacing(14);
    brandLayout->addWidget(logoWrap, 0, Qt::AlignTop);
    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(6);
    textLayout->addWidget(titleLabel);
    textLayout->addWidget(subtitleLabel);
    QHBoxLayout* badgeLayout = new QHBoxLayout();
    badgeLayout->addWidget(badgeLabel, 0, Qt::AlignLeft);
    badgeLayout->addStretch();
    textLayout->addLayout(badgeLayout);
    brandLayout->addLayout(textLayout, 1);

    QGridLayout* formGrid = new QGridLayout();
    formGrid->setHorizontalSpacing(12);
    formGrid->setVerticalSpacing(10);
    formGrid->addWidget(hostLabel, 0, 0);
    formGrid->addWidget(m_hostEdit, 1, 0);
    formGrid->addWidget(portLabel, 0, 1);
    formGrid->addWidget(m_portSpin, 1, 1);
    formGrid->setColumnStretch(0, 3);
    formGrid->setColumnStretch(1, 1);

    QWidget* rightPane = new QWidget(card);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);
    rightLayout->addLayout(formGrid);
    rightLayout->addWidget(tipLabel);
    rightLayout->addWidget(statusFrame);
    rightLayout->addStretch();
    rightLayout->addLayout(buttonLayout);

    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(14);
    contentLayout->addWidget(infoPanel);
    contentLayout->addWidget(rightPane, 1);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 20, 22, 20);
    cardLayout->setSpacing(14);
    cardLayout->addLayout(brandLayout);
    cardLayout->addLayout(contentLayout, 1);

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 24, 24, 24);
    rootLayout->addWidget(card);

    connect(m_verifyButton, &QPushButton::clicked, this, &ServerWelcomeDialog::onVerifyClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &ServerWelcomeDialog::reject);
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated,
            this, &ServerWelcomeDialog::reject);
    connect(m_hostEdit, &QLineEdit::returnPressed, this, &ServerWelcomeDialog::onVerifyClicked);
}

void ServerWelcomeDialog::onVerifyClicked()
{
    setUiBusy(true);
    setStatusMessage(QStringLiteral(u"\u6b63\u5728\u9a8c\u8bc1\u670d\u52a1\u5668\uff0c\u8bf7\u7a0d\u5019..."), false);

    QString errorMessage;
    if (verifyServer(&errorMessage)) {
        setStatusMessage(QStringLiteral(u"\u670d\u52a1\u5668\u9a8c\u8bc1\u901a\u8fc7\uff0c\u6b63\u5728\u8fdb\u5165\u4e3b\u754c\u9762..."), false);
        accept();
        return;
    }

    setStatusMessage(errorMessage.isEmpty()
                         ? QStringLiteral(u"\u670d\u52a1\u5668\u9a8c\u8bc1\u5931\u8d25\uff0c\u8bf7\u68c0\u67e5\u5730\u5740\u3001\u7aef\u53e3\u4e0e\u670d\u52a1\u72b6\u6001\u3002")
                         : errorMessage,
                     true);
    setUiBusy(false);
}

bool ServerWelcomeDialog::verifyServer(QString* errorMessage)
{
    int port = m_portSpin->value();
    const QString host = normalizeHostInput(m_hostEdit->text(), &port);
    if (host.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(u"\u8bf7\u8f93\u5165\u6709\u6548\u7684\u670d\u52a1\u5668 IP \u6216\u57df\u540d\u3002");
        }
        return false;
    }
    m_hostEdit->setText(host);
    m_portSpin->setValue(port);

    const QString baseUrl = QStringLiteral("http://%1:%2/").arg(host, QString::number(port));

    QJsonObject pingRoot;
    if (!getJson(baseUrl + "client/ping", &pingRoot, errorMessage)) {
        return false;
    }
    const int pingCode = pingRoot.value("code").toInt(0);
    if (pingCode != 0) {
        if (errorMessage) {
            const QString message = extractResponseMessage(pingRoot);
            *errorMessage = message.isEmpty()
                    ? QStringLiteral(u"\u8fde\u901a\u6027\u68c0\u67e5\u5931\u8d25\uff0c\u8bf7\u786e\u8ba4\u670d\u52a1\u7aef\u7f51\u5173\u53ef\u8bbf\u95ee\u3002")
                    : message;
        }
        return false;
    }
    const QJsonObject pingData = pingRoot.value("data").toObject();
    const QString pingStatus = pingData.value("status").toString().trimmed().toLower();
    if (!pingStatus.isEmpty() && pingStatus != "ok") {
        if (errorMessage) {
            *errorMessage = QStringLiteral(u"\u670d\u52a1\u7aef /client/ping \u8fd4\u56de\u72b6\u6001\u5f02\u5e38\u3002");
        }
        return false;
    }

    QJsonObject bootstrapRoot;
    if (!getJson(baseUrl + "client/bootstrap", &bootstrapRoot, errorMessage)) {
        return false;
    }
    const int bootstrapCode = bootstrapRoot.value("code").toInt(0);
    if (bootstrapCode != 0) {
        if (errorMessage) {
            const QString message = extractResponseMessage(bootstrapRoot);
            *errorMessage = message.isEmpty()
                    ? QStringLiteral(u"\u670d\u52a1\u7aef\u5f15\u5bfc\u68c0\u67e5\u5931\u8d25\uff0c\u8bf7\u7a0d\u540e\u91cd\u8bd5\u3002")
                    : message;
        }
        return false;
    }

    const QJsonObject bootstrapData = bootstrapRoot.value("data").toObject();
    if (bootstrapData.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(u"\u670d\u52a1\u7aef\u5f15\u5bfc\u54cd\u5e94\u7f3a\u5c11 data \u5b57\u6bb5\u3002");
        }
        return false;
    }

    const bool ready = bootstrapData.value("ready").toBool(false);
    if (!ready) {
        const QJsonObject checks = bootstrapData.value("checks").toObject();
        const bool dbReady = checks.value("database").toBool(false);
        const bool redisReady = checks.value("redis").toBool(false);
        if (errorMessage) {
            *errorMessage = QStringLiteral(u"\u670d\u52a1\u7aef\u5c1a\u672a\u5c31\u7eea\uff08database=%1, redis=%2\uff09\uff0c\u8bf7\u7a0d\u540e\u91cd\u8bd5\u3002")
                    .arg(dbReady ? QStringLiteral("true") : QStringLiteral("false"),
                         redisReady ? QStringLiteral("true") : QStringLiteral("false"));
        }
        return false;
    }

    SettingsManager::instance().setServerEndpoint(host, port);
    return true;
}

bool ServerWelcomeDialog::getJson(const QString& fullUrl, QJsonObject* root, QString* errorMessage)
{
    QNetworkRequest request{QUrl(fullUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    QNetworkReply* reply = m_networkManager->get(request);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    bool timedOut = false;

    connect(&timer, &QTimer::timeout, this, [&]() {
        timedOut = true;
        if (reply) {
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start(5000);
    loop.exec();
    timer.stop();

    const QByteArray body = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorText = reply->errorString();
    reply->deleteLater();

    if (timedOut) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(u"\u8bf7\u6c42\u8d85\u65f6\uff1a%1").arg(fullUrl);
        }
        return false;
    }

    if (networkError != QNetworkReply::NoError) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(u"\u8bf7\u6c42\u5931\u8d25\uff1a%1\uff08%2\uff09").arg(fullUrl, networkErrorText);
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(u"\u670d\u52a1\u7aef\u54cd\u5e94\u4e0d\u662f\u6709\u6548 JSON\uff1a%1").arg(fullUrl);
        }
        return false;
    }

    if (root) {
        *root = doc.object();
    }
    return true;
}

QString ServerWelcomeDialog::normalizeHostInput(const QString& rawHost, int* portInOut)
{
    QString host = rawHost.trimmed();
    if (host.isEmpty()) {
        return QString();
    }

    QUrl url(host);
    if (url.isValid() && !url.scheme().isEmpty()) {
        if (!url.host().isEmpty()) {
            host = url.host().trimmed();
        }
        if (portInOut && url.port() > 0) {
            *portInOut = url.port();
        }
        return host;
    }

    int slashIndex = host.indexOf('/');
    if (slashIndex >= 0) {
        host = host.left(slashIndex).trimmed();
    }

    const int colonIndex = host.lastIndexOf(':');
    const bool hasSinglePort = colonIndex > 0 && host.indexOf(':') == colonIndex;
    if (hasSinglePort) {
        bool ok = false;
        const int parsedPort = host.mid(colonIndex + 1).toInt(&ok);
        if (ok && parsedPort > 0 && parsedPort <= 65535) {
            if (portInOut) {
                *portInOut = parsedPort;
            }
            host = host.left(colonIndex).trimmed();
        }
    }
    return host.trimmed();
}

void ServerWelcomeDialog::setUiBusy(bool busy)
{
    if (m_hostEdit) {
        m_hostEdit->setEnabled(!busy);
    }
    if (m_portSpin) {
        m_portSpin->setEnabled(!busy);
    }
    if (m_verifyButton) {
        m_verifyButton->setEnabled(!busy);
        m_verifyButton->setText(busy
                                ? QStringLiteral(u"\u9a8c\u8bc1\u4e2d...")
                                : QStringLiteral(u"\u9a8c\u8bc1\u5e76\u8fdb\u5165"));
    }
    if (m_cancelButton) {
        m_cancelButton->setEnabled(!busy);
    }
}

void ServerWelcomeDialog::setStatusMessage(const QString& message, bool isError)
{
    if (!m_statusLabel) {
        return;
    }
    const QString prefix = isError
            ? QStringLiteral(u"\u3010\u9a8c\u8bc1\u5931\u8d25\u3011")
            : QStringLiteral(u"\u3010\u7cfb\u7edf\u63d0\u793a\u3011");
    m_statusLabel->setText(prefix + QStringLiteral(" ") + message);
    if (isError) {
        m_statusLabel->setStyleSheet(QStringLiteral("color: #D64848; font-size: 12px;"));
    } else {
        m_statusLabel->setStyleSheet(QStringLiteral("color: #1F8E49; font-size: 12px;"));
    }
}
