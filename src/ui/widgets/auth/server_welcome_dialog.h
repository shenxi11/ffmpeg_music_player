#ifndef SERVER_WELCOME_DIALOG_H
#define SERVER_WELCOME_DIALOG_H

#include <QDialog>
#include <QJsonObject>

class QLineEdit;
class QSpinBox;
class QLabel;
class QPushButton;
class QNetworkAccessManager;

class ServerWelcomeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ServerWelcomeDialog(QWidget* parent = nullptr);

private slots:
    void onVerifyClicked();

private:
    bool verifyServer(QString* errorMessage);
    bool getJson(const QString& fullUrl, QJsonObject* root, QString* errorMessage);
    static QString normalizeHostInput(const QString& rawHost, int* portInOut);
    void setUiBusy(bool busy);
    void setStatusMessage(const QString& message, bool isError);

private:
    QLineEdit* m_hostEdit = nullptr;
    QSpinBox* m_portSpin = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_verifyButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QNetworkAccessManager* m_networkManager = nullptr;
};

#endif // SERVER_WELCOME_DIALOG_H
