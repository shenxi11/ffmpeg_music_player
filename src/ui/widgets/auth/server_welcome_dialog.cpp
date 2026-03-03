#include "server_welcome_dialog.h"

#include "settings_manager.h"

#include <QEventLoop>
#include <QFormLayout>
#include <QFrame>
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
    setWindowTitle(QStringLiteral(u"欢迎使用 CloudMusic 私域播放器"));
    setModal(true);
    setFixedSize(640, 420);

    setStyleSheet(QStringLiteral(
        "QDialog#ServerWelcomeDialog {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "              stop:0 #f7f9fc, stop:0.55 #f2f5fa, stop:1 #eef2f8);"
        "}"
        "QFrame#MainCard {"
        "  background: #ffffff;"
        "  border: 1px solid #e8edf5;"
        "  border-radius: 16px;"
        "}"
        "QLabel#TitleLabel {"
        "  color: #1f2733;"
        "  font-size: 22px;"
        "  font-weight: 700;"
        "}"
        "QLabel#SubTitleLabel {"
        "  color: #6b7785;"
        "  font-size: 13px;"
        "}"
        "QLabel#TipLabel {"
        "  color: #7a8696;"
        "  font-size: 12px;"
        "}"
        "QLineEdit, QSpinBox {"
        "  min-height: 36px;"
        "  border: 1px solid #dce3ee;"
        "  border-radius: 10px;"
        "  padding: 0 10px;"
        "  background: #ffffff;"
        "  color: #1f2733;"
        "}"
        "QLineEdit:focus, QSpinBox:focus {"
        "  border: 1px solid #e64545;"
        "}"
        "QPushButton {"
        "  min-height: 36px;"
        "  border-radius: 10px;"
        "  padding: 0 16px;"
        "  font-size: 13px;"
        "}"
        "QPushButton#CancelButton {"
        "  border: 1px solid #d7deea;"
        "  background: #ffffff;"
        "  color: #4d5a69;"
        "}"
        "QPushButton#CancelButton:hover {"
        "  background: #f4f7fc;"
        "}"
        "QPushButton#PrimaryButton {"
        "  border: none;"
        "  background: #e64545;"
        "  color: #ffffff;"
        "  font-weight: 600;"
        "}"
        "QPushButton#PrimaryButton:hover {"
        "  background: #d93b3b;"
        "}"
        "QPushButton#PrimaryButton:disabled {"
        "  background: #f3a5a5;"
        "  color: #fff5f5;"
        "}"
    ));

    QFrame* card = new QFrame(this);
    card->setObjectName(QStringLiteral("MainCard"));

    QLabel* logoLabel = new QLabel(card);
    logoLabel->setFixedSize(52, 52);
    const QPixmap logo(QStringLiteral(":/new/prefix1/icon/netease.png"));
    if (!logo.isNull()) {
        logoLabel->setPixmap(logo.scaled(52, 52, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    QLabel* titleLabel = new QLabel(QStringLiteral(u"CloudMusic 私域流媒体客户端"), card);
    titleLabel->setObjectName(QStringLiteral("TitleLabel"));
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    QLabel* subtitleLabel = new QLabel(
        QStringLiteral(u"启动前先完成服务端连通性验证。验证通过后进入主界面，后续登录/注册流程保持不变。"), card);
    subtitleLabel->setObjectName(QStringLiteral("SubTitleLabel"));
    subtitleLabel->setWordWrap(true);

    m_hostEdit = new QLineEdit(card);
    m_hostEdit->setPlaceholderText(QStringLiteral(u"例如：192.168.1.208"));
    m_hostEdit->setText(SettingsManager::instance().serverHost());

    m_portSpin = new QSpinBox(card);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(SettingsManager::instance().serverPort());

    QLabel* tipLabel = new QLabel(QStringLiteral(u"支持输入 IP、域名，或 host:port（将自动拆分）。"), card);
    tipLabel->setObjectName(QStringLiteral("TipLabel"));
    tipLabel->setWordWrap(true);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setHorizontalSpacing(12);
    formLayout->setVerticalSpacing(10);
    formLayout->addRow(QStringLiteral(u"服务端 IP"), m_hostEdit);
    formLayout->addRow(QStringLiteral(u"端口"), m_portSpin);

    QFrame* statusFrame = new QFrame(card);
    statusFrame->setStyleSheet(QStringLiteral(
        "QFrame { background: #f6f9ff; border: 1px solid #dce8ff; border-radius: 10px; }"));
    QVBoxLayout* statusLayout = new QVBoxLayout(statusFrame);
    statusLayout->setContentsMargins(12, 10, 12, 10);
    statusLayout->setSpacing(4);
    m_statusLabel = new QLabel(QStringLiteral(u"将依次验证 /client/ping 与 /client/bootstrap"), statusFrame);
    m_statusLabel->setWordWrap(true);
    statusLayout->addWidget(m_statusLabel);

    m_verifyButton = new QPushButton(QStringLiteral(u"验证并进入"), card);
    m_verifyButton->setObjectName(QStringLiteral("PrimaryButton"));
    m_verifyButton->setDefault(true);
    m_cancelButton = new QPushButton(QStringLiteral(u"退出"), card);
    m_cancelButton->setObjectName(QStringLiteral("CancelButton"));

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_verifyButton);

    QHBoxLayout* brandLayout = new QHBoxLayout();
    brandLayout->setSpacing(12);
    brandLayout->addWidget(logoLabel, 0, Qt::AlignTop);
    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(4);
    textLayout->addWidget(titleLabel);
    textLayout->addWidget(subtitleLabel);
    brandLayout->addLayout(textLayout, 1);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 18);
    cardLayout->setSpacing(12);
    cardLayout->addLayout(brandLayout);
    cardLayout->addWidget(tipLabel);
    cardLayout->addLayout(formLayout);
    cardLayout->addWidget(statusFrame);
    cardLayout->addStretch();
    cardLayout->addLayout(buttonLayout);

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);
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
    setStatusMessage(QStringLiteral(u"正在验证服务端，请稍候..."), false);

    QString errorMessage;
    if (verifyServer(&errorMessage)) {
        setStatusMessage(QStringLiteral(u"服务端验证通过，正在进入主界面。"), false);
        accept();
        return;
    }

    setStatusMessage(errorMessage.isEmpty()
                         ? QStringLiteral(u"服务端验证失败，请检查地址、端口与服务状态。")
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
            *errorMessage = QStringLiteral(u"请输入有效的服务端 IP 地址。");
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
                    ? QStringLiteral(u"连通性检查失败，请确认服务端网关可访问。")
                    : message;
        }
        return false;
    }
    const QJsonObject pingData = pingRoot.value("data").toObject();
    const QString pingStatus = pingData.value("status").toString().trimmed().toLower();
    if (!pingStatus.isEmpty() && pingStatus != "ok") {
        if (errorMessage) {
            *errorMessage = QStringLiteral(u"服务端 /client/ping 返回状态异常。");
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
                    ? QStringLiteral(u"服务端引导检查失败，请稍后重试。")
                    : message;
        }
        return false;
    }

    const QJsonObject bootstrapData = bootstrapRoot.value("data").toObject();
    if (bootstrapData.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(u"服务端引导响应缺少 data 字段。");
        }
        return false;
    }

    const bool ready = bootstrapData.value("ready").toBool(false);
    if (!ready) {
        const QJsonObject checks = bootstrapData.value("checks").toObject();
        const bool dbReady = checks.value("database").toBool(false);
        const bool redisReady = checks.value("redis").toBool(false);
        if (errorMessage) {
            *errorMessage = QStringLiteral(u"服务端尚未就绪（database=%1, redis=%2），请稍后重试。")
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
            *errorMessage = QStringLiteral(u"请求超时：%1").arg(fullUrl);
        }
        return false;
    }

    if (networkError != QNetworkReply::NoError) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(u"请求失败：%1（%2）").arg(fullUrl, networkErrorText);
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(u"服务端响应不是有效 JSON：%1").arg(fullUrl);
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
    m_statusLabel->setText(message);
    if (isError) {
        m_statusLabel->setStyleSheet(QStringLiteral("color: #d84a4a;"));
    } else {
        m_statusLabel->setStyleSheet(QStringLiteral("color: #2e7d32;"));
    }
}
